
libpldm_sources += files(
  'base.c',
  'bios.c',
  'platform.c',
  'bios_table.c',
  'firmware_update.c',
  'fru.c',
  'pdr.c',
  'utils.c'
  )

if get_option('oem-ibm').enabled()
   add_project_arguments('-DOEM_IBM', language : 'c')
   add_project_arguments('-DOEM_IBM', language : 'cpp')
   subdir('oem/ibm')
endif

if get_option('requester-api').enabled()
   subdir('requester')
endif
