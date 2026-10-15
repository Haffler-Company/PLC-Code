#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @file haffler_io_diag.h
 * @brief Haffler Hardware Diagnostics & Headless Serial Engine
 * 14px Circular Neon Indicators + Active-LOW Relay Toggle + Headless UART0 (921600 Baud)
 */

#ifdef __cplusplus
extern "C" {
#endif

// Initialize diagnostics subsystem and hardware status tracking
void io_diag_init(void);

// Process incoming headless ASCII commands from USB/UART0 (PING, STATUS, IO_READ, RELAY_SET, SHOT)
void io_diag_process_serial_line(const char *line);

// Safely toggle a relay channel with cycle safety interlock (disabled when machine FSM is RUNNING)
bool io_diag_toggle_relay_manual(uint8_t relay_idx, bool is_machine_idle);

// Query optocoupled input bitmask (bit 0 = I0, bit 1 = I1, ...)
uint8_t io_diag_get_inputs_mask(void);

// Query active relay output bitmask from s_active_relay_mask
uint16_t io_diag_get_relays_mask(void);

// Query keypad resistor ladder millivolts
uint16_t io_diag_get_key_f_mv(void);
uint16_t io_diag_get_key_m_mv(void);

#ifdef __cplusplus
}
#endif
