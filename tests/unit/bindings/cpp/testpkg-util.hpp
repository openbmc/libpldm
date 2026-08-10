#pragma once

#include <cstdint>
#include <vector>

// helper for appending a checksum to a fw update package in unit tests

void appendCRC(std::vector<uint8_t>& pkg);

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
