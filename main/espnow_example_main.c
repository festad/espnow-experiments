/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
   This example shows how to use ESPNOW.
   Prepare two device, one for sending ESPNOW data and another for receiving
   ESPNOW data.
*/


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

// for esp32 -> 0x3ff73d20
// #define WIFI_DMA_OUTLINK 0x600a4d6c
#define MAC_TX_PLCP0_BASE 0x600a4d6c
#define MAC_TX_PLCP0_OS (-4)

#define WIFI_TX_CONFIG_BASE 0x600a4d68 // 0x120000a0
#define WIFI_TX_CONFIG_OS (-4)

#define HAL_TX_PTI_1_BASE 0x600a4d68
#define HAL_TX_PTI_1_OS (-4)

#define HAL_TX_PTI_2_BASE 0x600a5490
#define HAL_TX_PTI_2_OS (-0x1d)

#define MAC_TX_PLCP1_BASE 0x600a5488
#define MAC_TX_PLCP1_OS (-0x1d)

#define MAC_TX_PLCP2_BASE 0x600a54bc
#define MAC_TX_PLCP2_OS (-0x1d)

#define MAC_TX_HT_SIG_BASE 0x600a54b0
#define MAC_TX_HT_SIG_OS (-0x1d)

#define MAC_TX_HT_SIG_BASE_2 0x600a5494
#define MAC_TX_HT_SIG_OS_2 (-0x1d)

#define MAC_TX_HE_SIG_BASE 0x600a54a0
#define MAC_TX_HE_SIG_OS (-0x1d)

#define MAC_TX_DURATION_BASE 0x600a54c0
#define MAC_TX_DURATION_OS (-0x1d)




static const char *TAG = "espnow_example";

void wifi_hw_start(int disable_power_management);

uint8_t beacon_raw[] = {0x80, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xba, 0xde, 0xaf, 0xfe, 0x00, 0x00,
	0xba, 0xde, 0xaf, 0xfe, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x64, 0x00, 0x31, 0x04, 0x00, 0x10, 0x45, 0x53, 0x50, 0x20, 0x62, 0x65, 0x61, 0x63, 0x6f, 0x6e, 0x20, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, 0x03, 0x01, 0x01, 0x05, 0x04, 0x01, 0x02, 0x00, 0x00,
	0xef, 0xbe, 0xad, 0xde // last 4 bytes are a place holder FCS, because it is calculated by the hardware itself
};


inline void write_register(uint32_t address, uint32_t value) {
	*((volatile uint32_t*) address) = value;
}

inline uint32_t read_register(uint32_t address) {
	return *((volatile uint32_t*) address);
}


void transmit_one(uint8_t index) {
	uint32_t buffer_len = sizeof(beacon_raw); // this includes the FCS
	uint32_t size_len = buffer_len + 32;
	// change the ssid, so that we're sure we're transmitting different packets
	beacon_raw[38] = 'a' + (index % 26);
	// owner 1, eof 1, unknown 6, lenght 12, size 12
	uint32_t dma_item_first = ((1 << 31) | (1 << 30) | (buffer_len << 12) | size_len);
	uint32_t dma_item[3] = {dma_item_first, ((uint32_t) beacon_raw), 0};
	write_register(WIFI_TX_CONFIG_BASE, read_register(WIFI_TX_CONFIG_BASE) | 0xa);
	write_register(MAC_TX_PLCP0_BASE,
		(((uint32_t)dma_item) & 0xfffff) |
		(0x00600000));
	write_register(MAC_TX_PLCP1_BASE, 0x02997000);
	write_register(MAC_TX_PLCP2_BASE, 0x00000244);
	write_register(MAC_TX_DURATION_BASE, 0);
	
	write_register(WIFI_TX_CONFIG_BASE, read_register(WIFI_TX_CONFIG_BASE) | 0x02000000);
	write_register(WIFI_TX_CONFIG_BASE, read_register(WIFI_TX_CONFIG_BASE) | 0x00003000);
    // write 0x120013bf
	
	// TRANSMIT!
	write_register(WIFI_DMA_OUTLINK, read_register(WIFI_DMA_OUTLINK) | 0xc0000000);
	ESP_LOGW(TAG, "packet should have been sent");
}


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
		// transmit_one(i);
        send_sample_packets(true, false, false, false);
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
