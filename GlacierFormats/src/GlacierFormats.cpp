#include "GlacierFormats.h"
#include <winreg.h>
#include <regex>

LONG GetStringRegKey(HKEY hKey, const std::string& strValueName, std::string& strValue, const std::string& strDefaultValue) {
	CHAR szBuffer[512];
	DWORD dwBufferSize = sizeof(szBuffer);
	ULONG nError;
	nError = RegQueryValueExA(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
	if (ERROR_SUCCESS == nError)
		strValue = szBuffer;
	else
		strValue = strDefaultValue;
	return nError;
}

std::filesystem::path getSteamInstallationPath() {
	HKEY steam_key_handle;
	auto res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", 0, KEY_READ, &steam_key_handle);
	if (res != ERROR_SUCCESS)
		return std::filesystem::path();

	std::string install_path_string;
	res = GetStringRegKey(steam_key_handle, "installPath", install_path_string, "");
	if (res != ERROR_SUCCESS)
		return std::filesystem::path();
	return install_path_string;
}

//Gets all steam library folders from libraryfolders.vdf.
void getSteamLibraryFolders(const std::filesystem::path& steam_install_path, std::vector<std::filesystem::path>& library_folders) {
	//Example content of libraryfolders.vdf file:
	//
	//"LibraryFolders"
	//{
	//	"TimeNextStatsReport"		"1581002847"
	//		"ContentStatsID"		"-7577160422684138761"
	//		"1"		"E:\\Steam"
	//		"2"		"D:\\Steam"
	//}

	auto library_folder_file = steam_install_path / "steamapps\\libraryfolders.vdf";
	if (!std::filesystem::exists(library_folder_file))
		return;

	std::ifstream ifs(library_folder_file);
	if (!ifs.is_open())
		return;

	std::regex re("\t\"[0-9]{1,}\"\t\t\"(.+?)\"$");
	std::smatch match;

	char line[256];
	while (ifs.getline(line, sizeof(line))) {
		auto line_str = std::string(line);
		if (std::regex_search(line_str, match, re))
			library_folders.emplace_back(match.str(1));
	}
	ifs.close();
}

std::filesystem::path findHitmanTwoRuntimeDirectoryPath() {
	//Find steam installation directory
	const auto steam_install_path = getSteamInstallationPath();
	if (steam_install_path.empty())
		return std::filesystem::path();

	//Check if game is installed in steam installation directory
	const std::string runtime_folder_extension = "steamapps\\common\\HITMAN2\\Runtime";
	auto runtime_dir = steam_install_path / runtime_folder_extension;
	if (std::filesystem::exists(runtime_dir))
		return runtime_dir;

	//If the game wasn't found, check other library folders.
	std::vector<std::filesystem::path> library_folders;
	getSteamLibraryFolders(steam_install_path, library_folders);
	for (const auto& library_folder : library_folders) {
		runtime_dir = library_folder / runtime_folder_extension;
		if (std::filesystem::exists(runtime_dir))
			return runtime_dir;
	}
	return std::filesystem::path();
}

void GlacierFormats::GlacierInit() {
	//Initilize runtime resource repository
	const auto glacier_runtime_directory = findHitmanTwoRuntimeDirectoryPath();
	if (glacier_runtime_directory.empty())
		throw std::runtime_error("Failed to find Hitman 2 runtime directory. Hitman2 might not be installed on this system. "\
								 "Use `GlacierInit(const std::filesystem::path& glacier_runtime_directory)` to manually specify a runtime directory.");
	GlacierInit(glacier_runtime_directory);
}

void GlacierFormats::GlacierInit(const std::filesystem::path& glacier_runtime_directory) {
	//Initilize COM system. This is required for DirectXTex.
	auto hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	if (FAILED(hr))
		throw std::runtime_error("Failed to initilize COM system");

	ResourceRepository::runtime_dir = glacier_runtime_directory;
	ResourceRepository::instance();
}
