#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "string.h"

#include "patch.h"
#include "peripherals.h"
#include "hardware.h"

// DISCLAIMER
// This code is all work of the ZEUS WPI research group and Jasper Devreker,
// my contribution level is practically zero, 
// I just tried to adapt it to work on an esp32c6. 
// This code mostly looks like their first commit.
// The first article of the series they wrote on the subject is here:
// https://zeus.ugent.be/blog/23-24/open-source-esp32-wifi-mac/
// Their code is here:
// https://github.com/esp32-open-mac/esp32-open-mac/


extern uint8_t beacon_raw[];
extern int frequencies[];
extern int n_frequencies;


static const char *TAG = "espnow_example";

void wifi_hw_start(int disable_power_management);

void tx_task(void *pvParameter) {

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.static_rx_buf_num = 4; // we won't use these buffers, so reduce the amount from default 10, so we don't waste as much memory
	// Disable AMPDU and AMSDU for now, we don't support this (yet)
	cfg.ampdu_rx_enable = false;
	cfg.ampdu_tx_enable = false;
	cfg.amsdu_tx_enable = false;
	cfg.nvs_enable = false;	
    ESP_LOGW(TAG, "calling esp_wifi_init");
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_LOGW(TAG, "done esp_wifi_init");

	ESP_LOGW(TAG, "calling esp_wifi_set_mode");
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_LOGW(TAG, "done esp_wifi_set_mode");

	ESP_LOGW(TAG, "calling esp_wifi_start");
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGW(TAG, "done esp_wifi_start");

	ESP_LOGW(TAG, "calling esp_wifi_set_promiscuous");
	esp_wifi_set_promiscuous(true);
	ESP_LOGW(TAG, "done esp_wifi_set_promiscuous");	

	ESP_LOGW(TAG, "Killing proprietary wifi task (ppTask)");
	pp_post(0xf, 0);	

	ESP_LOGW(TAG, "setting up buffers");
	setup_tx_buffers();

	// ESP_LOGW(TAG, "wifi_hw_start");
	// wifi_hw_start(1);
	// vTaskDelay(200 / portTICK_PERIOD_MS);



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
	for (int i = 0; ; i++) {
		ESP_LOGW(TAG, "transmit iter %d", i);
		int next_index = (i+1) % n_frequencies; // +1 is to start from the 0th element intead of the (n-1)th
		int next_frequency = frequencies[next_index];
		// next_frequency = 0x96c;
		ESP_LOGI(TAG, "Switching to frequency 0x%08x", next_frequency);
		// Set the frequency
		switch_channel(next_frequency, 0);		
		vTaskDelay(200 / portTICK_PERIOD_MS);
        // Increase the length of the payload by i, 
        *(uint32_t*)(beacon_raw) = *(uint32_t*)(beacon_raw) + i;
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

	// ESP_LOGW(TAG, "calling esp_netif_init");
	// ESP_ERROR_CHECK(esp_netif_init());
	// ESP_LOGW(TAG, "done esp_netif_init");

    xTaskCreate(&tx_task, "tx_task", 4096, NULL, 5, NULL);

}
