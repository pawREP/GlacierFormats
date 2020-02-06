#pragma once 

#include "TEXD.h"

#include <string>

namespace GlacierFormats {

	class IMaterial {
	public:
		virtual int getTextureCount() const = 0;
		virtual std::vector<std::string> getTextureNames() const = 0;

		virtual std::unique_ptr<TEXD> getTextureResourceByIndex(int idx) const = 0;

		virtual std::unique_ptr<TEXD> getDiffuseMap() const = 0;
		virtual std::unique_ptr<TEXD> getNormalMap() const = 0;
		virtual std::unique_ptr<TEXD> getSpecularMap() const = 0;

	};
}