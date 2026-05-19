#include "plcboot.h"

#ifdef ENABLE_PLCBOOT

#include "ccs32_globals.h"
#include "embedded_images.h"
#include "nvm.h"

#include <string.h>

#define MME_HDR_LEN                    20u
#define VS_SW_VER                      0xA000u
#define VS_HOST_ACTION                 0xA060u
#define VS_WRITE_AND_EXECUTE_APPLET    0xA098u
#define VS_MODULE_OPERATION            0xA0B0u

#define MMTYPE_REQ                     0x0000u
#define MMTYPE_CNF                     0x0001u
#define MMTYPE_IND                     0x0002u
#define MMTYPE_RSP                     0x0003u

#define PLC_MODULE_EXECUTE             (1u << 0)
#define PLC_MODULE_ABSOLUTE            (1u << 1)
#define PLC_MODULE_SIZE                1400u

#define MOD_OP_START_SESSION           0x0010u
#define MOD_OP_WRITE_MODULE            0x0011u
#define MOD_OP_CLOSE_SESSION           0x0012u

#define MODULE_ID_FW                   0x7001u
#define MODULE_ID_PIB                  0x7002u
#define MODULE_ID_SOFTLOADER           0x7003u

#define PLC_COMMIT_FORCE               (1u << 0)
#define PLC_COMMIT_NORESET             (1u << 1)
#define COMMIT_CODE_SOFTLOADER         (PLC_COMMIT_FORCE | PLC_COMMIT_NORESET)
#define COMMIT_CODE_FW_PIB             (PLC_COMMIT_FORCE)

#define SESSION_ID                     0x78563412u
#define RESPONSE_TIMEOUT_MS            2000u
#define SESSION_TIMEOUT_MS             5000u
#define RUNTIME_TIMEOUT_MS             30000u
#define GET_SW_RETRY_MS                500u

static const uint8_t kDstMac[6] = {0x00, 0xB0, 0x52, 0x00, 0x00, 0x01};
static const uint8_t kSrcMac[6] = {0x00, 0xB0, 0x52, 0x00, 0x00, 0x00};

enum PlcbootState {
    PLCBOOT_IDLE,
    PLCBOOT_WAIT_WRITE_EXECUTE_CNF,
    PLCBOOT_WAIT_RUNTIME,
    PLCBOOT_WAIT_START_SESSION_CNF,
    PLCBOOT_WAIT_WRITE_MODULE_CNF,
    PLCBOOT_WAIT_CLOSE_SESSION_CNF,
    PLCBOOT_WAIT_FINAL_RUNTIME,
};

enum WriteExecuteKind {
    WRITE_EXECUTE_NONE,
    WRITE_EXECUTE_MEMCTL,
    WRITE_EXECUTE_PIB,
    WRITE_EXECUTE_FIRMWARE,
};

enum SessionKind {
    SESSION_NONE,
    SESSION_SOFTLOADER,
    SESSION_FLASH,
};

enum ModuleKind {
    MODULE_NONE,
    MODULE_SOFTLOADER_KIND,
    MODULE_PIB_KIND,
    MODULE_FIRMWARE_KIND,
};

typedef struct __attribute__((packed)) {
    uint16_t module_id;
    uint16_t module_sub_id;
    uint32_t module_length;
    uint32_t module_chksum;
} module_spec_t;

typedef struct {
    const uint8_t *data;
    const nvm_header2_t *hdr;
    uint32_t offset;
    uint32_t currentChunk;
    uint32_t currentOffset;
    bool execute;
    bool isPib;
    WriteExecuteKind kind;
} write_execute_transfer_t;

typedef struct {
    const uint8_t *data;
    uint32_t size;
    uint32_t offset;
    uint32_t currentChunk;
    uint16_t moduleId;
    uint8_t moduleIdx;
    uint8_t numModules;
    ModuleKind kind;
} module_transfer_t;

static PlcbootState plcbootState = PLCBOOT_IDLE;
static uint32_t plcbootDeadline;
static uint32_t plcbootPollDeadline;
static bool plcbootAttempted;
static bool plcbootLoggingMuted;
static int savedLoggingMask;
static uint8_t progressStep;
static const char *progressLabel;

static const embedded_images_t *images;
static const nvm_header2_t *memctlHdr;
static const uint8_t *memctlData;
static const nvm_header2_t *firmwareHdr;
static const uint8_t *firmwareData;
static const nvm_header2_t *pibHdr;
static const uint8_t *pibData;

static write_execute_transfer_t writeExecuteTransfer;
static module_transfer_t moduleTransfer;
static SessionKind sessionKind;

static uint16_t rd16le(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t rd32le(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static void wr16le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void wr32le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint32_t compute_module_checksum(const uint8_t *data, uint32_t len)
{
    uint32_t checksum = 0;

    while (len >= 4u) {
        checksum ^= rd32le(data);
        data += 4;
        len -= 4u;
    }

    if (len > 0u) {
        uint8_t tail[4] = {0, 0, 0, 0};
        for (uint32_t i = 0; i < len; i++) {
            tail[i] = data[i];
        }
        checksum ^= rd32le(tail);
    }

    return ~checksum;
}

static bool nvm_find_image(const uint8_t *file, uint32_t fileSize, uint32_t imageType, const nvm_header2_t **hdrOut, const uint8_t **dataOut)
{
    uint32_t offset = 0;

    if (hdrOut != NULL) {
        *hdrOut = NULL;
    }
    if (dataOut != NULL) {
        *dataOut = NULL;
    }

    while ((offset + (uint32_t)sizeof(nvm_header2_t)) <= fileSize) {
        const nvm_header2_t *hdr = reinterpret_cast<const nvm_header2_t *>(file + offset);

        if ((hdr->MajorVersion != 1u) || (hdr->MinorVersion != 1u)) {
            break;
        }

        if (hdr->ImageType == imageType) {
            if (hdrOut != NULL) {
                *hdrOut = hdr;
            }
            if (dataOut != NULL) {
                *dataOut = file + offset + (uint32_t)sizeof(nvm_header2_t);
            }
            return true;
        }

        if (hdr->NextHeader == NVM_NO_NEXT_HEADER) {
            break;
        }

        offset = hdr->NextHeader;
    }

    return false;
}

static void plcbootLog(const char *message)
{
    printf("[%u] [QCAFLASH] %s\r\n", rtc_get_ms(), message);
}

static void plcbootProgress(uint32_t done, uint32_t total)
{
    if ((progressLabel == NULL) || (total == 0u)) {
        return;
    }

    uint8_t nextStep = (uint8_t)((done * 10u) / total);

    if (nextStep > progressStep) {
        progressStep = nextStep;
        printf("[%u] [QCAFLASH] %s %u%%\r\n", rtc_get_ms(), progressLabel, progressStep * 10u);
    }
}

static void muteLogging(void)
{
    if (!plcbootLoggingMuted) {
        savedLoggingMask = Param::GetInt(Param::logging);
        Param::SetInt(Param::logging, 0);
        plcbootLoggingMuted = true;
    }
}

static void restoreLogging(void)
{
    if (plcbootLoggingMuted) {
        Param::SetInt(Param::logging, savedLoggingMask);
        plcbootLoggingMuted = false;
    }
}

static void resetTransfers(void)
{
    memset(&writeExecuteTransfer, 0, sizeof(writeExecuteTransfer));
    memset(&moduleTransfer, 0, sizeof(moduleTransfer));
    sessionKind = SESSION_NONE;
    progressLabel = NULL;
    progressStep = 0;
}

static void plcbootFinish(void)
{
    plcbootLog("Flash sequence finished");
    resetTransfers();
    plcbootState = PLCBOOT_IDLE;
    restoreLogging();
}

static void plcbootFail(const char *reason)
{
    printf("[%u] [QCAFLASH] Flash failed: %s\r\n", rtc_get_ms(), reason);
    resetTransfers();
    plcbootState = PLCBOOT_IDLE;
    restoreLogging();
}

static bool sendFrame(uint16_t len)
{
    if (len > MY_ETH_TRANSMIT_BUFFER_LEN) {
        plcbootFail("frame too large");
        return false;
    }

    if (len < 60u) {
        memset(&myethtransmitbuffer[len], 0, 60u - len);
        len = 60u;
    }

    myethtransmitbufferLen = len;
    myEthTransmit();
    return true;
}

static uint16_t buildMmeHeader(uint8_t *buf, uint16_t mmtype)
{
    memcpy(buf, kDstMac, 6);
    memcpy(buf + 6, kSrcMac, 6);
    buf[12] = 0x88;
    buf[13] = 0xE1;
    buf[14] = 0x00;
    buf[15] = (uint8_t)(mmtype & 0xFF);
    buf[16] = (uint8_t)(mmtype >> 8);
    buf[17] = 0x00;
    buf[18] = 0xB0;
    buf[19] = 0x52;
    return MME_HDR_LEN;
}

static uint16_t frameMmtype(const uint8_t *frame, uint16_t len)
{
    if (len < MME_HDR_LEN) {
        return 0;
    }
    if ((frame[12] != 0x88) || (frame[13] != 0xE1)) {
        return 0;
    }
    if ((frame[17] != 0x00) || (frame[18] != 0xB0) || (frame[19] != 0x52)) {
        return 0;
    }
    return rd16le(frame + 15);
}

static void sendHostActionRsp(void)
{
    uint16_t pos = buildMmeHeader(myethtransmitbuffer, VS_HOST_ACTION | MMTYPE_RSP);
    myethtransmitbuffer[pos++] = 0x00;
    sendFrame(pos);
}

static void sendGetSwReq(void)
{
    composeGetSwReq();
    myEthTransmit();
}

static bool sendWriteExecuteChunk(void);
static bool startWriteExecute(WriteExecuteKind kind, const char *label, const uint8_t *data, const nvm_header2_t *hdr, bool execute, bool isPib);
static bool startSession(SessionKind kind);
static bool startModuleTransfer(ModuleKind kind, const char *label, const uint8_t *data, uint32_t size, uint16_t moduleId, uint8_t moduleIdx, uint8_t numModules);

static bool prepareImages(void)
{
    images = embedded_images();

    if (!nvm_find_image(images->firmware.data, images->firmware.size, NVM_IMAGE_MEMCTL, &memctlHdr, &memctlData)) {
        return false;
    }
    if (!nvm_find_image(images->firmware.data, images->firmware.size, NVM_IMAGE_FIRMWARE, &firmwareHdr, &firmwareData)) {
        return false;
    }
    if (!nvm_find_image(images->pib.data, images->pib.size, NVM_IMAGE_PIB, &pibHdr, &pibData)) {
        return false;
    }

    return true;
}

static bool beginPlcboot(void)
{
    if (!prepareImages()) {
        plcbootFail("embedded images are invalid");
        return false;
    }

    muteLogging();
    plcbootLog("BootLoader detected");

    return startWriteExecute(WRITE_EXECUTE_MEMCTL, "Uploading MemCtl", memctlData, memctlHdr, true, false);
}

static bool startWriteExecute(WriteExecuteKind kind, const char *label, const uint8_t *data, const nvm_header2_t *hdr, bool execute, bool isPib)
{
    writeExecuteTransfer.data = data;
    writeExecuteTransfer.hdr = hdr;
    writeExecuteTransfer.offset = 0;
    writeExecuteTransfer.currentChunk = 0;
    writeExecuteTransfer.currentOffset = 0;
    writeExecuteTransfer.execute = execute;
    writeExecuteTransfer.isPib = isPib;
    writeExecuteTransfer.kind = kind;

    progressLabel = label;
    progressStep = 0;
    plcbootLog(label);

    plcbootState = PLCBOOT_WAIT_WRITE_EXECUTE_CNF;
    return sendWriteExecuteChunk();
}

static bool sendWriteExecuteChunk(void)
{
    uint16_t pos;
    uint32_t totalLen = writeExecuteTransfer.hdr->ImageLength;
    uint32_t chunk = totalLen - writeExecuteTransfer.offset;
    uint32_t flags = PLC_MODULE_ABSOLUTE;
    uint32_t currentOffset = writeExecuteTransfer.hdr->ImageAddress + writeExecuteTransfer.offset;

    if (chunk > PLC_MODULE_SIZE) {
        chunk = PLC_MODULE_SIZE;
    }
    if ((writeExecuteTransfer.offset + chunk) >= totalLen && writeExecuteTransfer.execute && (writeExecuteTransfer.hdr->EntryPoint != NVM_NO_NEXT_HEADER)) {
        flags |= PLC_MODULE_EXECUTE;
    }

    pos = buildMmeHeader(myethtransmitbuffer, VS_WRITE_AND_EXECUTE_APPLET | MMTYPE_REQ);
    wr32le(myethtransmitbuffer + pos, SESSION_ID); pos += 4;
    wr32le(myethtransmitbuffer + pos, 0); pos += 4;
    wr32le(myethtransmitbuffer + pos, flags); pos += 4;
    memset(myethtransmitbuffer + pos, 0, 8);
    if (!writeExecuteTransfer.isPib) {
        myethtransmitbuffer[pos] = 1;
    }
    pos += 8;
    wr32le(myethtransmitbuffer + pos, totalLen); pos += 4;
    wr32le(myethtransmitbuffer + pos, chunk); pos += 4;
    wr32le(myethtransmitbuffer + pos, currentOffset); pos += 4;
    wr32le(myethtransmitbuffer + pos, writeExecuteTransfer.hdr->EntryPoint); pos += 4;
    wr32le(myethtransmitbuffer + pos, writeExecuteTransfer.hdr->ImageChecksum); pos += 4;
    memset(myethtransmitbuffer + pos, 0, 8); pos += 8;

    if ((uint32_t)pos + chunk > MY_ETH_TRANSMIT_BUFFER_LEN) {
        plcbootFail("transfer chunk exceeds tx buffer");
        return false;
    }

    memcpy(myethtransmitbuffer + pos, writeExecuteTransfer.data + writeExecuteTransfer.offset, chunk);
    pos = (uint16_t)(pos + chunk);

    writeExecuteTransfer.currentChunk = chunk;
    writeExecuteTransfer.currentOffset = currentOffset;
    plcbootDeadline = rtc_get_ms() + RESPONSE_TIMEOUT_MS;

    return sendFrame(pos);
}

static bool sendStartSessionRequest(SessionKind kind)
{
    module_spec_t specs[2];
    uint8_t numModules = 0;
    uint16_t pos;

    memset(specs, 0, sizeof(specs));

    if (kind == SESSION_SOFTLOADER) {
        specs[0].module_id = MODULE_ID_SOFTLOADER;
        specs[0].module_length = images->softloader.size;
        specs[0].module_chksum = compute_module_checksum(images->softloader.data, images->softloader.size);
        numModules = 1;
    } else {
        specs[0].module_id = MODULE_ID_PIB;
        specs[0].module_length = images->pib.size;
        specs[0].module_chksum = compute_module_checksum(images->pib.data, images->pib.size);
        specs[1].module_id = MODULE_ID_FW;
        specs[1].module_length = images->firmware.size;
        specs[1].module_chksum = compute_module_checksum(images->firmware.data, images->firmware.size);
        numModules = 2;
    }

    pos = buildMmeHeader(myethtransmitbuffer, VS_MODULE_OPERATION | MMTYPE_REQ);
    memset(myethtransmitbuffer + pos, 0, 4); pos += 4;
    myethtransmitbuffer[pos++] = 1;
    wr16le(myethtransmitbuffer + pos, MOD_OP_START_SESSION); pos += 2;
    wr16le(myethtransmitbuffer + pos, (uint16_t)(13u + (uint16_t)numModules * 12u)); pos += 2;
    memset(myethtransmitbuffer + pos, 0, 4); pos += 4;
    wr32le(myethtransmitbuffer + pos, SESSION_ID); pos += 4;
    myethtransmitbuffer[pos++] = numModules;

    for (uint8_t i = 0; i < numModules; i++) {
        wr16le(myethtransmitbuffer + pos, specs[i].module_id); pos += 2;
        wr16le(myethtransmitbuffer + pos, specs[i].module_sub_id); pos += 2;
        wr32le(myethtransmitbuffer + pos, specs[i].module_length); pos += 4;
        wr32le(myethtransmitbuffer + pos, specs[i].module_chksum); pos += 4;
    }

    sessionKind = kind;
    plcbootState = PLCBOOT_WAIT_START_SESSION_CNF;
    plcbootDeadline = rtc_get_ms() + SESSION_TIMEOUT_MS;
    return sendFrame(pos);
}

static bool startSession(SessionKind kind)
{
    sessionKind = kind;
    return sendStartSessionRequest(kind);
}

static bool startModuleTransfer(ModuleKind kind, const char *label, const uint8_t *data, uint32_t size, uint16_t moduleId, uint8_t moduleIdx, uint8_t numModules)
{
    moduleTransfer.data = data;
    moduleTransfer.size = size;
    moduleTransfer.offset = 0;
    moduleTransfer.currentChunk = 0;
    moduleTransfer.moduleId = moduleId;
    moduleTransfer.moduleIdx = moduleIdx;
    moduleTransfer.numModules = numModules;
    moduleTransfer.kind = kind;

    progressLabel = label;
    progressStep = 0;
    plcbootLog(label);

    plcbootState = PLCBOOT_WAIT_WRITE_MODULE_CNF;
    plcbootDeadline = rtc_get_ms() + RESPONSE_TIMEOUT_MS;

    uint32_t chunk = size;
    if (chunk > PLC_MODULE_SIZE) {
        chunk = PLC_MODULE_SIZE;
    }

    uint16_t pos = buildMmeHeader(myethtransmitbuffer, VS_MODULE_OPERATION | MMTYPE_REQ);
    memset(myethtransmitbuffer + pos, 0, 4); pos += 4;
    myethtransmitbuffer[pos++] = 1;
    wr16le(myethtransmitbuffer + pos, MOD_OP_WRITE_MODULE); pos += 2;
    wr16le(myethtransmitbuffer + pos, (uint16_t)(23u + chunk)); pos += 2;
    memset(myethtransmitbuffer + pos, 0, 4); pos += 4;
    wr32le(myethtransmitbuffer + pos, SESSION_ID); pos += 4;
    myethtransmitbuffer[pos++] = moduleIdx;
    wr16le(myethtransmitbuffer + pos, moduleId); pos += 2;
    wr16le(myethtransmitbuffer + pos, 0); pos += 2;
    wr16le(myethtransmitbuffer + pos, (uint16_t)chunk); pos += 2;
    wr32le(myethtransmitbuffer + pos, 0); pos += 4;

    if ((uint32_t)pos + chunk > MY_ETH_TRANSMIT_BUFFER_LEN) {
        plcbootFail("module chunk exceeds tx buffer");
        return false;
    }

    memcpy(myethtransmitbuffer + pos, data, chunk);
    pos = (uint16_t)(pos + chunk);

    moduleTransfer.currentChunk = chunk;
    return sendFrame(pos);
}

static bool sendNextModuleChunk(void)
{
    uint32_t chunk = moduleTransfer.size - moduleTransfer.offset;
    uint16_t pos;

    if (chunk > PLC_MODULE_SIZE) {
        chunk = PLC_MODULE_SIZE;
    }

    pos = buildMmeHeader(myethtransmitbuffer, VS_MODULE_OPERATION | MMTYPE_REQ);
    memset(myethtransmitbuffer + pos, 0, 4); pos += 4;
    myethtransmitbuffer[pos++] = 1;
    wr16le(myethtransmitbuffer + pos, MOD_OP_WRITE_MODULE); pos += 2;
    wr16le(myethtransmitbuffer + pos, (uint16_t)(23u + chunk)); pos += 2;
    memset(myethtransmitbuffer + pos, 0, 4); pos += 4;
    wr32le(myethtransmitbuffer + pos, SESSION_ID); pos += 4;
    myethtransmitbuffer[pos++] = moduleTransfer.moduleIdx;
    wr16le(myethtransmitbuffer + pos, moduleTransfer.moduleId); pos += 2;
    wr16le(myethtransmitbuffer + pos, 0); pos += 2;
    wr16le(myethtransmitbuffer + pos, (uint16_t)chunk); pos += 2;
    wr32le(myethtransmitbuffer + pos, moduleTransfer.offset); pos += 4;

    if ((uint32_t)pos + chunk > MY_ETH_TRANSMIT_BUFFER_LEN) {
        plcbootFail("module chunk exceeds tx buffer");
        return false;
    }

    memcpy(myethtransmitbuffer + pos, moduleTransfer.data + moduleTransfer.offset, chunk);
    pos = (uint16_t)(pos + chunk);

    moduleTransfer.currentChunk = chunk;
    plcbootDeadline = rtc_get_ms() + RESPONSE_TIMEOUT_MS;
    return sendFrame(pos);
}

static bool sendCloseSessionRequest(uint32_t commitCode)
{
    uint16_t pos = buildMmeHeader(myethtransmitbuffer, VS_MODULE_OPERATION | MMTYPE_REQ);

    memset(myethtransmitbuffer + pos, 0, 4); pos += 4;
    myethtransmitbuffer[pos++] = 1;
    wr16le(myethtransmitbuffer + pos, MOD_OP_CLOSE_SESSION); pos += 2;
    wr16le(myethtransmitbuffer + pos, 36); pos += 2;
    memset(myethtransmitbuffer + pos, 0, 4); pos += 4;
    wr32le(myethtransmitbuffer + pos, SESSION_ID); pos += 4;
    wr32le(myethtransmitbuffer + pos, commitCode); pos += 4;
    memset(myethtransmitbuffer + pos, 0, 20); pos += 20;

    plcbootState = PLCBOOT_WAIT_CLOSE_SESSION_CNF;
    plcbootDeadline = rtc_get_ms() + RESPONSE_TIMEOUT_MS;
    return sendFrame(pos);
}

static bool handleWriteExecuteConfirm(const uint8_t *frame, uint16_t len)
{
    if (len < (MME_HDR_LEN + 36u)) {
        plcbootFail("short write-and-execute confirm");
        return true;
    }
    if (rd32le(frame + MME_HDR_LEN) != 0u) {
        plcbootFail("write-and-execute status error");
        return true;
    }
    if (rd32le(frame + MME_HDR_LEN + 28u) != writeExecuteTransfer.currentChunk || rd32le(frame + MME_HDR_LEN + 32u) != writeExecuteTransfer.currentOffset) {
        plcbootFail("write-and-execute confirm mismatch");
        return true;
    }

    writeExecuteTransfer.offset += writeExecuteTransfer.currentChunk;
    plcbootProgress(writeExecuteTransfer.offset, writeExecuteTransfer.hdr->ImageLength);

    if (writeExecuteTransfer.offset < writeExecuteTransfer.hdr->ImageLength) {
        sendWriteExecuteChunk();
        return true;
    }

    switch (writeExecuteTransfer.kind) {
    case WRITE_EXECUTE_MEMCTL:
        startWriteExecute(WRITE_EXECUTE_PIB, "Uploading PIB", pibData, pibHdr, false, true);
        break;
    case WRITE_EXECUTE_PIB:
        startWriteExecute(WRITE_EXECUTE_FIRMWARE, "Uploading firmware", firmwareData, firmwareHdr, true, false);
        break;
    case WRITE_EXECUTE_FIRMWARE:
        plcbootLog("Waiting for runtime");
        plcbootState = PLCBOOT_WAIT_RUNTIME;
        plcbootDeadline = rtc_get_ms() + RUNTIME_TIMEOUT_MS;
        plcbootPollDeadline = 0;
        break;
    default:
        plcbootFail("invalid upload state");
        break;
    }

    return true;
}

static bool handleStartSessionConfirm(const uint8_t *frame, uint16_t len)
{
    if (len < (MME_HDR_LEN + 2u)) {
        plcbootFail("short start-session confirm");
        return true;
    }
    if (rd16le(frame + MME_HDR_LEN) != 0u) {
        plcbootFail("start-session status error");
        return true;
    }

    if (sessionKind == SESSION_SOFTLOADER) {
        startModuleTransfer(MODULE_SOFTLOADER_KIND, "Flashing softloader", images->softloader.data, images->softloader.size, MODULE_ID_SOFTLOADER, 0, 1);
    } else if (sessionKind == SESSION_FLASH) {
        startModuleTransfer(MODULE_PIB_KIND, "Flashing PIB", images->pib.data, images->pib.size, MODULE_ID_PIB, 0, 2);
    } else {
        plcbootFail("unexpected session state");
    }

    return true;
}

static bool handleWriteModuleConfirm(const uint8_t *frame, uint16_t len)
{
    if (len < (MME_HDR_LEN + 2u)) {
        plcbootFail("short write-module confirm");
        return true;
    }
    if (rd16le(frame + MME_HDR_LEN) != 0u) {
        plcbootFail("write-module status error");
        return true;
    }

    moduleTransfer.offset += moduleTransfer.currentChunk;
    plcbootProgress(moduleTransfer.offset, moduleTransfer.size);

    if (moduleTransfer.offset < moduleTransfer.size) {
        sendNextModuleChunk();
        return true;
    }

    if (moduleTransfer.kind == MODULE_SOFTLOADER_KIND) {
        sendCloseSessionRequest(COMMIT_CODE_SOFTLOADER);
    } else if (moduleTransfer.kind == MODULE_PIB_KIND) {
        startModuleTransfer(MODULE_FIRMWARE_KIND, "Flashing firmware", images->firmware.data, images->firmware.size, MODULE_ID_FW, 1, 2);
    } else if (moduleTransfer.kind == MODULE_FIRMWARE_KIND) {
        sendCloseSessionRequest(COMMIT_CODE_FW_PIB);
    } else {
        plcbootFail("invalid module state");
    }

    return true;
}

static bool handleCloseSessionConfirm(const uint8_t *frame, uint16_t len)
{
    if (len < (MME_HDR_LEN + 2u)) {
        plcbootFail("short close-session confirm");
        return true;
    }
    if (rd16le(frame + MME_HDR_LEN) != 0u) {
        plcbootFail("close-session status error");
        return true;
    }

    if (sessionKind == SESSION_SOFTLOADER) {
        startSession(SESSION_FLASH);
    } else if (sessionKind == SESSION_FLASH) {
        plcbootLog("Waiting for final runtime");
        plcbootState = PLCBOOT_WAIT_FINAL_RUNTIME;
        plcbootDeadline = rtc_get_ms() + RUNTIME_TIMEOUT_MS;
        plcbootPollDeadline = 0;
    } else {
        plcbootFail("unexpected close-session state");
    }

    return true;
}

bool plcboot_isActive(void)
{
    return plcbootState != PLCBOOT_IDLE;
}

void plcboot_Mainfunction(void)
{
    if (plcbootState == PLCBOOT_IDLE) {
        return;
    }

    uint32_t now = rtc_get_ms();

    if ((plcbootState == PLCBOOT_WAIT_RUNTIME) || (plcbootState == PLCBOOT_WAIT_FINAL_RUNTIME)) {
        if (now >= plcbootDeadline) {
            if (plcbootState == PLCBOOT_WAIT_FINAL_RUNTIME) {
                plcbootFinish();
            } else {
                plcbootFail("timeout waiting for runtime");
            }
            return;
        }

        if (now >= plcbootPollDeadline) {
            sendGetSwReq();
            plcbootPollDeadline = now + GET_SW_RETRY_MS;
        }
        return;
    }

    if (now >= plcbootDeadline) {
        plcbootFail("timeout waiting for device response");
    }
}

bool plcboot_handle_homeplug_packet(uint16_t mmtype, const uint8_t *frame, uint16_t len)
{
    if (plcbootState == PLCBOOT_IDLE) {
        return false;
    }

    if (mmtype == (VS_HOST_ACTION | MMTYPE_IND)) {
        sendHostActionRsp();
        return true;
    }

    if ((frameMmtype(frame, len) == 0u) && (mmtype != (VS_HOST_ACTION | MMTYPE_IND))) {
        return false;
    }

    if ((mmtype == (VS_WRITE_AND_EXECUTE_APPLET | MMTYPE_CNF)) && (plcbootState == PLCBOOT_WAIT_WRITE_EXECUTE_CNF)) {
        return handleWriteExecuteConfirm(frame, len);
    }
    if ((mmtype == (VS_MODULE_OPERATION | MMTYPE_CNF)) && (plcbootState == PLCBOOT_WAIT_START_SESSION_CNF)) {
        return handleStartSessionConfirm(frame, len);
    }
    if ((mmtype == (VS_MODULE_OPERATION | MMTYPE_CNF)) && (plcbootState == PLCBOOT_WAIT_WRITE_MODULE_CNF)) {
        return handleWriteModuleConfirm(frame, len);
    }
    if ((mmtype == (VS_MODULE_OPERATION | MMTYPE_CNF)) && (plcbootState == PLCBOOT_WAIT_CLOSE_SESSION_CNF)) {
        return handleCloseSessionConfirm(frame, len);
    }

    return false;
}

bool plcboot_handle_software_version(const char *version, const uint8_t *sourceMac)
{
    bool isBootLoader = strncmp(version, "BootLoader", 10) == 0;

    (void)sourceMac;

    if (plcbootState == PLCBOOT_IDLE) {
        if (!plcbootAttempted && isBootLoader) {
            plcbootAttempted = true;
            return beginPlcboot();
        }
        return false;
    }

    if (plcbootState == PLCBOOT_WAIT_RUNTIME) {
        if (!isBootLoader) {
            return startSession(SESSION_SOFTLOADER);
        }
        return true;
    }

    if (plcbootState == PLCBOOT_WAIT_FINAL_RUNTIME) {
        if (!isBootLoader) {
            plcbootFinish();
        }
        return true;
    }

    return true;
}

#endif
