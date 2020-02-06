#pragma once
#include "../IRenderAsset.h"
#include "../IMesh.h"
#include <filesystem>

namespace GlacierFormats {
	namespace Export {

		class GLTFExporter : public IExporter {
		public:
			void operator()(const IRenderAsset& model, const std::filesystem::path& export_dir) const override final;
			void operator()(const IMesh& mesh, const std::filesystem::path& export_dir) const;
		};

	}
}

//The GLTF Exporter emits quite unoptimized files. Materials, armatures, joints, etc. shared
//between mesh primitives get duplicated and exported for every mesh. This is by design,
//since Blender's GLTF IO plugin seems to strungle with shared data.