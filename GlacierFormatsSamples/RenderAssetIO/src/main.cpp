#include "GlacierFormats.h"
#include <filesystem>

using namespace GlacierFormats;

//Exports prim resoruce and it's resource refernces to a GLTF file. 
void exportGlacierRenderAssetToGLTF(RuntimeId prim_id, const std::filesystem::path& export_directory) {
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
std::unique_ptr<PRIM> importAssetFromGLTF(const std::filesystem::path& path) {
	RuntimeId prim_id = path.stem().generic_string();
	//Get mapping between bone names and bone indices of the original model.
	auto bone_map = Util::getBoneNameToIdMap(prim_id);

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

int main(int argc, char** argv) {
	//Initilize GlacierFormats with a runtime directory.
	GlacierInit();
	//GlacierInit(R"(E:\Steam\steamapps\common\HITMAN2\Runtime)");

	const RuntimeId agent47_head_prim_id = 0x002F5293D4F41A8D;
	const auto current_path = std::filesystem::current_path();

	exportGlacierRenderAssetToGLTF(agent47_head_prim_id, current_path);

	auto import_dir = current_path / (static_cast<std::string>(agent47_head_prim_id) + ".gltf");
	auto prim = importAssetFromGLTF(import_dir);
}