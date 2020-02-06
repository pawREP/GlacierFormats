#include <cinttypes>
#include <string>

namespace GlacierFormats {

	namespace hash {

		//Compile time string hash function.
		inline constexpr uint32_t fnv1a(const char* const str, const uint32_t hash = 0x811c9dc5) noexcept {
			if (str[0] == '\0')
				return hash;
			return fnv1a(&str[1], (hash ^ uint32_t(str[0])) * 0x1000193);
		}

		inline uint32_t fnv1a(const std::string& str) noexcept {
			return fnv1a(str.c_str());
		}
	}
}