#pragma once
#include "stdint.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_vad.h"

#define AFE_CONFIG_MY_DEFAULT()                                                                                         \
    {                                                                                                                   \
        .aec_init = true,                                                                                               \
        .se_init = true,                                                                                                \
        .vad_init = true,                                                                                               \
        .wakenet_init = true,                                                                                           \
        .voice_communication_init = false,                                                                              \
        .voice_communication_agc_init = false,                                                                          \
        .voice_communication_agc_gain = 15,                                                                             \
        .vad_mode = VAD_MODE_3,                                                                                         \
        .wakenet_model_name = NULL,                                                                                     \
        .wakenet_model_name_2 = NULL,                                                                                   \
        .wakenet_mode = DET_MODE_2CH_90,                                                                                \
        .afe_mode = SR_MODE_LOW_COST,                                                                                   \
        .afe_perferred_core = 0,                                                                                        \
        .afe_perferred_priority = 5,                                                                                    \
        .afe_ringbuf_size = 50,                                                                                         \
        .memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM,                                                               \
        .afe_linear_gain = 1.0,                                                                                         \
        .agc_mode = AFE_MN_PEAK_AGC_MODE_2,                                                                             \
        .pcm_config = {                                                                                                 \
            .total_ch_num = 3,                                                                                          \
            .mic_num = 2,                                                                                               \
            .ref_num = 1,                                                                                               \
            .sample_rate = 16000,                                                                                       \
        },                                                                                                              \
        .debug_init = false, .debug_hook = {{AFE_DEBUG_HOOK_MASE_TASK_IN, NULL}, {AFE_DEBUG_HOOK_FETCH_TASK_IN, NULL}}, \
    }