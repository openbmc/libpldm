libpldm_include_dir += include_directories('.')

libpldm_headers += files(
  'base.h',
  'bios.h',
  'bios_table.h',
  'entity.h',
  'firmware_update.h',
  'fru.h',
  'instance-id.h',
  'pdr.h',
  'platform.h',
  'pldm_types.h',
  'requester/pldm.h',
  'state_set.h',
  'states.h',
  'transport.h',
  'utils.h',
  )

libpldm_transport_headers += files(
  'transport/af-mctp.h',
  'transport/mctp-demux.h'
)

if get_option('oem-ibm').allowed()
  subdir('oem/ibm')
endif
