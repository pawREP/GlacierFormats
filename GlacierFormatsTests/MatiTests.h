#include <gtest/gtest.h>
#include "GlacierFormats.h"

using namespace GlacierFormats;

GTEST_TEST(MATI, Parse) {
    const RuntimeId mati_id = 0x0000945079441bae16;
    const auto mati = ResourceRepository::instance()->getResource<MATI>(mati_id);
    ASSERT_TRUE(mati);
}

GTEST_TEST(MATI, Reserialize) {
    const auto& repo = ResourceRepository::instance();
    const RuntimeId mati_id = 0x0000945079441bae16;
    
    const auto mati = repo->getResource<MATI>(mati_id);
    ASSERT_TRUE(mati);

    auto original_mati_data = repo->getResource(mati_id);
    auto reserialized_mati_data = mati->serializeToBuffer();

    ASSERT_EQ(original_mati_data.size(), reserialized_mati_data.size());
    ASSERT_TRUE((std::memcmp(original_mati_data.data(), reserialized_mati_data.data(), original_mati_data.size()) == 0));
}

GTEST_TEST(MATI, materialName) {
    const auto& repo = ResourceRepository::instance();
    const RuntimeId mati_id = 0x0000945079441bae16;

    const auto mati = repo->getResource<MATI>(mati_id);
    ASSERT_TRUE(mati);
    ASSERT_STREQ(mati->instanceName().c_str(), "travel_case_a.mi");
}

GTEST_TEST(MATI, Type) {
    const auto& repo = ResourceRepository::instance();
    const RuntimeId mati_id = 0x0000945079441bae16;

    const auto mati = repo->getResource<MATI>(mati_id);
    ASSERT_TRUE(mati);
    ASSERT_STREQ(mati->materialType().c_str(), "Standard");
}

GTEST_TEST(MATI, CompleteRepoReserialize) {
    const auto& repo = ResourceRepository::instance();
    const auto mati_ids = repo->getIdsByType("MATI");

    for (const auto& mati_id : mati_ids) {
        const auto mati = repo->getResource<MATI>(mati_id);
        ASSERT_TRUE(mati);

        auto original_mati_data = repo->getResource(mati_id);
        auto reserialized_mati_data = mati->serializeToBuffer();

        ASSERT_EQ(original_mati_data.size(), reserialized_mati_data.size());
        ASSERT_TRUE((std::memcmp(original_mati_data.data(), reserialized_mati_data.data(), original_mati_data.size()) == 0));
    }
}