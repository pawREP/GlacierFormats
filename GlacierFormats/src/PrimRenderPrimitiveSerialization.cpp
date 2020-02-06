#include "PrimRenderPrimitiveSerialization.h"
#include "PrimRenderPrimitiveBuilder.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "Exceptions.h"

using namespace GlacierFormats;

	uint32_t RenderPrimitiveSerializer::serialize(BinaryWriter* bw, const ZRenderPrimitive* prim) {
		SPrimSubMesh submesh{};

		//Only weighted and standard are supported.
		if (prim->isWeightedMesh())
			submesh.sub_type = SPrimObject::SUBTYPE_WEIGHTED;
		else
			submesh.sub_type = SPrimObject::SUBTYPE_STANDARD;

		//TODO: Move the hard coded submesh fields into the ZRenderPrimitive builder.
		submesh.properties = prim->remnant.properties;
		submesh.unk2 = -1;
		submesh.num_uv_channels = 1;//TODO: Note multiple UV channels can now be supported when using the GLTF exchange format.

		submesh.index_buffer = bw->tell();
		submesh.num_indices = prim->index_buffer->size();
		prim->index_buffer->serialize(bw);

		submesh.vertex_buffer = bw->tell();
		submesh.num_vertex = prim->vertex_buffer->size();

		BoundingBox vertex_buffer_bb = prim->vertex_buffer->getBoundingBox();
		for (int i = 0; i < 3; ++i) {
			submesh.min[i] = vertex_buffer_bb.min[i];
			submesh.max[i] = vertex_buffer_bb.max[i];
		}
		prim->vertex_buffer->serialize(bw);

		if (prim->bone_weight_buffer)
			prim->bone_weight_buffer->serialize(bw);

		float texture_scale[2];
		float texture_bias[2];
		prim->vertex_data->serialize(bw, texture_scale, texture_bias);

		//Bone weight flags are required in weighted meshes, create dummy data if prim doesn't have flags
		if (prim->bone_flags)
			prim->bone_flags->serialize(bw);
		else if (prim->isWeightedMesh()) {
			for (int i = 0; i < 4 * prim->vertexCount(); ++i)
				bw->write<unsigned char>(0xFF);
		}

		//This alignment might be before bone flags, not sure.
		bw->align();

		if (prim->cloth_data) {
			submesh.cloth = bw->tell();
			prim->cloth_data->serialize(bw);
		}
		else {
			submesh.cloth = 0;
		}
		bw->align();

		if (prim->collision_data) {
			submesh.collision = bw->tell();
			prim->collision_data->serialize(bw);
		}
		else {
			submesh.collision = 0;
		}

		bw->align();

		uint32_t submesh_offset = static_cast<uint32_t>(bw->tell());
		bw->write(submesh);

		//submesh table
		uint32_t submesh_table_offset = static_cast<uint32_t>(bw->tell());
		bw->write(submesh_offset);
		bw->align();

		SPrimMeshWeighted prim_mesh{};
		prim_mesh.type = SPrimObjectHeader::EPrimType::PTMESH;

		if (prim->bone_info) {
			prim_mesh.bone_info = bw->tell();
			prim->bone_info->serialize(bw);
		}
		bw->align();

		if (prim->m_unk_tabl2) {
			prim_mesh.m_unk_tabl2_offset = bw->tell();
			prim->m_unk_tabl2->serialize(bw);
		}
		bw->align();

		prim_mesh.sub_mesh_table = submesh_table_offset;

		vertex_buffer_bb.getIntegerRangeCompressionParameters(prim_mesh.pos_scale, prim_mesh.pos_bias);
		prim_mesh.pos_scale[3] = 0.5; //The fourth entry in scale and bias has a hard coded value for some reason.
		prim_mesh.pos_bias[3] = 0.5;

		for (int i = 0; i < 3; ++i) {
			prim_mesh.min[i] = vertex_buffer_bb.min[i];
			prim_mesh.max[i] = vertex_buffer_bb.max[i];
		}

		for (int i = 0; i < 2; ++i) {
			prim_mesh.uv_scale[i] = texture_scale[i];
			prim_mesh.uv_bias[i] = texture_bias[i];
		}

		if (prim->isWeightedMesh())
			prim_mesh.sub_type = SPrimMeshWeighted::SUBTYPE_WEIGHTED;
		else
			prim_mesh.sub_type = SPrimMeshWeighted::SUBTYPE_STANDARD;

		prim_mesh.properties = prim->remnant.properties;
		prim_mesh.lod_mask = prim->remnant.lod_mask;
		prim_mesh.variant_id = prim->remnant.variant_id;
		prim_mesh.bias = prim->remnant.bias;
		prim_mesh.offset = prim->remnant.offset;
		prim_mesh.material_id = prim->remnant.material_id;
		prim_mesh.wire_color = prim->remnant.wire_color;
		prim_mesh.unk2 = prim->remnant.unk2;
		prim_mesh.unk17 = prim->remnant.unk17;

		//TODO: Add bone stuff
		uint32_t object_offset = static_cast<uint32_t>(bw->tell());

		if (prim->isWeightedMesh()) {
			bw->write(prim_mesh);
		}
		else {
			SPrimMesh m;
			memcpy_s(&m, sizeof(m), &prim_mesh, sizeof(m));
			bw->write(m);
		}
		bw->align();

		return object_offset;
	}


	std::unique_ptr<ZRenderPrimitive> RenderPrimitiveDeserializer::deserialize(BinaryReader* br, const SPrimObjectHeader* const prim_object_header) {
		auto object_offset = br->tell();
		auto prim_mesh = br->read<SPrimMeshWeighted>();

		GLACIER_ASSERT_TRUE(prim_mesh.draw_destination == 0);
		GLACIER_ASSERT_TRUE(prim_mesh.pack_type == 0);
		GLACIER_ASSERT_TRUE(prim_mesh.type == SPrimObjectHeader::EPrimType::PTMESH);
		GLACIER_ASSERT_TRUE(prim_mesh.m_unk_tabl1_offset == 0);
		if (prim_mesh.sub_type == SPrimSubMesh::SUBTYPE_STANDARD)
			GLACIER_ASSERT_TRUE(prim_mesh.m_unk_tabl2_offset == 0);

		br->seek(prim_mesh.sub_mesh_table);
		auto submesh_offset = br->read<uint32_t>();
		br->align();//This align will fail if a mesh has more than one submesh which I don't think is possible.

		br->seek(submesh_offset);
		auto prim_submesh = br->read<SPrimSubMesh>();

		//Throw on sub_meshes with multiple uv channels.
		//TODO: Note for later: This restriction was originally made because SMD doesn't support multiple UV channels.
		//Multiple UVs could be supported now since the switch to GLTF and FBX export. Might still not be worth it since apparently
		//only skybox meshes use this feature in the first place. That's not 100% confirmed atm though.
		if (prim_submesh.num_uv_channels != 1)
			throw UnsupportedFeatureException("Meshes with more than one UV channel are not supported.");

		ZRenderPrimitiveBuilder builder;

		//TODO: Add all the missing remnant values here
		//remnant.sub_type = prim_mesh.sub_type;
		//remnant.properties = prim_mesh.properties;
		builder.setWireColor(prim_mesh.wire_color);
		builder.setLodMask(prim_mesh.lod_mask);
		builder.setVariantId(prim_mesh.variant_id);
		//remnant.bias = prim_mesh.bias;
		//remnant.offset = prim_mesh.offset;
		builder.setMaterialId(prim_mesh.material_id);
		//remnant.unk2 = prim_mesh.unk2;
		//remnant.unk17 = prim_mesh.unk17;

		br->seek(prim_submesh.index_buffer);
		auto index_buffer = std::make_unique<IndexBuffer>(br, &prim_submesh);
		builder.setIndexBuffer(std::move(index_buffer));

		br->seek(prim_submesh.vertex_buffer);
		auto vertex_buffer = std::make_unique<VertexBuffer>(br, prim_object_header, &prim_mesh, &prim_submesh);
		builder.setVertexBuffer(std::move(vertex_buffer));

		if (prim_mesh.sub_type == SPrimSubMesh::SUBTYPE_WEIGHTED) {
			auto boneweight_buffer = std::make_unique<BoneWeightBuffer>(br, &prim_submesh);
			builder.setBoneWeightBuffer(std::move(boneweight_buffer));
		}

		auto vertex_data_buffer = std::make_unique<VertexDataBuffer>(br, &prim_mesh, &prim_submesh);
		builder.setVertexDataBuffer(std::move(vertex_data_buffer));

		if (prim_mesh.sub_type == SPrimSubMesh::SUBTYPE_WEIGHTED) {
			auto bone_flags = std::make_unique<BoneFlags>(br, &prim_submesh);
			builder.setBoneFlags(std::move(bone_flags));
		}

		//TODO: reenable cloth data parsing once the size of this struct is better understood.
		if (prim_submesh.cloth) {
			br->seek(prim_submesh.cloth);
			auto cloth_data = std::make_unique<ClothData>(br, &prim_submesh);
			builder.setClothData(std::move(cloth_data));
		}

		if (prim_submesh.collision) {
			br->seek(prim_submesh.collision);
			auto collision_buffer = std::make_unique<CollisionData>(br);
			builder.setCollisionBuffer(std::move(collision_buffer));
		}
		br->align();

		if (prim_mesh.sub_type == SPrimSubMesh::SUBTYPE_WEIGHTED && prim_mesh.bone_info) {
			br->seek(prim_mesh.bone_info);
			auto bone_info = std::make_unique<BoneInfo>(br);
			builder.setBoneInfo(std::move(bone_info));
		}
		br->align();

		if (prim_mesh.m_unk_tabl2_offset) {
			br->seek(prim_mesh.m_unk_tabl2_offset);
			auto m2 = std::make_unique<MUnkTabl2>(br);
			builder.setMUnkTabl2(std::move(m2));
		}
		br->align();

		return builder.build();
	}
