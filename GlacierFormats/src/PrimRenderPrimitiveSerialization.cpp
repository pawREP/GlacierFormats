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

	//TODO: It might be better to factor parsing for different mesh subtypes into seperate functions, like in the original code. Duplicate code/readabiliy tradoff.
	std::unique_ptr<ZRenderPrimitive> RenderPrimitiveDeserializer::deserializeStandardMesh(BinaryReader* br, const SPrimObjectHeader* const prim_object_header) {
		auto prim_mesh = br->read<SPrimMesh>();

		GLACIER_ASSERT_TRUE(prim_mesh.draw_destination == 0);
		GLACIER_ASSERT_TRUE(prim_mesh.pack_type == 0);
		GLACIER_ASSERT_TRUE(prim_mesh.type == SPrimObjectHeader::EPrimType::PTMESH);
		GLACIER_ASSERT_TRUE(prim_mesh.m_unk_tabl1_offset == 0);
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

		auto prim = std::make_unique<ZRenderPrimitive>();

		//TODO: Add all the missing remnant values here
		prim->remnant.wire_color = prim_mesh.wire_color;
		prim->remnant.lod_mask = prim_mesh.lod_mask;
		prim->remnant.material_id = prim_mesh.material_id;
		prim->remnant.variant_id = prim_mesh.variant_id;

		br->seek(prim_submesh.index_buffer);
		prim->index_buffer = std::make_unique<IndexBuffer>(br, &prim_submesh);

		br->seek(prim_submesh.vertex_buffer);
		prim->vertex_buffer = std::make_unique<VertexBuffer>(br, prim_object_header, &prim_mesh, &prim_submesh);
		prim->vertex_data = std::make_unique<VertexDataBuffer>(br, &prim_mesh, &prim_submesh);

		if (prim_submesh.cloth) {
			br->seek(prim_submesh.cloth);
			prim->cloth_data = std::make_unique<ClothData>(br, &prim_submesh);
		}

		if (prim_submesh.collision) {
			br->seek(prim_submesh.collision);
			prim->collision_data = std::make_unique<CollisionData>(br, CollisionType::STANDARD);
		}
		br->align();

		return prim;
	}


std::unique_ptr<ZRenderPrimitive> RenderPrimitiveDeserializer::deserializeWeightedMesh(BinaryReader* br, const SPrimObjectHeader* const prim_object_header) {
	auto object_offset = br->tell();
	auto prim_mesh = br->read<SPrimMeshWeighted>();
	br->align();

	GLACIER_ASSERT_TRUE(prim_mesh.draw_destination == 0);
	GLACIER_ASSERT_TRUE(prim_mesh.pack_type == 0);
	GLACIER_ASSERT_TRUE(prim_mesh.type == SPrimObjectHeader::EPrimType::PTMESH);
	GLACIER_ASSERT_TRUE(prim_mesh.m_unk_tabl1_offset == 0);

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

	auto prim = std::make_unique<ZRenderPrimitive>();

	//TODO: Add all the missing remnant values here
	prim->remnant.wire_color = prim_mesh.wire_color;
	prim->remnant.lod_mask = prim_mesh.lod_mask;
	prim->remnant.material_id = prim_mesh.material_id;
	prim->remnant.variant_id = prim_mesh.variant_id;

	br->seek(prim_submesh.index_buffer);
	prim->index_buffer = std::make_unique<IndexBuffer>(br, &prim_submesh);

	br->seek(prim_submesh.vertex_buffer);
	prim->vertex_buffer = std::make_unique<VertexBuffer>(br, prim_object_header, &prim_mesh, &prim_submesh);
	prim->bone_weight_buffer = std::make_unique<BoneWeightBuffer>(br, &prim_submesh);
	prim->vertex_data = std::make_unique<VertexDataBuffer>(br, &prim_mesh, &prim_submesh);
	prim->bone_flags = std::make_unique<BoneFlags>(br, &prim_submesh);

	if (prim_submesh.cloth) {
		br->seek(prim_submesh.cloth);
		prim->cloth_data = std::make_unique<ClothData>(br, &prim_submesh);
	}

	if (prim_submesh.collision) {
		br->seek(prim_submesh.collision);
		prim->collision_data = std::make_unique<CollisionData>(br, CollisionType::WEIGHTED);
	}
	br->align();

	if (prim_mesh.bone_indices) {
		br->seek(prim_mesh.bone_indices);
		prim->bone_indices = std::make_unique<BoneIndices>(br);
	}
	br->align();

	if (prim_mesh.m_unk_tabl2_offset) {
		br->seek(prim_mesh.m_unk_tabl2_offset);
		prim->m_unk_tabl2 = std::make_unique<MUnkTabl2>(br);
	}
	br->align();

	return prim;
}

std::unique_ptr<ZRenderPrimitive> RenderPrimitiveDeserializer::deserializeLinkedMesh(BinaryReader* br, const SPrimObjectHeader* const prim_object_header) {
	auto object_offset = br->tell();
	auto prim_mesh = br->read<SPrimMeshWeighted>();
	br->align();

	GLACIER_ASSERT_TRUE(prim_mesh.draw_destination == 0);
	GLACIER_ASSERT_TRUE(prim_mesh.pack_type == 0);
	GLACIER_ASSERT_TRUE(prim_mesh.type == SPrimObjectHeader::EPrimType::PTMESH);
	//GLACIER_ASSERT_TRUE(prim_mesh.m_unk_tabl1_offset == 0); //TODO: TmUnkTabl1 is part of some linked meshes, implement

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

	auto prim = std::make_unique<ZRenderPrimitive>();

	//TODO: Add all the missing remnant values here
	prim->remnant.wire_color = prim_mesh.wire_color;
	prim->remnant.lod_mask = prim_mesh.lod_mask;
	prim->remnant.material_id = prim_mesh.material_id;
	prim->remnant.variant_id = prim_mesh.variant_id;

	br->seek(prim_submesh.index_buffer);
	prim->index_buffer = std::make_unique<IndexBuffer>(br, &prim_submesh);

	br->seek(prim_submesh.vertex_buffer);
	prim->vertex_buffer = std::make_unique<VertexBuffer>(br, prim_object_header, &prim_mesh, &prim_submesh);
	prim->vertex_data = std::make_unique<VertexDataBuffer>(br, &prim_mesh, &prim_submesh);

	if (prim_submesh.cloth) {
		br->seek(prim_submesh.cloth);
		prim->cloth_data = std::make_unique<ClothData>(br, &prim_submesh);
	}

	if (prim_submesh.collision) {
		br->seek(prim_submesh.collision);
		prim->collision_data = std::make_unique<CollisionData>(br, CollisionType::LINKED);
	}
	br->align();

	if (prim_mesh.bone_indices) {
		br->seek(prim_mesh.bone_indices);
		prim->bone_indices = std::make_unique<BoneIndices>(br);
	}
	br->align();

	if (prim_mesh.m_unk_tabl2_offset) {
		br->seek(prim_mesh.m_unk_tabl2_offset);
		prim->m_unk_tabl2 = std::make_unique<MUnkTabl2>(br);
	}
	br->align();

	return prim;
}



	////TODO: It might be better to factor parsing for different mesh subtypes into seperate functions, like in the original code. Duplicate code/readabiliy tradoff.
	//std::unique_ptr<ZRenderPrimitive> RenderPrimitiveDeserializer::deserializeStandardMesh(BinaryReader* br, const SPrimObjectHeader* const prim_object_header) {
	//	auto object_offset = br->tell();
	//	auto prim_mesh_base = br->read<SPrimMesh>();

	//	std::unique_ptr<SPrimMesh> prim_mesh = nullptr;
	//	switch (prim_mesh_base.sub_type) {
	//	case SPrimSubMesh::SUBTYPE_STANDARD:
	//		prim_mesh = std::make_unique<SPrimMesh>();
	//		memcpy_s(prim_mesh.get(), sizeof(SPrimMesh), &prim_mesh_base, sizeof(SPrimMesh));
	//		break;
	//	case SPrimSubMesh::SUBTYPE_LINKED:
	//		prim_mesh = std::make_unique<SPrimMeshLinked>();
	//		br->seek(object_offset);
	//		br->read(prim_mesh.get(), sizeof(SPrimMeshLinked));
	//		break;
	//	case SPrimSubMesh::SUBTYPE_WEIGHTED:
	//		prim_mesh = std::make_unique<SPrimMeshWeighted>();
	//		br->seek(object_offset);
	//		br->read(prim_mesh.get(), sizeof(SPrimMeshWeighted));
	//		break;
	//	default:
	//		throw std::runtime_error("Invalid prim submesh type");
	//	}
	//	br->align();

	//	GLACIER_ASSERT_TRUE(prim_mesh->draw_destination == 0);
	//	GLACIER_ASSERT_TRUE(prim_mesh->pack_type == 0);
	//	GLACIER_ASSERT_TRUE(prim_mesh->type == SPrimObjectHeader::EPrimType::PTMESH);
	//	GLACIER_ASSERT_TRUE(prim_mesh->m_unk_tabl1_offset == 0);
	//	if (prim_mesh->sub_type == SPrimSubMesh::SUBTYPE_STANDARD)
	//		GLACIER_ASSERT_TRUE(prim_mesh->m_unk_tabl2_offset == 0);

	//	br->seek(prim_mesh->sub_mesh_table);
	//	auto submesh_offset = br->read<uint32_t>();
	//	br->align();//This align will fail if a mesh has more than one submesh which I don't think is possible.

	//	br->seek(submesh_offset);
	//	auto prim_submesh = br->read<SPrimSubMesh>();

	//	//Throw on sub_meshes with multiple uv channels.
	//	//TODO: Note for later: This restriction was originally made because SMD doesn't support multiple UV channels.
	//	//Multiple UVs could be supported now since the switch to GLTF and FBX export. Might still not be worth it since apparently
	//	//only skybox meshes use this feature in the first place. That's not 100% confirmed atm though.
	//	if (prim_submesh.num_uv_channels != 1)
	//		throw UnsupportedFeatureException("Meshes with more than one UV channel are not supported.");

	//	ZRenderPrimitiveBuilder builder;

	//	//TODO: Add all the missing remnant values here
	//	//remnant.sub_type = prim_mesh.sub_type;
	//	//remnant.properties = prim_mesh.properties;
	//	builder.setWireColor(prim_mesh->wire_color);
	//	builder.setLodMask(prim_mesh->lod_mask);
	//	builder.setVariantId(prim_mesh->variant_id);
	//	//remnant.bias = prim_mesh.bias;
	//	//remnant.offset = prim_mesh.offset;
	//	builder.setMaterialId(prim_mesh->material_id);
	//	//remnant.unk2 = prim_mesh.unk2;
	//	//remnant.unk17 = prim_mesh.unk17;

	//	if (prim_mesh_base.sub_type == SPrimSubMesh::SUBTYPE_LINKED) {
	//		//auto link_table_offset = reinterpret_cast<SPrimMeshLinked*>(prim_mesh.get())->link_table;
	//		//br->seek(link_table_offset);
	//		//auto link_table = std::make_unique<LinkTable>(br);
	//		//builder.setLinkTable(std::move(link_table));
	//	}

	//	br->seek(prim_submesh.index_buffer);
	//	auto index_buffer = std::make_unique<IndexBuffer>(br, &prim_submesh);
	//	builder.setIndexBuffer(std::move(index_buffer));

	//	br->seek(prim_submesh.vertex_buffer);
	//	auto vertex_buffer = std::make_unique<VertexBuffer>(br, prim_object_header, prim_mesh.get(), &prim_submesh);
	//	builder.setVertexBuffer(std::move(vertex_buffer));

	//	if (prim_mesh->sub_type == SPrimSubMesh::SUBTYPE_WEIGHTED) {
	//		auto boneweight_buffer = std::make_unique<BoneWeightBuffer>(br, &prim_submesh);
	//		builder.setBoneWeightBuffer(std::move(boneweight_buffer));
	//	}

	//	auto vertex_data_buffer = std::make_unique<VertexDataBuffer>(br, prim_mesh.get(), &prim_submesh);
	//	builder.setVertexDataBuffer(std::move(vertex_data_buffer));

	//	if (prim_mesh->sub_type == SPrimSubMesh::SUBTYPE_WEIGHTED) {
	//		auto bone_flags = std::make_unique<BoneFlags>(br, &prim_submesh);
	//		builder.setBoneFlags(std::move(bone_flags));
	//	}

	//	if (prim_submesh.cloth) {
	//		br->seek(prim_submesh.cloth);
	//		auto cloth_data = std::make_unique<ClothData>(br, &prim_submesh);
	//		builder.setClothData(std::move(cloth_data));
	//	}

	//	//TODO: Collision parsing is temporarily disabled due to incompatibilities with LINKED mesh sub type meshes.
	//	//if (prim_submesh.collision) {
	//	//	br->seek(prim_submesh.collision);
	//	//	auto collision_buffer = std::make_unique<CollisionData>(br);
	//	//	builder.setCollisionBuffer(std::move(collision_buffer));
	//	//}
	//	//br->align();

	//	if (prim_mesh->sub_type == SPrimSubMesh::SUBTYPE_WEIGHTED) {
	//		auto bone_info_offset = reinterpret_cast<SPrimMeshWeighted*>(prim_mesh.get())->bone_indices;
	//		if (bone_info_offset) {
	//			br->seek(bone_info_offset);
	//			auto bone_indices = std::make_unique<BoneIndices>(br);
	//			builder.setBoneInfo(std::move(bone_indices));
	//		}
	//	}
	//	br->align();

	//	if (prim_mesh->m_unk_tabl2_offset) {
	//		br->seek(prim_mesh->m_unk_tabl2_offset);
	//		auto m2 = std::make_unique<MUnkTabl2>(br);
	//		builder.setMUnkTabl2(std::move(m2));
	//	}
	//	br->align();

	//	return builder.build();
	//}
