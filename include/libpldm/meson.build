libpldm_headers = files(
    'api.h',
    'base.h',
    'bcd.h',
    'bios.h',
    'bios_table.h',
    'compiler.h',
    'control.h',
    'edac.h',
    'entity.h',
    'file.h',
    'firmware_fd.h',
    'firmware_update.h',
    'fru.h',
    'instance-id.h',
    'pdr.h',
    'platform.h',
    'pldm.h',
    'pldm_types.h',
    'rde.h',
    'smbios.h',
    'state_set.h',
    'states.h',
    'transport.h',
    'transport/af-mctp.h',
    'transport/mctp-demux.h',
)

if get_option('oem').contains('ibm')
    libpldm_headers += files(
        'oem/ibm/entity.h',
        'oem/ibm/file_io.h',
        'oem/ibm/fru.h',
        'oem/ibm/host.h',
        'oem/ibm/platform.h',
        'oem/ibm/state_set.h',
    )
endif

if get_option('oem').contains('meta')
    libpldm_headers += files('oem/meta/file_io.h')
endif


install_headers(libpldm_headers, subdir: 'libpldm', preserve_path: true)

# TODO: these should depend on the input headers so they rebuild
# on changes, unclear how to do that.
include_src = include_directories('../../src', is_system: true)
sizeof_pldm_fd = cc.sizeof(
    'struct pldm_fd',
    prefix: '#include "firmware_device/fd-internal.h"',
    include_directories: [include_src, include_root],
)
sizeof_pldm_control = cc.sizeof(
    'struct pldm_control',
    prefix: '#include "control-internal.h"',
    include_directories: [include_src, include_root],
)
sizes_h = configure_file(
    configuration: {
        'sizeof_pldm_fd': sizeof_pldm_fd,
        'sizeof_pldm_control': sizeof_pldm_control,
    },
    input: 'sizes.h.in',
    output: 'sizes.h',
    install: true,
    install_dir: get_option('includedir') / 'libpldm',
)
