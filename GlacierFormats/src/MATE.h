#pragma once
#include "GlacierResource.h"
#include "BinaryReader.hpp"

namespace GlacierFormats {

	//Contains material shader data. Every MATI has one MATE child. 
	//This format is only implemented in a very barebone manner. 
	class MATE : public GlacierResource<MATE> {
	private:
		std::vector<char> data;

	public:
		enum class TextureType {
			Albedo,
			Normal,
			Specular,
			Height,
			Detail,//CompoundNormal
			Unknown,
		};

		MATE(BinaryReader& br, RuntimeId id);

		void serialize(BinaryWriter& bw);

		TextureType getTextureType(const std::string& texture_name) const;
		bool contains(const std::string& string) const;
	};

}
