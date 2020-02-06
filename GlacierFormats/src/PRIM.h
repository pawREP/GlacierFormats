#pragma once
#include "GlacierResource.h"
#include "PrimRenderPrimitive.h"

#include <vector>
#include <functional>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	//Glacier file format class containing render mesh data and meta data. 
	class PRIM : public GlacierResource<PRIM> {
	public:
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



