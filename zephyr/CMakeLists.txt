# Copyright (c) 2026 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PLDM)
	set(PLDM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

	zephyr_interface_library_named(pldm)
	zephyr_include_directories(${PLDM_ROOT}/include)
	zephyr_include_directories(${PLDM_ROOT}/src)

	zephyr_library_named(modules_pldm)
	zephyr_library_link_libraries(pldm)


	# While we follow what is done in the meson.build and mark all symbols as hidden
	# by default, that is somewhat useless for Zephyr, as everything is compiled together.
	# Do this anyway so visibility attributes are properly set. One could enforce
	# them on Zephyr during post-build.
	set_target_properties(modules_pldm PROPERTIES C_VISIBILITY_PRESET hidden)
	set_target_properties(modules_pldm PROPERTIES CXX_VISIBILITY_PRESET hidden)

	target_compile_definitions(modules_pldm PUBLIC LIBPLDM_ABI_STABLE=__attribute__\(\(visibility\("default"\)\)\) _GNU_SOURCE)

if (CONFIG_PLDM_ABI_TESTING)
	target_compile_definitions(modules_pldm PUBLIC LIBPLDM_ABI_TESTING=__attribute__\(\(visibility\("default"\)\)\))
else()
	target_compile_definitions(modules_pldm PUBLIC LIBPLDM_ABI_TESTING= )
endif()

if (CONFIG_PLDM_ABI_DEPRECATED)
	target_compile_definitions(modules_pldm PUBLIC LIBPLDM_ABI_DEPRECATED=__attribute__\(\(visibility\("default"\)\)\))
else()
	target_compile_definitions(modules_pldm PUBLIC LIBPLDM_ABI_DEPRECATED= )
endif()

if (CONFIG_PLDM_ABI_DEPRECATED_UNSAFE)
	target_compile_definitions(modules_pldm PUBLIC LIBPLDM_ABI_DEPRECATED_UNSAFE=__attribute__\(\(visibility\("default"\)\)\))
else()
	target_compile_definitions(modules_pldm PUBLIC LIBPLDM_ABI_DEPRECATED_UNSAFE= )
endif()

	zephyr_library_sources(
		${PLDM_ROOT}/src/utils.c
		${PLDM_ROOT}/src/responder.c

		${PLDM_ROOT}/src/dsp/base.c
		${PLDM_ROOT}/src/dsp/bios.c
		${PLDM_ROOT}/src/dsp/bios_table.c
		${PLDM_ROOT}/src/dsp/firmware_update.c
		${PLDM_ROOT}/src/dsp/fru.c
		${PLDM_ROOT}/src/dsp/pdr.c
		${PLDM_ROOT}/src/dsp/platform.c
	)

endif()
