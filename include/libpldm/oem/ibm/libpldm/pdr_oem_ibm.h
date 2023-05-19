#ifndef PDR_OEM_IBM_H
#define PDR_OEM_IBM_H
#ifdef __cplusplus
extern "C" {
#endif
#include "platform.h"
#include "pdr.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/** @brief find the container ID of the contained entity
 *
 *  @param[in] repo - opaque pointer acting as a PDR repo handle
 *  @param[in] entity_type - entity type
 *  @param[in] entity_instance - instance of the entity
 *  @param[in] first_record_handle - first record handle of the remote endpoint
 *  @param[in] last_record_handle - last record handle of the remote endpoint
 *
 *  @return container id of the PDR record found
 */
uint16_t pldm_pdr_find_container_id(const pldm_pdr *repo, uint16_t entity_type,
				    uint16_t entity_instance,
				    uint32_t first_record_handle,
				    uint32_t last_record_handle);
#ifdef __cplusplus
}
#endif
#endif /* PDR_OEM_IBM_H */
