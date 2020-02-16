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

		template<typename T>
		inline size_t fnv1a(const std::vector<T> vec) {
			static_assert(std::is_trivially_copyable<T>::value);
			const char* bytes = reinterpret_cast<const char*>(vec.data());//UB abuse
			size_t byte_count = vec.size() * sizeof(T);

			size_t hash = 0xcbf29ce484222325;
			for (size_t i = 0; i < byte_count; ++i) {
				hash ^= bytes[i];
				hash *= 0x00000100000001B3;
			}

			return hash;
		}
	}
}