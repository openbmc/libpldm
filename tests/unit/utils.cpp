#include <libpldm/pldm_types.h>

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

// Prevent clang-format from re-ordering the included source file
// clang-format: off
// Include the implementation so we can test internal interfaces
// NOLINTNEXTLINE(bugprone-suspicious-include)
#include "utils.c"
// clang-format: on

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
