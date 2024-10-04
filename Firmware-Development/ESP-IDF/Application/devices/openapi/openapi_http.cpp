#include <esp_http_client.h>
#include "esp_tls.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "openapi_http.hpp"
#include "cJSON.h"

static const char *TAG = "openapi-http";
// #define ESP_LOGD ESP_LOGE
char buffer[MAX_HTTP_RECV_BUFFER] = {0};
uint32_t size = 0;

void OpenAI::spiffs_init()
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true};

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }
}

const char *getFileName(const char *filePath)
{
    // Find the position of the last '/' in the file path
    char *lastSlash = strrchr(filePath, '/');

    if (lastSlash != NULL)
    {
        // Extract the filename with extension
        return lastSlash + 1;
    }
    else
    {
        // There is no '/' in the file path, so the entire path is the filename
        return filePath;
    }
}

// Event handler for HTTP events
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
         *  However, event handler can also be used in case chunked encoding is used.
         */
        // if (!esp_http_client_is_chunked_response(evt->client)) {
        // If user_data buffer is configured, copy the response into the buffer
        if (evt->user_data)
        {
            memcpy(evt->user_data + output_len, evt->data, evt->data_len);
        }
        else
        {
            if (output_buffer == NULL)
            {
                output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                output_len = 0;
                if (output_buffer == NULL)
                {
                    ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                    return ESP_FAIL;
                }
            }
            memcpy(output_buffer + output_len, evt->data, evt->data_len);
        }
        output_len += evt->data_len;
        // }
        // else{
        //     ESP_LOGE(TAG, "elseeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
        // }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        ESP_LOGE(TAG, "HTTP RESPONSE LENGTH : %d", output_len);
        size = output_len;
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(static_cast<esp_tls_last_error *>(evt->data), &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        if (output_buffer != NULL)
        {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    }
    return ESP_OK;
}

void OpenAI::send_post_request(const char *api_key, const char *file_path)
{
    esp_http_client_config_t config = {
        .url = "https://api.openai.com/v1/audio/transcriptions", //"http://192.168.18.191:8000",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        ESP_LOGE(TAG, "Failed to open file");
        esp_http_client_cleanup(client);
        return;
    }

    // Calculate file size
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char auth_value[100];
    snprintf(auth_value, sizeof(auth_value), "Bearer %s", api_key);
    esp_http_client_set_header(client, "Authorization", auth_value);

    esp_http_client_set_header(client, "Content-Type", "multipart/form-data; boundary=esp32boundary");
    // esp_http_client_set_header(client, "Content-Length", "12667");

    const char *head_boundary = "--esp32boundary\r\n";
    const char *head_template = "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n\r\n";
    const char *filename = OpenAI::getFileName(file_path);
    size_t resultSize = strlen(head_template) + strlen(filename);

    // Allocate memory for the final string
    char *head_final = (char *)malloc(resultSize + 1);

    // Create the final string by formatting the head_template with the filename
    snprintf(head_final, resultSize + 1, head_template, filename);

    const char *model_boundary = "\r\n--esp32boundary\r\n";
    const char *model = "Content-Disposition: form-data; name=\"model\"\r\n\r\nwhisper-1\r\n";
    const char *tail = "--esp32boundary--\r\n";

    // Set content length
    size_t content_length = strlen(head_boundary) + strlen(head_final) + strlen(model_boundary) + strlen(model) + file_size + strlen(tail);
    ESP_LOGE("", "Length : %d", content_length);

    esp_err_t ret = esp_http_client_open(client, content_length);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(ret));
        esp_http_client_cleanup(client);
    }

    // Start building the request body
    esp_http_client_write(client, head_boundary, strlen(head_boundary));
    esp_http_client_write(client, head_final, strlen(head_final));

    // Loop to read and send file content
    while (file_size > 0)
    {
        int read_len = fread(buffer, 1, MAX_HTTP_RECV_BUFFER, file);
        if (read_len <= 0)
        {
            ESP_LOGE(TAG, "Error reading file");
            break;
        }
        esp_http_client_write(client, buffer, read_len);
        file_size -= read_len;
    }

    // Add model parameter to the request body
    esp_http_client_write(client, model_boundary, strlen(model_boundary));
    esp_http_client_write(client, model, strlen(model));
    esp_http_client_write(client, tail, strlen(tail));

    fclose(file);

    // Perform HTTP request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(head_final);
}

void OpenAI::fillWavHeader(WavHeader *header, int dataSize)
{
    // Fill in the header values
    memcpy(header->riff, "RIFF", 4);
    header->fileSize = dataSize + 36; // Wave file size excluding first 44 bytes
    memcpy(header->wave, "WAVE", 4);
    memcpy(header->fmt, "fmt ", 4);
    header->fmtSize = 16;       // Size of the fmt chunk
    header->audioFormat = 1;    // PCM format
    header->numChannels = 1;    // Mono
    header->sampleRate = 16000; // Sample rate
    header->bitsPerSample = 16; // Bits per sample
    header->blockAlign = header->numChannels * header->bitsPerSample / 8;
    header->byteRate = header->sampleRate * header->numChannels * header->bitsPerSample / 8;
    memcpy(header->data, "data", 4);
    header->dataSize = dataSize; // Size of the data chunk
}

void OpenAI::transcribe_wav_file(int16_t *stream, uint32_t size, char *received_buf)
{
    esp_http_client_config_t config =
        {
            .url = "https://api.openai.com/v1/audio/transcriptions",
            .method = HTTP_METHOD_POST,
            .event_handler = _http_event_handler,
            .user_data = received_buf,
        };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    char auth_value[100];
    snprintf(auth_value, sizeof(auth_value), "Bearer %s", API_KEY);
    esp_http_client_set_header(client, "Authorization", auth_value);

    esp_http_client_set_header(client, "Content-Type", "multipart/form-data; boundary=esp32boundary");

    const char *head_boundary = "--esp32boundary\r\n";
    const char *head_template = "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n\r\n";
    size_t resultSize = strlen(head_template);

    // Allocate memory for the final string
    char *head_final = (char *)malloc(resultSize + 1);

    // Create the final string by formatting the head_template with the filename
    snprintf(head_final, resultSize + 1, head_template);

    const char *model_boundary = "\r\n--esp32boundary\r\n";
    const char *model = "Content-Disposition: form-data; name=\"model\"\r\n\r\nwhisper-1\r\n";
    const char *tail = "--esp32boundary--\r\n";

    uint8_t header[44];
    uint32_t byteSize = size * sizeof(int16_t);
    fillWavHeader((WavHeader *)header, byteSize);

    // Set content length
    size_t content_length = strlen(head_boundary) + strlen(head_final) + strlen(model_boundary) + strlen(model) + byteSize + sizeof(header) + strlen(tail);
    ESP_LOGE("", "Length : %d", content_length);
    ESP_LOGE("", "data length : %d", ((WavHeader *)header)->dataSize);

    esp_err_t ret = esp_http_client_open(client, content_length);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(ret));
        esp_http_client_cleanup(client);
    }

    // Start building the request body
    esp_http_client_write(client, head_boundary, strlen(head_boundary));
    esp_http_client_write(client, head_final, strlen(head_final));

    esp_http_client_write(client, (char *)header, sizeof(header));
    esp_http_client_write(client, (char *)stream, byteSize);

    // Add model parameter to the request body
    esp_http_client_write(client, model_boundary, strlen(model_boundary));
    esp_http_client_write(client, model, strlen(model));
    esp_http_client_write(client, tail, strlen(tail));

    // Perform HTTP request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));

        received_buf[esp_http_client_get_content_length(client)] = '\0';
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(head_final);
}

char *OpenAI::createJson(const char *key, const char *value, ...)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, key, value);

    va_list args;
    va_start(args, value);

    const char *nextKey = va_arg(args, const char *);
    while (nextKey != NULL)
    {
        const char *nextValue = va_arg(args, const char *);
        cJSON_AddStringToObject(root, nextKey, nextValue);
        nextKey = va_arg(args, const char *);
    }

    va_end(args);

    char *jsonString = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);

    return jsonString;
}

void OpenAI::text_completion(char *transmit_buf, char *received_buf)
{
    esp_http_client_config_t config =
        {
            .url = "https://api.openai.com/v1/completions",
            .method = HTTP_METHOD_POST,
            .event_handler = _http_event_handler,
            .user_data = received_buf,
        };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    char auth_value[100];
    snprintf(auth_value, sizeof(auth_value), "Bearer %s", API_KEY);
    esp_http_client_set_header(client, "Authorization", auth_value);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_http_client_set_post_field(client, transmit_buf, strlen(transmit_buf));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));

        received_buf[esp_http_client_get_content_length(client)] = '\0';
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

char *OpenAI::process_response(const char *response)
{
    char *result = NULL;

    // Parse the JSON response
    cJSON *json = cJSON_Parse(response);
    if (json == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse JSON response.");
    }
    else
    {
        // Extract the "text" field from the "choices" array
        cJSON *choices = cJSON_GetObjectItemCaseSensitive(json, "choices");
        if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0)
        {
            cJSON *choice = cJSON_GetArrayItem(choices, 0);
            cJSON *textField = cJSON_GetObjectItemCaseSensitive(choice, "text");
            if (cJSON_IsString(textField))
            {
                ESP_LOGI(TAG, "%s\n", textField->valuestring);
                // Allocate memory for the result string
                result = strdup(textField->valuestring);
            }
        }
    }

    // Free the allocated memory for the cJSON object
    cJSON_Delete(json);

    return result;
}

uint32_t OpenAI::create_speech(char *transmit_buf, char *received_buf)
{
    esp_http_client_config_t config =
        {
            .url = "https://api.openai.com/v1/audio/speech",
            .method = HTTP_METHOD_POST,
            .event_handler = _http_event_handler,
            .user_data = received_buf,
        };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    char auth_value[100];
    snprintf(auth_value, sizeof(auth_value), "Bearer %s", API_KEY);
    esp_http_client_set_header(client, "Authorization", auth_value);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_http_client_set_post_field(client, transmit_buf, strlen(transmit_buf));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);

    return size;
}

char *OpenAI::processTranscribeResponse(char *response)
{
    char *result = NULL;

    // Parse the JSON string
    cJSON *json = cJSON_Parse(response);

    if (json == NULL)
    {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
    }
    else
    {
        // Extract the "text" field
        cJSON *textField = cJSON_GetObjectItemCaseSensitive(json, "text");

        if (cJSON_IsString(textField))
        {
            ESP_LOGI(TAG, "%s\n", textField->valuestring);
            // Allocate memory for the result string
            result = strdup(textField->valuestring);
        }
        else
        {
            fprintf(stderr, "Failed to extract 'text' field from JSON.\n");
        }
    }

    // Free the allocated memory for the cJSON object
    cJSON_Delete(json);

    return result;
}