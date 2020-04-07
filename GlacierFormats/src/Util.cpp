#include "Util.h"
#include "ResourceRepository.h"
#include "BORG.h"
#include "RPKG.h"

#include <regex>
#include <fstream>
#include <filesystem>

using namespace GlacierFormats;

//Dev only function. Returns hash usd for install-time resource identification
uint32_t GlacierFormats::Util::meshInstallDebugHookHash(RuntimeId id) {
	//Bad hash because I was too lazy to write anything else in assembly
	auto repo = ResourceRepository::instance();
	std::unique_ptr<char[]> data;
	auto data_size = repo->getResource(id, data);
	uint32_t hash = 0;
	for (int i = 0; i < data_size / 4; ++i)
		hash ^= reinterpret_cast<uint32_t*>(data.get())[i];
	return hash;
}

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

void GlacierFormats::Util::mergePatchFiles(std::vector<std::string> in_patch_file_paths, std::string out_patch_file_path) {
	RPKG out_patch;

	for (const auto& in_patch_path : in_patch_file_paths) {
		if (!std::filesystem::exists(in_patch_path) || !std::filesystem::is_regular_file(in_patch_path))
			throw std::runtime_error("Invalid in path");

		RPKG in_patch(in_patch_path);
		for (const auto& file : in_patch.files) {
			auto id = file.entry_info.runtimeID;
			auto type = file.entry_descriptor.type;
			auto refs = file.entry_descriptor.references;
			char* data;
			auto size = in_patch.getFileData(id, &data);

			out_patch.insertFile(id, type, data, size, &refs);
			for (const auto& del_entry : in_patch.deletion_list)
				out_patch.deletion_list.push_back(del_entry);
		}
	}

	out_patch.write(out_patch_file_path);
}
