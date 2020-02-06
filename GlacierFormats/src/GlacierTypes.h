#pragma once
#include <guiddef.h>
#include <cinttypes>
#include <string>
#include <assert.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace GlacierFormats {

	struct RuntimeId {
	private:
		uint64_t id;
	public:

		RuntimeId() : id(0xFFFFFFFFFFFFFFFF) {};
		RuntimeId(uint64_t id) : id(id) {}
		RuntimeId(const std::string& id_string) : id(std::stoull(id_string, nullptr, 16)) {}
		RuntimeId(const char* id_string) : RuntimeId(std::string(id_string)) {}

		operator uint64_t() const { return id; }
		operator std::string() const {
			std::stringstream ss;
			ss << std::hex;
			ss << id;
			return ss.str();
		}

	};
	
	using ResourceId = GUID;
}

template<>
struct std::hash<GlacierFormats::RuntimeId> {
	size_t operator()(const GlacierFormats::RuntimeId& id) const {
		return std::hash<uint64_t>{}(static_cast<uint64_t>(id));
	}

};