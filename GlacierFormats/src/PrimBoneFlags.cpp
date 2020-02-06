#include "PrimBoneFlags.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	BoneFlags::BoneFlags(int size) noexcept {
		flags = std::vector<unsigned char>(size, 0xFF);
	}

	BoneFlags::BoneFlags(BinaryReader* br, const SPrimSubMesh* prim_submesh) {
		auto size = 4 * prim_submesh->num_vertex;
		flags.resize(size);
		br->read(flags.data(), size);
	}

	void BoneFlags::serialize(BinaryWriter* bw) const {
		bw->write(flags.data(), flags.size());
	};
