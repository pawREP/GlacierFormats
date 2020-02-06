#pragma once
#include <memory>

namespace GlacierFormats {

	class BinaryWriter;
	class BinaryReader;

	//Holds collison data.
	//Structure not reverse engineered yet.
	//CollsionData buffers can be dropped from PRIM primitives with seemingly no visible effect in-game. Only a few models have been tested though. 

	class CollisionData {
	public:
		size_t data_size;
		std::unique_ptr<char[]> data;

		CollisionData();
		CollisionData(BinaryReader* br);
		void serialize(BinaryWriter* bw);
	};

}