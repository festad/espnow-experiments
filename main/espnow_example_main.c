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
#include "mac.h"

// DISCLAIMER
// This code is all work of the ZEUS WPI research group and Jasper Devreker,
// my contribution level is practically zero, 
// I just tried to adapt it to work on an esp32c6. 
// This code mostly looks like their first commit.
// The first article of the series they wrote on the subject is here:
// https://zeus.ugent.be/blog/23-24/open-source-esp32-wifi-mac/
// Their code is here:
// https://github.com/esp32-open-mac/esp32-open-mac/


// extern uint8_t beacon_raw[];
// extern int frequencies[];
// extern int n_frequencies;


static const char *TAG = "espnow_example";

// void wifi_hw_start(int disable_power_management);

// hardware_mac_args open_hw_args = {
//     ._rx_callback = open_mac_rx_callback,
//     ._tx_func_callback = open_mac_tx_func_callback
// };

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

    // args: osi_thread_run, name, stack size, &start_arg, priority, &thread->thread_handle, core
    xTaskCreatePinnedToCore(&wifi_hardware_task, "wifi_hardware_task", 4096, NULL /* &open_hw_args*/ , 5, NULL, 0);
    xTaskCreatePinnedToCore(&reading_task,           "open_mac",           4096, NULL,          3, NULL, 1);

}
