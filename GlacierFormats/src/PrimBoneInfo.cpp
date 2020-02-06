#include "PrimBoneInfo.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	BoneInfo::BoneInfo(BinaryReader* br) {
		auto pos = br->tell();
		data_size = br->read<uint16_t>() - sizeof(uint16_t);

		data = std::make_unique<char[]>(data_size);
		br->read(data.get(), data_size);
	}

	void BoneInfo::serialize(BinaryWriter* bw) const {
		bw->write(data_size);
		bw->write(data.get(), data_size);
	};

