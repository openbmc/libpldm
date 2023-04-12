#ifndef INSTANCE_ID_H
#define INSTANCE_ID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libpldm/base.h"
#include <stdint.h>

typedef uint8_t pldm_inst_id_t;
struct pldm_instance_id;

/**
 * @brief Allocates an instance ID object for a given database path
 *
 * @param[out] ctx - *ctx will point to a pldm instance ID object on success
 * @param[in] dbpath - the path to the instance ID database file to use
 *
 * @return int
 * */
int pldm_instance_id_init(struct pldm_instance_id **ctx, const char *dbpath);

/**
 * @brief Allocates an instance ID object for the default database path
 *
 * @param[out] ctx - *ctx will point to a pldm instance ID object on success
 *
 * @return int
 * */
int pldm_instance_id_init_default(struct pldm_instance_id **ctx);

/**
 * @brief Destroys an instance ID instance
 *
 * @param[in] ctx - pldm instance ID instance
 *
 * @return int
 * */
int pldm_instance_id_destroy(struct pldm_instance_id *ctx);

/**
 * @brief Allocates an instance ID for a destination TID
 *
 * @param[in] ctx - pldm instance ID object
 * @param[in] tid - PLDM TID
 * @param[in] iid - caller owned pointer to a PLDM instance ID object. Return
 * 	      EINVAL if this is NULL. On success, this points to an instance ID
 * 	      to use for a PLDM request message. If there are no instance IDs
 * 	      available, EAGAIN is returned.
 *
 * @return int
 */
int pldm_instance_id_alloc(struct pldm_instance_id *ctx, pldm_tid_t tid,
			   pldm_inst_id_t *iid);

/**
 * @brief Frees an instance id previously allocated by
 * 	  pldm_instance_id_alloc
 *
 * @param[in] ctx - pldm instance ID object
 * @param[in] tid - PLDM TID
 * @param[in] iid - If this instance ID was not previously allocated by
 * 	      pldm_instance_id_alloc then EINVAL is returned.
 *
 * @return int
 */
int pldm_instance_id_free(struct pldm_instance_id *ctx, pldm_tid_t tid,
			  pldm_inst_id_t iid);

#ifdef __cplusplus
}
#endif

#endif /* INSTANCE_ID_H */
