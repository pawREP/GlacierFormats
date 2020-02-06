#pragma once
#include <filesystem>
#include "IRenderAsset.h"

#include <unordered_map>

namespace GlacierFormats {

	class GLTFAsset : public IRenderAsset {
	private://TODO: fix priv/pub 
		std::string name_;
	public:
		const std::filesystem::path source_directory;

		std::vector<std::unique_ptr<IMesh>> meshes_;
		std::vector<std::string> textures;

		GLTFAsset(const std::filesystem::path& gltf_file_path, const std::unordered_map<std::string, int>* bone_name_to_index_map = nullptr);

		const std::vector<IMaterial*> materials() const;
		const std::vector<IMesh*> meshes()  const;
		const GlacierFormats::IRig* rig() const;
		std::string name() const;
	};
}