#include "Util.h"
#include "ResourceRepository.h"
#include "BORG.h"
#include <regex>

std::unordered_map<std::string, int> GlacierFormats::Util::getBoneMapping(RuntimeId prim_id) {
	auto repo = ResourceRepository::instance();
	auto borg_ref = repo->getResourceReferences(prim_id, "BORG");
	if (!borg_ref.size())
		return std::unordered_map<std::string, int>();
	auto borg_resource = repo->getResource<BORG>(borg_ref.front().id);
	return borg_resource->getNameToBoneIndexMap();
}

std::string GlacierFormats::Util::incrementArchiveName(const std::string& archiveName) {
	std::regex re("(.+?patch)([0-9]{1,})");
	std::smatch match;

	std::regex_search(archiveName, match, re);
	switch (match.size()) {
	case 0:
		return archiveName + "patch1";
	case 3:
		return match.str(1) + std::to_string(std::stoi(match.str(2)) + 1);
	default:
		throw std::runtime_error("Invalid archive name");
	}
}

bool GlacierFormats::Util::isRuntimeIdString(const std::string& str) {
	try {
		RuntimeId id(str);
	}
	catch (...) {
		return false;
	}
	return true;
}
