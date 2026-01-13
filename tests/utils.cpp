#include <libpldm/pldm_types.h>
#include <libpldm/utils.h>

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

TEST(Ver2string, Ver2string)
{
    ver32_t version{0x61, 0x10, 0xf7, 0xf3};
    const char* vstr = "3.7.10a";
    char buffer[1024];
    auto rc = pldm_base_ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x00, 0xf0, 0xf0, 0xf1};
    vstr = "1.0.0";
    rc = pldm_base_ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x00, 0xf7, 0x01, 0x10};
    vstr = "10.01.7";
    rc = pldm_base_ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x00, 0xff, 0xf1, 0xf3};
    vstr = "3.1";
    rc = pldm_base_ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x61, 0xff, 0xf0, 0xf1};
    vstr = "1.0a";
    rc = pldm_base_ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    rc = pldm_base_ver2str(&version, buffer, 3);
    EXPECT_EQ(rc, 2);
    EXPECT_STREQ("1.", buffer);

    rc = pldm_base_ver2str(&version, buffer, 1);
    EXPECT_EQ(rc, 0);

    rc = pldm_base_ver2str(&version, buffer, 0);
    EXPECT_EQ(rc, -1);
}

TEST(TimeLegal, TimeLegal)
{
    EXPECT_EQ(true, is_time_legal(30, 25, 16, 18, 8, 2019));
    EXPECT_EQ(true, is_time_legal(30, 25, 16, 29, 2, 2020)); // leap year

    EXPECT_EQ(false, is_time_legal(30, 25, 16, 18, 8, 1960));  // year illegal
    EXPECT_EQ(false, is_time_legal(30, 25, 16, 18, 15, 2019)); // month illegal
    EXPECT_EQ(false, is_time_legal(30, 25, 16, 18, 0, 2019));  // month illegal
    EXPECT_EQ(false, is_time_legal(30, 25, 16, 0, 8, 2019));   // day illegal
    EXPECT_EQ(false, is_time_legal(30, 25, 16, 29, 2, 2019));  // day illegal
    EXPECT_EQ(false, is_time_legal(30, 25, 25, 18, 8, 2019));  // hours illegal
    EXPECT_EQ(false, is_time_legal(30, 70, 16, 18, 8, 2019)); // minutes illegal
    EXPECT_EQ(false, is_time_legal(80, 25, 16, 18, 8, 2019)); // seconds illegal
}
