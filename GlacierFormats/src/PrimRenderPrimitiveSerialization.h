#pragma once
#include <cinttypes>
#include <memory>
#include "PrimSerializationTypes.h"

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;
	class ZRenderPrimitive;

	class RenderPrimitiveSerializer {
	public:
		uint32_t serialize(BinaryWriter* br, const ZRenderPrimitive* prim);
	};

	class RenderPrimitiveDeserializer {
	public:
		std::unique_ptr<ZRenderPrimitive> deserializeMesh(BinaryReader* br, const SPrimObjectHeader* const prim_object_header);
	};

}