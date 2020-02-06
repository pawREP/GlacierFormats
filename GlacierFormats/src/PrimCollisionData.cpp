#include "PrimCollisionData.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	CollisionData::CollisionData() {
		throw; //TODO:What is this, why do i need it
	}

	CollisionData::CollisionData(BinaryReader* br) {
		auto base_offset = br->tell();
		auto header_short = br->read<short>();
		size_t size = 0x10;
		if (header_short != 0)
			size = 6 * static_cast<size_t>(header_short) + 4;

		data_size = size;
		data = std::make_unique<char[]>(size);
		br->seek(base_offset);
		br->read(data.get(), size);

		br->align();
	}

	void CollisionData::serialize(BinaryWriter* bw) {
		bw->write(data.get(), data_size);
		bw->align();
	}