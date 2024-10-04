set(SOURCES main.cpp)
idf_component_register(SRCS "main.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES 
                            # hardware_driver
                        )
# spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)

