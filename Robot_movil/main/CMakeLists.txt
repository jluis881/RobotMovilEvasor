idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ble cmotor rgbled driver esp_timer nvs_flash led_strip)
