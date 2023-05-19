#ifndef PDR_OEM_IBM_H
#define PDR_OEM_IBM_H
#ifdef __cplusplus
extern "C" {
#endif
#include "platform.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/** @brief find the container ID of the contained entity
 *
 *  @param[in] repo - opaque pointer acting as a PDR repo handle
 *  @param[in] entityType - entity type
 *  @param[in] entityInstance - instance of the entity
 *  @param[in] firstRecordHandle - first record handle of the remote endpoint
 *  @param[in] lastRecordHandle - last record handle of the remote endpoint
 *
 *  @return container id of the PDR record found
 */
uint16_t pldm_find_container_id(const pldm_pdr *repo, uint16_t entityType,
				uint16_t entityInstance,
				uint32_t firstRecordHandle,
				uint32_t lastRecordHandle);
#ifdef __cplusplus
}
#endif
#endif /* PDR_OEM_IBM_H */
