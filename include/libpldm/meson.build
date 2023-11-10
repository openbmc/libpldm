libpldm_headers = files(
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
  'pldm.h',
  'state_set.h',
  'states.h',
  'transport.h',
  'transport/af-mctp.h',
  'transport/mctp-demux.h',
  'utils.h',
  )

if get_option('oem-ibm').allowed()
  libpldm_headers += files(
    'oem/ibm/entity.h',
    'oem/ibm/file_io.h',
    'oem/ibm/fru.h',
    'oem/ibm/host.h',
    'oem/ibm/platform.h',
    'oem/ibm/state_set.h'
  )

  install_symlink('file_io.h', pointing_to: 'oem/ibm/file_io.h', install_dir: 'include/libpldm')
  install_symlink('entity_oem_ibm.h', pointing_to: 'oem/ibm/entity.h', install_dir: 'include/libpldm')
  install_symlink('fru_oem_ibm.h', pointing_to: 'oem/ibm/fru.h', install_dir: 'include/libpldm')
  install_symlink('host.h', pointing_to: 'oem/ibm/host.h', install_dir: 'include/libpldm')
  install_symlink('platform_oem_ibm.h', pointing_to: 'oem/ibm/platform.h', install_dir: 'include/libpldm')
  install_symlink('state_set_oem_ibm.h', pointing_to: 'oem/ibm/state_set.h', install_dir: 'include/libpldm')
endif

install_headers(libpldm_headers, subdir: 'libpldm', preserve_path: true)
