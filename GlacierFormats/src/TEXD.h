#pragma once
#include "TextureResource.h"

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	//Main texture format used in Glacier 2. 
	class TEXD : public TextureResource<TEXD> {
	public:
		TEXD();
		TEXD(const TEXD&);
		TEXD(BinaryReader& br, RuntimeId id);

		void serialize(BinaryWriter& bw);

		std::string name() const;
	};

}