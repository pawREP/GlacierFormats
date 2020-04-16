#include "TEXT.h"
#include "TEXD.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "ResourceRepository.h"

using namespace GlacierFormats;

TEXT::TEXT() : TextureResource<TEXT>() {

}

TEXT::TEXT(const TEXT& other) : TextureResource<TEXT>(other) {

}

TEXT::TEXT(const TEXD& other) : TextureResource<TEXT>() {
	initializeTextFromTexd(other, *this);

	auto parent_text_id = ResourceRepository::instance()->getResourceBackReferences(other.id, "TEXT");
	GLACIER_ASSERT_TRUE(parent_text_id.size() == 1);// TEXDs only have one TEXT parent. If this ever fails, check if it's one of those cases where the repo contains duplicate files.
	id = parent_text_id.front();
}

TEXT::TEXT(BinaryReader& br, RuntimeId id) : TextureResource<TEXT>(br, id) {

}

std::string TEXT::name() const {
	return TextureResource<TEXT>::name();
}

void TEXT::serialize(BinaryWriter& bw) {
	TextureResource<TEXT>::serialize(bw);
}

