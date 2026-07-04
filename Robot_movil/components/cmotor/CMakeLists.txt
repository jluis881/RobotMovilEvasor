idf_component_register(SRCS "cmotor.c"
                    INCLUDE_DIRS "include"
                    REQUIRES ble ssonido driver esp_timer nvs_flash global)
