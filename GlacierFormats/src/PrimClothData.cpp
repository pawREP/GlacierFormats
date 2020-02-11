#include "PrimClothData.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	ClothData::ClothData(BinaryReader* br, const SPrimSubMesh* prim_submesh) {
		//TODO: Very ugly. Needs better understanding of the buffer size calculation. Check IDA. Current impl fails in some cases
		auto base_pos = br->tell();
		
		try {
			auto size = br->peek<uint16_t>();
			br->seek(base_pos + size);
			br->align();//Throws if size calc was bad.
			data.resize(size);
			br->seek(base_pos);
			br->read(data.data(), data.size());
			br->align();
			return;
		}
		catch (...) {
			
		}
		try {
			br->seek(base_pos);
			auto size = 0x14 * prim_submesh->num_vertex;
			br->seek(base_pos + size);
			br->align();//Throws if size calc was bad.
			data.resize(size);
			br->seek(base_pos);
			br->read(data.data(), data.size());
			br->align();
			return;
		}
		catch (...) {
			throw std::runtime_error("Bad ClothData size");
		}
	}

	void ClothData::serialize(BinaryWriter* bw) {
		bw->write(data.data(), data.size());
	}