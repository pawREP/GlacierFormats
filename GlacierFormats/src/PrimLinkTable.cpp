#include "PrimLinkTable.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

GlacierFormats::LinkTable::LinkTable(BinaryReader* br) {
	auto s = br->read<uint16_t>();
	data_size = s - 2;
	data = std::make_unique<char[]>(data_size);
	br->read(data.get(), data_size);
}

void GlacierFormats::LinkTable::serialize(BinaryWriter* bw) const {
	bw->write(static_cast<uint16_t>(data_size + 2));
	bw->write(data.get(), data_size);
}
