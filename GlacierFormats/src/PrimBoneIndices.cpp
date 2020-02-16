#include "PrimBoneIndices.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	BoneIndices::BoneIndices(BinaryReader* br) {
		data_size = br->read<uint32_t>() - 2;
		data = std::make_unique<uint16_t[]>(data_size);
		br->read(data.get(), data_size);
	}

	void BoneIndices::serialize(BinaryWriter* bw) {
		bw->write(data_size + 2);
		bw->write(data.get(), data_size);
	}