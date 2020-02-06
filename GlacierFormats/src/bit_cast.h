#pragma once
#include <type_traits>
#include <cstring>

namespace GlacierFormats {

	template <typename To, typename From>
	typename std::enable_if< (sizeof(To) == sizeof(From)) && (alignof(To) == alignof(From)) && std::is_trivially_copyable<From>::value && std::is_trivial<To>::value, To>::type
		bit_cast(const From& src) noexcept {
		To dst;
		std::memcpy(&dst, &src, sizeof(To));
		return dst;
	}

	template <typename To>
	To bit_cast(const char* src) noexcept {
		To dst;
		std::memcpy(&dst, src, sizeof(To));
		return dst;
	}

}

