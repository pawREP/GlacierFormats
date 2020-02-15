#pragma once
#include "PrimSerializationTypes.h"
#include "Vector.h"
#include "PrimVertexBuffer.h"
#include "PrimIndexBuffer.h"
#include "PrimCollisionData.h"
#include "PrimBoundingBox.h"
#include "PrimClothData.h"
#include "PrimVertexDataBuffer.h"
#include "BoneWeightBuffer.h"
#include "PrimBoneFlags.h"
#include "PrimBoneInfo.h"
#include "PrimMUnkTabl2.h"
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

		//TODO: remnant should be exported into a manifest or embeded into the expoted mesh. Current implementation relys on reading 
		//some of the remnant data from the ResourceRepo, or the props are guessed.
		struct {
			SPrimObject::SUBTYPE mesh_subtype = static_cast<SPrimObject::SUBTYPE>(0);
			SPrimObject::PROPERTY_FLAGS submesh_properties = static_cast<SPrimObject::PROPERTY_FLAGS>(0);
			SPrimObject::PROPERTY_FLAGS mesh_properties = static_cast<SPrimObject::PROPERTY_FLAGS>(0);
			//For exporting the only relevant property is PROPERTY_COLOR1. Which has to be set on export for meshes that
			//natively use this property. Otherwise meshes will appear in-game in a weird purple color. 

			unsigned char lod_mask = 1;
			//Mask that indicates lod range for submesh model, one bit per lod range. Higher bits indicate farther draw distance.
			//0 invis
			//1 shorter than cam range vis, 
			//2 visible between 0.5-3 cam ranges.

			char variant_id = 0;
			char bias = 0;
			char offset = 0;
			short material_id = 0;
			int wire_color = 0;
			int debug_color = 0;
			float unk17 = 0;
		} remnant;

		std::unique_ptr<VertexBuffer> vertex_buffer;
		std::unique_ptr<IndexBuffer> index_buffer;
		std::unique_ptr<VertexDataBuffer> vertex_data;
		std::unique_ptr<BoneWeightBuffer> bone_weight_buffer;
		std::unique_ptr<VertexColors> vertex_colors;
		std::unique_ptr<CollisionData> collision_data;
		std::unique_ptr<ClothData> cloth_data;
		std::unique_ptr<BoneIndices> bone_indices;
		std::unique_ptr<MUnkTabl2> m_unk_tabl2;

		ZRenderPrimitive(ZRenderPrimitive&&);

		uint32_t serialize(BinaryWriter* bw) const;

		[[nodiscard]] bool isWeightedMesh() const;
		[[nodiscard]] std::string name(RuntimeId prim_id) const noexcept;

		//IMesh interface functions
		[[nodiscard]] std::string name() const noexcept override final;
		[[nodiscard]] int materialId() const noexcept override final;
		[[nodiscard]] size_t vertexCount() const override final;
		[[nodiscard]] std::vector<float> getVertexBuffer() const override final;
		[[nodiscard]] std::vector<unsigned short> getIndexBuffer() const override final;
		[[nodiscard]] std::vector<float> getNormals() const override final;
		[[nodiscard]] std::vector<float> getTangents() const override final;
		[[nodiscard]] std::vector<float> getUVs() const override final;
		[[nodiscard]] std::vector<IMesh::BoneWeight> getBoneWeights() const override final;

		void setVertexBuffer(const std::vector<float>&) override final;
		void setIndexBuffer(const std::vector<unsigned short>&) override final;
		void setNormals(const std::vector<float>&) override final;
		void setTangents(const std::vector<float>&) override final;
		void setUVs(const std::vector<float>&) override final;
		void setBoneWeight(const std::vector<BoneWeight>&) override final;
		void setName(const std::string& name) override final;
	};


}