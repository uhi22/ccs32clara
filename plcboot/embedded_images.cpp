#include "embedded_images.h"

extern "C" {
extern const uint8_t _binary_softloader_nvm_start[];
extern const uint8_t _binary_softloader_nvm_end[];
extern const uint8_t _binary_firmware_nvm_start[];
extern const uint8_t _binary_firmware_nvm_end[];
extern const uint8_t _binary_pev_pib_start[];
extern const uint8_t _binary_pev_pib_end[];
}

const embedded_images_t *embedded_images(void)
{
    static embedded_images_t img;

    img.softloader.data = _binary_softloader_nvm_start;
    img.softloader.size = (uint32_t)(_binary_softloader_nvm_end - _binary_softloader_nvm_start);
    img.firmware.data = _binary_firmware_nvm_start;
    img.firmware.size = (uint32_t)(_binary_firmware_nvm_end - _binary_firmware_nvm_start);
    img.pib.data = _binary_pev_pib_start;
    img.pib.size = (uint32_t)(_binary_pev_pib_end - _binary_pev_pib_start);

    return &img;
}
