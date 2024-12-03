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

// DISCLAIMER
// This code is all work of the ZEUS WPI research group and Jasper Devreker,
// my contribution level is practically zero, 
// I just tried to adapt it to work on an esp32c6. 
// This code mostly looks like their first commit.
// The first article of the series they wrote on the subject is here:
// https://zeus.ugent.be/blog/23-24/open-source-esp32-wifi-mac/
// Their code is here:
// https://github.com/esp32-open-mac/esp32-open-mac/

inline void write_register(uint32_t address, uint32_t value) {
	*((volatile uint32_t*) address) = value;
}

inline uint32_t read_register(uint32_t address) {
	return *((volatile uint32_t*) address);
}

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

typedef struct __attribute__((packed)) dma_list_item {
	uint16_t _unknown_2 : 12;
	uint16_t length : 12;
	uint8_t _unknown_1 : 4;
	uint8_t has_data : 1;
	uint8_t owner : 1; // What does this mean?
	void* packet;
	struct dma_list_item* next;
} dma_list_item;

uint8_t beacon_raw[] = {
	0x4d, 0x00, 0x00, 0x00, // Length
	0x00, 0x00, 0x00, 0x00, // Empty word
	0xd0, 0x00, 0x00, 0x00, // Start of frame
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x40, 0x4c, 0xca, 0x51, 0x57, 0xd8,
	0xba, 0xde, 0xaf, 0xfe, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x64, 0x00, 0x31, 0x04, 0x00, 0x10, 0x45, 0x53, 0x50, 0x20, 0x62, 0x65, 0x61, 0x63, 0x6f, 0x6e, 0x20, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, 0x03, 0x01, 0x01, 0x05, 0x04, 0x01, 0x02, 0x00, 0x00,
	0xef, 0xbe, 0xad, 0xde // last 4 bytes are a place holder FCS, because it is calculated by the hardware itself
};

dma_list_item* tx_item = NULL;

void setup_tx_buffers() {
	tx_item = calloc(1, sizeof(dma_list_item));
	// tx_buffer = calloc(1, 1600);
}


void transmit_one(uint8_t index) {
	// uint32_t buffer_len = sizeof(beacon_raw) - 8; // this includes the FCS
	// uint32_t size_len = buffer_len + 32;
	// The length of the payload is in the first 4 bytes of the packet
	uint32_t payload_length = *((uint32_t*) beacon_raw);
	uint32_t packet_length = payload_length + 15;
	uint32_t len_m_7 = packet_length - 7;
	uint32_t len_m_15 = packet_length - 15; // = payload_length
	// change the ssid, so that we're sure we're transmitting different packets
	beacon_raw[38] = 'a' + (index % 26);
	// owner 1, eof 1, unknown 6, lenght 12, size 12
	// uint32_t dma_item_first = ((1 << 31) | (1 << 30) | (buffer_len << 12) | size_len);
	// Now I have to change this formula according to the rule of length - 7
	// I want 
	// 1100 0000 0001 0101 0100 0000 0000 0000
	// uint32_t dma_item_first = ((1 << 31) | (1 << 30) | (len_m_7 << 14));
	// uint32_t dma_item_first = 0xc0154000;
	// uint32_t dma_item[3] = {dma_item_first, ((uint32_t) beacon_raw), 0};
	tx_item->owner = 1;
	tx_item->has_data = 1;
	tx_item->length = len_m_7;
	tx_item->packet = (uint32_t)beacon_raw;
	tx_item->next = NULL;

	write_register(WIFI_TX_CONFIG_BASE, read_register(WIFI_TX_CONFIG_BASE) | 0xa);
	write_register(MAC_TX_PLCP0_BASE, ((uint32_t)tx_item) & 0xfffff | 0x00600000);
	write_register(MAC_TX_PLCP1_BASE, len_m_15);
	write_register(MAC_TX_PLCP2_BASE, 0);
	write_register(MAC_TX_DURATION_BASE, 0);
	
	write_register(WIFI_TX_CONFIG_BASE, read_register(WIFI_TX_CONFIG_BASE) | 0x02000000);
	write_register(WIFI_TX_CONFIG_BASE, read_register(WIFI_TX_CONFIG_BASE) | 0x00000300);
    // write 0x120013bf
	
	// TRANSMIT!
	write_register(MAC_TX_PLCP0_BASE, read_register(MAC_TX_PLCP0_BASE) | 0xc0000000);
	ESP_LOGW(TAG, "packet should have been sent");
}
