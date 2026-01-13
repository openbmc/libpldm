/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <libpldm/base.h>
#include <libpldm/utils.h>

static int day_map(uint8_t month)
{
	switch (month) {
	case 1:
		return 31;
	case 2:
		return 28;
	case 3:
		return 31;
	case 4:
		return 30;
	case 5:
		return 31;
	case 6:
		return 30;
	case 7:
	case 8:
		return 31;
	case 9:
		return 30;
	case 10:
		return 31;
	case 11:
		return 30;
	case 12:
		return 31;
	default:
		return 0;
	}
}

LIBPLDM_ABI_DEPRECATED
bool is_time_legal(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t day,
		   uint8_t month, uint16_t year)
{
	if (month < 1 || month > 12) {
		return false;
	}
	int rday = day_map(month);
	if (month == 2 &&
	    ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
		rday += 1;
	}
	if (year < 1970 || day < 1 || day > rday || seconds > 59 ||
	    minutes > 59 || hours > 23) {
		return false;
	}
	return true;
}

LIBPLDM_ABI_DEPRECATED
bool is_transfer_flag_valid(uint8_t transfer_flag)
{
	switch (transfer_flag) {
	case PLDM_START:
	case PLDM_MIDDLE:
	case PLDM_END:
	case PLDM_START_AND_END:
		return true;

	default:
		return false;
	}
}
