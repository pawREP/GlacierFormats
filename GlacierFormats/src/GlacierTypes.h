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
		//TODO: Add static isRuntimeIdString(const std::string& to avoid throwing in constructor if invalid string is passed.)
		RuntimeId() : id(0xFFFFFFFFFFFFFFFF) {};
		RuntimeId(uint64_t id) : id(id) {}
		RuntimeId(const std::string& id_string) : id(0) {
			if (id_string == "")
				return;
			if (id_string.find_first_not_of("0123456789abcdefABCDEF", 0) != std::string::npos)
				return; //invalid input string.
			id = std::stoull(id_string, nullptr, 16);
		}
		RuntimeId(const char* id_string) : RuntimeId(std::string(id_string)) {}

		operator uint64_t() const { return id; }
		operator std::string() const {
			std::stringstream ss;
			ss << std::setw(0x10);
			ss << std::setfill('0');
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