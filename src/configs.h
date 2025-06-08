// Sensor Node Configuration
#define TOF1_PIN 5
#define TOF2_PIN 6
#define FORCE1_PIN A0
#define FORCE2_PIN A3
#define WEIGHT1_PIN A4
#define WEIGHT2_PIN A5
#define TURBIDITY1_PIN A6
#define TURBIDITY2_PIN A7   
#define ULTRASONIC1_TRIG_PIN 7
#define ULTRASONIC1_ECHO_PIN 8
#define ULTRASONIC2_TRIG_PIN 9
#define ULTRASONIC2_ECHO_PIN 10

// Supabase Configuration
const char* SUPABASE_URL = "https://efyzoqrkvaxvuphvmlnt.supabase.co";
const char* SUPABASE_ANON_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImVmeXpvcXJrdmF4dnVwaHZtbG50Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDkzNjgwNTIsImV4cCI6MjA2NDk0NDA1Mn0.T9B5jFhXX9XrD_AJ-i1fGj5Auzx7pj7DK5tzY-C7Uhg";

// Device Configuration - IMPORTANT: Make each device unique
#define DEVICE_ID "esp32_sensor_001"  // Change this for each device
#define DEVICE_VERSION "1.0.0"
#define DEVICE_LOCATION "Test_Location_A"

// Network Configuration (adjust for your carrier)
#ifndef NETWORK_APN
#define NETWORK_APN "internet"  // Change to your carrier's APN
#endif