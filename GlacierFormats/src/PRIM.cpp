#include "PrimRenderPrimitive.h"
#include "PrimRenderPrimitiveBuilder.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "PrimBoundingBox.h"
#include "PrimReusableRecord.h"
#include "PrimRenderPrimitiveSerialization.h"
#include "Exceptions.h"
#include "PRIM.h"
#include <typeindex>
#include <utility>
#include <unordered_map>


using namespace GlacierFormats;

	PRIM::PRIM(RuntimeId id) : GlacierResource<PRIM>(id) {
		//TODO: Initilize manifest?
	}

	PRIM::PRIM(BinaryReader& br, RuntimeId id) : GlacierResource<PRIM>(id) {
		auto primary_offset = br.read<uint32_t>();
		br.align();

		br.seek(primary_offset);
		SPrimObjectHeader prim_object_header = br.read<SPrimObjectHeader>();

		//SPrimObjectHeader should always be at the end of the resource file. The only exception are speedtree meshes which aren't supported.
		//This test is not sufficient and could be avoided by simply not aligning but that would mess with the read coverage debug reader.
		if(br.size() != br.tell() + 4)
			throw UnsupportedFeatureException("SPrimObject::SUBTYPE_SPEEDTREE not supported");

		prim_object_header.Assert();
		GLACIER_ASSERT_TRUE(prim_object_header.type == SPrimHeader::EPrimType::PTOBJECTHEADER);
		br.align();


		if (((int)prim_object_header.property_flags & (int)SPrimObjectHeader::PROPERTY_FLAGS::HAS_FRAMES) != (int)SPrimObjectHeader::PROPERTY_FLAGS::HAS_FRAMES) {
			int a = 0;
		}

		manifest.rig_index = prim_object_header.bone_rig_resource_index;
		manifest.properties = prim_object_header.property_flags;

		br.seek(prim_object_header.object_table);
		std::vector<uint32_t> object_table;
		for (int i = 0; i < prim_object_header.num_objects; i++)
			object_table.push_back(br.read<uint32_t>());
		br.align();

		//printf("%s\n", static_cast<std::string>(id).c_str());
		for (const auto& object_offset : object_table) {
			br.seek(object_offset);
			auto object = br.peek<SPrimObject>();
			object.Assert();
			GLACIER_ASSERT_TRUE(object.type == SPrimHeader::EPrimType::PTMESH);

			RenderPrimitiveDeserializer deserializer;
			std::unique_ptr<ZRenderPrimitive> prim = nullptr;
			switch (object.sub_type) {
			case SPrimObject::SUBTYPE::SUBTYPE_STANDARD:		
			case SPrimObject::SUBTYPE::SUBTYPE_WEIGHTED:
			case SPrimObject::SUBTYPE::SUBTYPE_LINKED:
				prim = deserializer.deserializeMesh(&br, &prim_object_header);
				primitives.push_back(std::move(prim));
				break;
			case SPrimObject::SUBTYPE::SUBTYPE_SPEEDTREE:
				throw UnsupportedFeatureException("SPrimObject::SUBTYPE_SPEEDTREE not supported");
				break;
			default:
				GLACIER_UNREACHABLE;
				break;
			}
		}
	}

	PRIM::PRIM(const std::vector<IMesh*>& meshes, RuntimeId id, std::function<void(ZRenderPrimitiveBuilder&, const std::string&)>* build_modifier) : GlacierResource<PRIM>(id) {
		for (const auto& mesh : meshes) {
			ZRenderPrimitiveBuilder builder;
			builder.initilizeFromIMesh(mesh);
			//TODO: Add vertex colors
			if (build_modifier)
				(*build_modifier)(builder, mesh->name());

			auto primitive = builder.build();

			if (primitive) {
				this->primitives.push_back(std::move(primitive));
			}
		}
	}

	PRIM::~PRIM() {

	}

	bool GlacierFormats::PRIM::isWeightedPrim() const {
		for (const auto& primitive : primitives) {
			if (primitive->isWeightedMesh())
				return true;
		}
		return false;
	}

	void PRIM::serialize(BinaryWriter& bw) {
		std::vector<uint32_t> object_table;

		bw.write(-1);//header offset placeholder
		bw.align();

		//Record of serialized resource buffers. Used for buffer reuse support.
		//key: pair of typeid of serialized record and std::hash of resource buffer
		//value: offset to resource.
		std::unordered_map<RecordKey, uint64_t> buffer_records;

		//TODO:
		//The orginal format also supports the reuse of entire primitives by
		//pointing the submesh table entry to an existing submesh
		//In this case only the PrimMesh struct is serialized. 
		//This optimization is currently not supported in this reimplementation of the serializer
		//Impl operator== for prim and all it's logical subtypes.
		for (const auto& prim : primitives) {
			auto off = prim->serialize(&bw, buffer_records);
			object_table.push_back(off);
		}

		auto object_table_offset = bw.tell();
		for (const auto& obj : object_table)
			bw.write(obj);
		bw.align();

		auto header_offset = bw.tell();
		SPrimObjectHeader header{};
		header.type = SPrimObjectHeader::EPrimType::PTOBJECTHEADER;

		header.bone_rig_resource_index = manifest.rig_index;//TODO: Build from primitve info.
		header.property_flags = manifest.properties;

		header.num_objects = object_table.size();
		header.object_table = object_table_offset;

		//Global bounding box
		auto bb = primitives[0]->vertex_buffer->getBoundingBox();
		for (int i = 1; i < primitives.size(); ++i) {
			bb = bb + primitives[i]->vertex_buffer->getBoundingBox();
		}
		for (int i = 0; i < 3; ++i) {
			header.min[i] = bb.min[i];
			header.max[i] = bb.max[i];
		}

		header.Assert();
		bw.write(header);
		bw.align();

		bw.seek(0);
		bw.write(header_offset);
	}

