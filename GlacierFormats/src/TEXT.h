#pragma once
#include "TextureResource.h"

//TODO: Deal with this macro collision;
#undef TEXT

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	//Low res textures used as placeholders while full scale TEXD textures are loaded.
	class TEXT : public TextureResource<TEXT> {
	public:
		TEXT();
		TEXT(const TEXT&);
		TEXT(BinaryReader& br, RuntimeId id);

		void serialize(BinaryWriter& bw);

		std::string name() const;
	};

}


