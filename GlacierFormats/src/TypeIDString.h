#pragma once
#include <string>
#include <algorithm>
#include <assert.h>
#include <type_traits>
#include <functional>
#include "bit_cast.h"

//"Little endian" string of length 4. Used in many formats to identify names or types of formats, properties, containers...
//Can be trivially copied to from glacier formats and can be used like, or implicitly converted to, std::string. 
class TypeIDString {
	private:
		static constexpr int type_str_size = 4;
		char data_[type_str_size];

	public:
		TypeIDString();
		TypeIDString(const char* cstr);
		TypeIDString(const std::string& str);
		TypeIDString(const TypeIDString& other) = default;
		
		TypeIDString& operator=(const TypeIDString& other) = default;

		constexpr const char* begin() const noexcept;
		constexpr const char* end() const noexcept;
		constexpr size_t size() const noexcept;

		operator std::string() const;
		std::string string() const;

		bool operator==(const char* str) const;
		bool operator==(const std::string& str) const;
};

template<>
struct std::hash<TypeIDString> {
	size_t operator()(const TypeIDString& str) const noexcept {
		static_assert(sizeof(int) == sizeof(TypeIDString));
		//TODO: replace with bit cast;
		int i;
		std::memcpy(&i, &str, sizeof(i));
		return i;
	}
};

static_assert(sizeof(TypeIDString) == 4);
static_assert(std::is_trivially_copyable_v<TypeIDString>);

