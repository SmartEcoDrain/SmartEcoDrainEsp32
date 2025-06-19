#pragma once

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

// Supabase Configuration
#define SUPABASE_URL "https://mpsrepfjrnjaciobvdrh.supabase.co"
#define SUPABASE_ANON_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im1wc3JlcGZqcm5qYWNpb2J2ZHJoIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDk0MDIwNjUsImV4cCI6MjA2NDk3ODA2NX0.l03peDJRu6VhvX6htZ5AIe10whscVC0GVe-51JGbx1Q"

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
