#include "TypeIDString.h"

using namespace GlacierFormats;

	TypeIDString::TypeIDString() {
	}

	TypeIDString::TypeIDString(const char* cstr) {
		std::copy(cstr, &cstr[size()], data_);
		std::reverse(std::begin(data_), std::end(data_));
	}

	TypeIDString::TypeIDString(const std::string& str) {
		assert((str.size() == size()));
		std::copy(str.begin(), str.end(), data_);
		std::reverse(std::begin(data_), std::end(data_));
	}

	constexpr const char* TypeIDString::begin() const noexcept {
		return std::begin(data_);
	}

	constexpr const char* TypeIDString::end() const noexcept {
		return std::end(data_);
	}

	constexpr size_t TypeIDString::size() const noexcept {
		return type_str_size;
	}

	TypeIDString::operator std::string() const {
		std::string str(data_, size());
		std::reverse(str.begin(), str.end());
		return str;
	}

	std::string TypeIDString::string() const {
		return *this;
	}

	bool TypeIDString::operator==(const char* str) const {
		for (int i = 0; i < size(); ++i)
			if (data_[i] != str[size() - 1 - i])
				return false;
		return true;
	}

	bool TypeIDString::operator==(const std::string& str) const {
		return *this == str.c_str();
	}
