#include "testpkg-util.hpp"

#include "packagebuilder.hpp"

#include <libpldm/edac.h>
#include <libpldm/firmware_update.h>

#include <cstdint>
#include <vector>

size_t appendComponentImageInfo1(std::vector<uint8_t>& pkg)
{
    // component classification
    appendLE16(pkg, 0x000A);
    // component identifier
    appendLE16(pkg, 0x64);
    // component comparison stamp
    appendLE32(pkg, 0xFFFFFFFF);
    // component options
    appendLE16(pkg, 0x0000);
    // requested component activation method
    appendLE16(pkg, 0x0000);

    size_t offset = pkg.size();

    // component location offset
    appendLE32(pkg, PLACEHOLDER);
    // component size
    appendLE32(pkg, 0x01);

    // component version string
    appendTypeLengthString(pkg, "VersionString3");

    return offset;
}

size_t appendComponentImageInfo2(std::vector<uint8_t>& pkg)
{
    // component classification
    appendLE16(pkg, 0x000A);
    // component identifier
    appendLE16(pkg, 0x64);
    // component comparison stamp
    appendLE32(pkg, 0xFFFFFFFF);
    // component options
    appendLE16(pkg, 0x0000);
    // requested component activation method
    appendLE16(pkg, 0x0000);

    size_t offset = pkg.size();

    // component location offset
    appendLE32(pkg, PLACEHOLDER);
    // component size
    appendLE32(pkg, 0x01);

    // component version string
    appendTypeLengthString(pkg, "VersionString5");

    return offset;
}

size_t appendComponentImageInfo3(std::vector<uint8_t>& pkg)
{
    // component classification
    appendLE16(pkg, 0x000A);
    // component identifier
    appendLE16(pkg, 0xC8);
    // component comparison stamp
    appendLE32(pkg, 0xFFFFFFFF);
    // component options
    appendLE16(pkg, 0x0000);
    // requested component activation method
    appendLE16(pkg, 0x0001);

    size_t offset = pkg.size();

    // component location offset
    appendLE32(pkg, PLACEHOLDER);
    // component size
    appendLE32(pkg, 0x01);

    // component version string
    appendTypeLengthString(pkg, "VersionString6");

    return offset;
}

size_t appendComponentImageInfo4(std::vector<uint8_t>& pkg)
{
    // component classification
    appendLE16(pkg, 0x000B);
    // component identifier
    appendLE16(pkg, 0x012C);
    // component comparison stamp
    appendLE32(pkg, 0xFFFFFFFF);
    // component options
    appendLE16(pkg, 0x0001);
    // requested component activation method
    appendLE16(pkg, 0x000C);

    size_t offset = pkg.size();

    // component location offset
    appendLE32(pkg, PLACEHOLDER);
    // component size
    appendLE32(pkg, 0x01);

    // component version string
    appendTypeLengthString(pkg, "VersionString7");

    return offset;
}

std::vector<size_t> appendComponentImageInfoArea1(std::vector<uint8_t>& pkg)
{

    // component image info area
    // component image count
    appendLE16(pkg, 0x01);

    return {appendComponentImageInfo1(pkg)};
}

std::vector<size_t> appendComponentImageInfoArea2(std::vector<uint8_t>& pkg)
{

    // component image info area
    // component image count
    appendLE16(pkg, 0x03);

    return {appendComponentImageInfo2(pkg), appendComponentImageInfo3(pkg),
            appendComponentImageInfo4(pkg)};
}

void appendComponentOpaqueData(std::vector<uint8_t>& pkg)
{
    const std::vector<uint8_t> componentOpaqueData{0x05, 0x04, 0x03, 0x02,
                                                   0x01};

    // ComponentOpaqueDataLength
    appendLE32(pkg, componentOpaqueData.size());

    // ComponentOpaqueData
    pkg.insert(pkg.end(), componentOpaqueData.begin(),
               componentOpaqueData.end());
}

void appendFirmwareDeviceIdRecord1(std::vector<uint8_t>& pkg)
{
    const size_t recordLengthOffset = pkg.size();
    // record 0: record length
    appendLE16(pkg, PLACEHOLDER);

    // record 0: descriptor count
    pkg.push_back(0x01);

    appendLE32(pkg, 0x01);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x0E);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x01);

    // component image set version string (14 bytes)
    appendString(pkg, "VersionString2");

    // InitialDescriptor
    appendDescriptorTLV(pkg, PLDM_FWUP_UUID,
                        std::vector<uint8_t>{
                            // clang-format off
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75,
                            // clang-format on
                        });

    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);
    // firmware device package data (empty here)
}

// same as above, but applicable component is OOB
void appendFirmwareDeviceIdRecord1InvalidApplicableComponentOOB(
    std::vector<uint8_t>& pkg)
{
    const size_t recordLengthOffset = pkg.size();
    // record 0: record length
    appendLE16(pkg, PLACEHOLDER);

    // record 0: descriptor count
    pkg.push_back(0x01);

    // record 0: device update option flags
    appendLE32(pkg, 0x01);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x0E);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x09);

    // component image set version string (14 bytes)
    appendString(pkg, "VersionString2");

    // InitialDescriptor
    appendDescriptorTLV(pkg, PLDM_FWUP_UUID,
                        std::vector<uint8_t>{
                            // clang-format off
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75,
                            // clang-format on
                        });

    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);
    // firmware device package data (empty here)
}

void appendFirmwareDeviceIdRecord2(std::vector<uint8_t>& pkg)
{
    // Firmware Device Identification Area
    const size_t recordLengthOffset = pkg.size();

    // record 0: record length, patched later
    appendLE16(pkg, PLACEHOLDER);

    // record 0: descriptor count
    pkg.push_back(0x01);

    appendLE32(pkg, 0x00);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x01);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x01);

    // component image set version string
    appendString(pkg, "v");

    // record descriptors below

    // InitialDescriptor
    appendDescriptorTLV(pkg, PLDM_FWUP_UUID,
                        std::vector<uint8_t>{
                            // clang-format off
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75,
                            // clang-format on
                        });

    // set Recordlength
    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);

    // firmware device package data (empty here)
}

void appendFirmwareDeviceIdRecord3(std::vector<uint8_t>& pkg)
{
    const size_t recordLengthOffset = pkg.size();

    // record 0: record length
    appendLE16(pkg, PLACEHOLDER);

    // record 0: descriptor count
    pkg.push_back(0x01);

    // record 0: device update option flags
    appendLE32(pkg, 0x01);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x0E);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // record 0: ReferenceManifestLength
    appendLE32(pkg, 0x08);

    // applicable components
    pkg.push_back(0x01);

    // component image set version string (14 bytes)
    appendString(pkg, "VersionString2");

    // record descriptors below
    // InitialDescriptor
    appendDescriptorTLV(pkg, PLDM_FWUP_UUID,
                        std::vector<uint8_t>{
                            // clang-format off
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75,
                            // clang-format on
                        });

    // firmware device package data (empty here)

    // ReferenceManifestData
    // SVHID,  (e.g. PCI SIG)
    pkg.push_back(0x03);

    // VendorIDLen
    pkg.push_back(0x02);

    // VendorID (e.g. Accelink)
    pkg.push_back(0x20);
    pkg.push_back(0xaf);

    const std::vector<uint8_t> referenceManifestData = {0x21, 0x22, 0x23, 0x24};
    pkg.insert(pkg.end(), referenceManifestData.begin(),
               referenceManifestData.end());

    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);
}

void appendFirmwareDeviceIdRecord4(std::vector<uint8_t>& pkg)
{
    const size_t recordLengthOffset = pkg.size();
    // record 0: record length
    appendLE16(pkg, PLACEHOLDER);

    // record 0: descriptor count
    pkg.push_back(0x02);

    appendLE32(pkg, 0x01);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x0E);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // ReferenceManifestLength
    appendLE32(pkg, 0x00);

    // applicable components
    pkg.push_back(0x01);

    // component image set version string (14 bytes)
    appendString(pkg, "VersionString2");

    // record descriptors below
    // record 0: descriptor type
    appendLE16(pkg, PLDM_FWUP_IANA_ENTERPRISE_ID);

    // InitialDescriptorLength
    appendLE16(pkg, 4);

    // record 0: InitialDescriptorData
    const std::vector<uint8_t> initialDescriptorData{0xD6, 0x75, 0x02, 0x38};
    pkg.insert(pkg.end(), initialDescriptorData.begin(),
               initialDescriptorData.end());

    // record 0: descriptor type
    appendLE16(pkg, PLDM_FWUP_IANA_ENTERPRISE_ID);

    // DescriptorLength
    appendLE16(pkg, 4);

    // record 0: InitialDescriptorData
    const std::vector<uint8_t> initialDescriptorData2{0xD6, 0x77, 0x03, 0x39};
    pkg.insert(pkg.end(), initialDescriptorData2.begin(),
               initialDescriptorData2.end());

    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);
    // firmware device package data (empty here)
}

static void appendFirmwareDeviceIdArea1Record0(std::vector<uint8_t>& pkg)
{
    const size_t recordLengthOffset = pkg.size();

    // record 0: record length
    appendLE16(pkg, PLACEHOLDER);

    // record 0: descriptor count
    pkg.push_back(0x03);

    // record 0: device update option flags
    appendLE32(pkg, 0x01);

    // record 0: component image set version string type
    pkg.push_back(0x01);

    // record 0: component image set version string length
    pkg.push_back(0x0E);

    // record 0: firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x03);

    // component image set version string (14 bytes)
    appendString(pkg, "VersionString2");

    // record 0: InitialDescriptor
    appendDescriptorTLV(pkg, PLDM_FWUP_UUID,
                        std::vector<uint8_t>{
                            // clang-format off
        0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18,
        0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5B,
                            // clang-format on
                        });

    // record 0: descriptor 1: additional descriptor
    appendDescriptorTLV(pkg, PLDM_FWUP_IANA_ENTERPRISE_ID,
                        std::vector<uint8_t>{0x47, 0x16, 0x00, 0x00});

    // record 0: descriptor 2: additional descriptor
    appendDescriptorTLV(pkg, PLDM_FWUP_VENDOR_DEFINED,
                        std::vector<uint8_t>{
                            // clang-format off
        0x01, 0x07, 0x4F, 0x70, 0x65, 0x6E, 0x42, 0x4D,
        0x43, 0x12, 0x34,
                            // clang-format on
                        });

    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);
}

static void appendFirmwareDeviceIdArea1Record1(std::vector<uint8_t>& pkg)
{
    const size_t recordLengthOffset = pkg.size();

    // record 1: record length
    appendLE16(pkg, PLACEHOLDER);

    // record 1: descriptor count
    pkg.push_back(0x01);

    // record 1: device update option flags
    appendLE32(pkg, 0x00);

    // record 1: component image set version string type
    pkg.push_back(0x01);

    // record 1: component image set version string length
    pkg.push_back(0x0E);

    // record 1: firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x07);

    appendString(pkg, "VersionString3");

    // record 1: descriptor 0:
    appendDescriptorTLV(pkg, PLDM_FWUP_UUID,
                        std::vector<uint8_t>{
                            // clang-format off
        0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0,
        0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5C, 0x11,
                            // clang-format on
                        });

    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);
}

static void appendFirmwareDeviceIdArea1Record2(std::vector<uint8_t>& pkg)
{
    const size_t recordLengthOffset = pkg.size();

    // record 2:
    // record length
    appendLE16(pkg, PLACEHOLDER);

    // descriptor count
    pkg.push_back(0x01);

    // device update option flags
    appendLE32(pkg, 0x00);

    // component image set version string type
    pkg.push_back(0x01);

    // component image set version string length
    pkg.push_back(0x0E);

    // firmware device package data length
    appendLE16(pkg, 0x00);

    // applicable components
    pkg.push_back(0x01);

    // component image set version string
    appendString(pkg, "VersionString4");

    // descriptor type
    appendDescriptorTLV(pkg, PLDM_FWUP_UUID,
                        std::vector<uint8_t>{
                            // clang-format off
        0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18, 0xA0,
        0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5D, 0x12,
                            // clang-format on
                        });

    patchLE16(pkg, recordLengthOffset, pkg.size() - recordLengthOffset);
}

void appendFirmwareDeviceIdArea1(std::vector<uint8_t>& pkg)
{
    // DeviceIDRecordCount
    pkg.push_back(0x03);

    appendFirmwareDeviceIdArea1Record0(pkg);
    appendFirmwareDeviceIdArea1Record1(pkg);
    appendFirmwareDeviceIdArea1Record2(pkg);
}

static void appendDownstreamDeviceIDRecordPrefix(
    std::vector<uint8_t>& pkg, const size_t descriptorCount,
    const size_t downstreamDevicePackageDataLength,
    const uint8_t downstreamApplicableComponents)
{
    std::vector<uint8_t> ddevidarea{};

    // DownstreamDeviceRecordLength
    appendLE16(ddevidarea, PLACEHOLDER);

    // DownstreamDeviceDescriptorCount
    ddevidarea.push_back(descriptorCount);

    // DownstreamDeviceUpdateOptionFlags
    appendLE32(ddevidarea, 0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionStringType
    ddevidarea.push_back(0x01);

    // DownstreamDeviceSelfContainedActivationMinVersionStringLength
    ddevidarea.push_back(0x0E);

    // DownstreamDevicePackageDataLength
    appendLE16(ddevidarea, downstreamDevicePackageDataLength);

    // DownstreamDeviceReferenceManifestLength
    appendLE32(ddevidarea, 0x08);

    // DownstreamDeviceApplicableComponents
    ddevidarea.push_back(downstreamApplicableComponents);

    // DownstreamDeviceSelfContainedActivationMinVersionString
    appendString(ddevidarea, "VersionString1");

    // DownstreamDeviceSelfContainedActivationMinVersionComparisonStamp
    appendLE32(ddevidarea, 0x0a090a09);

    pkg.insert(pkg.end(), ddevidarea.begin(), ddevidarea.end());
}

static void appendDownstreamDeviceReferenceManifestData(
    std::vector<uint8_t>& ddevidarea)
{
    // DownstreamDeviceReferenceManifestData (as per Table 10)
    // SVHID,  (e.g. PCI SIG)
    ddevidarea.push_back(0x03);

    // VendorIDLen
    ddevidarea.push_back(0x02);

    // (e.g. 3M Company)
    ddevidarea.push_back(0x1d);
    ddevidarea.push_back(0xa0);

    // actual manifest data (completely made up and without meaning)
    const std::vector<uint8_t> rmd = {0x31, 0x32, 0x33, 0x34};
    ddevidarea.insert(ddevidarea.end(), rmd.begin(), rmd.end());
}

void appendDownstreamDeviceIDRecords1(std::vector<uint8_t>& pkg,
                                      uint8_t downstreamApplicableComponents)
{
    std::vector<uint8_t> ddevidarea{};
    const std::vector<uint8_t> ddpd = {0xde, 0xde, 0xfe};

    appendDownstreamDeviceIDRecordPrefix(ddevidarea, 0x1, ddpd.size(),
                                         downstreamApplicableComponents);

    // DownstreamDeviceRecordDescriptors
    // record descriptors below

    // descriptor 0: descriptor type
    appendLE16(ddevidarea, PLDM_FWUP_UUID);

    const std::vector<uint8_t> idd = {
        0x16, 0x20, 0x23, 0xC9, 0x3E, 0xC5, 0x41, 0x15,
        0x95, 0xF4, 0x48, 0x70, 0x1D, 0x49, 0xD6, 0x75,
    };

    // descriptor 0: InitialDescriptorLength
    appendLE16(ddevidarea, idd.size());

    // descriptor 0: InitialDescriptorData
    ddevidarea.insert(ddevidarea.end(), idd.begin(), idd.end());

    // DownstreamDevicePackageData
    ddevidarea.insert(ddevidarea.end(), ddpd.begin(), ddpd.end());

    // DownstreamDeviceReferenceManifestData (as per Table 10)
    appendDownstreamDeviceReferenceManifestData(ddevidarea);

    // set DownstreamDeviceRecordLength
    patchLE16(ddevidarea, 0, ddevidarea.size());

    pkg.insert(pkg.end(), ddevidarea.begin(), ddevidarea.end());
}

void appendDownstreamDeviceIDRecords2(std::vector<uint8_t>& pkg)
{
    std::vector<uint8_t> ddevidarea{};
    const std::vector<uint8_t> ddpd = {0xde, 0xde, 0xfe};

    appendDownstreamDeviceIDRecordPrefix(ddevidarea, 0x2, ddpd.size(), 0x1);

    // DownstreamDeviceRecordDescriptors
    // record descriptors below

    const std::vector<uint8_t> iana1 = {
        0x16,
        0x20,
        0x23,
        0xC9,
    };
    const std::vector<uint8_t> iana2 = {
        0x16,
        0x20,
        0x23,
        0xCA,
    };

    // descriptor 0: descriptor type
    appendLE16(ddevidarea, PLDM_FWUP_IANA_ENTERPRISE_ID);

    // descriptor 0: InitialDescriptorLength
    appendLE16(ddevidarea, iana1.size());

    // descriptor 0: InitialDescriptorData
    ddevidarea.insert(ddevidarea.end(), iana1.begin(), iana1.end());

    // descriptor 0: descriptor type
    appendLE16(ddevidarea, PLDM_FWUP_IANA_ENTERPRISE_ID);

    // descriptor 0: InitialDescriptorLength
    appendLE16(ddevidarea, iana2.size());

    // descriptor 0: InitialDescriptorData
    ddevidarea.insert(ddevidarea.end(), iana2.begin(), iana2.end());

    // DownstreamDevicePackageData
    ddevidarea.insert(ddevidarea.end(), ddpd.begin(), ddpd.end());

    // DownstreamDeviceReferenceManifestData (as per Table 10)
    appendDownstreamDeviceReferenceManifestData(ddevidarea);

    // set DownstreamDeviceRecordLength
    patchLE16(ddevidarea, 0, ddevidarea.size());

    pkg.insert(pkg.end(), ddevidarea.begin(), ddevidarea.end());
}
