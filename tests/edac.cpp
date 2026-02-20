/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <libpldm/edac.h>

#include <gtest/gtest.h>

TEST(Crc32, CheckSumTest)
{
    const char* password = "123456789";
    auto checksum = pldm_edac_crc32(password, 9);
    EXPECT_EQ(checksum, 0xcbf43926);
}

#if HAVE_LIBPLDM_API_TESTING
TEST(Crc32, CumulativeCheckSumTest)
{
    const char* password = "123456789";
    auto partial_checksum = pldm_edac_crc32_extend(password, 4, 0);
    auto final_checksum =
        pldm_edac_crc32_extend(password + 4, 5, partial_checksum);
    EXPECT_EQ(final_checksum, 0xcbf43926);
}
#endif

TEST(Crc8, CheckSumTest)
{
    const char* data = "123456789";
    auto checksum = pldm_edac_crc8(data, 9);
    EXPECT_EQ(checksum, 0xf4);
}
