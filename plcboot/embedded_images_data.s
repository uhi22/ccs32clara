/* Embedded QCA flash images */

    .section .rodata
    .align 4

    .global _binary_softloader_nvm_start
    .type   _binary_softloader_nvm_start, %object
_binary_softloader_nvm_start:
    .incbin "flash/softloader.nvm"
_binary_softloader_nvm_end:
    .global _binary_softloader_nvm_end
    .type   _binary_softloader_nvm_end, %object

    .align 4

    .global _binary_firmware_nvm_start
    .type   _binary_firmware_nvm_start, %object
_binary_firmware_nvm_start:
    .incbin "flash/firmware.nvm"
_binary_firmware_nvm_end:
    .global _binary_firmware_nvm_end
    .type   _binary_firmware_nvm_end, %object

    .align 4

    .global _binary_pev_pib_start
    .type   _binary_pev_pib_start, %object
_binary_pev_pib_start:
    .incbin "flash/pev.pib"
_binary_pev_pib_end:
    .global _binary_pev_pib_end
    .type   _binary_pev_pib_end, %object
