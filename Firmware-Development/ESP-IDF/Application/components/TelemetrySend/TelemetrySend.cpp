#include <esp_http_client.h>
#include "TelemetrySend.hpp"
#include "esp_log.h"


// Define the server endpoint URL
#define SERVER_URL "http://192.168.18.138:8080/"
#define TAG "TelemetrySend" // Change this tag to suit your needs

// Telemetry data structure (customize based on your needs)

// Function to send telemetry data to the server using a POST request
// Function to send telemetry data to the server using a POST request
void TelemetrySend::send_telemetry_data(const char *jsonData)
{
    esp_http_client_config_t config = {
        .url = SERVER_URL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    

    esp_http_client_set_post_field(client, jsonData, strlen(jsonData));
    // Perform the HTTP POST request
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Telemetry data sent successfully");
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    // Clean up the HTTP client and free allocated memory for JSON data
    esp_http_client_cleanup(client);
    
}
