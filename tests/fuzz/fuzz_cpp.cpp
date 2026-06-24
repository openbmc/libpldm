#include <libpldm/api.h>
#include <libpldm/firmware_update.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <libpldm++/firmware_update.hpp>

#include "array.h"

static int fuzz_package_parser_parse(const uint8_t* data, size_t size)
{
    static const uint8_t package_header_identifiers[][16] = {
        PLDM_PACKAGE_HEADER_IDENTIFIER_V1_0,
        PLDM_PACKAGE_HEADER_IDENTIFIER_V1_1,
        PLDM_PACKAGE_HEADER_IDENTIFIER_V1_2,
        PLDM_PACKAGE_HEADER_IDENTIFIER_V1_3,
    };

    const uint8_t (*id)[16];
    uint8_t* package;

    if (size < sizeof(*id))
    {
        return -1;
    }

    package = (uint8_t*)calloc(size, 1);
    if (!package)
    {
        return 0;
    }

    memcpy(package, data, size);

    // the fuzzing data will choose which package header to use
    const uint8_t id_idx = data[0] % ARRAY_SIZE(package_header_identifiers);

    id = &package_header_identifiers[id_idx];

    memcpy(package, package_header_identifiers[id_idx], sizeof(*id));

    const std::span<uint8_t> p(package, size);

    std::expected<std::unique_ptr<pldm::fw_update::Package>,
                  pldm::fw_update::PackageParserError>
        res = pldm::fw_update::PackageParser::parse(
            p, pldm::fw_update::PackagePin::v1);

    if (!res.has_value())
    {
        free(package);
        return 0;
    }

    const auto& parsed = res.value();

    for (const auto& fwdevidrecord : parsed->firmwareDeviceIdRecords)
    {

        std::cout << fwdevidrecord.componentImageSetVersionString << std::endl;

        for (const auto& descriptorType : fwdevidrecord.getDescriptorTypes())
        {
            std::cout << descriptorType << std::endl;
        }

        for (const auto& ac : fwdevidrecord.applicableComponents)
        {
            std::cout << ac << std::endl;
        }
    }

    free(package);
    return 0;
}

static int (*const fuzz_tests[])(const uint8_t*, size_t) = {
    fuzz_package_parser_parse,
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    int rc = 0;
    for (const auto& fuzz_test : fuzz_tests)
    {
        rc += fuzz_test(data, size);
    }
    return -rc == ARRAY_SIZE(fuzz_tests) ? -1 : 0;
}
