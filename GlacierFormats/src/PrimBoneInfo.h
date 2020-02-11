#pragma once
#include "PrimSerializationTypes.h"
#include <memory>
#include <vector>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	//Function not entirely clear, anim/rigging related acceleration struct ???
	//Optional, can be dropped from primitives with seemingly no adverse effect. Small sample size. 

	class BoneIndices
	{
		std::vector<uint8_t> data;

	public:
		BoneIndices(BinaryReader* br);
		void serialize(BinaryWriter* bw) const;
	};

}
