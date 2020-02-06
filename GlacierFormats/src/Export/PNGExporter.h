#pragma once
#include "../IRenderAsset.h"

namespace GlacierFormats {
	namespace Export {

		class PNGExporter : public IExporter {
		public:
			void operator()(const IRenderAsset& asset, const std::filesystem::path& export_dir) const override final;
		};

	}
}