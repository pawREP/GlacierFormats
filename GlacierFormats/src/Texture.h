#pragma once
#include <filesystem>
#include <memory>

#include "TEXT.h"
#include "TEXD.h"

namespace GlacierFormats {

	class Texture {
	public:
		std::unique_ptr<TEXT> text;
		std::unique_ptr<TEXD> texd;

		Texture(RuntimeId text_id);

		static std::unique_ptr<Texture> loadFromTGAFile(const std::filesystem::path& path);
	};
}