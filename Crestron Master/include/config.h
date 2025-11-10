#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Timing constants for Crestron protocol
namespace CrestronTiming {
    constexpr uint32_t PING_INTERVAL_MS = 23;
    constexpr uint32_t PING_TIMEOUT_MS = 4;
    constexpr uint32_t REPING_DELAY_MS = 320;
    constexpr uint32_t CONFIG_STEP_DELAY_MS = 2;
    constexpr uint32_t INTER_COMMAND_DELAY_MS = 4;
    constexpr uint32_t BREAK_DURATION_US = 260;
}

// RS485 communication settings
namespace RS485Config {
    constexpr uint8_t RX_PIN = 16;
    constexpr uint8_t TX_PIN = 17;
    constexpr uint8_t DE_RE_PIN = 2; // Driver Enable / Receiver Enable
    constexpr uint32_t BAUD_RATE = 38400;
    constexpr uart_word_length_t DATA_BITS = UART_DATA_8_BITS;
    constexpr uart_parity_t PARITY = UART_PARITY_DISABLE;
    constexpr uart_stop_bits_t STOP_BITS = UART_STOP_BITS_2;
}

// Slave device definitions
namespace SlaveDevices {
    constexpr uint8_t IO_48_ADDRESS = 0x11;
    constexpr uint8_t DIM8_ADDRESS = 0x0B;
    constexpr uint8_t DIMU8_ADDRESS = 0x0C;
    constexpr uint8_t MAX_SLAVES = 3;
}

// Message types and protocol constants
namespace CrestronProtocol {
    constexpr uint8_t TO_MASTER_PREFIX = 0x02;
    constexpr uint8_t PING_COMMAND = 0x00;
    constexpr uint8_t CONFIG_REQUEST = 0x03;
    constexpr uint8_t TIME_SYNC_CMD = 0x08;
    constexpr uint8_t DIM_COMMAND = 0x1D;
    constexpr uint8_t DIMU_COMMAND = 0x20;
    
    constexpr size_t MAX_MESSAGE_LENGTH = 128;
    constexpr size_t MESSAGE_QUEUE_SIZE = 32;
}

// Task priorities (higher number = higher priority)
namespace TaskPriorities {
    constexpr UBaseType_t RS485_HANDLER = 5;
    constexpr UBaseType_t SLAVE_MANAGER = 4;
    constexpr UBaseType_t PING_SCHEDULER = 3;
    constexpr UBaseType_t UI_HANDLER = 2;
    constexpr UBaseType_t STATUS_MONITOR = 1;
}

// Stack sizes for tasks (in words, not bytes)
namespace StackSizes {
    constexpr uint32_t RS485_HANDLER = 4096;
    constexpr uint32_t SLAVE_MANAGER = 3072;
    constexpr uint32_t PING_SCHEDULER = 2048;
    constexpr uint32_t UI_HANDLER = 2048;
    constexpr uint32_t STATUS_MONITOR = 1536;
}