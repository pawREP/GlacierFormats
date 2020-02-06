#pragma once
#include "PrimSerializationTypes.h"
#include <vector>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	class BoneFlags
	{
	public:
		std::vector<unsigned char> flags;

		explicit BoneFlags(int size) noexcept;
		BoneFlags(BinaryReader* br, const SPrimSubMesh* prim_submesh);
		void serialize(BinaryWriter* bw) const;
	};
}
