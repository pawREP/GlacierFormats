#include "PNGExporter.h"

using namespace GlacierFormats;
using namespace GlacierFormats::Export;

void PNGExporter::operator()(const IRenderAsset& asset, const std::filesystem::path& export_dir) const {
	for (const auto& material : asset.materials()) {
		for (int i = 0; i < material->getTextureCount(); ++i) {
			auto texd = material->getTextureResourceByIndex(i);
			if (!texd)
				continue;
			texd->saveToTGAFile(export_dir);
		}
	}
}