#include "TEXT.h"
#include "TEXD.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

TEXT::TEXT() : TextureResource<TEXT>() {

}

TEXT::TEXT(const TEXT& other) : TextureResource<TEXT>(other) {

}

TEXT::TEXT(const TEXD& other) : TextureResource<TEXT>() {
	initializeTextFromTexd(other, *this);
}

TEXT::TEXT(BinaryReader& br, RuntimeId id) : TextureResource<TEXT>(br, id) {

}

std::string TEXT::name() const {
	return TextureResource<TEXT>::name();
}

void TEXT::serialize(BinaryWriter& bw) {
	TextureResource<TEXT>::serialize(bw);
}

