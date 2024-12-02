#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "string.h"

#include "patch.h"
#include "peripherals.h"
#include "hardware.h"





static const char *TAG = "espnow_example";

void wifi_hw_start(int disable_power_management);

void tx_task(void *pvParameter) {
	ESP_LOGW(TAG, "wifi_hw_start");
	wifi_hw_start(1);
	vTaskDelay(200 / portTICK_PERIOD_MS);

    ESP_LOGW(TAG, "switch channel to 0x96c");
    switch_channel(0x96c, 0);
    ESP_LOGW(TAG, "done switch channel to 0x96c");
    vTaskDelay(200 / portTICK_PERIOD_MS);

	for (int i = 0; i < 2; i++) {
		uint8_t mac[6] = {0};
		if (esp_wifi_get_mac(i, mac) == ESP_OK) {
			ESP_LOGW(TAG, "MAC %d = %02x:%02x:%02x:%02x:%02x:%02x", i, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		}
	}

	for (int i = 0; i < 3; i++) {
		ESP_LOGW(TAG, "going to transmit in %d", 3 - i);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	
	ESP_LOGW(TAG, "transmitting now!");
	for (int i = 0; i < 10; i++) {
		ESP_LOGW(TAG, "transmit iter %d", i);
		transmit_one(i);
        // send_sample_packets(true, false, false, false);
		ESP_LOGW(TAG, "still alive");
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}


void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_LOGW(TAG, "calling esp_wifi_init");
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_LOGW(TAG, "done esp_wifi_init");

    xTaskCreate(&tx_task, "tx_task", 4096, NULL, 5, NULL);

}
