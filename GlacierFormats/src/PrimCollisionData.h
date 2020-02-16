#pragma once
#include "PrimReusableRecord.h"
#include <vector>

namespace GlacierFormats {

	class BinaryWriter;
	class BinaryReader;

	//Holds collison data.
	//Structure not reverse engineered yet.
	//CollsionData buffers can be dropped from PRIM primitives with seemingly no visible effect in-game. Only a few models have been tested though. 

	enum class CollisionType {
		STANDARD = 0,
		WEIGHTED = 0,
		LINKED = 1,
	};

	class CollisionData : public ReusableRecord {
	public:
		std::vector<char> data;
		CollisionType type;
			
		CollisionData(BinaryReader* br, CollisionType type);
		void serialize(BinaryWriter* bw) const;

		RecordKey recordKey() const override final;
	};

	
}