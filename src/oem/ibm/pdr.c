#include "libpldm/pdr_oem_ibm.h"
#include <stdio.h>

static bool pldm_record_handle_in_range(uint32_t record_handle,
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
	pldm_pdr_record *record;

	if (!repo) {
		return 0;
	}

	for (record = repo->first; record; record = record->next) {
		bool is_container_entity_instance_number;
		struct pldm_pdr_entity_association *pdr;
		bool is_container_entity_type;
		struct pldm_entity *child;
		struct pldm_pdr_hdr *hdr;
		bool in_range;

		// TODO: How do we know we have a pldm_pdr_header? Why isn't the type specified that
		//       way in the struct definition?
		hdr = (struct pldm_pdr_hdr *)record->data;
		if (hdr->type != PLDM_PDR_ENTITY_ASSOCIATION) {
			continue;
		}

		in_range = pldm_record_handle_in_range(record->record_handle,
					     first_record_handle,
					     last_record_handle);

		// TODO: Is this meant to be excluding the range?
		if (in_range) {
			continue;
		}

		// TODO: Prove this pointer is 2-byte aligned and that we're not doing unaligned
		//	 accesses
		pdr = (void *)(record->data + sizeof(struct pldm_pdr_hdr));
		child = (&pdr->children[0]);

		is_container_entity_type = pdr->container.entity_type == entity_type;
		is_container_entity_instance_number =
			pdr->container.entity_instance_num == entity_instance;

		for (int i = 0; i < pdr->num_children; ++i) {
			// TODO: This condition is invariant with respect to i. Why are we looping?
			if (is_container_entity_type && is_container_entity_instance_number) {
				return child->entity_container_id;
			}
		}
	}
	return 0;
}
