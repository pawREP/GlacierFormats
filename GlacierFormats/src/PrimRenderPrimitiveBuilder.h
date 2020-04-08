#pragma once
#include <vector>
#include "PrimSerializationTypes.h"
#include "PrimRenderPrimitive.h"

namespace GlacierFormats {

	class ZRenderPrimitiveBuilder {
	private:
		std::unique_ptr<ZRenderPrimitive> prim;

		void validate() const;
	public:
		ZRenderPrimitiveBuilder();
		ZRenderPrimitiveBuilder(std::unique_ptr<ZRenderPrimitive> prim);

		void initilizeFromIMesh(const IMesh* mesh);

		//TODO:Add move variants
		void setVertexBuffer(const std::vector<float>& vertices);
		void setIndexBuffer(const std::vector<unsigned short>& indices);
		void setUvBuffer(const std::vector<float>& uvs);
		void setNormalBuffer(const std::vector<float>& normals);
		void setTangentBuffer(const std::vector<float>& tangents);

		void setVertexBuffer(std::unique_ptr<VertexBuffer> vertex_buffer);
		void setIndexBuffer(std::unique_ptr<IndexBuffer> index_buffer);
		void setVertexDataBuffer(std::unique_ptr<VertexDataBuffer> vertex_data_buffer);
		void setBoneWeightBuffer(std::unique_ptr<VertexWeightBuffer> boneweight_buffer);
		void setBoneFlags(std::unique_ptr<VertexColors> vertex_colors);
		void setCollisionBuffer(std::unique_ptr<CollisionData> collision);
		void setClothData(std::unique_ptr<ClothData> collision);
		void setBoneInfo(std::unique_ptr<BoneInfo> bone_info);
		void setBoneIndices(std::unique_ptr<BoneIndices> m2);

		//TODO: ONly sets submesh properties. 
		void setPropertyFlags(SPrimObject::PROPERTY_FLAGS flags);
		void setLodMask(unsigned char lod_mask);
		void setVariantId(char variant_id);
		void setMaterialId(short material_id);
		void setColor1(uint32_t color1);
		void setMeshSubtype(SPrimObject::SUBTYPE subtype);

		std::unique_ptr<ZRenderPrimitive> build();
	};

}
//TODO: Inherit from IMesh for consistency?