#pragma once
#include <typeindex>
#include <functional>

namespace GlacierFormats {

	using RecordKey = std::pair<std::type_index, size_t>;

	class ReusableRecord {
		virtual RecordKey recordKey() const = 0;
	};

}

template<>
struct std::hash<GlacierFormats::RecordKey> {
	size_t operator()(const GlacierFormats::RecordKey& pair) const {
		return pair.first.hash_code() ^ pair.second;
	}
};
