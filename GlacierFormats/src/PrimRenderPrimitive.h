#pragma once
#include "PrimSerializationTypes.h"
#include "Vector.h"
#include "PrimVertexBuffer.h"
#include "PrimIndexBuffer.h"
#include "PrimCollisionData.h"
#include "PrimBoundingBox.h"
#include "PrimClothData.h"
#include "PrimCopyBones.h"
#include "PrimVertexDataBuffer.h"
#include "BoneWeightBuffer.h"
#include "PrimVertexColors.h"
#include "PrimBoneInfo.h"
#include "PrimBoneIndices.h"
#include "IMesh.h"
#include "IRig.h"
#include "GlacierTypes.h"
#include <vector>
#include <memory>

namespace GlacierFormats {

	class ZRenderPrimitive : public IMesh {
	public:
		ZRenderPrimitive();

		friend class ZRenderPrimitiveBuilder;
		friend class RenderPrimitiveSerializer;

		struct {
			SPrimObject::SUBTYPE mesh_subtype = SPrimObject::SUBTYPE::SUBTYPE_STANDARD;
			unsigned char lod_mask = 0xFF;
			char variant_id = 0;
			char bias = 0;
			char offset = 0;
			short material_id = 0;

			SPrimObject::PROPERTY_FLAGS submesh_properties = SPrimObject::PROPERTY_FLAGS::NONE;
			int submesh_color1 = 0;
		} remnant;

		std::unique_ptr<VertexBuffer> vertex_buffer;
		std::unique_ptr<IndexBuffer> index_buffer;
		std::unique_ptr<VertexDataBuffer> vertex_data;
		std::unique_ptr<VertexWeightBuffer> bone_weight_buffer;
		std::unique_ptr<VertexColors> vertex_colors;
		std::unique_ptr<CollisionData> collision_data;
		std::unique_ptr<ClothData> cloth_data;
		std::unique_ptr<CopyBones> copy_bones;
		std::unique_ptr<BoneInfo> bone_info;
		std::unique_ptr<BoneIndices> bone_indices;

		ZRenderPrimitive(ZRenderPrimitive&&);

		uint32_t serialize(BinaryWriter* bw, std::unordered_map<RecordKey, uint64_t>& ) const;

		[[nodiscard]] bool isWeightedMesh() const;

		//IMesh interface functions
		[[nodiscard]] std::string name() const noexcept override final;
		[[nodiscard]] int materialId() const noexcept override final;
		[[nodiscard]] size_t vertexCount() const override final;
		[[nodiscard]] std::vector<float> getVertexBuffer() const override final;
		[[nodiscard]] std::vector<unsigned short> getIndexBuffer() const override final;
		[[nodiscard]] std::vector<float> getNormals() const override final;
		[[nodiscard]] std::vector<float> getTangents() const override final;
		[[nodiscard]] std::vector<float> getUVs() const override final;
		[[nodiscard]] std::vector<IMesh::VertexWeight> getBoneWeights() const override final;

		void setVertexBuffer(const std::vector<float>&) override final;
		void setIndexBuffer(const std::vector<unsigned short>&) override final;
		void setNormals(const std::vector<float>&) override final;
		void setTangents(const std::vector<float>&) override final;
		void setUVs(const std::vector<float>&) override final;
		void setBoneWeight(const std::vector<VertexWeight>&) override final;
		void setName(const std::string& name) override final;
	};


}