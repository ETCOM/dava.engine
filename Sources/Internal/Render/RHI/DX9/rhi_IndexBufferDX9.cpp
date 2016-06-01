#include "../Common/rhi_Private.h"
    #include "../Common/rhi_Pool.h"
    #include "rhi_DX9.h"

    #include "Debug/DVAssert.h"
    #include "Logger/Logger.h"
using DAVA::Logger;

    #include "_dx9.h"

namespace rhi
{
//==============================================================================

class
IndexBufferDX9_t
: public ResourceImpl<IndexBufferDX9_t, IndexBuffer::Descriptor>
{
public:
    IndexBufferDX9_t();
    ~IndexBufferDX9_t();

    bool Create(const IndexBuffer::Descriptor& desc, bool force_immediate = false);
    void Destroy(bool force_immediate = false);

    unsigned size = 0;
    IDirect3DIndexBuffer9* buffer = nullptr;
    DAVA::Vector<uint8> mappedData;
    unsigned isMapped : 1;
    unsigned updatePending : 1;
};

RHI_IMPL_RESOURCE(IndexBufferDX9_t, IndexBuffer::Descriptor)

typedef ResourcePool<IndexBufferDX9_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, true> IndexBufferDX9Pool;
RHI_IMPL_POOL(IndexBufferDX9_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, true);

//==============================================================================

IndexBufferDX9_t::IndexBufferDX9_t()
    : size(0)
    , buffer(nullptr)
    , isMapped(false)
    , updatePending(false)
{
}

//------------------------------------------------------------------------------

IndexBufferDX9_t::~IndexBufferDX9_t()
{
}

//------------------------------------------------------------------------------

bool IndexBufferDX9_t::Create(const IndexBuffer::Descriptor& desc, bool force_immediate)
{
    DVASSERT(desc.size);

    bool success = false;
    UpdateCreationDesc(desc);

    if (desc.size)
    {
        DWORD usage = D3DUSAGE_WRITEONLY;

        switch (desc.usage)
        {
        case USAGE_DEFAULT:
            usage = D3DUSAGE_WRITEONLY;
            break;
        case USAGE_STATICDRAW:
            usage = D3DUSAGE_WRITEONLY;
            break;
        case USAGE_DYNAMICDRAW:
            usage = D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC;
            break;
        }

        uint32 cmd_cnt = 2;
        DX9Command cmd[] =
        {
          { DX9Command::CREATE_INDEX_BUFFER, { desc.size, usage, (desc.indexSize == INDEX_SIZE_32BIT) ? D3DFMT_INDEX32 : D3DFMT_INDEX16, D3DPOOL_DEFAULT, uint64_t(&buffer), NULL } },
          { DX9Command::UPDATE_INDEX_BUFFER, { uint64_t(&buffer), uint64_t(desc.initialData), desc.size } }
        };

        if (!desc.initialData)
        {
            cmd[1].func = DX9Command::NOP;
            cmd_cnt = 1;
        }

        ExecDX9(cmd, cmd_cnt, force_immediate);

        if (SUCCEEDED(cmd[0].retval))
        {
            size = desc.size;
            success = true;
        }
        else
        {
            Logger::Error("FAILED to create index-buffer:\n%s\n", D3D9ErrorText(cmd[0].retval));
        }
    }

    return success;
}

//------------------------------------------------------------------------------

void IndexBufferDX9_t::Destroy(bool force_immediate)
{
    if (buffer)
    {
        DX9Command cmd[] = { DX9Command::RELEASE, { reinterpret_cast<uint64_t>(buffer) } };
        ExecDX9(cmd, countof(cmd), force_immediate);
        buffer = nullptr;
    }

    size = 0;
}

//------------------------------------------------------------------------------

static Handle
dx9_IndexBuffer_Create(const IndexBuffer::Descriptor& desc)
{
    CommandBufferDX9::BlockNonRenderThreads();

    Handle handle = IndexBufferDX9Pool::Alloc();
    IndexBufferDX9_t* ib = IndexBufferDX9Pool::Get(handle);

    if (ib->Create(desc) == false)
    {
        IndexBufferDX9Pool::Free(handle);
        handle = InvalidHandle;
    }

    return handle;
}

//------------------------------------------------------------------------------

static void
dx9_IndexBuffer_Delete(Handle ib)
{
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);
    self->MarkRestored();
    self->Destroy();
    IndexBufferDX9Pool::Free(ib);
}

//------------------------------------------------------------------------------

static bool
dx9_IndexBuffer_Update(Handle ib, const void* data, unsigned offset, unsigned size)
{
    bool success = false;
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);

    DVASSERT(!self->isMapped);

    if (offset + size <= self->size)
    {
        void* ptr = nullptr;
        DX9Command cmd1 = { DX9Command::LOCK_INDEX_BUFFER, { uint64_t(&(self->buffer)), offset, size, uint64_t(&ptr), 0 } };

        ExecDX9(&cmd1, 1);
        if (SUCCEEDED(cmd1.retval))
        {
            memcpy(ptr, data, size);

            DX9Command cmd2 = { DX9Command::UNLOCK_INDEX_BUFFER, { uint64_t(&(self->buffer)) } };

            ExecDX9(&cmd2, 1);
            success = true;

            self->MarkRestored();
        }
    }

    return success;
}

//------------------------------------------------------------------------------

static void*
dx9_IndexBuffer_Map(Handle ib, unsigned offset, unsigned size)
{
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);
    DVASSERT(self->CreationDesc().usage == Usage::USAGE_DYNAMICDRAW);

    if (self->mappedData.size() < self->size)
    {
        self->mappedData.resize(self->size);
    }

    DVASSERT(!self->isMapped);
    self->isMapped = true;

    return self->mappedData.data() + offset;
}

//------------------------------------------------------------------------------

static void
dx9_IndexBuffer_Unmap(Handle ib)
{
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);
    DVASSERT(self->isMapped);
    DVASSERT(self->CreationDesc().usage != Usage::USAGE_STATICDRAW);

    self->isMapped = false;

    if (self->CreationDesc().usage == Usage::USAGE_DYNAMICDRAW)
    {
        self->updatePending = true;
    }
    else
    {
        self->updatePending = false;

        DX9Command cmd = { DX9Command::UPDATE_INDEX_BUFFER, { uint64_t(&self->buffer), uint64_t(self->mappedData.data()), self->mappedData.size() } };
        ExecDX9(&cmd, 1, true);

        DAVA::Vector<uint8> emptyVector;
        self->mappedData.swap(emptyVector);
        self->MarkRestored();
    }
}

//------------------------------------------------------------------------------

static bool
dx9_IndexBuffer_NeedRestore(Handle ib)
{
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);

    return self->NeedRestore();
}

//------------------------------------------------------------------------------

namespace IndexBufferDX9
{
void Init(uint32 maxCount)
{
    IndexBufferDX9Pool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_IndexBuffer_Create = &dx9_IndexBuffer_Create;
    dispatch->impl_IndexBuffer_Delete = &dx9_IndexBuffer_Delete;
    dispatch->impl_IndexBuffer_Update = &dx9_IndexBuffer_Update;
    dispatch->impl_IndexBuffer_Map = &dx9_IndexBuffer_Map;
    dispatch->impl_IndexBuffer_Unmap = &dx9_IndexBuffer_Unmap;
    dispatch->impl_IndexBuffer_NeedRestore = &dx9_IndexBuffer_NeedRestore;
}

void SetToRHI(Handle ib)
{
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);

    DVASSERT(!self->isMapped);
    if (self->updatePending)
    {
        void* bufferData = nullptr;
        HRESULT hr = self->buffer->Lock(0, static_cast<UINT>(self->mappedData.size()), &bufferData, 0);
        DVASSERT(SUCCEEDED(hr));

        memcpy(bufferData, self->mappedData.data(), self->mappedData.size());

        hr = self->buffer->Unlock();
        DVASSERT(SUCCEEDED(hr));

        self->updatePending = false;
    }

    HRESULT hr = _D3D9_Device->SetIndices(self->buffer);

    if (FAILED(hr))
        Logger::Error("SetIndices failed:\n%s\n", D3D9ErrorText(hr));
}

void ReleaseAll()
{
    IndexBufferDX9Pool::Lock();
    for (IndexBufferDX9Pool::Iterator b = IndexBufferDX9Pool::Begin(), b_end = IndexBufferDX9Pool::End(); b != b_end; ++b)
    {
        b->Destroy(true);
    }
    IndexBufferDX9Pool::Unlock();
}

void ReCreateAll()
{
    IndexBufferDX9Pool::ReCreateAll();
}

unsigned
NeedRestoreCount()
{
    return IndexBufferDX9Pool::PendingRestoreCount();
}
}

//==============================================================================
} // namespace rhi
