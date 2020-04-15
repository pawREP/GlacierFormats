#include <gtest/gtest.h>
#include "GlacierFormats.h"

using namespace GlacierFormats;

GTEST_TEST(Texture, RoundtripTEXD) {
    RuntimeId texd_id = 0x00f21881494e8789;
    auto texd = ResourceRepository::instance()->getResource<TEXD>(texd_id);
    ASSERT_TRUE(texd);

    auto buf = texd->serializeToBuffer();
    ASSERT_TRUE(buf.size());
}

GTEST_TEST(Texture, RoundtripTEXT) {
    RuntimeId text_id = 0x001e94b73957fb3b;
    auto text = ResourceRepository::instance()->getResource<TEXT>(text_id);
    ASSERT_TRUE(text);

    auto buf = text->serializeToBuffer();
    ASSERT_TRUE(buf.size());
}

GTEST_TEST(Texture, GenerateTextureFromRepo) {
    Texture texture(0x00f21881494e8789);
    ASSERT_TRUE(texture.texd);
    ASSERT_TRUE(texture.text);
}