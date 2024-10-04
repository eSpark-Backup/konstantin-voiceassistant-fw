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
