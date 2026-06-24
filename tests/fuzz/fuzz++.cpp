#include <libpldm/api.h>
#include <libpldm/firmware_update.h>

#include <cstdlib>
#include <cstring>
#include <libpldm++/firmware_update.hpp>

#include "array.h"
#include "compiler.h"

extern "C" int pldm_edac_crc32_validate(uint32_t expected LIBPLDM_CC_UNUSED,
                                        const void* data LIBPLDM_CC_UNUSED,
                                        size_t size LIBPLDM_CC_UNUSED)
{
    return 0;
}

static volatile uint64_t sink = 0;

template <typename T>
static void consumeCopy(T value)
{
    sink ^= static_cast<uint64_t>(value);
}

static void consume(const std::string& s)
{
    for (const char c : s)
    {
        consumeCopy(c);
    }
}

static void consume(const variable_field& vf)
{
    for (size_t i = 0; i < vf.length; i++)
    {
        consumeCopy(vf.ptr[i]);
    }
}

template <typename T>
static void consume(const std::optional<T>& opt)
{
    if (opt.has_value())
    {
        consume(opt.value());
    }
}

template <typename Range>
static void consumeRange(const Range& range)
{
    for (const auto& item : range)
    {
        consumeCopy(item);
    }
}

static void
    consumePackage(const std::unique_ptr<pldm::fw_update::Package>& parsed)
{
    // attempt to read the returned struct to catch e.g. null pointers
    // and broken non-owning objects.

    for (const auto& fwdevidrecord : parsed->firmwareDeviceIdRecords)
    {
        consumeRange(fwdevidrecord.getDescriptorTypes());
        consumeCopy(fwdevidrecord.deviceUpdateOptionFlags.to_ulong());
        consumeRange(fwdevidrecord.applicableComponents);
        consume(fwdevidrecord.componentImageSetVersionString);
        for (const auto& [key, value] : fwdevidrecord.recordDescriptors)
        {

            consumeCopy(key);
            const pldm::fw_update::DescriptorData* dd = value.get();
            consume(dd->vendorDefinedDescriptorTitle);
            consumeRange(dd->data);
        }
        consumeRange(fwdevidrecord.firmwareDevicePackageData);
    }

    for (const auto& comp : parsed->componentImageInformation)
    {
        consumeCopy(comp.componentClassification);
        consumeCopy(comp.componentIdentifier);
        consumeCopy(comp.compComparisonStamp);
        consumeCopy(comp.componentOptions.to_ulong());
        consumeCopy(comp.requestedComponentActivationMethod.to_ulong());
        consume(comp.componentLocation);
        consume(comp.componentVersion);
    }
}

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

    if (res.has_value())
    {
        consumePackage(res.value());
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
