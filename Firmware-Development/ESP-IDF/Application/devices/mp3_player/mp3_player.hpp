#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <freertos/queue.h>

typedef struct 
{
    const void* data;
    uint32_t len;
} AudioFrame;

void play(AudioFrame audio);
void play_http_stream(const char* arg_stream_endpoint);

