#include <gtest/gtest.h>
#include <filesystem>
#include "GlacierFormats.h"

using namespace GlacierFormats;

//TODO: Create list of resources, representative of particular subtypes. For PRIM,  find meshes that use different sets of features for example.
//Weighted, standard, with and without cloth, submesh tables etc. 
//For TEXT, at least one file for each compression type. etc.
GTEST_TEST(PRIM, PrimSerialization) {
    auto repo = ResourceRepository::instance();
    auto prim = repo->getResource<PRIM>(0x002F5293D4F41A8D);
    ASSERT_TRUE(prim);

    std::unique_ptr<char[]> data = nullptr;
    auto data_size = prim->serializeToBuffer(data);
    ASSERT_TRUE(data_size);
}

GTEST_TEST(BinaryReader, BufferRead)
{
    char test_data[] = {
            0x01, 0xFF, 0x01, 0x38, 0x31, 0x34, 0x35, 0x54, 0x65, 0x73, 0x74, 0x54,
            0x65, 0x73, 0x74, 0x54, 0x65, 0x73, 0x74, 0x00, 0x01, 0x02, 0x03, 0x04
    };

    BinaryReader br(test_data, sizeof(test_data));
    ASSERT_TRUE(br.read<char>() == 0x01);
    ASSERT_TRUE(br.read<short>() == 0x01FF);
    ASSERT_TRUE(br.read<int>() == 0x35343138);
    ASSERT_TRUE((br.readString<4, BinaryReader::Endianness::LE>() == std::string("tseT")));
    ASSERT_TRUE((br.readString<4, BinaryReader::Endianness::BE>() == std::string("Test")));
    ASSERT_TRUE((br.readCString() == std::string("Test")));

    char buf[4];
    br.read(buf, 4);
    ASSERT_TRUE(buf[0] == 1);
    ASSERT_TRUE(buf[1] == 2);
    ASSERT_TRUE(buf[2] == 3);
    ASSERT_TRUE(buf[3] == 4);

    ASSERT_THROW(br.read<char>(), InvalidArgumentsException);
}

GTEST_TEST(BinaryReader, BufferSeekTellAlign)
{
    char buf[] = { 1,2, 0,0, 1,2,3,4, 0,0,0,0 };
    BinaryReader br(buf, sizeof(buf));
    ASSERT_TRUE(br.tell() == 0);
    br.seek(2);
    ASSERT_TRUE(br.tell() == 2);
    br.align<4>();
    ASSERT_TRUE(br.tell() == 4);
    br.align<4>();
    ASSERT_TRUE(br.tell() == 4);
    br.seek(sizeof(buf));
    br.align<12>();
    ASSERT_THROW(br.seek(sizeof(buf) + 1), InvalidArgumentsException);
}

GTEST_TEST(BinaryWriter, BufferWrite) {
    char soll_data[] = {
        0x01, 0xFF, 0x01, 0x38, 0x31, 0x34, 0x35, 0x74, 0x73, 0x65, 0x54, 0x54,
        0x65, 0x73, 0x74, 0x54, 0x65, 0x73, 0x74, 0x00, 0x01, 0x02, 0x03, 0x04,
        0x00, 0x00, 0x00, 0x00
    };

    BinaryWriter bw;
    bw.write<char>(0x01);
    bw.write<short>(0x01FF);
    bw.write<int>(0x35343138);
    bw.writeLEString("Test");
    bw.writeBEString("Test");
    bw.writeCString("Test");
    char buf[] = { 1, 2, 3, 4 };
    bw.write(buf, sizeof(buf));
    bw.align<8>();//align while already aligned 
    bw.write<char>('\0');
    bw.align<4>();//align while not aligned.
    auto is_data = bw.release();

    ASSERT_TRUE(is_data.size() == sizeof(soll_data));
    ASSERT_TRUE(memcmp(is_data.data(), soll_data, sizeof(soll_data)) == 0);
}

GTEST_TEST(BinaryWriter, BufferSeekTell) {
    char soll_data[] = {
        0x01, 0x01, 0x01, 0x01, 0x31, 0x34, 0x35
    };

    BinaryWriter bw;
    bw.write<char>(0x01);
    bw.write<short>(0x01FF);
    bw.write<int>(0x35343138);
    ASSERT_TRUE(bw.tell() == 7);
    bw.seek(0);
    ASSERT_TRUE(bw.tell() == 0);
    bw.write(0x01010101);
    ASSERT_TRUE(bw.tell() == 4);
    auto is_data = bw.release();

    ASSERT_TRUE(is_data.size() == sizeof(soll_data));
    ASSERT_TRUE(memcmp(is_data.data(), soll_data, sizeof(soll_data)) == 0);
}


int main(int argc, char** argv)
{
    //Warning, GlacierInit initilizes the ResourceRepository singleton which is used by all tests.
    //This violates the principle of test independence but short of getting rid of the singleton entirely,
    //this is the best we can do here. 
    GlacierInit();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}