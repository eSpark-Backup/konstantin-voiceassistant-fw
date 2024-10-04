idf_component_register(SRCS "TelemetrySend.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES esp_http_client MyJson
                        )
