#include "PrimMUnkTabl2.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	MUnkTabl2::MUnkTabl2(BinaryReader* br) {
		data_size = br->read<uint32_t>() - 2;
		data = std::make_unique<uint16_t[]>(data_size);
		br->read(data.get(), data_size);
	}

	void MUnkTabl2::serialize(BinaryWriter* bw) {
		bw->write(data_size + 2);
		bw->write(data.get(), data_size);
	}