#pragma once
#include "PrimSerializationTypes.h"
#include <vector>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	class VertexColors
	{
	public:
		std::vector<unsigned char> colors;

		explicit VertexColors(int size) noexcept;
		VertexColors(BinaryReader* br, const SPrimSubMesh* prim_submesh);
		void serialize(BinaryWriter* bw) const;
	};
}
