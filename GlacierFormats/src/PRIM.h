#pragma once
#include "GlacierResource.h"
#include "PrimRenderPrimitive.h"

#include <vector>
#include <functional>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	class PrimManifest {
		int rig_index;
		SPrimObjectHeader::PROPERTY_FLAGS properties;
	};

	struct PrimitiveManifest {
		SPrimObject::SUBTYPE mesh_subtype = static_cast<SPrimObject::SUBTYPE>(0);
		SPrimObject::PROPERTY_FLAGS submesh_properties = static_cast<SPrimObject::PROPERTY_FLAGS>(0);
		SPrimObject::PROPERTY_FLAGS mesh_properties = static_cast<SPrimObject::PROPERTY_FLAGS>(0);

		unsigned char lod_mask = 1;
		//Mask that indicates lod range for submesh model, one bit per lod range. Higher bits indicate farther draw distance.
		//0 invis
		//1 shorter than cam range vis, 
		//2 visible between 0.5-3 cam ranges.

		char variant_id = 0;
		char bias = 0;
		char offset = 0;
		short material_id = 0;
		int wire_color = 0;
		int debug_color = 0;
		float unk17 = 0;
	};

	//Glacier file format class containing render mesh data and meta data. 
	class PRIM : public GlacierResource<PRIM> {
	public:
		struct Manifest {
			int rig_index;
			SPrimObjectHeader::PROPERTY_FLAGS properties;
		} manifest;

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



