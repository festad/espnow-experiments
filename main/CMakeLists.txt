idf_component_register(SRCS "espnow_example_main.c" "patch.c" "peripherals.c" "hardware.c"
                    INCLUDE_DIRS "."
		    PRIV_REQUIRES nvs_flash esp_event esp_netif esp_wifi
		    REQUIRES spi_flash )
