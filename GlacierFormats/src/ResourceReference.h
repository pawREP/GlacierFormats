#pragma once 
#include "GlacierTypes.h"

namespace GlacierFormats {

	struct ResourceReference {
		RuntimeId id;
		char flags;
	};

}

//template<>
//struct std::hash<GlacierFormats::ResourceReference> {
//	size_t operator()(const GlacierFormats::ResourceReference& ref) const {
//		return ref.id;
//	}
//
//};
