#include "PrimRenderPrimitiveSerialization.h"
#include "PrimRenderPrimitiveBuilder.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "Exceptions.h"

using namespace GlacierFormats;

	uint32_t RenderPrimitiveSerializer::serialize(BinaryWriter* bw, const ZRenderPrimitive* prim) {
		SPrimSubMesh submesh{};

		submesh.properties = prim->remnant.submesh_properties;
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

		//TODO: Add dummy data again, alternativelty, add finalize() to PRIM
		//Bone weight flags are required in weighted meshes, create dummy data if prim doesn't have flags
		if (prim->vertex_colors)//TODO: Those flags are probably vertex colors, not bone related
			prim->vertex_colors->serialize(bw);
		//else if (prim->isWeightedMesh()) {
		//	for (int i = 0; i < 4 * prim->vertexCount(); ++i)
		//		bw->write<unsigned char>(0xFF);
		//}

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

		if (prim->bone_indices) {
			prim_mesh.bone_indices = bw->tell();
			prim->bone_indices->serialize(bw);
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

		prim_mesh.type = SPrimObjectHeader::EPrimType::PTMESH;
		prim_mesh.sub_type = prim->remnant.mesh_subtype;
		prim_mesh.properties = prim->remnant.mesh_properties;
		if ((int)prim_mesh.properties & (int)SPrimObject::PROPERTY_FLAGS::PROPERTY_HIRES_POSITIONS)
			(int&)prim_mesh.properties -= (int)SPrimObject::PROPERTY_FLAGS::PROPERTY_HIRES_POSITIONS;

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

		if ((prim->remnant.mesh_subtype == SPrimObject::SUBTYPE::SUBTYPE_WEIGHTED) || (prim->remnant.mesh_subtype == SPrimObject::SUBTYPE::SUBTYPE_LINKED)) {
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

std::unique_ptr<ZRenderPrimitive> RenderPrimitiveDeserializer::deserializeMesh(BinaryReader* br, const SPrimObjectHeader* const prim_object_header) {
	std::unique_ptr<SPrimMesh> prim_mesh = nullptr;
	switch (br->peek<SPrimMesh>().sub_type) {
	case SPrimObject::SUBTYPE::SUBTYPE_STANDARD:
		prim_mesh = std::make_unique<SPrimMesh>(br->read<SPrimMesh>());
		break;
	case SPrimObject::SUBTYPE::SUBTYPE_WEIGHTED:
	case SPrimObject::SUBTYPE::SUBTYPE_LINKED:
		prim_mesh = std::make_unique<SPrimMeshWeighted>(br->read<SPrimMeshWeighted>());
	}
	br->align();

	GLACIER_ASSERT_TRUE(prim_mesh->draw_destination == 0);
	GLACIER_ASSERT_TRUE(prim_mesh->pack_type == 0);
	GLACIER_ASSERT_TRUE(prim_mesh->type == SPrimObjectHeader::EPrimType::PTMESH);
	GLACIER_ASSERT_TRUE(prim_mesh->unk2 == 0);
	if (prim_mesh->m_unk_tabl1_offset)
		throw UnsupportedFeatureException("Mesh UnkTable1 not supported");

	br->seek(prim_mesh->sub_mesh_table);
	auto submesh_offset = br->read<uint32_t>();
	br->align();

	br->seek(submesh_offset);
	auto prim_submesh = br->read<SPrimSubMesh>();

	if (prim_submesh.num_uv_channels != 1)
		throw UnsupportedFeatureException("Meshes with more than one UV channel are not supported.");

	auto prim = std::make_unique<ZRenderPrimitive>();
	prim->remnant.submesh_properties = prim_submesh.properties;
	prim->remnant.mesh_properties = prim_mesh->properties;

	//TODO: Add all the missing remnant values here
	prim->remnant.wire_color = prim_mesh->wire_color;
	prim->remnant.lod_mask = prim_mesh->lod_mask;
	prim->remnant.material_id = prim_mesh->material_id;
	prim->remnant.variant_id = prim_mesh->variant_id;
	prim->remnant.mesh_subtype = prim_mesh->sub_type;

	//Index buffer
	br->seek(prim_submesh.index_buffer);
	prim->index_buffer = std::make_unique<IndexBuffer>(br, &prim_submesh);
	br->align();

	//Vertex buffer
	br->seek(prim_submesh.vertex_buffer);
	prim->vertex_buffer = std::make_unique<VertexBuffer>(br, prim_object_header, prim_mesh.get(), &prim_submesh);

	//Vertex weights
	if(prim_mesh->sub_type == SPrimObject::SUBTYPE::SUBTYPE_WEIGHTED)
		prim->bone_weight_buffer = std::make_unique<BoneWeightBuffer>(br, &prim_submesh);
	
	//Per vertex data (normals, uv, ...)
	prim->vertex_data = std::make_unique<VertexDataBuffer>(br, prim_mesh.get(), &prim_submesh);
	
	//Vertex colors
	if (prim_mesh->sub_type == SPrimObject::SUBTYPE::SUBTYPE_WEIGHTED)
		prim->vertex_colors = std::make_unique<VertexColors>(br, &prim_submesh);
	else if (((int)prim_submesh.properties & (int)SPrimObject::PROPERTY_FLAGS::PROPERTY_COLOR1) == 0)
		prim->vertex_colors = std::make_unique<VertexColors>(br, &prim_submesh);
	br->align();

	//Cloth
	if (prim_submesh.cloth) {
		br->seek(prim_submesh.cloth);
		prim->cloth_data = std::make_unique<ClothData>(br, &prim_submesh, prim_mesh.get());
	}

	//Collision
	if (prim_submesh.collision) {
		br->seek(prim_submesh.collision);
		switch (prim_mesh->sub_type) {
		case SPrimObject::SUBTYPE::SUBTYPE_STANDARD:
		case SPrimObject::SUBTYPE::SUBTYPE_WEIGHTED:
				prim->collision_data = std::make_unique<CollisionData>(br, CollisionType::STANDARD);
				break;
		case SPrimObject::SUBTYPE::SUBTYPE_LINKED:
				prim->collision_data = std::make_unique<CollisionData>(br, CollisionType::LINKED);
				break;
		}
	}
	br->align();

	//Bone indices
	if (prim_mesh->sub_type != SPrimObject::SUBTYPE::SUBTYPE_STANDARD) {
		auto prim_mesh_weigthed = reinterpret_cast<SPrimMeshWeighted*>(prim_mesh.get());
		if (prim_mesh_weigthed->bone_indices) {
			br->seek(prim_mesh_weigthed->bone_indices);
			prim->bone_indices = std::make_unique<BoneIndices>(br);
		}
		br->align();
	}

	//MUnkTabl2
	if (prim_mesh->m_unk_tabl2_offset) {
		br->seek(prim_mesh->m_unk_tabl2_offset);
		prim->m_unk_tabl2 = std::make_unique<MUnkTabl2>(br);
	}
	br->align();

	return prim;
}