#pragma once

#include <cstdint>
#include <libpldm++/firmware_update.hpp>
#include <vector>

// helper to create little endian byte vector from uint32_t
std::vector<uint8_t> le32Vector(uint32_t x);

// helper to append uint32_t value as little endian
void appendLE32(std::vector<uint8_t>& pkg, uint32_t x);

// helper to create little endian byte vector from uint16_t
std::vector<uint8_t> le16Vector(uint16_t x);

// helper to append uint16_t value as little endian
void appendLE16(std::vector<uint8_t>& pkg, uint16_t x);

// helper for appending the PackageHeaderIdentifier (UUID)
// and PackageHeaderFormatRevision
void appendPackageHeaderIdentifier(std::vector<uint8_t>& pkg,
                                   pldm::fw_update::PackagePin pin);

// helper for appending a checksum to a fw update package in unit tests
void appendCRC(std::vector<uint8_t>& pkg);

// helper for appending a type-length-string field to a fw update package
void appendTypeLengthString(std::vector<uint8_t>& pkg,
                            const std::vector<uint8_t>& str);

// helper for appending a timestamp104 value
void appendTimestamp104(std::vector<uint8_t>& pkg);

// appends a component image info record to a fw update package
void appendComponentImageInfo1(std::vector<uint8_t>& pkg);
void appendComponentImageInfo2(std::vector<uint8_t>& pkg);
void appendComponentImageInfo3(std::vector<uint8_t>& pkg);
void appendComponentImageInfo4(std::vector<uint8_t>& pkg);

// append a complete component image info area
void appendComponentImageInfoArea1(std::vector<uint8_t>& pkg);
void appendComponentImageInfoArea2(std::vector<uint8_t>& pkg);

// append ComponentOpaqueData
void appendComponentOpaqueData(std::vector<uint8_t>& pkg);

// append FirmwareDeviceIdRecord
void appendFirmwareDeviceIdRecord1(std::vector<uint8_t>& pkg);
void appendFirmwareDeviceIdRecord1InvalidApplicableComponentOOB(
    std::vector<uint8_t>& pkg);
void appendFirmwareDeviceIdRecord2(std::vector<uint8_t>& pkg);
void appendFirmwareDeviceIdRecord3(std::vector<uint8_t>& pkg);

void appendFirmwareDeviceIdArea1(std::vector<uint8_t>& pkg);
