
    #include "../Common/rhi_Private.h"
    #include "../Common/rhi_Pool.h"
    #include "../Common/format_convert.h"
    #include "rhi_GLES2.h"

    #include "Debug/DVAssert.h"
    #include "FileSystem/Logger.h"
    using DAVA::Logger;

    #include "_gl.h"

    #include <string.h>


namespace rhi
{
//==============================================================================

class
TextureGLES2_t
{
public:

                    TextureGLES2_t();


    unsigned        uid;
    GLuint          fbo;
    GLuint          depth;

    unsigned        width;
    unsigned        height;
    TextureFormat   format;
    void*           mappedData;
    uint32          mappedLevel;
    GLenum          mappedFace;
    uint32          isCubeMap:1;
    uint32          isRenderTarget:1;
    uint32          isMapped:1;

    SamplerState::Descriptor::Sampler   samplerState;
    uint32                              forceSetSamplerState:1;
};


TextureGLES2_t::TextureGLES2_t()
  : uid(0),
    fbo(0),
    depth(0),
    width(0),
    height(0),
    isCubeMap(false),
    isRenderTarget(false),
    mappedData(nullptr),
    isMapped(false),
    forceSetSamplerState(true)
{
}

typedef Pool<TextureGLES2_t,RESOURCE_TEXTURE>   TextureGLES2Pool;
RHI_IMPL_POOL(TextureGLES2_t,RESOURCE_TEXTURE);

//------------------------------------------------------------------------------

static void
gles2_Texture_Delete( Handle tex )
{
    if( tex != InvalidHandle )
    {
        TextureGLES2_t* self = TextureGLES2Pool::Get( tex );
        GLCommand       cmd[] =
        {
            { GLCommand::DELETE_TEXTURES, { 1, uint64(&(self->uid)) } },
            { GLCommand::DELETE_TEXTURES, { 1, uint64(&(self->depth)) } },
            { GLCommand::DELETE_FRAMEBUFFERS, { 1, uint64(&(self->fbo)) } }
        };

        ExecGL( cmd, countof(cmd) );


        DVASSERT(!self->isMapped);
        
        if( self->mappedData )
        {
            ::free( self->mappedData );
            
            self->mappedData = nullptr;
            self->width      = 0;
            self->height     = 0;
        }

        TextureGLES2Pool::Free( tex );
    }
}


//------------------------------------------------------------------------------

static Handle
gles2_Texture_Create( const Texture::Descriptor& desc )
{
    DVASSERT(desc.levelCount);

    Handle      handle = InvalidHandle;
    GLuint      uid    = InvalidIndex;
    GLCommand   cmd1   = { GLCommand::GEN_TEXTURES, { 1, (uint64)(&uid) } };

    ExecGL( &cmd1, 1 );
    if( cmd1.status == GL_NO_ERROR )
    {
        GLenum      target = (desc.type == TEXTURE_TYPE_CUBE)  ? GL_TEXTURE_CUBE_MAP  : GL_TEXTURE_2D;
        GLCommand   cmd2[] =
        {
            { GLCommand::BIND_TEXTURE, { target, uid } },
            { (desc.autoGenMipmaps) ? GLCommand::GENERATE_MIPMAP : GLCommand::NOP, { } },
//            #if !defined __DAVAENGINE_IPHONE__
//            { GLCommand::TEX_PARAMETER_I, { target, GL_TEXTURE_BASE_LEVEL, 0 } },
//            { GLCommand::TEX_PARAMETER_I, { target, GL_TEXTURE_MAX_LEVEL, 0 } },
//            #endif
            { GLCommand::BIND_TEXTURE, { target, 0 } }
        };

        ExecGL( cmd2, countof(cmd2) );
    }

    if( uid != InvalidIndex )
    {
        TextureGLES2_t* self = nullptr;
        
        handle = TextureGLES2Pool::Alloc();
        self   = TextureGLES2Pool::Get( handle );

        self->uid                   = uid;
        self->mappedData            = nullptr;
        self->width                 = desc.width;
        self->height                = desc.height;
        self->format                = desc.format;
        self->isCubeMap             = desc.type == TEXTURE_TYPE_CUBE;
        self->isMapped              = false;
        self->forceSetSamplerState  = true;
        
        if( desc.isRenderTarget )
        {
            GLuint      fbo    = 0;
            GLuint      depth  = 0;
            GLCommand   cmd3[] = 
            {
                { GLCommand::GEN_FRAMEBUFFERS, { 1, (uint64)(&fbo) } },
                { GLCommand::GEN_TEXTURES, { 1, (uint64)(&depth) } } 
            };
            
            ExecGL( cmd3, countof(cmd3) );

            if( fbo  &&  depth )
            {
                GLenum      b[1]   = { GL_COLOR_ATTACHMENT0 };
                GLCommand   cmd4[] =
                {
                    { GLCommand::BIND_TEXTURE, { GL_TEXTURE_2D, uid } },
                    { GLCommand::TEX_IMAGE2D, { GL_TEXTURE_2D, 0, GL_RGBA, desc.width, desc.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0 } },
                    { GLCommand::TEX_PARAMETER_I, { GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST } },
                    { GLCommand::TEX_PARAMETER_I, { GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST } },
                    { GLCommand::BIND_TEXTURE, { GL_TEXTURE_2D, 0 } },

                    { GLCommand::BIND_TEXTURE, { GL_TEXTURE_2D, depth } },
                    { GLCommand::TEX_IMAGE2D, { GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, desc.width, desc.height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL } },
                    { GLCommand::BIND_TEXTURE, { GL_TEXTURE_2D, 0 } },

                    { GLCommand::BIND_FRAMEBUFFER, { GL_FRAMEBUFFER, fbo } },                    
                    { GLCommand::FRAMEBUFFER_TEXTURE, { GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, uid, 0 } },                    
                    { GLCommand::FRAMEBUFFER_TEXTURE, { GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0 } },                    
                    { GLCommand::DRAWBUFFERS, { 1, (uint64)b } },
                    { GLCommand::FRAMEBUFFER_STATUS, { GL_FRAMEBUFFER } },
                    { GLCommand::BIND_FRAMEBUFFER, { GL_FRAMEBUFFER, 0 } }
                };

                ExecGL( cmd4, countof(cmd4) );

                if( cmd4[ countof(cmd4)-2 ].retval == GL_FRAMEBUFFER_COMPLETE )
                {
                    self->fbo            = fbo;
                    self->depth          = depth;
                    self->isRenderTarget = true;
                }
                else
                {
/*
GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
GL_FRAMEBUFFER_UNSUPPORTED
*/
                    Logger::Error( "glCheckFramebufferStatus= %08X", cmd4[ countof(cmd4)-2 ].retval );
                    DVASSERT(cmd4[ countof(cmd4)-2 ].retval == GL_FRAMEBUFFER_COMPLETE);
                }
            }
        }
    }

    return handle;
}


//------------------------------------------------------------------------------

static void*
gles2_Texture_Map( Handle tex, unsigned level, TextureFace face )
{
    TextureGLES2_t* self = TextureGLES2Pool::Get( tex );
    void*              mem = nullptr;
    uint32 textureDataSize = TextureSize(self->format, self->width, self->height, level);

    DVASSERT(!self->isMapped);
    self->mappedData = ::realloc( self->mappedData, textureDataSize );
    if( self->mappedData )
    {
        if( self->isRenderTarget )
        {
            DVASSERT(level==0);
            GLenum      target = (self->isCubeMap)  ? GL_TEXTURE_CUBE_MAP  : GL_TEXTURE_2D;
            GLCommand   cmd[]  =
            {
                { GLCommand::BIND_FRAMEBUFFER, {GL_FRAMEBUFFER, self->fbo} },
                { GLCommand::READ_PIXELS, {0,0,self->width,self->height,GL_RGBA,GL_UNSIGNED_BYTE,uint64(self->mappedData)} },
                { GLCommand::BIND_FRAMEBUFFER, {GL_FRAMEBUFFER, 0} },
            };

            ExecGL( cmd, countof(cmd) );            
            
            mem               = self->mappedData;
            self->mappedLevel = 0;
            self->isMapped    = true;
        }
        else
        {
            mem               = self->mappedData;
            self->mappedLevel = level;
            self->isMapped    = true;

            switch( face )
            {
                case TEXTURE_FACE_POSITIVE_X: self->mappedFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X; break;
                case TEXTURE_FACE_NEGATIVE_X: self->mappedFace = GL_TEXTURE_CUBE_MAP_NEGATIVE_X; break;
                case TEXTURE_FACE_POSITIVE_Y: self->mappedFace = GL_TEXTURE_CUBE_MAP_POSITIVE_Y; break;
                case TEXTURE_FACE_NEGATIVE_Y: self->mappedFace = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y; break;
                case TEXTURE_FACE_POSITIVE_Z: self->mappedFace = GL_TEXTURE_CUBE_MAP_POSITIVE_Z; break;
                case TEXTURE_FACE_NEGATIVE_Z: self->mappedFace = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; break;
            }
        }
    }
    
    if( self->format == TEXTURE_FORMAT_R4G4B4A4 )
    {
        _FlipRGBA4_ABGR4( self->mappedData, textureDataSize );
    }
    else if (self->format == TEXTURE_FORMAT_R5G5B5A1)
    {
        _ABGR1555toRGBA5551(self->mappedData, textureDataSize);
    }

    return mem;
}


//------------------------------------------------------------------------------

static void
gles2_Texture_Unmap( Handle tex )
{
    TextureGLES2_t* self = TextureGLES2Pool::Get( tex );
    Size2i          sz   = TextureExtents( Size2i(self->width,self->height), self->mappedLevel );
    GLint           int_fmt;
    GLint           fmt;
    GLenum          type;
    bool            compressed;
    
    uint32 textureDataSize = TextureSize(self->format, sz.dx, sz.dy);
    GetGLTextureFormat( self->format, &int_fmt, &fmt, &type, &compressed );

    DVASSERT(self->isMapped);
    if( self->format == TEXTURE_FORMAT_R4G4B4A4 )
    {
        _FlipRGBA4_ABGR4( self->mappedData, textureDataSize );
    }
    else if (self->format == TEXTURE_FORMAT_R5G5B5A1)
    {
        _RGBA5551toABGR1555(self->mappedData, textureDataSize);
    }

    GLenum      target = (self->isCubeMap)  ? GL_TEXTURE_CUBE_MAP  : GL_TEXTURE_2D;
    GLCommand   cmd[]  =
    {
        { GLCommand::BIND_TEXTURE, {target, self->uid} },
        { GLCommand::TEX_IMAGE2D, {target, self->mappedLevel, uint64(int_fmt), uint64(sz.dx), uint64(sz.dy), 0, uint64(fmt), type, uint64(textureDataSize), (uint64)(self->mappedData), compressed} },
        { GLCommand::BIND_TEXTURE, {target, 0} }
    };

    ExecGL( cmd, countof(cmd) );

    self->isMapped = false;
}


//------------------------------------------------------------------------------

void
gles2_Texture_Update( Handle tex, const void* data, uint32 level, TextureFace face )
{
    TextureGLES2_t* self = TextureGLES2Pool::Get( tex );
    Size2i          sz   = TextureExtents( Size2i(self->width,self->height), level );
    GLint           int_fmt;
    GLint           fmt;
    GLenum          type;
    bool            compressed;
    
    uint32 textureDataSize = TextureSize(self->format, sz.dx, sz.dy);
    GetGLTextureFormat( self->format, &int_fmt, &fmt, &type, &compressed);
    
    DVASSERT(!self->isMapped);
    if (self->format == TEXTURE_FORMAT_R4G4B4A4 || self->format == TEXTURE_FORMAT_R5G5B5A1)
    {
        gles2_Texture_Map( tex, level, face );
        memcpy( self->mappedData, data, textureDataSize );
        gles2_Texture_Unmap( tex );
    }
    else
    {
        GLenum      ttarget = (self->isCubeMap)  ? GL_TEXTURE_CUBE_MAP  : GL_TEXTURE_2D;
        GLenum      target  = GL_TEXTURE_2D;
        
        if( self->isCubeMap )
        {
            switch( face )
            {
                case TEXTURE_FACE_POSITIVE_X: target = GL_TEXTURE_CUBE_MAP_POSITIVE_X; break;
                case TEXTURE_FACE_NEGATIVE_X: target = GL_TEXTURE_CUBE_MAP_NEGATIVE_X; break;
                case TEXTURE_FACE_POSITIVE_Y: target = GL_TEXTURE_CUBE_MAP_POSITIVE_Y; break;
                case TEXTURE_FACE_NEGATIVE_Y: target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y; break;
                case TEXTURE_FACE_POSITIVE_Z: target = GL_TEXTURE_CUBE_MAP_POSITIVE_Z; break;
                case TEXTURE_FACE_NEGATIVE_Z: target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; break;
            }
        }

        GLCommand   cmd[]  =
        {
            { GLCommand::BIND_TEXTURE, {ttarget, self->uid} },
            { GLCommand::TEX_IMAGE2D, {target, uint64(level), uint64(int_fmt), uint64(sz.dx), uint64(sz.dy), 0, uint64(fmt), type, uint64(textureDataSize), (uint64)(data), compressed} },
            { GLCommand::BIND_TEXTURE, {ttarget, 0} }
        };

        ExecGL( cmd, countof(cmd) );
    }
}



//==============================================================================

struct
SamplerStateGLES2_t
{
    SamplerState::Descriptor::Sampler   fragmentSampler[MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT];
    uint32                              fragmentSamplerCount;
    SamplerState::Descriptor::Sampler   vertexSampler[MAX_VERTEX_TEXTURE_SAMPLER_COUNT];
    uint32                              vertexSamplerCount;
};

typedef Pool<SamplerStateGLES2_t,RESOURCE_SAMPLER_STATE>    SamplerStateGLES2Pool;
RHI_IMPL_POOL(SamplerStateGLES2_t,RESOURCE_SAMPLER_STATE);
static const SamplerStateGLES2_t*                           _CurSamplerState = nullptr;


//------------------------------------------------------------------------------

static Handle
gles2_SamplerState_Create( const SamplerState::Descriptor& desc )
{
    Handle                  handle = SamplerStateGLES2Pool::Alloc();
    SamplerStateGLES2_t*    state  = SamplerStateGLES2Pool::Get( handle );
    
    state->fragmentSamplerCount = desc.fragmentSamplerCount;
    for( unsigned i=0; i!=desc.fragmentSamplerCount; ++i )    
    {
        state->fragmentSampler[i] = desc.fragmentSampler[i];
    }
    
    state->vertexSamplerCount = desc.vertexSamplerCount;
    for( unsigned i=0; i!=desc.vertexSamplerCount; ++i )    
    {
        state->vertexSampler[i] = desc.vertexSampler[i];
    }
    
    // force no-filtering on vertex-textures
    for( uint32 s=0; s!=MAX_VERTEX_TEXTURE_SAMPLER_COUNT; ++s )
    {
        state->vertexSampler[s].minFilter = TEXFILTER_NEAREST;
        state->vertexSampler[s].magFilter = TEXFILTER_NEAREST;
        state->vertexSampler[s].mipFilter = TEXMIPFILTER_NONE;
    }

    return handle;
}


//------------------------------------------------------------------------------

static void
gles2_SamplerState_Delete( Handle hstate )
{
    SamplerStateGLES2_t*    state  = SamplerStateGLES2Pool::Get( hstate );
    
    if( _CurSamplerState == state )
        _CurSamplerState = nullptr;

    SamplerStateGLES2Pool::Free( hstate );
}


//==============================================================================

namespace SamplerStateGLES2
{

void
SetupDispatch( Dispatch* dispatch )
{
    dispatch->impl_SamplerState_Create = &gles2_SamplerState_Create;
    dispatch->impl_SamplerState_Delete = &gles2_SamplerState_Delete;
}

void
SetToRHI( Handle hstate )
{
    SamplerStateGLES2_t*    state  = SamplerStateGLES2Pool::Get( hstate );

    _CurSamplerState = state;
}

}


//==============================================================================

static GLenum
_TextureFilter( TextureFilter filter )
{
    GLenum   f = GL_LINEAR;

    switch( filter )
    {
        case TEXFILTER_NEAREST  : f = GL_NEAREST; break;
        case TEXFILTER_LINEAR   : f = GL_LINEAR; break;
    }

    return f;
}


//------------------------------------------------------------------------------

static GLenum
_TextureMipFilter( TextureMipFilter filter )
{
    GLenum   f = GL_LINEAR_MIPMAP_LINEAR;
    
    switch( filter )
    {
        case TEXMIPFILTER_NONE    : f = GL_NEAREST_MIPMAP_NEAREST; break;
        case TEXMIPFILTER_NEAREST : f = GL_LINEAR_MIPMAP_NEAREST; break;
        case TEXMIPFILTER_LINEAR  : f = GL_LINEAR_MIPMAP_LINEAR; break;
    }
    
    return f;
}


//------------------------------------------------------------------------------

static GLenum
_AddrMode( TextureAddrMode mode )
{
    GLenum   m = GL_REPEAT;
    
    switch( mode )
    {
        case TEXADDR_WRAP   : m = GL_REPEAT; break;
        case TEXADDR_CLAMP  : m = GL_CLAMP_TO_EDGE; break;
        case TEXADDR_MIRROR : m = GL_MIRRORED_REPEAT; break;
    }
    
    return m;
}



//==============================================================================

namespace TextureGLES2
{ 

void
SetupDispatch( Dispatch* dispatch )
{
    dispatch->impl_Texture_Create = &gles2_Texture_Create;
    dispatch->impl_Texture_Delete = &gles2_Texture_Delete;
    dispatch->impl_Texture_Map    = &gles2_Texture_Map;
    dispatch->impl_Texture_Unmap  = &gles2_Texture_Unmap;
    dispatch->impl_Texture_Update = &gles2_Texture_Update;
}

void
SetToRHI( Handle tex, unsigned unit_i, uint32 base_i )
{
    TextureGLES2_t* self        = TextureGLES2Pool::Get( tex );
    bool            fragment    = base_i != InvalidIndex;
    uint32          sampler_i   = (base_i == InvalidIndex)  ? unit_i  :  base_i+unit_i;
    GLenum          target      = (self->isCubeMap)  ? GL_TEXTURE_CUBE_MAP  : GL_TEXTURE_2D;

    const SamplerState::Descriptor::Sampler* sampler = (fragment) 
                                                        ? _CurSamplerState->fragmentSampler + unit_i
                                                        : _CurSamplerState->vertexSampler + unit_i;

    GL_CALL(glActiveTexture( GL_TEXTURE0+sampler_i ));
    GL_CALL(glBindTexture( target, self->uid ));

    if(     _CurSamplerState
        &&  (self->forceSetSamplerState  ||  memcmp( &(self->samplerState), sampler, sizeof(rhi::SamplerState::Descriptor::Sampler)) )
       )
    {
        if( sampler->mipFilter != TEXMIPFILTER_NONE )
        {
            GL_CALL(glTexParameteri( target, GL_TEXTURE_MIN_FILTER, _TextureMipFilter( TextureMipFilter(sampler->mipFilter) ) ));
        }
        else
        {
            GL_CALL(glTexParameteri( target, GL_TEXTURE_MIN_FILTER, _TextureFilter( TextureFilter(sampler->minFilter) ) ));
        }
        
        GL_CALL(glTexParameteri( target, GL_TEXTURE_MAG_FILTER, _TextureFilter( TextureFilter(sampler->magFilter) ) ));

        GL_CALL(glTexParameteri( target, GL_TEXTURE_WRAP_S, _AddrMode( TextureAddrMode(sampler->addrU) ) ));
        GL_CALL(glTexParameteri( target, GL_TEXTURE_WRAP_T, _AddrMode( TextureAddrMode(sampler->addrV) ) ));
        
        self->samplerState          = *sampler;
        self->forceSetSamplerState  = false;
    }
}

void
SetAsRenderTarget( Handle tex )
{
    TextureGLES2_t* self = TextureGLES2Pool::Get( tex );

    glBindFramebuffer( GL_FRAMEBUFFER, self->fbo );
    glViewport( 0, 0, self->width, self->height );
    _GLES2_Binded_FrameBuffer = self->fbo;
}


} // namespace TextureGLES2

//==============================================================================
} // namespace rhi

