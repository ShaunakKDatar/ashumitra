#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "sra_board.h"


#include "sra_board.h"

// Remove or comment out this line since it conflicts with the later TAG definition
// #define TAG "MCPWM_SERVO_CONTROL"

servo_config servo_a = {
	.servo_pin = SERVO_A,
	.min_pulse_width = CONFIG_SERVO_A_MIN_PULSEWIDTH,
	.max_pulse_width = CONFIG_SERVO_A_MAX_PULSEWIDTH,
	.max_degree = CONFIG_SERVO_A_MAX_DEGREE,
	.mcpwm_num = MCPWM_UNIT_0,
	.timer_num = MCPWM_TIMER_0,
	.gen = MCPWM_OPR_A,
};

servo_config servo_b = {
	.servo_pin = SERVO_B,
	.min_pulse_width = CONFIG_SERVO_B_MIN_PULSEWIDTH,
	.max_pulse_width = CONFIG_SERVO_B_MAX_PULSEWIDTH,
	.max_degree = CONFIG_SERVO_B_MAX_DEGREE,
	.mcpwm_num = MCPWM_UNIT_0,
	.timer_num = MCPWM_TIMER_0,
	.gen = MCPWM_OPR_B,
};

servo_config servo_c = {
	.servo_pin = SERVO_C,
	.min_pulse_width = CONFIG_SERVO_C_MIN_PULSEWIDTH,
	.max_pulse_width = CONFIG_SERVO_C_MAX_PULSEWIDTH,
	.max_degree = CONFIG_SERVO_C_MAX_DEGREE,
	.mcpwm_num = MCPWM_UNIT_0,
	.timer_num = MCPWM_TIMER_1,
	.gen = MCPWM_OPR_A,
};

servo_config servo_d = {
	.servo_pin = SERVO_D,
	.min_pulse_width = CONFIG_SERVO_D_MIN_PULSEWIDTH,
	.max_pulse_width = CONFIG_SERVO_D_MAX_PULSEWIDTH,
	.max_degree = CONFIG_SERVO_D_MAX_DEGREE,
	.mcpwm_num = MCPWM_UNIT_0,
	.timer_num = MCPWM_TIMER_1,
	.gen = MCPWM_OPR_B,
};

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

// WiFi credentials
#define WIFI_SSID      "Pixie"
#define WIFI_PASS      "Prajwal5"
#define MAXIMUM_RETRY  5

// Keep this TAG definition
static const char *TAG = "webserver";
static int s_retry_num = 0;
static int current_med_box = 0;
static httpd_handle_t server = NULL;

// HTML content stored in a separate string
static const char *html_page =
"<!DOCTYPE html>"
"<html>"
"<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<title>Asumitra Medicine Manager</title>"
    "<style>"
        "body { background: #f3f4f6; padding: 20px; max-width: 600px; margin: 0 auto; }"
        ".header { background: linear-gradient(to right, #7c3aed, #8b5cf6); color: white; padding: 20px; border-radius: 12px; margin-bottom: 20px; text-align: center; }"
        ".add-medicine { background: white; padding: 20px; border-radius: 12px; margin-bottom: 20px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
        ".button { background: #7c3aed; color: white; padding: 12px 20px; border: none; border-radius: 8px; cursor: pointer; width: 100%; }"
        ".medicine-list { background: white; padding: 20px; border-radius: 12px; }"
        "input, select { width: 100%; padding: 12px; margin: 5px 0; border-radius: 8px; border: 1px solid #ddd; }"
    "</style>"
"</head>"
"<body>"
    "<div class=\"header\">"
        "<h1>Asumitra Medicine Manager</h1>"
        "<p>Manage your medicine schedule</p>"
    "</div>"
    "<div class=\"add-medicine\">"
        "<h2>Add New Medicine</h2>"
        "<input type=\"text\" id=\"medName\" placeholder=\"Medicine Name\">"
        "<input type=\"text\" id=\"medDosage\" placeholder=\"Dosage\">"
        "<input type=\"time\" id=\"medTime\">"
        "<select id=\"medContainer\">"
            "<option value=\"\">Select Container</option>"
            "<option value=\"1\">Container #1</option>"
            "<option value=\"2\">Container #2</option>"
            "<option value=\"3\">Container #3</option>"
            "<option value=\"4\">Container #4</option>"
            "<option value=\"5\">Container #5</option>"
        "</select>"
        "<button class=\"button\" onclick=\"addMedicine()\">Add Medicine</button>"
    "</div>"
    "<div class=\"medicine-list\" id=\"medicineList\"></div>"
    "<script>"
        "function addMedicine() {"
            "const name = document.getElementById('medName').value;"
            "const dosage = document.getElementById('medDosage').value;"
            "const time = document.getElementById('medTime').value;"
            "const container = document.getElementById('medContainer').value;"
            "if (!name || !dosage || !time || !container) {"
                "alert('Please fill all fields');"
                "return;"
            "}"
            "const medicine = {name, dosage, time, container};"
            "const medicines = JSON.parse(localStorage.getItem('medicines') || '[]');"
            "medicines.push(medicine);"
            "localStorage.setItem('medicines', JSON.stringify(medicines));"
            "renderMedicines();"
            "clearForm();"
        "}"
        "function deployMedicine(container) {"
            "fetch('/deploy', {"
                "method: 'POST',"
                "headers: {'Content-Type': 'application/json'},"
                "body: JSON.stringify({med_box_number: container})"
            "})"
            ".then(response => response.json())"
            ".then(data => alert('Medicine deployed from container ' + container))"
            ".catch(error => alert('Error deploying medicine'));"
        "}"
        "function renderMedicines() {"
            "const medicines = JSON.parse(localStorage.getItem('medicines') || '[]');"
            "const list = document.getElementById('medicineList');"
            "list.innerHTML = medicines.map(med => `"
                "<div style='border: 1px solid #ddd; padding: 10px; margin: 5px 0; border-radius: 8px;'>"
                    "<div>${med.name} - ${med.dosage} - ${med.time}</div>"
                    "<button onclick='deployMedicine(${med.container})' style='background: #10b981; color: white; border: none; padding: 5px 10px; border-radius: 4px; margin-top: 5px;'>"
                        "Deploy from Container ${med.container}"
                    "</button>"
                "</div>"
            "`).join('');"
        "}"
        "function clearForm() {"
            "document.getElementById('medName').value = '';"
            "document.getElementById('medDosage').value = '';"
            "document.getElementById('medTime').value = '';"
            "document.getElementById('medContainer').value = '';"
        "}"
        "renderMedicines();"
    "</script>"
"</body>"
"</html>";

// HTTP Handlers
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, strlen(html_page));
    return ESP_OK;
}

static esp_err_t deploy_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *med_box = cJSON_GetObjectItem(root, "med_box_number");
    if (cJSON_IsNumber(med_box)) {
        current_med_box = med_box->valueint;
        ESP_LOGI(TAG, "Deploying medicine from box: %d", current_med_box);

        // More comprehensive servo movement
        ESP_LOGI(TAG, "REEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
        set_angle_servo(&servo_d, current_med_box*13);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "IT IS DONE SIR");
        httpd_resp_set_type(req, "application/json");
        char resp[64];
        snprintf(resp, sizeof(resp), "{\"status\":\"success\",\"med_box\":%d}", current_med_box);
        httpd_resp_send(req, resp, strlen(resp));
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid box number");
    }

    cJSON_Delete(root);
    return ESP_OK;
}

// Server setup
static httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_get = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = root_get_handler,
            .user_ctx = NULL
        };
        httpd_uri_t uri_post = {
            .uri      = "/deploy",
            .method   = HTTP_POST,
            .handler  = deploy_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

// WiFi Event Handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        }
        ESP_LOGI(TAG,"Connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        start_webserver();
    }
}

// WiFi initialization
static void wifi_init_sta(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

esp_err_t ret;

void app_main(void) {
    // Initialize servo first with error checking
    esp_err_t servo_err = enable_servo();
    if (servo_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable servo! Error: %d", servo_err);
        return;
    }
    ESP_LOGI(TAG, "Servo initialized successfully");

    // Test servo movement immediately after initialization
    ESP_LOGI(TAG, "Testing servo movement...");
    set_angle_servo(&servo_d, 0);    // Move to 0 degrees
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    set_angle_servo(&servo_d, 90);   // Move to 90 degrees
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP32 Medicine Manager Starting...");

    // Initialize WiFi
    wifi_init_sta();
}
