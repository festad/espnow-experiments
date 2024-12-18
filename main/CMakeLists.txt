idf_component_register(SRCS "hwinit.c" "espnow_example_main.c" "patch.c" "peripherals.c" "hardware.c" "mac.c"
                    INCLUDE_DIRS "."
		    PRIV_REQUIRES nvs_flash esp_event esp_netif esp_wifi esp_timer
		    REQUIRES spi_flash )
