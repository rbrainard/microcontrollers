#include <M5Stack.h>
#include <esp_dmx.h>

// DMX Configuration for M5Stack DMX Base Module
#define DMX_START_CHANNEL 1    // Starting DMX channel (1-512) - SET THIS TO MATCH YOUR CONTROLLER
#define TOTAL_CHANNELS 32      // Total channels on your controller
#define DMX_UART_NUM 1         // UART port for DMX (don't use 0 - reserved for serial)
#define USE_16_BIT false       // Set to true for 16-bit mode, false for 8-bit mode

// RGBW Channel grouping (8 groups of 4 channels each for 32 total channels)
#define GROUPS 8
#define CHANNELS_PER_GROUP 4   // R, G, B, W

// Global variables
uint8_t dmxData[DMX_MAX_PACKET_SIZE]; // DMX data array
QueueHandle_t queue;              // Queue for DMX events
uint8_t currentGroup = 0;         // Currently selected group (0-7)
uint8_t currentChannel = 0;       // Currently selected channel within group (0-3: R,G,B,W)
uint8_t channelValue = 0;         // Current channel value (0-255)
bool dmxEnabled = true;

// Pin configuration for M5Stack DMX Base Module
gpio_num_t transmitPin = GPIO_NUM_13;  // TX pin for M5Stack BASIC/GRAY/GO/FIRE
gpio_num_t receivePin = GPIO_NUM_35;   // RX pin for M5Stack BASIC/GRAY/GO/FIRE  
gpio_num_t enablePin = GPIO_NUM_12;    // EN pin for M5Stack BASIC/GRAY/GO/FIRE

// Channel names for display
const char* channelNames[] = {"R", "G", "B", "W"};
const char* groupNames[] = {"Grp1", "Grp2", "Grp3", "Grp4", "Grp5", "Grp6", "Grp7", "Grp8"};

// Function prototypes
void initializeDMX();
void updateDMXChannel(uint8_t group, uint8_t channel, uint8_t value);
void updateDMXGroup(uint8_t group, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void setAllChannels(uint8_t value);
void displayUI();
void handleButtons();
void runColorCycle();
void runDimmerEffect();
void sendDMXData();

void setup() {
    // Initialize M5Stack
    M5.begin();
    M5.Power.begin();
    
    // Initialize display
    M5.Lcd.clear(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("DMX Controller");
    M5.Lcd.println("Initializing...");
    
    // Initialize Serial for debugging
    Serial.begin(115200);
    Serial.println("M5Stack DMX Controller Starting...");
    
    // Initialize DMX
    initializeDMX();
    
    // Clear all channels initially
    setAllChannels(0);
    
    // Show initial UI
    delay(1000);
    displayUI();
    
    Serial.println("Setup complete!");
}

void loop() {
    M5.update();
    handleButtons();
    
    // Send DMX data continuously
    if (dmxEnabled) {
        sendDMXData();
    }
    
    delay(50); // Small delay for stability
}

void initializeDMX() {
    // Configure DMX with default settings
    dmx_config_t dmxConfig = DMX_DEFAULT_CONFIG;
    dmx_param_config(DMX_UART_NUM, &dmxConfig);
    
    // Set the pins for DMX communication based on M5Stack DMX Base module
    dmx_set_pin(DMX_UART_NUM, transmitPin, receivePin, enablePin);
    
    // Install the DMX driver
    dmx_driver_install(DMX_UART_NUM, DMX_MAX_PACKET_SIZE, 1, &queue, 1);
    
    // Set DMX mode to transmit
    dmx_set_mode(DMX_UART_NUM, DMX_MODE_WRITE);
    
    // Initialize DMX data array
    for (int i = 0; i < DMX_MAX_PACKET_SIZE; i++) {
        dmxData[i] = 0;
    }
    
    Serial.println("DMX initialized for M5Stack DMX Base");
    Serial.printf("TX Pin: %d, RX Pin: %d, EN Pin: %d\n", transmitPin, receivePin, enablePin);
}

void updateDMXChannel(uint8_t group, uint8_t channel, uint8_t value) {
    if (group < GROUPS && channel < CHANNELS_PER_GROUP) {
        uint16_t dmxChannel = (group * CHANNELS_PER_GROUP) + channel + DMX_START_CHANNEL;
        
        if (USE_16_BIT) {
            // 16-bit mode: each channel uses 2 DMX addresses
            uint16_t dmxChannel16 = dmxChannel * 2 - 1; // Convert to 16-bit addressing
            if (dmxChannel16 < DMX_MAX_PACKET_SIZE - 1) {
                uint16_t value16 = value * 257; // Convert 8-bit to 16-bit (0-255 to 0-65535)
                dmxData[dmxChannel16] = (value16 >> 8) & 0xFF;     // MSB (high byte)
                dmxData[dmxChannel16 + 1] = value16 & 0xFF;        // LSB (low byte)
                Serial.printf("Updated Group %d, Channel %s to %d (16-bit DMX Ch: %d-%d)\n", 
                             group + 1, channelNames[channel], value, dmxChannel16, dmxChannel16 + 1);
            }
        } else {
            // 8-bit mode: each channel uses 1 DMX address
            if (dmxChannel < DMX_MAX_PACKET_SIZE) {
                dmxData[dmxChannel] = value;
                Serial.printf("Updated Group %d, Channel %s to %d (8-bit DMX Ch: %d)\n", 
                             group + 1, channelNames[channel], value, dmxChannel);
            }
        }
    }
}

void updateDMXGroup(uint8_t group, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    if (group < GROUPS) {
        updateDMXChannel(group, 0, r); // Red
        updateDMXChannel(group, 1, g); // Green
        updateDMXChannel(group, 2, b); // Blue
        updateDMXChannel(group, 3, w); // White
    }
}

void setAllChannels(uint8_t value) {
    for (int group = 0; group < GROUPS; group++) {
        for (int ch = 0; ch < CHANNELS_PER_GROUP; ch++) {
            updateDMXChannel(group, ch, value);
        }
    }
    Serial.printf("Set all channels to %d\n", value);
}

void sendDMXData() {
    // Write DMX packet
    dmx_write_packet(DMX_UART_NUM, dmxData, DMX_MAX_PACKET_SIZE);
    
    // Wait for transmission to complete and then send the packet
    if (dmx_wait_send_done(DMX_UART_NUM, DMX_PACKET_TIMEOUT_TICK) != ESP_ERR_TIMEOUT) {
        dmx_send_packet(DMX_UART_NUM, DMX_MAX_PACKET_SIZE);
    }
}

void displayUI() {
    M5.Lcd.clear(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    
    // Title
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.printf("DMX %s-bit", USE_16_BIT ? "16" : "8");
    
    // Current group and channel
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.printf("Group: %s", groupNames[currentGroup]);
    
    M5.Lcd.setCursor(10, 70);
    M5.Lcd.printf("Channel: %s", channelNames[currentChannel]);
    
    // Current value and DMX channel
    M5.Lcd.setCursor(10, 100);
    uint16_t dmxCh = (currentGroup * CHANNELS_PER_GROUP) + currentChannel + DMX_START_CHANNEL;
    if (USE_16_BIT) {
        uint16_t dmxCh16 = dmxCh * 2 - 1;
        uint16_t value16 = (dmxData[dmxCh16] << 8) | dmxData[dmxCh16 + 1];
        uint8_t value8 = value16 / 257; // Convert back to 8-bit for display
        M5.Lcd.printf("Value: %d", value8);
        M5.Lcd.setCursor(10, 130);
        M5.Lcd.printf("DMX: %d-%d", dmxCh16, dmxCh16 + 1);
    } else {
        M5.Lcd.printf("Value: %d", dmxData[dmxCh]);
        M5.Lcd.setCursor(10, 130);
        M5.Lcd.printf("DMX Ch: %d", dmxCh);
    }
    
    // Status
    M5.Lcd.setCursor(10, 160);
    M5.Lcd.printf("Start: %d", DMX_START_CHANNEL);
    
    // Button labels
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 200);
    M5.Lcd.println("A: Value  B: Channel  C: Group");
    M5.Lcd.setCursor(10, 220);
    M5.Lcd.println("Hold A: Effects  Hold C: All Off");
}

void handleButtons() {
    static unsigned long buttonPressTime = 0;
    static bool longPressHandled = false;
    
    // Button A - Adjust value (or effects on long press)
    if (M5.BtnA.wasPressed()) {
        buttonPressTime = millis();
        longPressHandled = false;
    }
    
    if (M5.BtnA.isPressed() && !longPressHandled && (millis() - buttonPressTime > 1000)) {
        // Long press - run effects
        longPressHandled = true;
        runColorCycle();
        displayUI();
    } else if (M5.BtnA.wasReleased() && !longPressHandled) {
        // Short press - adjust value
        uint16_t dmxCh = (currentGroup * CHANNELS_PER_GROUP) + currentChannel + DMX_START_CHANNEL;
        
        if (USE_16_BIT) {
            uint16_t dmxCh16 = dmxCh * 2 - 1;
            uint16_t currentValue16 = (dmxData[dmxCh16] << 8) | dmxData[dmxCh16 + 1];
            uint8_t currentValue8 = currentValue16 / 257;
            currentValue8 += 32;
            if (currentValue8 > 255) currentValue8 = 0;
            uint16_t newValue16 = currentValue8 * 257;
            dmxData[dmxCh16] = (newValue16 >> 8) & 0xFF;
            dmxData[dmxCh16 + 1] = newValue16 & 0xFF;
        } else {
            dmxData[dmxCh] += 32;
            if (dmxData[dmxCh] > 255) dmxData[dmxCh] = 0;
        }
        displayUI();
    }
    
    // Button B - Change channel
    if (M5.BtnB.wasPressed()) {
        currentChannel = (currentChannel + 1) % CHANNELS_PER_GROUP;
        displayUI();
    }
    
    // Button C - Change group (or all off on long press)
    if (M5.BtnC.wasPressed()) {
        buttonPressTime = millis();
        longPressHandled = false;
    }
    
    if (M5.BtnC.isPressed() && !longPressHandled && (millis() - buttonPressTime > 1000)) {
        // Long press - all channels off
        longPressHandled = true;
        setAllChannels(0);
        M5.Lcd.clear(BLACK);
        M5.Lcd.setCursor(50, 100);
        M5.Lcd.println("ALL OFF");
        delay(1000);
        displayUI();
    } else if (M5.BtnC.wasReleased() && !longPressHandled) {
        // Short press - change group
        currentGroup = (currentGroup + 1) % GROUPS;
        displayUI();
    }
}

void runColorCycle() {
    M5.Lcd.clear(BLACK);
    M5.Lcd.setCursor(50, 100);
    M5.Lcd.println("Color Cycle");
    
    // Cycle through colors on current group
    for (int cycle = 0; cycle < 3; cycle++) {
        // Red
        updateDMXGroup(currentGroup, 255, 0, 0, 0);
        delay(500);
        
        // Green
        updateDMXGroup(currentGroup, 0, 255, 0, 0);
        delay(500);
        
        // Blue
        updateDMXGroup(currentGroup, 0, 0, 255, 0);
        delay(500);
        
        // White
        updateDMXGroup(currentGroup, 0, 0, 0, 255);
        delay(500);
        
        // All colors
        updateDMXGroup(currentGroup, 255, 255, 255, 255);
        delay(500);
        
        // Off
        updateDMXGroup(currentGroup, 0, 0, 0, 0);
        delay(500);
    }
}

void runDimmerEffect() {
    M5.Lcd.clear(BLACK);
    M5.Lcd.setCursor(50, 100);
    M5.Lcd.println("Dimmer Effect");
    
    // Fade in and out on current group (white channel)
    for (int brightness = 0; brightness <= 255; brightness += 5) {
        updateDMXChannel(currentGroup, 3, brightness); // White channel
        delay(50);
    }
    
    for (int brightness = 255; brightness >= 0; brightness -= 5) {
        updateDMXChannel(currentGroup, 3, brightness); // White channel
        delay(50);
    }
}