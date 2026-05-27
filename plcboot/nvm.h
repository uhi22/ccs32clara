#pragma once

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint32_t ExecuteMask;
    uint32_t ImageNvmAddress;
    uint32_t ImageAddress;
    uint32_t ImageLength;
    uint32_t ImageChecksum;
    uint32_t EntryPoint;
    uint32_t NextHeader;
    uint32_t PrevHeader;
    uint32_t ImageType;
    uint16_t ModuleID;
    uint16_t ModuleSubID;
    uint16_t AppletEntryVersion;
    uint16_t Reserved0;
    uint32_t Reserved[11];
    uint32_t HeaderChecksum;
} nvm_header2_t;

#define NVM_NO_NEXT_HEADER  0xFFFFFFFFu

#define NVM_IMAGE_MEMCTL        0x0007u
#define NVM_IMAGE_FIRMWARE      0x0004u
#define NVM_IMAGE_SOFTLOADER    0x000Bu
#define NVM_IMAGE_PIB           0x000Fu
