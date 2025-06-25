#include <libpldm/pldm_types.h>
#include <libpldm/utils.h>

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

TEST(Crc32, CheckSumTest)
{
    const char* password = "123456789";
    auto checksum = pldm_edac_crc32(password, 9);
    EXPECT_EQ(checksum, 0xcbf43926);
}

TEST(Crc32, ValidateCheckSum)
{
    const char* password = "123456789";
    uint32_t expected = 0xcbf43926;

    /* Case 1: checksum matches */
    EXPECT_EQ(pldm_edac_crc32_validate(expected, password, 9), 0);

    /* Case 2: checksum does not match */
    EXPECT_EQ(pldm_edac_crc32_validate(0xdeadbeef, password, 9), -EUCLEAN);

    /* Case 3: data is NULL but size is not zero (invalid argument) */
    EXPECT_EQ(pldm_edac_crc32_validate(expected, nullptr, 9), -EFAULT);

    /* Case 4: data is NULL and size is zero (should match, as empty data is
     * valid) */
    EXPECT_EQ(pldm_edac_crc32_validate(0, nullptr, 0), 0);
}

TEST(Crc8, CheckSumTest)
{
    const char* data = "123456789";
    auto checksum = pldm_edac_crc8(data, 9);
    EXPECT_EQ(checksum, 0xf4);
}

TEST(Crc8, ValidateCheckSum)
{
    const char* data = "123456789";
    uint8_t expected = 0xf4;

    /* Case 1: checksum matches */
    EXPECT_EQ(pldm_edac_crc8_validate(expected, data, 9), 0);

    /* Case 2: checksum does not match */
    EXPECT_EQ(pldm_edac_crc8_validate(0x12, data, 9), -EUCLEAN);

    /* Case 3: data is NULL but size is not zero (invalid argument) */
    EXPECT_EQ(pldm_edac_crc8_validate(expected, nullptr, 9), -EFAULT);

    /* Case 4: data is NULL and size is zero (should match, as empty data is
     * valid) */
    EXPECT_EQ(pldm_edac_crc8_validate(0, nullptr, 0), 0);
}

TEST(Ver2string, Ver2string)
{
    ver32_t version{0x61, 0x10, 0xf7, 0xf3};
    const char* vstr = "3.7.10a";
    char buffer[1024];
    auto rc = ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x00, 0xf0, 0xf0, 0xf1};
    vstr = "1.0.0";
    rc = ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x00, 0xf7, 0x01, 0x10};
    vstr = "10.01.7";
    rc = ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x00, 0xff, 0xf1, 0xf3};
    vstr = "3.1";
    rc = ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    version = {0x61, 0xff, 0xf0, 0xf1};
    vstr = "1.0a";
    rc = ver2str(&version, buffer, sizeof(buffer));
    EXPECT_EQ(rc, (signed)std::strlen(vstr));
    EXPECT_STREQ(vstr, buffer);

    rc = ver2str(&version, buffer, 3);
    EXPECT_EQ(rc, 2);
    EXPECT_STREQ("1.", buffer);

    rc = ver2str(&version, buffer, 1);
    EXPECT_EQ(rc, 0);

    rc = ver2str(&version, buffer, 0);
    EXPECT_EQ(rc, -1);
}

TEST(BcdConversion, BcdCoversion)
{
    EXPECT_EQ(12u, bcd2dec8(0x12));
    EXPECT_EQ(99u, bcd2dec8(0x99));
    EXPECT_EQ(1234u, bcd2dec16(0x1234));
    EXPECT_EQ(9999u, bcd2dec16(0x9999));
    EXPECT_EQ(12345678u, bcd2dec32(0x12345678));
    EXPECT_EQ(99999999u, bcd2dec32(0x99999999));

    EXPECT_EQ(0x12u, dec2bcd8(12));
    EXPECT_EQ(0x99u, dec2bcd8(99));
    EXPECT_EQ(0x1234u, dec2bcd16(1234));
    EXPECT_EQ(0x9999u, dec2bcd16(9999));
    EXPECT_EQ(0x12345678u, dec2bcd32(12345678));
    EXPECT_EQ(0x99999999u, dec2bcd32(99999999));
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
