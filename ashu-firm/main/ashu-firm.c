#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Define GPIO pins
#define IR_GPIO GPIO_NUM_4     // IR sensor output pin
#define SERVO_GPIO GPIO_NUM_18 // Servo signal pin

// LEDC (PWM) configuration for servo
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // 13-bit resolution
#define LEDC_FREQUENCY 50              // 50 Hz for servo control

// Function to initialize the IR sensor GPIO
void setup_ir_sensor() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << IR_GPIO), // Select the GPIO pin
        .mode = GPIO_MODE_INPUT,           // Set as input mode
        .pull_up_en = GPIO_PULLUP_DISABLE, // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE     // Disable interrupt
    };
    gpio_config(&io_conf);
}

// Function to initialize the servo motor
void setup_servo() {
    // Configure the LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Configure the LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num = SERVO_GPIO,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0, // Initial duty cycle
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

// Function to set the servo angle
void set_servo_angle(int angle) {
    // Constrain the angle to 0-180 degrees
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    // Calculate the pulse width (1 ms to 2 ms)
    float pulse_width = 1.0 + (angle / 180.0); // in milliseconds
    float period = 20.0; // 20 ms period for 50 Hz
    uint32_t duty = (pulse_width / period) * (1 << LEDC_DUTY_RES);

    // Set the duty cycle
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// Main application
void app_main() {
    // Initialize the IR sensor and servo
    setup_ir_sensor();
    setup_servo();

    set_servo_angle(0);

    while (1) {
        // Read the IR sensor value
        int ir_value = gpio_get_level(IR_GPIO);

        if (ir_value == 0) {
            // Object detected: rotate the servo
            printf("Object detected! Rotating servo...\n");
            set_servo_angle(90); // Rotate to 90 degrees
        } else {
            // No object detected: stop the servo
            printf("No object detected. Servo stopped.\n");
            set_servo_angle(0); // Stop the servo
        }

        // Add a small delay to avoid flooding the output
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}