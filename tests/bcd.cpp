#include <libpldm/bcd.h>

#include <gtest/gtest.h>

TEST(BcdConversion, BcdCoversion)
{
    EXPECT_EQ(12u, pldm_bcd_bcd2dec8(0x12));
    EXPECT_EQ(99u, pldm_bcd_bcd2dec8(0x99));
    EXPECT_EQ(1234u, pldm_bcd_bcd2dec16(0x1234));
    EXPECT_EQ(9999u, pldm_bcd_bcd2dec16(0x9999));
    EXPECT_EQ(12345678u, pldm_bcd_bcd2dec32(0x12345678));
    EXPECT_EQ(99999999u, pldm_bcd_bcd2dec32(0x99999999));

    EXPECT_EQ(0x12u, pldm_bcd_dec2bcd8(12));
    EXPECT_EQ(0x99u, pldm_bcd_dec2bcd8(99));
    EXPECT_EQ(0x1234u, pldm_bcd_dec2bcd16(1234));
    EXPECT_EQ(0x9999u, pldm_bcd_dec2bcd16(9999));
    EXPECT_EQ(0x12345678u, pldm_bcd_dec2bcd32(12345678));
    EXPECT_EQ(0x99999999u, pldm_bcd_dec2bcd32(99999999));
}
