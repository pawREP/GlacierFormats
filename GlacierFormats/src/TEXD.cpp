#include "TEXD.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

TEXD::TEXD() : TextureResource<TEXD>() {

}

TEXD::TEXD(const TEXD& other) : TextureResource<TEXD>(other) {

}

TEXD::TEXD(BinaryReader& br, RuntimeId id) : TextureResource<TEXD>(br, id) {

}

std::string TEXD::name() const {
	return TextureResource<TEXD>::name();
}

void TEXD::serialize(BinaryWriter& bw) {
	TextureResource<TEXD>::serialize(bw);
}