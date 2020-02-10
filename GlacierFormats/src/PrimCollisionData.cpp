#include "PrimCollisionData.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	CollisionData::CollisionData() {
		throw; //TODO: Is this needed?
	}

	CollisionData::CollisionData(BinaryReader* br, CollisionType type) : type(type) {
		auto size = br->peek<uint16_t>();
		if(type == CollisionType::STANDARD || type == CollisionType::WEIGHTED)
			size = 6 * size + 4;
		data.resize(size);
		br->read(data.data(), size);
		br->align();
	}

	void CollisionData::serialize(BinaryWriter* bw) const {
		if (type == CollisionType::STANDARD || type == CollisionType::WEIGHTED)
			GLACIER_ASSERT_TRUE(*reinterpret_cast<const uint16_t*>(data.data()) == (data.size() - 4) / 6)
		else
			GLACIER_ASSERT_TRUE(*reinterpret_cast<const uint16_t*>(data.data()) == data.size());
		bw->write(data.data(), data.size());
		bw->align();
	}