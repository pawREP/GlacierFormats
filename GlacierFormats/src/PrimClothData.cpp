#include "PrimClothData.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	ClothData::ClothData(BinaryReader* br, const SPrimSubMesh* prim_submesh) {
		//TODO: Calculating the size of this buffer is a bit adhoc and sketchy at the moment, do some reverse engieering!
		//It looks like the format of cloth data is actually dependent on wether the submesh is the first submesh.
		//This code uses some bad heuristic to figure out the correct size. The function throws if the result is unreasonable
		size = br->read<uint16_t>();

		auto base = br->tell();
		auto det = br->read<uint16_t>();
		br->seek(base);

		if (det) {
			data_size = 0x14 * prim_submesh->num_vertex - 2;
		}
		else {
			data_size = size - 2;
		}

		if (data_size > 0x14 * 0xFFFFF)
			throw std::runtime_error("Bad ClothData size");

		data = std::make_unique<char[]>(data_size);
		br->read(data.get(), data_size);

	}

	void ClothData::serialize(BinaryWriter* bw) {
		bw->write(size);
		//bw->write(det);
		bw->write(data.get(), data_size);
	}