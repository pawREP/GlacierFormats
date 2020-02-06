#pragma once
#include "GlacierTypes.h"
#include <vector>
#include <functional>
#include <unordered_map>

namespace GlacierFormats {

	template<typename T>
	size_t vectorSizeInBytes(const std::vector<T> v) {
		return v.size() * sizeof(decltype(v)::value_type);
	}

	namespace Util {
		std::unordered_map<std::string, int> getBoneNameToIdMap(RuntimeId prim_id);
	}

}



