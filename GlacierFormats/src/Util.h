#pragma once
#include "GlacierTypes.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace GlacierFormats {

	template<typename T>
	size_t vectorSizeInBytes(const std::vector<T> v) {
		return v.size() * sizeof(decltype(v)::value_type);
	}

	namespace Util {

		uint32_t meshInstallDebugHookHash(RuntimeId id);

		//Returns the mapping between bone names and bone ids of the rig used by the 
		//PRIM specified by the Runtime id argument.
		//Returns an empty map if the prim id doesn't exist or if it doesn't reference a BORG.
		std::unordered_map<std::string, int> getBoneMapping(RuntimeId prim_id);

		//Takes the name of an RPKG archive file and logically increments it.
		//For example: 
		//	dlc12 -> dlc12patch1
		//	chunk0patch1 -> chunk0patch2
		//	...
		std::string incrementArchiveName(const std::string& archiveName);

		bool isRuntimeIdString(const std::string& str);

		void mergePatchFiles(std::vector<std::string> in_patch_file_paths, std::string out_patch_file_path);

		RuntimeId runtimeIdFromFilePath(const std::filesystem::path& path);
	}

}



