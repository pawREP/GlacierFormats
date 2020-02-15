#pragma once
#include "PrimSerializationTypes.h"
#include <memory>
#include <cinttypes>
#include <vector>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	class ClothData
	{
		std::vector<char> data;

	public:
		ClothData(BinaryReader* br, const SPrimSubMesh* prim_submesh, const SPrimMesh* prim_mesh);
		void serialize(BinaryWriter* bw);
	};

}
