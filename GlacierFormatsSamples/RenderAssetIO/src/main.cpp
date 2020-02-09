#include "GlacierFormats.h"
#include <filesystem>
#include <iostream>
#include <regex>

using namespace GlacierFormats;

//Exports prim resoruce and it's resource refernces to a GLTF file. 
void doExport(RuntimeId prim_id, const std::filesystem::path& export_directory) {
	auto repo = ResourceRepository::instance();

	//RenderAssets are created from PRIM resources and their dependecies.
	//Throw if a runtime id of wrong type was passed.
	if (repo->getResourceType(prim_id) != "PRIM")
		throw std::runtime_error("runtime id corresponds to invalid type");

	//Construct render asset from prim and it's dependencies.
	//The IRenderAsset interface provides methods useful for export and convertion of assets.
	std::unique_ptr<IRenderAsset> asset = std::make_unique<GlacierRenderAsset>(prim_id);
	if (!asset)
		throw std::runtime_error("Render asset construction failed");

	//Export asset to GLTF.
	//The full path of the gltf file will be "export_directory / (asset->name() + ".gltf")"
	//The name returned by IRenderAsset::name should be the runtime id of the asset's root prim  resource in the form of a hex string.
	Export::GLTFExporter{}(*asset, export_directory);
}

//Imports rigged render assets.
//Import of weighted assets is a bit more complex since the bone order in externally generated or modified GLTF files might differ
//from the bone order found in the original model. This is an issue since the files that define the rigs (BORG) are shared between 
//different meshes so they can't be updated easily. To get around this issue, all bone ids have to be remapped to match the order 
//of the original PRIM model.
std::unique_ptr<PRIM> importGLTFtoPRIM(const std::filesystem::path& path) {
	RuntimeId prim_id = path.stem().generic_string();
	//Get mapping between bone names and bone indices of the original model.
	auto bone_map = Util::getBoneMapping(prim_id);

	std::unique_ptr<IRenderAsset> asset = nullptr;
	if (bone_map.size()) {
		//Construct GLTFAsset from GLTF file and bone_map. Attempting to construct a weighted GLTFAsset without passing a bone_map will
		//result in a runtime_error.
		asset = std::make_unique<GLTFAsset>(path, &bone_map);
	}
	else {
		asset = std::make_unique<GLTFAsset>(path);
	}
	
	//Construct PRIM resource from vector of IMeshs. 
	auto prim = std::make_unique<PRIM>(asset->meshes(), asset->name());
	return prim;
}

[[noreturn]] void Exit(const std::string& msg) {
	printf("%s\nPress Enter to exit.", msg.c_str());
	std::cin.ignore();
}


void doImport(std::filesystem::path gltf_file_path) {
	//Get ResourceRepository instance;
	auto repo = ResourceRepository::instance();

	//File name is expected to be valid 64 bit hex integer string
	auto prim = importGLTFtoPRIM(gltf_file_path);

	//Serialize the imported prim resource.
	auto bytes = prim->serializeToBuffer();

	//Get the resource references of the original prim. 
	//References describe dependency relations between resources and specify how they should be loaded at runtime.
	//There is rarely any reason to not use the references of the origianl model.
	auto references = repo->getResourceReferences(prim->id);

	//Init patch file
	RPKG rpkg{};

	//Insert serialized prim into patch.
	rpkg.insertFile(prim->id, "PRIM", bytes.data(), bytes.size(), &references);

	//Generate a sensible name for the new patch file based on the nameof the archive the orgininal PRIM came from.
	auto source_archive_name = repo->getSourceStreamName(prim->id);
	auto patch_archive_name = Util::incrementArchiveName(source_archive_name);
	auto out_path = std::filesystem::current_path();
	out_path /= patch_archive_name;
	out_path.extension() = ".rpkg";

	//Serialize patch to file.
	rpkg.write(out_path);

}

//Checks if the argument is a either a runtime id or a file path to a gltf file with a runtimeid stem.
bool validateArguments(int argc, char** argv) {
	if (argc != 2)
		false;

	std::string arg1(argv[1]);
	if (std::filesystem::is_regular_file(arg1)) {
		auto path = std::filesystem::path(arg1);
		if (path.extension() != ".gltf")
			return false;
		auto stem = path.stem().generic_string();
		return Util::isRuntimeIdString(stem);
	}
	else {
		return Util::isRuntimeIdString(arg1);
	}
}

/*
This sample CLI program demonstrates import and export of Glacer 2 render meshes to and from GLTF and PNG files.
*/
int main(int argc, char** argv) {
	//Initilize GlacierFormats library.
	GlacierInit();

	//Validate arguments
	if(!validateArguments(argc, argv))
		Exit("Invalid arguments");

	std::string arg1 = std::string(argv[1]);
	if (std::filesystem::is_regular_file(arg1))
		//Import gltf file to archive patch;
		doImport(arg1);
	else
		doExport(arg1, std::filesystem::current_path());
}