#pragma once
#include <esp_http_client.h>
#include <stdarg.h>

#define FILE_PATH "/spiffs/audiow.wav"
#define API_KEY "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"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_RESPONSE_BUFFER 1024

typedef struct
{
    char riff[4];        // "RIFF"
    int fileSize;        // Total file size - 8 bytes
    char wave[4];        // "WAVE"
    char fmt[4];         // "fmt "
    int fmtSize;         // Size of the fmt chunk
    short audioFormat;   // Audio format (1 for PCM)
    short numChannels;   // Number of channels (1 for mono, 2 for stereo)
    int sampleRate;      // Sample rate
    int byteRate;        // Byte rate (SampleRate * NumChannels * BitsPerSample / 8)
    short blockAlign;    // Block align (NumChannels * BitsPerSample / 8)
    short bitsPerSample; // Bits per sample
    char data[4];        // "data"
    int dataSize;        // Size of the data chunk
} WavHeader;

class OpenAI
{
public:
    void spiffs_init();     
    static const char *getFileName(const char *filePath);                                                    // n
    void send_post_request(const char *api_key, const char *file_path);           // n
    void fillWavHeader(WavHeader *header, int dataSize);                          // n
    void transcribe_wav_file(int16_t *stream, uint32_t size, char *received_buf); // y-1
    void text_completion(char *transmit_buf, char *received_buf);                 // y-1
    char *createJson(const char *key, const char *value, ...);
    char *process_response(const char *response);                   // y-1
    uint32_t create_speech(char *transmit_buf, char *received_buf); // y-1
    char *processTranscribeResponse(char *response);                // y-1
};
