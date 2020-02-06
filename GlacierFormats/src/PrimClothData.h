#pragma once
#include "PrimSerializationTypes.h"
#include <memory>
#include <cinttypes>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	class ClothData
	{
		uint16_t size;
		//uint16_t det;
		uint32_t data_size;
		std::unique_ptr<char[]> data;

	public:
		ClothData(BinaryReader* br, const SPrimSubMesh* prim_submesh);
		void serialize(BinaryWriter* bw);
	};

}
