#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef ENABLE_PLCBOOT
bool plcboot_isActive(void);
void plcboot_Mainfunction(void);
bool plcboot_handle_homeplug_packet(uint16_t mmtype, const uint8_t *frame, uint16_t len);
bool plcboot_handle_software_version(const char *version, const uint8_t *sourceMac);
#else
static inline bool plcboot_isActive(void) { return false; }
static inline void plcboot_Mainfunction(void) {}
static inline bool plcboot_handle_homeplug_packet(uint16_t, const uint8_t*, uint16_t) { return false; }
static inline bool plcboot_handle_software_version(const char*, const uint8_t*) { return false; }
#endif
