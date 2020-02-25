#pragma once
#include "GlacierResource.h"
#include "PrimRenderPrimitive.h"
#include "PrimManifest.h"

#include <vector>
#include <functional>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	struct PrimitiveManifest {
		SPrimObject::SUBTYPE mesh_subtype = static_cast<SPrimObject::SUBTYPE>(0);
		SPrimObject::PROPERTY_FLAGS mesh_properties = static_cast<SPrimObject::PROPERTY_FLAGS>(0);

		unsigned char lod_mask = 1;
		char variant_id = 0;
		char bias = 0;
		char offset = 0;
		short material_id = 0;
	};

	//Glacier file format class containing render mesh data and meta data. 
	class PRIM : public GlacierResource<PRIM> {
	public:
		PrimManifest manifest;

		std::vector<std::unique_ptr<ZRenderPrimitive>> primitives;

	public:
		PRIM(RuntimeId id);
		PRIM(BinaryReader& br, RuntimeId id);
		PRIM(const std::vector<IMesh*>& meshes, RuntimeId id, std::function<void(ZRenderPrimitiveBuilder&, const std::string&)>* build_modifier = nullptr);

		PRIM(const PRIM& prim) = delete;
		PRIM& operator=(const PRIM& p) = delete;
		~PRIM();

		bool isWeightedPrim() const;

		void serialize(BinaryWriter& bw);
	};
}



