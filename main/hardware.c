#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "string.h"

#include "hardware.h"

static const char* TAG = "hardware.c";


// for esp32 -> 0x3ff73d20
// #define WIFI_DMA_OUTLINK _MMIO_ADDR(0x600a4d6c)
#define MAC_TX_PLCP0_BASE _MMIO_ADDR(0x600a4d6c)
#define MAC_TX_PLCP0_OS (-4)

#define WIFI_TX_CONFIG_BASE _MMIO_ADDR(0x600a4d68) // 0x120000a0
#define WIFI_TX_CONFIG_OS (-4)

#define HAL_TX_PTI_1_BASE _MMIO_ADDR(0x600a4d68)
#define HAL_TX_PTI_1_OS (-4)

#define HAL_TX_PTI_2_BASE _MMIO_ADDR(0x600a5490)
#define HAL_TX_PTI_2_OS (-0x1d)

#define MAC_TX_PLCP1_BASE _MMIO_ADDR(0x600a5488)
#define MAC_TX_PLCP1_OS (-0x1d)

#define MAC_TX_PLCP2_BASE _MMIO_ADDR(0x600a54bc)
#define MAC_TX_PLCP2_OS (-0x1d)

#define MAC_TX_HT_SIG_BASE _MMIO_ADDR(0x600a54b0)
#define MAC_TX_HT_SIG_OS (-0x1d)

#define MAC_TX_HT_SIG_BASE_2 _MMIO_ADDR(0x600a5494)
#define MAC_TX_HT_SIG_OS_2 (-0x1d)

#define MAC_TX_HE_SIG_BASE _MMIO_ADDR(0x600a54a0)
#define MAC_TX_HE_SIG_OS (-0x1d)

#define MAC_TX_DURATION_BASE _MMIO_ADDR(0x600a54c0)
#define MAC_TX_DURATION_OS (-0x1d)

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
	write_register(MAC_TX_PLCP0_BASE, read_register(MAC_TX_PLCP0_BASE) | 0xc0000000);
	ESP_LOGW(TAG, "packet should have been sent");
}
