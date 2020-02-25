#pragma once
#include <vector>
#include "PrimSerializationTypes.h"

namespace GlacierFormats {

	struct PrimManifest {
		int rig_index;
		SPrimObjectHeader::PROPERTY_FLAGS properties;
	};

}