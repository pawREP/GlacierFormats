#pragma once
#include "../src/GlacierTypes.h"
#include "../src/RenderAsset.h"
#include "../src/IRenderAsset.h"
#include "../src/ResourceRepository.h"
#include "../src/PrimRenderPrimitiveBuilder.h"
#include "../src/RPKG.h"
#include "../src/PRIM.h"
#include "../src/MATI.h"
#include "../src/TEXT.h"
#include "../src/TEXD.h"
#include "../src/Util.h"
#include "../src/PrimSerializationTypes.h"

#include "../src/GLTFAsset.h"
#include "../src/Export/GLTFExporter.h"
#include "../src/Export/TGAExporter.h"


namespace GlacierFormats {

	void GlacierInit();
	void GlacierInit(const std::filesystem::path& glacier_runtime_directory);

}

