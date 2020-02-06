#include "MATE.h"
#include <string_view>

namespace GlacierFormats {

	using namespace std::string_literals;

	const std::string mapDiffuse = "mapDiffuse";
	const std::string mapNormal = "mapNormal";
	const std::string mapSpecular = "mapSpecular";
	const std::string mapHeight = "mapHeight";
	const std::string mapDetail = "mapDetail";

	MATE::MATE(BinaryReader& br, RuntimeId id) : GlacierResource<MATE>(id) {
		data.resize(br.size());
		br.read<char>(data.data(), data.size());
	}

	void MATE::serialize(BinaryWriter& bw) {
		std::runtime_error("Not implemented");
	}

	MATE::TextureType MATE::getTextureType(const std::string& texture_name) const {
		auto target = texture_name;

		//pad with '\0' to next 4 byte boundry.
		auto padding_len = 4 - (target.size() % 4);
		for (int i = 0; i < padding_len; ++i)
			target += "\0"s;

		std::string search_string = target + mapDiffuse;
		if (contains(search_string))
			return TextureType::Albedo;
		search_string = target + mapNormal;
		if (contains(search_string))
			return TextureType::Normal;
		search_string = target + mapSpecular;
		if (contains(search_string))
			return TextureType::Specular;
		search_string = target + mapHeight;
		if (contains(search_string))
			return TextureType::Height;
		search_string = target + mapDetail;
		if (contains(search_string))
			return TextureType::Detail;

		return TextureType::Unknown;


	}

	bool MATE::contains(const std::string& str) const {
		auto it = std::search(data.begin(), data.end(), str.begin(), str.end());
		if (it != data.end())
			return true;
		return false;
	}


}