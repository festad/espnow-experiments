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


void send_sample_packets(bool patchedtx, bool disable_lp_feature, bool posthmac, bool coexrequest)
{
    switch_channel(0x96c, 0);
    ESP_LOGI(TAG, "Channel changed to 0x96c");

    // uint32_t base_address = 0x4081fc28;
    // struct Packet* packet = (struct Packet*)base_address;
    // initialize_packet(packet, base_address);
    // uint32_t deadbeef_address = (uint32_t)&(packet->deadbeef);

    // struct SubStruct* substruct = (struct SubStruct*)0x4081CA14;
    // initialize_substruct(substruct, deadbeef_address);

    for(int i=0; i<10; i++)
    {
        ESP_LOGI(TAG, "Sending packet %d", i);
        // ESP_LOGI(TAG, "Calling patched_ieee80211_post_hmac_tx");
        // patched_ieee80211_post_hmac_tx(base_address);
        // ESP_LOGI(TAG, "Calling patched_lmacTxFrame");
        // patched_lmacTxFrame(base_address, 0);
        // ESP_LOGI(TAG, "Finished calling patched_lmacTxFrame");
        // ppProcTxDone();

        struct Packet* packet = (struct Packet*)malloc(sizeof(struct Packet));
        if (packet == NULL) 
        {
            ESP_LOGE(TAG, "Failed to allocate memory for packet");
            break;
        }
        uint32_t base_address = (uint32_t)packet;
        initialize_packet(packet, base_address);
        uint32_t deadbeef_address = (uint32_t)&(packet->deadbeef);

        struct SubStruct* substruct = (struct SubStruct*)malloc(sizeof(struct SubStruct));
        if(substruct == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for substruct");
            free(packet);
            break;
        }

        initialize_substruct(substruct, deadbeef_address);

        if(posthmac)
        {
            ESP_LOGI(TAG, "Calling patched_ieee80211_post_hmac_tx");
            int ret = patched_ieee80211_post_hmac_tx((uint32_t)packet);
            if(ret != 0)
            {
                ESP_LOGE(TAG, "Failed to post packet");
                free(packet);
                free(substruct);

            }
        }

        if(coexrequest)
        {
            ESP_LOGI(TAG, "Calling pp_coex_tx_request");
            pp_coex_tx_request((uint32_t)packet);
        }

        if(patchedtx)
        {
            ESP_LOGI(TAG, "Calling patched_lmacTxFrame");
            patched_lmacTxFrame((uint32_t)packet, 0);
        }

        if(disable_lp_feature && i==0)
        {
            ESP_LOGI(TAG, "Calling pm_disconnected_stop");
            pm_disconnected_stop();
        }

        // ESP_LOGI(TAG, "Calling ppProcTxDone");
        // ppProcTxDone();

        // ESP_LOGI(TAG, "Freed substruct");
        // free(substruct);
        // ESP_LOGI(TAG, "Freed packet");
        // free(packet);

        ESP_LOGI(TAG, "Finished sending packet %d", i);

        vTaskDelay(500/portTICK_PERIOD_MS);
    }    
}

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
	for (int i = 0; i < 2; i++) {
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
