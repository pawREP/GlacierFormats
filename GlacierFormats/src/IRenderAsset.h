#pragma once
#include <vector>
#include <string>
#include "IMaterial.h"
#include "IMesh.h"
#include "IRig.h"

namespace GlacierFormats {

	class IRenderAsset {
	public:
		virtual const std::vector<IMaterial*> materials() const = 0;
		virtual const std::vector<IMesh*> meshes()  const = 0;
		virtual const IRig* rig()  const = 0;
		virtual std::string name() const = 0;

		template<typename Exporter>
		static void Export(const IRenderAsset& model, const std::string& export_dir) {
			Exporter{}(model, export_dir);
		}
	};


	class IExporter {
	public:
		virtual void operator()(const IRenderAsset& model, const std::filesystem::path& export_dir) const = 0;
	};
}