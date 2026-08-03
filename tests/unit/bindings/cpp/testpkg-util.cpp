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
