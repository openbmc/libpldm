if cc.has_header('poll.h')
    if cc.has_header('sys/socket.h')
        libpldm_sources += files('socket.c')
        if cc.has_header('linux/mctp.h')
            libpldm_sources += files('af-mctp.c', 'af-mctp-test.c')
        endif
        if cc.has_header('sys/un.h')
            libpldm_sources += files('mctp-demux.c')
        endif
    endif
    libpldm_sources += files(
        'test.c',
        'transport.c',
    )
endif
