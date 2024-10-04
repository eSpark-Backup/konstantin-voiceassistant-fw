idf_component_register(SRCS "openapi_http.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES esp_http_client spiffs json esp-tls )