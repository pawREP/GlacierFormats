#pragma once
#include "GlacierTypes.h"
#include "GlacierResource.h"

//TODO: Deal with this macro collision;
#undef TEXT
	
//TODO: Impl. 
namespace GlacierFormats {

	//Currently only a dummy class. impl is almost identical to TEXD, read 010 template for details.
	class TEXT : public GlacierResource<TEXT> {
	public:
		TEXT(BinaryReader& br, RuntimeId id);

		void serialize(BinaryWriter& bw);
	};

}


