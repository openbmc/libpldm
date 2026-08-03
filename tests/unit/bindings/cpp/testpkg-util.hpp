#pragma once

#include <cstdint>
#include <vector>

// helper for appending a checksum to a fw update package in unit tests

void appendCRC(std::vector<uint8_t>& pkg);
