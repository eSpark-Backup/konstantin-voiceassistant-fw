idf_component_register(SRCS "mp3_player.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES audio_pipeline audio_stream esp_peripherals audio_hal)