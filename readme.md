
# ccs32clara

STM32 talks with QCA7005 homeplug modem

## News / Change History / Functional Status

### 2023-07-18 Charging loop reached

Using the nucleo F303RE development board, the STM32 talks via SPI to the QCA7005 on the Ioniq CCM. The ccs32clara reaches the charging
loop, and shows the charging progress on the serial console in the Cube IDE.

## Todos
- [ ] Implement TCP retry to compensate for single lost packets
- [ ] Takeover latest state machine updates from pyPLC
- [ ] Control the CP line and the contactor outputs
- [ ] Add CAN
- [ ] (much more)

## Cross References

* The Hyundai Ioniq/Kona Charge Control Module (CCM): https://github.com/uhi22/Ioniq28Investigations/tree/main/CCM_ChargeControlModule_PLC_CCS
* The ccs32berta "reference project" (which uses an ESP32, talking via SPI to a QCA7005: https://github.com/uhi22/ccs32berta
* The ccs32 "reference project" (which uses ethernet instead of SPI, hardware is an ESP32 WT32-ETH01): https://github.com/uhi22/ccs32
* Hardware board which integrates an STM32, QCA7005 and more: https://github.com/uhi22/foccci
* pyPLC as test environment: https://github.com/uhi22/pyPLC
* Discussion on openinverter forum: https://openinverter.org/forum/viewtopic.php?t=3727
* Similar project discussed on SmartEVSE github: https://github.com/SmartEVSE/SmartEVSE-3/issues/25#issuecomment-1608227152
