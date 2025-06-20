// EEPROM Configuration
#define EEPROM_SIZE 512           // Total EEPROM size
#define SETUP_FLAG_ADDR 0         // Address for setup completion flag (uses 2 bytes)
#define DEVICE_DATA_ADDR 4        // Start address for other device data (if needed)
#define SETUP_MAGIC_NUMBER 0xABCD // Magic number to verify setup completion

// Reserve specific EEPROM sections to avoid conflicts
#define EEPROM_SETUP_SECTION_START 0
#define EEPROM_SETUP_SECTION_SIZE 4   // 4 bytes for setup data
#define EEPROM_USER_SECTION_START (EEPROM_SETUP_SECTION_START + EEPROM_SETUP_SECTION_SIZE)
#define EEPROM_USER_SECTION_SIZE (EEPROM_SIZE - EEPROM_USER_SECTION_START)

// Sensor Node Configuration
#define FORCE0_ANALOG_PIN 35
#define FORCE1_ANALOG_PIN 34
#define TOF_SCL_PIN 22
#define TOF_SDA_PIN 21
#define WEIGHT_DATA_PIN 12
#define WEIGHT_SCK_PIN 14
#define TURBIDITY_ANALOG_PIN 32
#define ULTRASONIC_TRIG_PIN 25
#define ULTRASONIC_ECHO_PIN 26

//Support A7670X/A7608X/SIM7670G
#define TINY_GSM_MODEM_A76XXSSL 

// API Configuration
#define SERVER_HOST "smart-echodrain.vercel.app"
#define SERVER_PORT 443
#define ESP32_API_KEY "smart-echo-drain-esp32-2025-secure-key"

#define DEVICE_ID "esp32_sensor_001"  // Change this for each device
#define DEVICE_VERSION "0.0.1"

// Setup mode configuration
#define SETUP_SSID "SmartEchoDrain"
#define SETUP_PASSWORD "echodrain25"
#define SETUP_TIMEOUT 300000  // 5 minutes setup timeout

// EEPROM addresses for storing setup completion flag
#define EEPROM_SIZE 512
#define SETUP_FLAG_ADDR 0
#define SETUP_MAGIC_NUMBER 0xABCD