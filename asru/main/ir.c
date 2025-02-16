#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define IR_SENSOR_GPIO 5

void app_main() {
    // Configure GPIO pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << IR_SENSOR_GPIO),  // Bit mask for GPIO 5
        .mode = GPIO_MODE_INPUT,                   // Set as input mode
        .pull_up_en = GPIO_PULLUP_ENABLE,         // Enable internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,    // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE            // Disable interrupt
    };
    gpio_config(&io_conf);

    while(1) {
        // Read sensor state
        int sensor_state = gpio_get_level(IR_SENSOR_GPIO);
        
        // Print state to console
        printf("IR Sensor State: %d\n", sensor_state);
        
        // Add delay (1 second)
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

