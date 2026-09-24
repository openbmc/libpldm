#pragma once

#include <cstdint>
#include <libpldm++/firmware_update.hpp>
#include <vector>

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
