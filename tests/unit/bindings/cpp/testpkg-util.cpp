#include "testpkg-util.hpp"

#include <libpldm/edac.h>

#include <cstdint>
#include <vector>

void appendCRC(std::vector<uint8_t>& pkg)
{

    // actual checksum
    const uint32_t check = pldm_edac_crc32(pkg.data(), pkg.size());

    // PackageHeaderChecksum
    auto checksum =
        std::vector<uint8_t>{static_cast<uint8_t>(check & 0xff),
                             static_cast<uint8_t>((check >> 8) & 0xff),
                             static_cast<uint8_t>((check >> 16) & 0xff),
                             static_cast<uint8_t>((check >> 24) & 0xff)};
    pkg.insert(pkg.end(), checksum.begin(), checksum.end());
}

void appendComponentImageInfo1(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> info = {
        // clang-format off
    0x0A, 0x00,             // component classification
    0x64, 0x00,             // component identifier
    0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
    0x00, 0x00,             // component options

    0x00, 0x00,             // requested component activation method
    0x8B, 0x00, 0x00, 0x00, // component location offset
    0x01, 0x00, 0x00, 0x00, // component size
    0x01,                   // component version string type
    0x0E,                   // component version string length

    0x56, 0x65, 0x72, 0x73, 0x69, 0x6F,
    0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
    0x67, 0x33,             // component version string
        // clang-format on
    };

    pkg.insert(pkg.end(), info.begin(), info.end());
}

void appendComponentImageInfo2(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> info = {
        // clang-format off
        0x0A, 0x00,             // component classification
        0x64, 0x00,             // component identifier
        0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
        0x00, 0x00,             // component options

        0x00, 0x00,             // requested component activation method
        0x46, 0x01, 0x00, 0x00, // component location offset
        0x01, 0x00, 0x00, 0x00, // component size
        0x01,                   // component version string type
        0x0E,                   // component version string length

        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x35, // component version string (14 bytes)
        // clang-format on
    };

    pkg.insert(pkg.end(), info.begin(), info.end());
}

void appendComponentImageInfo3(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> info = {
        // clang-format off
        0x0A, 0x00,             // component classification
        0xC8, 0x00,             // component identifier
        0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
        0x00, 0x00,             // component options

        0x01, 0x00,             // requested component activation method
        0x46, 0x01, 0x00, 0x00, // component location offset
        0x01, 0x00, 0x00, 0x00, // component size
        0x01,                   // component version string type
        0x0E,                   // component version string length
        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x36, // component version string
        // clang-format on
    };

    pkg.insert(pkg.end(), info.begin(), info.end());
}

void appendComponentImageInfo4(std::vector<uint8_t>& pkg)
{

    std::vector<uint8_t> info = {
        // clang-format off
        0xB,  0x00,             // component classification
        0x2C, 0x01,             // component identifier
        0xFF, 0xFF, 0xFF, 0xFF, // component comparison stamp
        0x01, 0x00,             // component options

        0x0C, 0x00,             // requested component activation method
        0x46, 0x01, 0x00, 0x00, // component location offset
        0x01, 0x00, 0x00, 0x00, // component size
        0x01,                   // component version string type
        0x0E,                   // component version string length
        0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69, 0x6E,
        0x67, 0x37, // component version string
        // clang-format on
    };

    pkg.insert(pkg.end(), info.begin(), info.end());
}

void appendComponentImageInfoArea1(std::vector<uint8_t>& pkg)
{

    // component image info area
    // component image count
    std::vector<uint8_t> buffer{0x01, 0x00};

    pkg.insert(pkg.end(), buffer.begin(), buffer.end());
    appendComponentImageInfo1(pkg);
}

void appendComponentImageInfoArea2(std::vector<uint8_t>& pkg)
{

    // component image info area
    // component image count
    std::vector<uint8_t> buffer{0x03, 0x00};

    pkg.insert(pkg.end(), buffer.begin(), buffer.end());
    appendComponentImageInfo2(pkg);
    appendComponentImageInfo3(pkg);
    appendComponentImageInfo4(pkg);
}

void appendComponentOpaqueData(std::vector<uint8_t>& pkg)
{
    const std::vector<uint8_t> componentOpaqueData{
        // clang-format off
      0x5, 0x00, 0x00, 0x00, // ComponentOpaqueDataLength
      0x05, 0x04, 0x03, 0x02, 0x01, // ComponentOpaqueData
        // clang-format on
    };
    pkg.insert(pkg.end(), componentOpaqueData.begin(),
               componentOpaqueData.end());
}
