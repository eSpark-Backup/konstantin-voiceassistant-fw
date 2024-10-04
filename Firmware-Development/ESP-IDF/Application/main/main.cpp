/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "model_path.h"
#include "string.h"
#include "esp_log.h"
#include "openapi_http.hpp"
#include "nvs_flash.h"
#include "output.hpp"
#include "mp3_player.hpp"
#include "audio_data.hpp"
#include "board.h"
#include "input_key_service.h"
#include "WifiSoftApp.hpp"
#include "TelemetrySend.hpp"
#include <cJSON.h>
#include "main.hpp"
#include <utility>

static esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;
static volatile int task_flag = 0;
bool recordStatus = false;
char *speechBuffer = NULL;
const char *TAG = "ESP-SKAINET";

#define RECORD_TIME_SECONDS 4
#define ONE_SECOND_FRAMES 31.25f

uint32_t record(int16_t *buff, uint16_t seconds);
static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx);
Json *json = new Json();
WifiProvisioning *wifiProvisioning = new WifiProvisioning();
TelemetrySend *telemetrySend = new TelemetrySend();
OpenAI *openAI = new OpenAI();
// PlayAudio *playAudio = new PlayAudio();

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = static_cast<esp_afe_sr_data_t *>(arg);
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch < feed_channel);
    int16_t *i2s_buff = static_cast<int16_t *>(malloc(audio_chunksize * sizeof(int16_t) * feed_channel));
    assert(i2s_buff);
    ESP_LOGE("", "chunk size : %d", audio_chunksize);

    while (1)
    {
        while (task_flag)
        {
            esp_get_feed_data(false, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

            afe_handle->feed(afe_data, i2s_buff);
        }
        vTaskDelay(10 / portTICK_RATE_MS);
    }

    if (i2s_buff)
    {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = static_cast<esp_afe_sr_data_t *>(arg);
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *buff = static_cast<int16_t *>(malloc(afe_chunksize * sizeof(int16_t)));
    assert(buff);
    printf("------------detect start------------\n");
    // int volume = 0;
    // esp_audio_set_play_vol(70);
    // esp_audio_get_play_vol(&volume);
    // ESP_LOGE(TAG, "Volume : %d", volume);

    int dataSize = (afe_chunksize * ONE_SECOND_FRAMES * RECORD_TIME_SECONDS);
    int16_t *audioBuffer = static_cast<int16_t *>(malloc(dataSize * sizeof(int16_t)));
    char *transcribeResponseBuf = static_cast<char *>(malloc(MAX_RESPONSE_BUFFER));
    char *completionResponseBuf = static_cast<char *>(malloc(MAX_RESPONSE_BUFFER));
    if (!audioBuffer || !transcribeResponseBuf || !completionResponseBuf)
    {
        ESP_LOGE(TAG, "memory not allocated");
        task_flag = 0;
    }

    while (1)
    {
        // afe_fetch_result_t* res = afe_handle->fetch(afe_data);
        // if (!res || res->ret_value == ESP_FAIL) {
        //     printf("fetch error!\n");
        //     break;
        // }

        // if (res->wakeup_state == WAKENET_DETECTED) {
        //     printf("wakeword detected\n");
        //     printf("model index:%d, word index:%d\n", res->wakenet_model_index, res->wake_word_index);
        //     printf("-----------LISTENING-----------\n");
        // }
        // esp_audio_play(res->data, res->data_size, 1000/ portTICK_RATE_MS);
        task_flag = 0;
        while (!recordStatus)
        {
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        task_flag = 1;
        uint32_t audioSize = record(audioBuffer, RECORD_TIME_SECONDS);

        task_flag = 0;

        openAI->transcribe_wav_file(audioBuffer, audioSize, transcribeResponseBuf);
        ESP_LOGI(TAG, "%s", transcribeResponseBuf);
        char *transcribedText = openAI->processTranscribeResponse(transcribeResponseBuf); // deallocation needed

        if (transcribedText)
        {

            char *completionRequestJson = json->createJson(std::make_pair("model", "gpt-3.5-turbo-instruct"),
                                                           std::make_pair("prompt", transcribedText),
                                                           std::make_pair("max_tokens", 32),
                                                           std::make_pair("temperature", 0.5)); // deallocation needed

            ESP_LOGI(TAG, "%s\n", completionRequestJson);
            openAI->text_completion(completionRequestJson, completionResponseBuf);

            char *completionText = openAI->process_response(completionResponseBuf); // deallocation needed
            if (completionText)
            {
                char *speechRequestJson = json->createJson(std::make_pair("model", "tts-1"),
                                                           std::make_pair("input", completionText),
                                                           std::make_pair("voice", "alloy"),
                                                           std::make_pair("response_data", "mp3")); // deallocation needed

                char *promptAndResponse = json->createJson(std::make_pair("prompt", transcribedText), std::make_pair("response", completionText));

                telemetrySend->send_telemetry_data(promptAndResponse); // Post prompt and respone telemetry to the server

                free(promptAndResponse);
                ESP_LOGI(TAG, "%s\n", speechRequestJson);

                uint32_t len = openAI->create_speech(speechRequestJson, speechBuffer);
                // ESP_LOGE(TAG, "free block %d ",heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
                AudioFrame audio = {.data = speechBuffer, .len = len};
                play(audio);

                free(speechRequestJson);
            }
            free(completionRequestJson);
            free(completionText);
        }
        free(transcribedText);
        task_flag = 1;
    }

    if (buff)
    {
        free(buff);
        buff = NULL;
    }
    vTaskDelete(NULL);
}

extern "C" void app_main()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));

    // Remember to set SofyAp for Wifi provision from menuConfig->Example configuration->provisoning transport -> set to Soft AP
    // Other wise the controller will not be able to detect the Wifi and it will reboot frequently and cause a problem
    wifiProvisioning->softap();
    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    printf("Wifi Connected Successfully\n");
    // Create JSON data
    
    char *jsonData = json->createJson(std::make_pair("Wifi Status", 1),
                                      std::make_pair("Ready to connect with the OpenAI", 0.5));

    telemetrySend->send_telemetry_data(jsonData); // Post Health telemetry to the server

    free(jsonData);
    // Free the allocated memory for jsonData
    ESP_LOGE(TAG, "esp_get_free_heap_size - %d", esp_get_free_heap_size());
    ESP_LOGE(TAG, "esp_get_minimum_free_heap_size: %u", esp_get_minimum_free_heap_size());
    speechBuffer = static_cast<char *>(malloc(300 * 1024));
    ESP_LOGE(TAG, "esp_get_free_heap_size - %d", esp_get_free_heap_size());
    ESP_LOGE(TAG, "esp_get_minimum_free_heap_size: %u", esp_get_minimum_free_heap_size());

    // ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));

    srmodel_list_t *models = esp_srmodel_init("model");
    char *wn_name = NULL;
    char *wn_name_2 = NULL;

    if (models != NULL)
    {
        for (int i = 0; i < models->num; i++)
        {
            if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL)
            {
                if (wn_name == NULL)
                {
                    wn_name = models->model_name[i];
                    printf("The first wakenet model: %s\n", wn_name);
                }
                else if (wn_name_2 == NULL)
                {
                    wn_name_2 = models->model_name[i];
                    printf("The second wakenet model: %s\n", wn_name_2);
                }
            }
        }
    }
    else
    {
        printf("Please enable wakenet model and select wake word by menuconfig!\n");
        return;
    }

    afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;
    afe_config_t afe_config = AFE_CONFIG_MY_DEFAULT();
    afe_config.memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
    afe_config.wakenet_init = true;
    afe_config.wakenet_model_name = wn_name;
    afe_config.wakenet_model_name_2 = wn_name_2;
    afe_config.voice_communication_init = false;

#if defined CONFIG_ESP32_S3_BOX_BOARD || defined CONFIG_ESP32_S3_EYE_BOARD
    afe_config.aec_init = false;
#if defined CONFIG_ESP32_S3_EYE_BOARD
    afe_config.pcm_config.total_ch_num = 2;
    afe_config.pcm_config.mic_num = 1;
    afe_config.pcm_config.ref_num = 1;
#endif
#endif
    afe_data = afe_handle->create_from_config(&afe_config);

    task_flag = 1;
    // esp_log_level_set("*", ESP_LOG_WARN);
    ESP_LOGI(TAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    board_handle->audio_hal->audio_codec_set_volume(100);
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    // Initialize Button peripheral
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    audio_board_key_init(set);
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    input_cfg.handle = set;
    periph_service_handle_t input_ser = input_key_service_create(&input_cfg);
    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, NULL);

    AudioFrame audio = {.data = audio_data, .len = sizeof(audio_data)};
    play(audio);

    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void *)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void *)afe_data, 5, NULL, 1);

    // // You can call afe_handle->destroy to destroy AFE.
    // task_flag = 0;

    // printf("destroy\n");
    // afe_handle->destroy(afe_data);
    // afe_data = NULL;
    // printf("successful\n");
}

uint32_t record(int16_t *buff, uint16_t seconds)
{
    printf("\nRecording.......!\n");
    size_t totalDataSize = 0;
    for (size_t i = 0; i < ((ONE_SECOND_FRAMES * seconds) - 1); i++)
    {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL)
        {
            printf("fetch error!\n");
            break;
        }
        if (recordStatus == false)
        {
            break;
        }
        memcpy(buff + totalDataSize, res->data, res->data_size * sizeof(int16_t));
        totalDataSize += res->data_size / 2;
        // esp_audio_play(res->data, res->data_size,  1000 / portTICK_RATE_MS);
    }
    totalDataSize *= 2;
    printf("\nStopped!\n");
    recordStatus = 0;
    return totalDataSize;
}

static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK)
    {
        switch ((int)evt->data)
        {
        case INPUT_KEY_USER_ID_MODE:
            ESP_LOGI(TAG, "[ * ] [Set] input key event, change mode ...");
            break;
        case INPUT_KEY_USER_ID_REC:
            ESP_LOGI(TAG, "[ * ] [Rec] input key event, start/stop recording ...");
            recordStatus = !recordStatus;
            break;
        }
    }
    else if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE || evt->type == INPUT_KEY_SERVICE_ACTION_PRESS_RELEASE)
    {
        switch ((int)evt->data)
        {
        case INPUT_KEY_USER_ID_REC:
            ESP_LOGI(TAG, "[ * ] [Rec] key released, start/stop key released ...");
            break;
        }
    }

    return ESP_OK;
}