#pragma once
#include "PrimSerializationTypes.h"
#include <memory>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	//Function not entirely clear, anim/rigging related acceleration struct ???
	//Optional, can be dropped from primitives with seemingly no adverse effect. Small sample size. 

	class BoneInfo
	{
		uint16_t data_size;
		std::unique_ptr<char[]> data;//TODO: Make vector, do the same with all the other structs of this type.

	public:
		BoneInfo(BinaryReader* br);
		void serialize(BinaryWriter* bw) const;
	};

}
