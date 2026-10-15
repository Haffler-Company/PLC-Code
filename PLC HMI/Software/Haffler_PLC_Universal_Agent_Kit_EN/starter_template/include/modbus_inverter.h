#pragma once
#include <Arduino.h>
#include "driver/uart.h"
#include "haffler_pins.h"

class ModbusInverter {
private:
    uint8_t  m_slave_addr;
    uint16_t m_cached_freq_x100;
    uint16_t m_cached_current_x10;
    uint16_t m_cached_fault_code;
    uint32_t m_last_poll_ms;
    uint32_t m_last_response_ms;
    bool     m_comm_ok;

    static uint16_t calc_crc(const uint8_t *buf, uint16_t len) {
        uint16_t crc = 0xFFFF;
        for (uint16_t pos = 0; pos < len; pos++) {
            crc ^= (uint16_t)buf[pos];
            for (int i = 8; i != 0; i--) {
                if ((crc & 0x0001) != 0) {
                    crc >>= 1;
                    crc ^= 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
        return crc;
    }

public:
    ModbusInverter(uint8_t slave_addr = 1)
        : m_slave_addr(slave_addr), m_cached_freq_x100(0),
          m_cached_current_x10(0), m_cached_fault_code(0),
          m_last_poll_ms(0), m_last_response_ms(0), m_comm_ok(false) {}

    void init() {
        uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT
        };
        uart_param_config(UART_NUM_2, &uart_config);
        uart_set_pin(UART_NUM_2, PIN_RS485_TX, PIN_RS485_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(UART_NUM_2, 256, 256, 0, NULL, 0);

        gpio_set_direction((gpio_num_t)PIN_RS485_DIR, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)PIN_RS485_DIR, 0);
    }

    void setRun(bool run) {
        // Register 0x2000: 0x0002 = Run, 0x0001 = Stop
        uint16_t cmd = run ? 0x0002 : 0x0001;
        writeSingleRegister(0x2000, cmd);
    }

    void setFrequency(float hz) {
        // e.g. 50.0 Hz -> 5000
        uint16_t val = (uint16_t)(hz * 100.0f);
        writeSingleRegister(0x2001, val);
    }

    void writeSingleRegister(uint16_t reg, uint16_t val) {
        uint8_t frame[8];
        frame[0] = m_slave_addr;
        frame[1] = 0x06; // Function code: Write Single Register
        frame[2] = (uint8_t)(reg >> 8);
        frame[3] = (uint8_t)(reg & 0xFF);
        frame[4] = (uint8_t)(val >> 8);
        frame[5] = (uint8_t)(val & 0xFF);
        uint16_t crc = calc_crc(frame, 6);
        frame[6] = (uint8_t)(crc & 0xFF);
        frame[7] = (uint8_t)(crc >> 8);

        gpio_set_level((gpio_num_t)PIN_RS485_DIR, 1);
        uart_write_bytes(UART_NUM_2, (const char*)frame, 8);
        uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(15));
        gpio_set_level((gpio_num_t)PIN_RS485_DIR, 0);
    }

    bool isCommunicationOk() const { return m_comm_ok; }
    float getActualFrequency() const { return (float)m_cached_freq_x100 / 100.0f; }
    float getActualCurrent() const { return (float)m_cached_current_x10 / 10.0f; }
    uint16_t getFaultCode() const { return m_cached_fault_code; }
};
