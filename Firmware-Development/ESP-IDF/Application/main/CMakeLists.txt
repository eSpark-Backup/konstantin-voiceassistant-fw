set(SOURCES main.cpp)
idf_component_register(SRCS "main.cpp"
                        INCLUDE_DIRS "."
                        )
# spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)

