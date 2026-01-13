/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#include <libpldm/bcd.h>

LIBPLDM_ABI_STABLE
uint8_t pldm_bcd_bcd2dec8(uint8_t bcd)
{
	return (bcd >> 4) * 10 + (bcd & 0x0f);
}

LIBPLDM_ABI_STABLE
uint8_t pldm_bcd_dec2bcd8(uint8_t dec)
{
	return ((dec / 10) << 4) + (dec % 10);
}

LIBPLDM_ABI_STABLE
uint16_t pldm_bcd_bcd2dec16(uint16_t bcd)
{
	return pldm_bcd_bcd2dec8(bcd >> 8) * 100 +
	       pldm_bcd_bcd2dec8(bcd & 0xff);
}

LIBPLDM_ABI_STABLE
uint16_t pldm_bcd_dec2bcd16(uint16_t dec)
{
	return pldm_bcd_dec2bcd8(dec % 100) |
	       ((uint16_t)(pldm_bcd_dec2bcd8(dec / 100)) << 8);
}

LIBPLDM_ABI_STABLE
uint32_t pldm_bcd_bcd2dec32(uint32_t bcd)
{
	return pldm_bcd_bcd2dec16(bcd >> 16) * 10000 +
	       pldm_bcd_bcd2dec16(bcd & 0xffff);
}

LIBPLDM_ABI_STABLE
uint32_t pldm_bcd_dec2bcd32(uint32_t dec)
{
	return pldm_bcd_dec2bcd16(dec % 10000) |
	       ((uint32_t)(pldm_bcd_dec2bcd16(dec / 10000)) << 16);
}
