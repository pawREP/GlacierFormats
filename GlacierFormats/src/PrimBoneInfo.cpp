#include "PrimBoneInfo.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	BoneIndices::BoneIndices(BinaryReader* br) {
		auto size = br->peek<uint16_t>();
		data.resize(size);
		br->read(data.data(), size);
	}

	void BoneIndices::serialize(BinaryWriter* bw) const {
		GLACIER_ASSERT_TRUE(*reinterpret_cast<const uint16_t*>(data.data()) == data.size());
		bw->write(data.data(), data.size());
	};

