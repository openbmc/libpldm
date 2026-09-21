#pragma once

#include <cstdint>
#include <libpldm++/firmware_update.hpp>
#include <string_view>
#include <vector>

constexpr uint16_t PLACEHOLDER = 0x3838;

// helper to create little endian byte vector from uint32_t
std::vector<uint8_t> le32Vector(uint32_t x);

// helper to append uint32_t value as little endian
void appendLE32(std::vector<uint8_t>& pkg, uint32_t x);

// helper to create little endian byte vector from uint16_t
std::vector<uint8_t> le16Vector(uint16_t x);

// helper to append uint16_t value as little endian
void appendLE16(std::vector<uint8_t>& pkg, uint16_t x);

// helper to patch uint32_t value as little endian
void patchLE32(std::vector<uint8_t>& pkg, size_t offset, uint32_t value);

// helper to patch uint16_t value as little endian
void patchLE16(std::vector<uint8_t>& pkg, size_t offset, uint32_t value);

// helper for appending the PackageHeaderIdentifier (UUID)
// and PackageHeaderFormatRevision
void appendPackageHeaderIdentifier(std::vector<uint8_t>& pkg,
                                   pldm::fw_update::PackagePin pin);

// helper for appending a checksum to a fw update package in unit tests
void appendCRC(std::vector<uint8_t>& pkg);

// helper for appending a type-length-string field to a fw update package
void appendTypeLengthString(std::vector<uint8_t>& pkg,
                            const std::string_view& str);
// helper for appending a (separate) string field to a fw update package
void appendString(std::vector<uint8_t>& pkg, const std::string_view& str);

// helper to append a descriptor with type, length, value
void appendDescriptorTLV(std::vector<uint8_t>& pkg, uint16_t descriptorType,
                         const std::vector<uint8_t>& descriptorData);

// helper for appending a timestamp104 value
void appendTimestamp104(std::vector<uint8_t>& pkg);

// appends a component image info record to a fw update package
// @returns offset of component location offset
size_t appendComponentImageInfo1(std::vector<uint8_t>& pkg);
// @returns offset of component location offset
size_t appendComponentImageInfo2(std::vector<uint8_t>& pkg);
// @returns offset of component location offset
size_t appendComponentImageInfo3(std::vector<uint8_t>& pkg);
// @returns offset of component location offset
size_t appendComponentImageInfo4(std::vector<uint8_t>& pkg);

// append a complete component image info area
// @returns offsets of component location offset
std::vector<size_t> appendComponentImageInfoArea1(std::vector<uint8_t>& pkg);
// @returns offsets of component location offset
std::vector<size_t> appendComponentImageInfoArea2(std::vector<uint8_t>& pkg);

// append ComponentOpaqueData
void appendComponentOpaqueData(std::vector<uint8_t>& pkg);

// append FirmwareDeviceIdRecord
void appendFirmwareDeviceIdRecord1(std::vector<uint8_t>& pkg);
void appendFirmwareDeviceIdRecord1InvalidApplicableComponentOOB(
    std::vector<uint8_t>& pkg);
void appendFirmwareDeviceIdRecord2(std::vector<uint8_t>& pkg);
void appendFirmwareDeviceIdRecord3(std::vector<uint8_t>& pkg);

void appendFirmwareDeviceIdArea1(std::vector<uint8_t>& pkg);
