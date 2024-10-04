#include <esp_http_client.h>
#include "Json.hpp"

class TelemetrySend
{
  public:
    void send_telemetry_data(const char *jsonData);
};
