/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef MCTP_DEFINES_H
#define MCTP_DEFINES_H

#ifdef __cplusplus
extern "C" {
#endif

#define MCTP_MSG_TYPE_PLDM 1

#define MCTP_NULL_EID 0
/**
 * In `Table 2 - Special endpoint IDs` of DSP0236.
 * EID from 1 to 7 is reserved EID. So the start valid EID is 8
 */
#define MCTP_START_VALID_EID 8
#define MCTP_BMC_EID	     8
#define MCTP_BROADCAST_EID   0xff
#define MCTP_MAX_NUM_EID     256

#ifdef __cplusplus
}
#endif

#endif // MCTP_DEFINES_H
