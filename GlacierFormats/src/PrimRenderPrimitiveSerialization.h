#pragma once
#include <cinttypes>
#include <memory>
#include "PrimSerializationTypes.h"
#include "PrimReusableRecord.h"

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;
	class ZRenderPrimitive;

	class RenderPrimitiveSerializer {
	public:
		uint32_t serialize(BinaryWriter* br, const ZRenderPrimitive* prim, std::unordered_map<RecordKey, uint64_t>&);
	};

	class RenderPrimitiveDeserializer {
	public:
		std::unique_ptr<ZRenderPrimitive> deserializeMesh(BinaryReader* br, const SPrimObjectHeader* const prim_object_header);
	};

}