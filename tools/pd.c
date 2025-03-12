/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
/* Copyright 2025 Code Construct */

/*
 * `grep 'p.*l.*d.*m' /usr/share/dict/words` found 'palladium', which has
 * element symbol Pd.
 */

#include <assert.h>
#include <err.h>
#include <inttypes.h>
#include <libpldm/firmware_update.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PD_PACKAGE_BUFFER (1ul * 1024ul * 1024ul)

static void pd_print_bytes(const char *head, const void *_buf, size_t len,
			   const char *tail)
{
	const uint8_t *buf = _buf;

	if (head) {
		printf("%s", head);
	}

	while (len-- > 1) {
		printf("%02" PRIx8 " ", *buf++);
	}

	printf("%02" PRIx8, *buf++);

	if (tail) {
		printf("%s", tail);
	}
}

static void pd_print_variable_field(const char *head,
				    const struct variable_field *field,
				    const char *tail)
{
	if (head) {
		printf("%s", head);
	}

	if (field && field->ptr) {
		fwrite(field->ptr, 1, field->length, stdout);
	}

	if (tail) {
		printf("%s", tail);
	}
}

static void pd_print_typed_string(const char *head, size_t type,
				  const struct variable_field *string,
				  const char *tail)
{
	switch (type) {
	case 1:
		pd_print_variable_field(head, string, tail);
		break;
	default:
		printf("Unsupported string type: %zu\n", type);
		break;
	}
}

static void pd_print_descriptor(const char *head,
				const struct pldm_descriptor *desc,
				const char *tail)
{
	if (head) {
		printf("%s", head);
	}

	assert(desc);
	switch (desc->descriptor_type) {
	case 1: {
		uint32_t pen;

		memcpy(&pen, desc->descriptor_data, sizeof(pen));
		pen = le32toh(pen);
		printf("IANA PEN: %" PRIu32, pen);
		break;
	}
	default:
		printf("Unsupported descriptor type: %" PRIu16,
		       desc->descriptor_type);
		break;
	}

	if (tail) {
		printf("%s", tail);
	}
}

int main(void)
{
	struct pldm_firmware_update_package_iter iter;
	struct pldm_downstream_device_id_record ddrec;
	pldm_component_image_information_pad info;
	pldm_firmware_device_id_record_pad fdrec;
	size_t nr_fdrecs = 0;
	size_t nr_ddrecs = 0;
	size_t nr_infos = 0;
	void *package;
	int status;
	size_t in;
	int rc;

	status = EXIT_SUCCESS;
	package = calloc(PD_PACKAGE_BUFFER, 1);
	if (!package) {
		err(EXIT_FAILURE, "malloc");
	}

	in = fread(package, 1, PD_PACKAGE_BUFFER, stdin);
	rc = decode_pldm_firmware_update_package(package, in, &iter);
	if (rc < 0) {
		warnx("Failed to parse PLDM package: %s\n",
		      strerrorname_np(-rc));
		status = EXIT_FAILURE;
		goto cleanup_package;
	}

	printf("Package header\n");
	pd_print_bytes("\tIdentifier: 0x [ ",
		       iter.hdr.package_header_identifier,
		       sizeof(iter.hdr.package_header_identifier), " ]\n");
	printf("\tFormat revision: %" PRIu8 "\n",
	       iter.hdr.package_header_format_revision);
	fwrite("\n", 1, 1, stdout);

	foreach_pldm_firmware_device_id_record(iter, fdrec, rc)
	{
		struct pldm_descriptor desc;

		printf("Firmware device ID record: %zu\n", nr_fdrecs++);
		printf("\tDevice update option flags: %#.8" PRIx32 "\n",
		       fdrec.device_update_option_flags.value);
		pd_print_typed_string(
			"\tComponent image set version: ",
			fdrec.component_image_set_version_string_type,
			&fdrec.component_image_set_version_string, "\n");
		pd_print_bytes("\tApplicable components: 0x [ ",
			       fdrec.applicable_components.bitmap.ptr,
			       fdrec.applicable_components.bitmap.length,
			       " ]\n");

		printf("\tDescriptors:\n");
		foreach_pldm_firmware_device_id_record_descriptor(iter, fdrec,
								  desc, rc)
		{
			pd_print_descriptor("\t\t", &desc, "\n");
		}
		if (rc) {
			warnx("Failed parsing firmware device ID record descriptors: %s\n",
			      strerrorname_np(-rc));
			status = EXIT_FAILURE;
			goto cleanup_package;
		}
		fwrite("\n", 1, 1, stdout);
	}
	if (rc) {
		warnx("Failed parsing firmware device ID records: %s\n",
		      strerrorname_np(-rc));
		status = EXIT_FAILURE;
		goto cleanup_package;
	}

	foreach_pldm_downstream_device_id_record(iter, ddrec, rc)
	{
		struct pldm_descriptor desc;

		printf("Downstream device ID record: %zu\n", nr_ddrecs++);
		printf("\tDevice update option flags: %#.4" PRIx32 "\n",
		       ddrec.update_option_flags.value);
		pd_print_typed_string(
			"\tSelf-contained activation min version: ",
			ddrec.self_contained_activation_min_version_string_type,
			&ddrec.self_contained_activation_min_version_string,
			"\n");
		pd_print_bytes("\tApplicable components: 0x [ ",
			       ddrec.applicable_components.bitmap.ptr,
			       ddrec.applicable_components.bitmap.length,
			       " ]\n");
		printf("\tDescriptors:\n");
		foreach_pldm_downstream_device_id_record_descriptor(iter, ddrec,
								    desc, rc)
		{
			pd_print_descriptor("\t\t", &desc, "\n");
		}
		if (rc) {
			warnx("Failed parsing downstream device ID record descriptors: %s\n",
			      strerrorname_np(-rc));
			status = EXIT_FAILURE;
			goto cleanup_package;
		}
		fwrite("\n", 1, 1, stdout);
	}
	if (rc) {
		warnx("Failed parsing downstream device ID records: %s\n",
		      strerrorname_np(-rc));
		status = EXIT_FAILURE;
		goto cleanup_package;
	}

	foreach_pldm_component_image_information(iter, info, rc)
	{
		printf("Component image info: %zu\n", nr_infos++);
		printf("\tComponent classification: %" PRIu16 "\n",
		       info.component_classification);
		printf("\tComponent identifier: %" PRIu16 "\n",
		       info.component_identifier);
		printf("\tComponent comparison stamp: %" PRIu32 "\n",
		       info.component_comparison_stamp);
		printf("\tComponent options: %#.4" PRIx16 "\n",
		       info.component_options.value);
		printf("\tRequested activation method: %#.4" PRIx16 "\n",
		       info.requested_component_activation_method.value);
		printf("\tComponent image: %p (%zu)\n",
		       (void *)info.component_image.ptr,
		       info.component_image.length);
		pd_print_typed_string("\tComponent version: ",
				      info.component_version_string_type,
				      &info.component_version_string, "\n");
		fwrite("\n", 1, 1, stdout);
	}
	if (rc) {
		warnx("Failed parsing component image information: %s\n",
		      strerrorname_np(-rc));
		status = EXIT_FAILURE;
		goto cleanup_package;
	}

cleanup_package:
	free(package);

	exit(status);
}
