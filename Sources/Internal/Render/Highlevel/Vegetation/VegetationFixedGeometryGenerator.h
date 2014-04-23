/*==================================================================================
 Copyright (c) 2008, binaryzebra
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the binaryzebra nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 =====================================================================================*/

#ifndef __DAVAENGINE_VEGETATIONFIXEDGEOMETRYGENERATOR_H__
#define __DAVAENGINE_VEGETATIONFIXEDGEOMETRYGENERATOR_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/FastName.h"
#include "Render/RenderBase.h"
#include "Base/BaseMath.h"

#include "Render/3D/PolygonGroup.h"
#include "Render/RenderDataObject.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/Material.h"
#include "Render/Material/NMaterial.h"

#include "Render/Highlevel/Vegetation/VegetationGeometryGenerator.h"

namespace DAVA
{
class VegetationFixedGeometryGenerator : public VegetationGeometryGenerator
{
public:

    VegetationFixedGeometryGenerator(uint32 _maxDensityLevels,
                                     uint32 _maxLayerTypes,
                                     const FilePath& textureSheetPath,
                                     uint32* _resolutionCellSquare,
                                     uint32 resolutionCellSquareCount,
                                     float32* _resolutionScale,
                                     uint32 resolutionScaleCount,
                                     const Vector<Vector2>& _resolutionRanges,
                                     uint32* _resolutionTilesPerRow,
                                     uint32 resolutionTilesPerRowCount);
        
    virtual void Build(VegetationRenderData& renderData);
    
private:

    void GenerateVertices(uint32 maxClusters,
                          size_t maxTotalClusters,
                          uint32 maxClusterRowSize,
                          uint32 tilesPerRow,
                          Vector2 unitSize,
                          Vector<uint32>& layerOffsets,
                          VegetationRenderData& renderData);
    
    void GenerateIndices(uint32 maxClusters,
                         uint32 maxClusterRowSize,
                         Vector<uint32>& layerOffsets,
                         VegetationRenderData& renderData);
    
    void PrepareIndexBufferData(uint32 indexBufferIndex,
                                uint32 maxClusters,
                                uint32 maxClusterRowSize,
                                size_t resolutionIndex,
                                uint32 resolutionOffset,
                                Vector<uint32>& layerOffsets,
                                Vector<int16>& preparedIndices,
                                AABBox3& indexBufferBBox,
                                VegetationRenderData& renderData);
    
    void PrepareSortedIndexBufferVariations(size_t& currentIndexIndex,
                                            uint32 indexBufferIndex,
                                            size_t polygonElementCount,
                                            AABBox3& indexBufferBBox,
                                            Vector<Vector3>& directionPoints,
                                            Vector<Vector<SortedBufferItem> >& currentResolutionIndexArray,
                                            Vector<PolygonSortData>& sortingArray,
                                            Vector<int16>& preparedIndices,
                                            VegetationRenderData& renderData);
    
    void GenerateRenderDataObjects(VegetationRenderData& renderData);
    
private:
    
    uint32 maxDensityLevels;
    uint32 maxLayerTypes;
    TextureSheet textureSheet;
    Vector<uint32> resolutionCellSquare;
    Vector<float32> resolutionScale;
    Vector<Vector2> resolutionRanges;
    Vector<uint32> resolutionTilesPerRow;
};

};


#endif /* defined(__DAVAENGINE_VEGETATIONFIXEDGEOMETRYGENERATOR_H__) */
