#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Slave device configuration
namespace SlaveConfig {
    constexpr uint8_t DEVICE_ADDRESS = 0x22;  // Default slave address (configurable)
    constexpr uint8_t DEVICE_TYPE = 0x01;     // Device type identifier
    
    // Pin definitions for RS485 (mapped to Station-485 HY2.0 Port C2)
    // Station-485 HY2.0 Port C2 lists pins as G17 and G16
    // Use G16 as RX and G17 as TX to match Master configuration
    constexpr uint8_t RS485_RX_PIN = 16;      // Station RX (G16)
    constexpr uint8_t RS485_TX_PIN = 17;      // Station TX (G17)
    constexpr uint8_t RS485_DE_RE_PIN = 2;    // Direction control pin (shared)
    
    // Communication settings
    constexpr uint32_t BAUD_RATE = 38400;
    constexpr uart_word_length_t DATA_BITS = UART_DATA_8_BITS;
    constexpr uart_parity_t PARITY = UART_PARITY_DISABLE;
    constexpr uart_stop_bits_t STOP_BITS = UART_STOP_BITS_1;
}

// Protocol timing constants
namespace ProtocolTiming {
    constexpr uint32_t PRE_TRANSMIT_DELAY_US = 100;
    constexpr uint32_t TRANSMIT_ENABLE_DELAY_US = 600;
    constexpr uint32_t POST_TRANSMIT_DELAY_US = 350;
    constexpr uint32_t RESPONSE_TIMEOUT_MS = 10;
    constexpr uint32_t DEBOUNCE_DELAY_MS = 50;
}

// Protocol constants
namespace Protocol {
    constexpr uint8_t TO_MASTER_PREFIX = 0x02;
    constexpr uint8_t PING_RESPONSE = 0x00;
    constexpr uint8_t COMMAND_RESPONSE = 0x03;
    constexpr uint8_t ON_COMMAND = 0x00;
    constexpr uint8_t OFF_COMMAND = 0x80;
    
    constexpr size_t MAX_MESSAGE_LENGTH = 32;
    constexpr size_t MESSAGE_QUEUE_SIZE = 16;
}

// Switch configuration
namespace SwitchConfig {
    constexpr uint8_t SWITCH_PIN = 0;         // Button A on M5Stack
    constexpr bool SWITCH_ACTIVE_LOW = true;
    constexpr uint32_t LONG_PRESS_MS = 1000;
}

// Task priorities
namespace TaskPriorities {
    constexpr UBaseType_t COMMUNICATION = 5;
    constexpr UBaseType_t PROTOCOL_HANDLER = 4;
    constexpr UBaseType_t SWITCH_HANDLER = 3;
    constexpr UBaseType_t UI_HANDLER = 2;
    constexpr UBaseType_t STATUS_MONITOR = 1;
}

// Stack sizes (in words)
namespace StackSizes {
    constexpr uint32_t COMMUNICATION = 3072;
    constexpr uint32_t PROTOCOL_HANDLER = 2048;
    constexpr uint32_t SWITCH_HANDLER = 1536;
    constexpr uint32_t UI_HANDLER = 2048;
    constexpr uint32_t STATUS_MONITOR = 1536;
}