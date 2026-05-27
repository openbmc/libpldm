/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
/*
 * platform_pd_responder - example PLDM Platform Device responder
 *
 * Reads one raw PLDM request message from stdin and writes the PLDM response
 * to stdout.  The responder supports GET_PDR_REPOSITORY_INFO, GET_PDR, and
 * GET_SENSOR_READING for sensor ID 1 (a uint8 temperature sensor).
 *
 * Usage:
 *   printf '\x80\x02\x11\x01\x00\x00' | ./platform_pd_responder | xxd
 *
 * The six-byte payload above is a GET_SENSOR_READING request for sensor 1
 * with no event rearm:
 *   80        - instance 0, request, D=0
 *   02        - PLDM type 2 (Platform)
 *   11        - command GET_SENSOR_READING (0x11)
 *   01 00     - sensor_id = 1 (little-endian)
 *   00        - rearm_event_state = false
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpldm/base.h>
#include <libpldm/pdr.h>
#include <libpldm/platform.h>
#include <libpldm/platform_pd.h>

/* Maximum size for a single PLDM message on stdin / stdout */
#define MSG_BUF_SIZE 4096

/* Simulated sensor reading: 72 degrees (arbitrary uint8) */
static uint8_t g_temperature = 72;

/*
 * get_sensor_reading callback - called by pldm_platform_pd_handle_msg when a
 * GET_SENSOR_READING request arrives for a sensor ID that has a matching PDR.
 *
 * ctx  - the pointer supplied in pldm_platform_pd_ops.ctx
 * pdr  - parsed numeric sensor PDR for the requested sensor
 * rearm_event_state - from the request
 * state - output struct to populate
 *
 * Returns a PLDM completion code (PLDM_SUCCESS on success).
 */
static uint8_t
get_sensor_reading(void *ctx,
		   const struct pldm_numeric_sensor_value_pdr *pdr
		   __attribute__((unused)),
		   bool8_t rearm_event_state __attribute__((unused)),
		   struct pldm_platform_pd_sensor_state *state)
{
	uint8_t *temperature = ctx;

	state->operational_state = PLDM_SENSOR_ENABLED;
	state->event_enable = PLDM_NO_EVENT_GENERATION;
	state->present_state = PLDM_SENSOR_NORMAL;
	state->previous_state = PLDM_SENSOR_NORMAL;
	state->event_state = PLDM_SENSOR_NORMAL;
	state->current_reading.value_u8 = *temperature;

	return PLDM_SUCCESS;
}

/*
 * Build and register a PLDM_NUMERIC_SENSOR_PDR for sensor ID 1.
 *
 * The PDR layout follows DSP0248 Table 78.  Only the fields that influence
 * GET_SENSOR_READING dispatch need non-zero values; everything else is zeroed.
 */
static int add_temperature_sensor_pdr(pldm_pdr *repo)
{
	/*
	 * Variable-length fields for PLDM_SENSOR_DATA_SIZE_UINT8:
	 *   hysteresis   1 byte
	 *   max_readable 1 byte
	 *   min_readable 1 byte
	 */
	static const uint8_t rec[] = {
		/* pldm_value_pdr_hdr (10 bytes) */
		0x00,
		0x00,
		0x00,
		0x00, /* record_handle - filled by pldm_pdr_add */
		0x01, /* version */
		PLDM_NUMERIC_SENSOR_PDR, /* type */
		0x00,
		0x00,			 /* record_change_num */
		/* length: PLDM_PDR_NUMERIC_SENSOR_PDR_MIN_LENGTH = 69 */
		0x45,
		0x00,

		/* pldm_numeric_sensor_value_pdr body */
		0x00,
		0x00,			     /* terminus_handle */
		0x01,
		0x00,			     /* sensor_id = 1 (little-endian) */
		0x00,
		0x00,			     /* entity_type */
		0x01,
		0x00,			     /* entity_instance_num */
		0x00,
		0x00,			     /* container_id */
		PLDM_NO_INIT,		     /* sensor_init */
		0x00,			     /* sensor_auxiliary_names_pdr */
		0x02,			     /* base_unit: degrees C */
		0x00,			     /* unit_modifier */
		0x00,			     /* rate_unit */
		0x00,			     /* base_oem_unit_handle */
		0x00,			     /* aux_unit */
		0x00,			     /* aux_unit_modifier */
		0x00,			     /* aux_rate_unit */
		0x00,			     /* rel */
		0x00,			     /* aux_oem_unit_handle */
		0x01,			     /* is_linear */
		PLDM_SENSOR_DATA_SIZE_UINT8, /* sensor_data_size */
		0x00,
		0x00,
		0x80,
		0x3f, /* resolution = 1.0f */
		0x00,
		0x00,
		0x00,
		0x00, /* offset = 0.0f */
		0x00,
		0x00, /* accuracy */
		0x00, /* plus_tolerance */
		0x00, /* minus_tolerance */
		0x00, /* hysteresis (1 byte for UINT8) */
		0x00, /* supported_thresholds */
		0x00, /* threshold_and_hysteresis_volatility */
		0x00,
		0x00,
		0x00,
		0x00, /* state_transition_interval */
		0x00,
		0x00,
		0x00,
		0x00,			       /* update_interval */
		0xff,			       /* max_readable = 255 */
		0x00,			       /* min_readable = 0 */
		PLDM_RANGE_FIELD_FORMAT_UINT8, /* range_field_format */
		0x00,			       /* range_field_support */
		/* range fields: nominal, normal_max, normal_min,
		 * warning_high, warning_low, critical_high, critical_low,
		 * fatal_high, fatal_low  (9 x 1 byte for UINT8) */
		0x00,
		0x00,
		0x00, /* nominal, normal_max, normal_min */
		0x00,
		0x00, /* warning_high, warning_low */
		0x00,
		0x00, /* critical_high, critical_low */
		0x00,
		0x00, /* fatal_high, fatal_low */
	};

	uint32_t handle = 0;
	return pldm_pdr_add(repo, rec, sizeof(rec), false, 0, &handle);
}

int main(void)
{
	uint8_t in_buf[MSG_BUF_SIZE];
	uint8_t out_buf[MSG_BUF_SIZE];
	size_t in_len;
	size_t out_len;
	pldm_pdr *repo;
	struct pldm_platform_pd *pd;
	struct pldm_platform_pd_ops ops;
	int rc;
	int status = EXIT_FAILURE;

	/* Read one PLDM message from stdin */
	in_len = fread(in_buf, 1, sizeof(in_buf), stdin);
	if (in_len == 0) {
		fprintf(stderr, "error: no input\n");
		return EXIT_FAILURE;
	}

	/* Set up PDR repository and register the temperature sensor */
	repo = pldm_pdr_init();
	if (!repo) {
		fprintf(stderr, "error: pldm_pdr_init failed\n");
		return EXIT_FAILURE;
	}

	rc = add_temperature_sensor_pdr(repo);
	if (rc) {
		fprintf(stderr, "error: add_temperature_sensor_pdr: %s\n",
			strerror(-rc));
		goto cleanup_repo;
	}

	/* Populate the ops table with our sensor reading callback */
	memset(&ops, 0, sizeof(ops));
	ops.get_sensor_reading = get_sensor_reading;
	ops.ctx = &g_temperature;

	/* Allocate and initialise the Platform Device responder */
	pd = pldm_platform_pd_new(repo, NULL, &ops, sizeof(ops));
	if (!pd) {
		fprintf(stderr, "error: pldm_platform_pd_new failed\n");
		goto cleanup_repo;
	}

	/* Dispatch the incoming request and produce a response */
	out_len = sizeof(out_buf);
	rc = pldm_platform_pd_handle_msg(pd, in_buf, in_len, out_buf, &out_len);
	if (rc) {
		fprintf(stderr, "error: pldm_platform_pd_handle_msg: %s\n",
			strerror(-rc));
		goto cleanup_pd;
	}

	/* Write the response to stdout */
	if (fwrite(out_buf, 1, out_len, stdout) != out_len) {
		fprintf(stderr, "error: fwrite response\n");
		goto cleanup_pd;
	}

	status = EXIT_SUCCESS;

cleanup_pd:
	free(pd);
cleanup_repo:
	pldm_pdr_destroy(repo);
	return status;
}
