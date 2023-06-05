libpldm_include_dir += include_directories('.')

libpldm_headers += files(
  'base.h',
  'bios.h',
  'bios_table.h',
  'entity.h',
  'firmware_update.h',
  'fru.h',
  'pdr.h',
  'platform.h',
  'pldm_types.h',
  'state_set.h',
  'states.h',
  'utils.h',
  )

if get_option('oem-ibm').enabled()
  libpldm_include_dir += include_directories('oem/ibm')
  libpldm_headers += files(
    'oem/ibm/libpldm/entity_oem_ibm.h',
    'oem/ibm/libpldm/file_io.h',
    'oem/ibm/libpldm/fru_oem_ibm.h',
    'oem/ibm/libpldm/host.h',
    'oem/ibm/libpldm/platform_oem_ibm.h',
    'oem/ibm/libpldm/state_set_oem_ibm.h'
  )
endif

if get_option('requester-api').enabled()
   libpldm_headers += files(
    'instance-id.h',
    'requester/pldm.h',
    'transport.h'
    )
   libpldm_transport_headers += files(
    'transport/af-mctp.h',
    'transport/mctp-demux.h'
   )
endif

