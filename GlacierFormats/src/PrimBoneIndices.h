#pragma once
#include <memory>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	//Unkown function
	//Can be dropped without adverse effects. (Tested with small sample size).

	class BoneIndices
	{
		uint32_t data_size;
		std::unique_ptr<uint16_t[]> data;

	public:
		BoneIndices(BinaryReader* br);
		void serialize(BinaryWriter* bw);
	};

}
