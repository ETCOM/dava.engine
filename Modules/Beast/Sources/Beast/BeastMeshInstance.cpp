#ifdef __DAVAENGINE_BEAST__

#include "BeastMeshInstance.h"
#include "BeastDebug.h"
#include "BeastManager.h"
#include "BeastMesh.h"
#include "SceneParser.h"

BeastMeshInstance::BeastMeshInstance(const DAVA::String& name, BeastManager* manager)
    : BeastResource(name, manager)
{
}

void BeastMeshInstance::InitWithRenderBatchAndTransform(DAVA::RenderBatch* batch, DAVA::int32 _partIndex, const DAVA::Matrix4& transform)
{
    if (nullptr == meshInstanceHandle)
    {
        renderBatch = batch;
        partIndex = _partIndex;
        buddyMesh = BeastMesh::FindResourceByName(resourceName);

        DVASSERT(buddyMesh);

        DAVA_BEAST::ILBMatrix4x4 beastMatrix = ConvertDavaMatrix(transform);
        BEAST_VERIFY(DAVA_BEAST::ILBCreateInstance(manager->GetILBScene(), buddyMesh->GetILBMesh(), STRING_TO_BEAST_STRING(resourceName), &beastMatrix, &meshInstanceHandle));
        BEAST_VERIFY(DAVA_BEAST::ILBSetRenderStats(meshInstanceHandle, DAVA_BEAST::ILB_RS_DOUBLE_SIDED, DAVA_BEAST::ILB_RSOP_ENABLE));
        GetCastReceiveShadowOptions(renderBatch);
    }
}

void BeastMeshInstance::InitWithLandscape(DAVA::Landscape* landscape, BeastMesh* beastMesh)
{
    if (nullptr == meshInstanceHandle)
    {
        isLandscape = true;
        renderBatch = landscape->GetRenderBatch(0);
        buddyMesh = beastMesh;

        DAVA_BEAST::ILBMatrix4x4 beastMatrix = ConvertDavaMatrix(DAVA::Matrix4::IDENTITY);
        BEAST_VERIFY(DAVA_BEAST::ILBCreateInstance(manager->GetILBScene(), beastMesh->GetILBMesh(), STRING_TO_BEAST_STRING(resourceName), &beastMatrix, &meshInstanceHandle));
        BEAST_VERIFY(DAVA_BEAST::ILBSetRenderStats(meshInstanceHandle, DAVA_BEAST::ILB_RS_DOUBLE_SIDED, DAVA_BEAST::ILB_RSOP_ENABLE));
        GetCastReceiveShadowOptions(renderBatch);
    }
}

void BeastMeshInstance::GetCastReceiveShadowOptions(DAVA::RenderBatch* batch)
{
    DAVA::NMaterial* material = batch->GetMaterial();

    DAVA_BEAST::int32 flags = 0;
    if (!material->GetEffectiveFlagValue(DAVA::NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_CASTER))
    {
        flags |= DAVA_BEAST::ILB_RS_CAST_SHADOWS | DAVA_BEAST::ILB_RS_VISIBLE_FOR_GI;
    }

    if (!material->GetEffectiveFlagValue(DAVA::NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_RECEIVER))
    {
        flags |= DAVA_BEAST::ILB_RS_RECEIVE_SHADOWS;
    }

    if (flags > 0)
    {
        // ** DISABLE ** selected flags, using ILB_RSOP_DISABLE
        BEAST_VERIFY(DAVA_BEAST::ILBSetRenderStats(meshInstanceHandle, flags, DAVA_BEAST::ILB_RSOP_DISABLE));
    }
}

DAVA_BEAST::ILBInstanceHandle BeastMeshInstance::GetILBMeshInstance()
{
    return meshInstanceHandle;
}

DAVA::int32 BeastMeshInstance::GetTextureCoordCount()
{
    return (buddyMesh == nullptr) ? 1 : buddyMesh->GetTextureCoordCount();
}

TextureTarget* BeastMeshInstance::GetTextureTarget()
{
    return textureTarget;
}

void BeastMeshInstance::SetTextureTarget(TextureTarget* val)
{
    textureTarget = val;
}

BeastMeshInstance::~BeastMeshInstance()
{
    DAVA::SafeDelete(textureTarget);
}

BeastMesh* BeastMeshInstance::GetBuddyMesh()
{
    return buddyMesh;
}

DAVA_BEAST::int32 BeastMeshInstance::GetLightmapSize()
{
    return lightmapSize;
}

void BeastMeshInstance::SetLightmapSize(DAVA_BEAST::int32 size)
{
    lightmapSize = size;
}

void BeastMeshInstance::SetLodLevel(DAVA_BEAST::int32 _lodLevel)
{
    lodLevel = _lodLevel;
}

DAVA_BEAST::int32 BeastMeshInstance::GetLodLevel()
{
    return lodLevel;
}

void BeastMeshInstance::UseLightMap()
{
    useLightmap = true;
}

bool BeastMeshInstance::IsUseLightmap()
{
    return useLightmap;
}

DAVA::String BeastMeshInstance::GetMeshInstancePointerString()
{
    return DAVA::String(DAVA::Format("%s", BeastMesh::PointerToString(renderBatch).c_str()));
}

DAVA::RenderBatch* BeastMeshInstance::GetRenderBatch()
{
    return renderBatch;
}

#endif //__DAVAENGINE_BEAST__
