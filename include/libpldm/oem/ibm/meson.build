libpldm_oem_headers = files(
    'entity.h',
    'file_io.h',
    'fru.h',
    'host.h',
    'platform.h',
    'state_set.h'
    )

install_headers(
  libpldm_oem_headers,
  subdir: 'libpldm/oem/ibm'
  )

install_symlink('file_io.h', pointing_to: 'oem/ibm/file_io.h', install_dir: 'include/libpldm')
install_symlink('entity_oem_ibm.h', pointing_to: 'oem/ibm/entity.h', install_dir: 'include/libpldm')
install_symlink('fru_oem_ibm.h', pointing_to: 'oem/ibm/fru.h', install_dir: 'include/libpldm')
install_symlink('host.h', pointing_to: 'oem/ibm/host.h', install_dir: 'include/libpldm')
install_symlink('platform_oem_ibm.h', pointing_to: 'oem/ibm/platform.h', install_dir: 'include/libpldm')
install_symlink('state_set_oem_ibm.h', pointing_to: 'oem/ibm/state_set.h', install_dir: 'include/libpldm')

