if get_option('abi').contains('testing')
    executable(
        'platform_pd_responder',
        'platform_pd_responder.c',
        dependencies: [libpldm_dep],
    )
    executable(
        'file_fd_responder',
        'file_fd_responder.c',
        dependencies: [libpldm_dep],
    )
endif
