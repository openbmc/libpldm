#ifndef PDR_OEM_IBM_H
#define PDR_OEM_IBM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>


pldm_pdr_record *pldm_pdr_find_last_local_record(const pldm_pdr *repo);

bool isHBRange(const uint32_t record_handle);

uint16_t pldm_find_container_id(const pldm_pdr *repo, uint16_t entityType,
				uint16_t entityInstance);
#ifdef __cplusplus
}
#endif

#endif /* PDR_OEM_IBM_H */
