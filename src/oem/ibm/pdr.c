#include "libpldm/pdr_oem_ibm.h"
#include <stdio.h>

static bool pldm_is_endpoint_range(uint32_t record_handle,
				   uint32_t first_record_handle,
				   uint32_t last_record_handle)
{
	return record_handle >= first_record_handle &&
	       record_handle <= last_record_handle;
}

uint16_t pldm_pdr_find_container_id(const pldm_pdr *repo, uint16_t entity_type,
				    uint16_t entity_instance,
				    uint32_t first_record_handle,
				    uint32_t last_record_handle)
{
	if (!repo) {
		return 0;
	}
	pldm_pdr_record *record = repo->first;
	while (record != NULL) {
		struct pldm_pdr_hdr *hdr = (struct pldm_pdr_hdr *)record->data;
		if (hdr->type == PLDM_PDR_ENTITY_ASSOCIATION &&
		    !(pldm_is_endpoint_range(record->record_handle,
					     first_record_handle,
					     last_record_handle))) {
			struct pldm_pdr_entity_association *pdr =
				(struct pldm_pdr_entity_association
					 *)((uint8_t *)record->data +
					    sizeof(struct pldm_pdr_hdr));
			struct pldm_entity *child = (&pdr->children[0]);
			bool is_container_entity_type;
			bool is_container_entity_instance_number;

			is_container_entity_type = pdr->container.entity_type ==
						   entity_type;
			is_container_entity_instance_number =
				pdr->container.entity_instance_num ==
				entity_instance;
			for (int i = 0; i < pdr->num_children; ++i) {
				if (is_container_entity_type &&
				    is_container_entity_instance_number) {
					uint16_t id =
						child->entity_container_id;
					return id;
				}
			}
		}
		record = record->next;
	}
	return 0;
}
