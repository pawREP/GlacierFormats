#include "GlacierFormats.h"
#include <filesystem>
#include <fstream>

using namespace GlacierFormats;

//This sample implements a RPKG patch builder. Those patches provide an easy mechanism to 
//incorporate user generated content into glacier engine games. 

int main(int argc, char** argv) {
	if (argc != 2)
		exit(EXIT_FAILURE);

	std::filesystem::path folder_path = argv[1];

	//Initilize glacier formats library;
	GlacierInit();

	RPKG rpkg{};
	auto dir = std::filesystem::directory_iterator(folder_path);
	for (const auto& file : dir) {
		if(!std::filesystem::is_regular_file(file.path()))
			continue;

		auto file_path = file.path();
		RuntimeId id(file_path.stem().generic_string()); 
		if (id == 0)
			continue;
		
		std::ifstream ifs(file_path.generic_string(), std::ios::binary);
		auto data_size = std::filesystem::file_size(file_path);
		char* data = new char[data_size];
		ifs.read(data, data_size);
		ifs.close();

		auto repo = ResourceRepository::instance();
		auto type = repo->getResourceType(id);
		auto references = repo->getResourceReferences(id);
		rpkg.insertFile(id, type, data, data_size, &references);
	}
	rpkg.write(std::filesystem::current_path() / "patch.rpkg");

}