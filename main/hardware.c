#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "string.h"

#include "soc/soc.h"
#include "soc/periph_defs.h"
#include "esp32c6/rom/ets_sys.h"

#include "hardware.h"
#include "patch.h"
#include "peripherals.h"
// #include "proprietary.h"
#include "mac.h"


#include <inttypes.h>

#define RX_BUFFER_AMOUNT 7
#define RX_RESOURCE_SEMAPHORE 15
#define MAX_RECEIVED_PACKETS 7 

#define TX_BUFFER_AMOUNT 10 
#define TX_RESOURCE_SEMAPHORE 10

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

#define WIFI_DMA_INT_STATUS _MMIO_ADDR(0x600a4c48)
#define WIFI_DMA_INT_CLR _MMIO_ADDR(0x600a4c4c)

#define WIFI_LAST_RX_DSCR _MMIO_ADDR(0x600a408c)
#define WIFI_NEXT_RX_DSCR _MMIO_ADDR(0x600a4088)
#define WIFI_BASE_RX_DSCR _MMIO_ADDR(0x600a4084)
#define WIFI_MAC_BITMASK _MMIO_ADDR(0x600a4080)
#define WIFI_MAC_INITMASK _MMIO_ADDR(0x600a407c)


typedef struct __attribute__((packed)) dma_list_item {
	uint16_t _unknown_2 : 14;
	uint16_t length : 12;
	uint8_t _unknown_1 : 4;
	uint8_t has_data : 1;
	uint8_t owner : 1; // What does this mean?
	void* packet;
	struct dma_list_item* next;
} dma_list_item;

typedef enum
{
	RX_ENTRY,
	TX_ENTRY
} hardware_queue_entry_type_t;

typedef struct
{
	uint32_t interrupt_received;
} rx_queue_entry_t;

typedef struct
{
	uint8_t* packet;
	uint32_t len;
} tx_queue_entry_t;

typedef struct
{
	hardware_queue_entry_type_t type;
	union 
	{
		rx_queue_entry_t rx;
		tx_queue_entry_t tx;
	} content;
} hardware_queue_entry_t;

SemaphoreHandle_t tx_queue_resources = NULL;
SemaphoreHandle_t rx_queue_resources = NULL;

QueueHandle_t hardware_event_queue = NULL;

dma_list_item* rx_chain_begin = NULL;
dma_list_item* rx_chain_last = NULL;

volatile int interrupt_count = 0;

dma_list_item *tx_item = NULL;
uint8_t *tx_buffer = NULL;

uint64_t last_transmit_timestamp = 0;
uint32_t seqnum = 0;

void setup_tx_buffers()
{
	tx_item = calloc(1, sizeof(dma_list_item));
	tx_buffer = calloc(1, 1600);
}

void log_dma_item(dma_list_item *item)
{
	ESP_LOGD("dma_item", "cur=%p owner=%d has_data=%d length=%d packet=%p next=%p", 
		item, item->owner, item->has_data, item->length, item->packet, item->next);
}



// uint8_t beacon_raw[] = {
// 	0x4d, 0x00, 0x00, 0x00, // Length
// 	0x00, 0x00, 0x00, 0x00, // Empty word
// 	0xd0, 0x00, 0x00, 0x00, // Start of frame
// 	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
// 	0x40, 0x4c, 0xca, 0x51, 0x57, 0xd8,
// 	0xba, 0xde, 0xaf, 0xfe, 0x00, 0x00,
// 	0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x64, 0x00, 0x31, 0x04, 0x00, 0x10, 0x45, 0x53, 0x50, 0x20, 0x62, 0x65, 0x61, 0x63, 0x6f, 0x6e, 0x20, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, 0x03, 0x01, 0x01, 0x05, 0x04, 0x01, 0x02, 0x00, 0x00,
// 	0xef, 0xbe, 0xad, 0xde // last 4 bytes are a place holder FCS, because it is calculated by the hardware itself
// };

extern uint8_t beacon_raw[];

uint8_t datarates[] = {0x01, 0x03, 0x05, 0x09, 0x0b, 0x0f, 0x11, 0x14, 0x17};
uint8_t n_datarates = 9;

extern int frequencies[];
extern int n_frequencies;

void set_datarate(uint8_t datarate, uint32_t length)
{
	// If the datarate has the form 0x01, 0x02, ..., 0x0f then
	// we are dealing with a simpler case where we just set
	// the address 0x600a5488 to 0x0000pabc where abc is the length
	// and p is the last four bits of the datarate parameter.
	// Then we set the next word as 0x00020000 and the next three 
	// to 0x0.
	// If the datarate has the form 0x11, 0x12, ..., 0x17 then
	// the case is slightly more complicated, we set
	// 0x600a5488 to 0x02000000, the next word to 0x00020000 as before,
	// the next one to 0x00111110 and the next one to 0x000abc0p
	// where abc is the length and p is the last four bits of the datarate
	// (so if the datarate is 0x11 then p is just 0b0001).

	if((datarate & 0x10) == 0) {
		// Set the first word to 0x0000pabc
		write_register(0x600a5488, ((datarate & 0xf ) << 12) | (length & 0xfff));
		// Set the second word to 0x00020000
		write_register(0x600a548c, 0x00020000);
		// Set the third word to 0x0
		write_register(0x600a5490, 0);
		// Set the fourth word to 0x0
		write_register(0x600a5494, 0);
	} else {
		// Set the first word to 0x02000000
		write_register(0x600a5488, 0x02000000);
		// Set the second word to 0x00020000
		write_register(0x600a548c, 0x00020000);
		// Set the third word to 0x00111110
		write_register(0x600a5490, 0x00111110);
		// Set the fourth word to 0x000abc0p
		write_register(0x600a5494, ((length & 0xfff) << 8) | (datarate & 0xf));
	}
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
	write_register(MAC_TX_PLCP0_BASE, (((uint32_t)tx_item) & 0xfffff) | 0x00600000);
	// write_register(MAC_TX_PLCP1_BASE, len_m_15);
	set_datarate(datarates[index % n_datarates], len_m_15);
	write_register(MAC_TX_PLCP2_BASE, 0);
	write_register(MAC_TX_DURATION_BASE, 0);
	
	write_register(WIFI_TX_CONFIG_BASE, read_register(WIFI_TX_CONFIG_BASE) | 0x02000000);
	write_register(WIFI_TX_CONFIG_BASE, read_register(WIFI_TX_CONFIG_BASE) | 0x00000300);
    // write 0x120013bf
	
	// TRANSMIT!
	write_register(MAC_TX_PLCP0_BASE, read_register(MAC_TX_PLCP0_BASE) | 0xc0000000);
	ESP_LOGW(TAG, "packet should have been sent");
}

void IRAM_ATTR wifi_interrupt_handler(void* args) 
{
	interrupt_count++;
	// Print the interrupt count
	ESP_LOGD(TAG, "Interrupt count: %d", interrupt_count);

	uint32_t cause = read_register(WIFI_DMA_INT_STATUS);
	// Print the cause of the interrupt
	ESP_LOGD(TAG, "Interrupt cause: %08x", (int)cause);
	if (cause == 0)
	{
		return;
	}
	write_register(WIFI_DMA_INT_CLR, cause);

	if (cause & 0x800)
	{
		ESP_LOGW(TAG, "panic watchdog()");
	}
	volatile bool tmp = false;
	if (xSemaphoreTakeFromISR(rx_queue_resources, &tmp))
	{
		hardware_queue_entry_t queue_entry;
		queue_entry.type = RX_ENTRY;
		queue_entry.content.rx.interrupt_received = cause;
		xQueueSendFromISR(hardware_event_queue, &queue_entry, NULL);
	}
}

// If I get to overwrite &s_intr_handlers+0x8 to point to wifi_interrupt_handler
// then wifi_interrupt_handler will be executed instead of wDev_ProcessFiq

// void setup_interrupt() 
// {
// 	intr_matrix_set(0, ETS_WIFI_MAC_INTR_SOURCE, ETS_WMAC_INUM);

// 	// // Wait for interrupt to be set, so we can replace it
// 	// while (_xt_interrupt_table[ETS_WMAC_INUM*portNUM_PROCESSORS + xPortGetCoreID()].handler == &xt_unhandled_interrupt)
// 	// {
// 	// 	vTaskDelay(100 / portTICK_PERIOD_MS);
// 	// 	ESP_LOGW(TAG, "Waiting for interrupt to become set");
// 	// }
// 	vTaskDelay(500*5 / portTICK_PERIOD_MS);

// 	// Replace the existing wDev_ProcessFiq interrupt
// 	xt_set_interrupt_handler(ETS_WMAC_INUM, wifi_interrupt_handler, NULL);
// 	xt_ints_on(1 << ETS_WMAC_INUM);
// }

void setup_interrupt()
{
	// Assign to &s_intr_handlers+0x8 the address of wifi_interrupt_handler
	*(uint32_t*)((char *)&s_intr_handlers + 0x8) = (uint32_t)wifi_interrupt_handler;
	// I know after debugging that the address containing
	// a pointer to wDev_ProcessFiq is 0x408188a8, so I will hardode it 
	// because I cannot import the symbol s_intr_handlers
	// *(uint32_t*)0x408188a8 = (uint32_t)wifi_interrupt_handler;
	// I actually managed to import s_intr_handlers by removing the
	// static keyword from the declaration in components/riscv/interrupt.c
}

void print_rx_chain(dma_list_item* item)
{
	// Debug print to display RX linked list
	int index = 0;
	ESP_LOGI("rx-chain", "base=%p next=%p last=%p", (dma_list_item*) read_register(WIFI_BASE_RX_DSCR), (dma_list_item*) read_register(WIFI_NEXT_RX_DSCR), (dma_list_item*) read_register(WIFI_LAST_RX_DSCR));
	while (item)
	{
		ESP_LOGI("rx-chain", "idx=%d cur=%p owner=%d has_data=%d length=%d packet=%p next=%p",
				index, item, item->owner, item->has_data, item->length, item->packet, item->next);
		item = item->next;
		index++;
	}
	ESP_LOGI("rx-chain", "base=%p next=%p last=%p",  (dma_list_item*) read_register(WIFI_BASE_RX_DSCR), (dma_list_item*) read_register(WIFI_NEXT_RX_DSCR), (dma_list_item*) read_register(WIFI_LAST_RX_DSCR));
}

void set_rx_base_address(dma_list_item *item)
{
	write_register(WIFI_BASE_RX_DSCR, (uint32_t) item);
}

void setup_rx_chain()
{
	ESP_LOGI(TAG, "Setting up RX chain");
	ESP_LOGI(TAG, "WIFI_MAC_INITMASK = 0x%08lx", read_register(WIFI_MAC_INITMASK));
	ESP_LOGI(TAG, "WIFI_MAC_BITMASK = 0x%08lx", read_register(WIFI_MAC_BITMASK));
	ESP_LOGI(TAG, "WIFI_BASE_RX_DSCR = 0x%08lx", read_register(WIFI_BASE_RX_DSCR));
	ESP_LOGI(TAG, "WIFI_NEXT_RX_DSCR = 0x%08lx", read_register(WIFI_NEXT_RX_DSCR));
	ESP_LOGI(TAG, "WIFI_LAST_RX_DSCR = 0x%08lx", read_register(WIFI_LAST_RX_DSCR));
	dma_list_item *prev = NULL;
	// for (int i = 0; i < RX_BUFFER_AMOUNT; i++)
	// {
	// 	dma_list_item *item = malloc(sizeof(dma_list_item));
	// 	item->has_data = 0;
	// 	item->owner = 1;
	// 	item->length = 1600;
	// 	item->_unknown_1 = 0;
	// 	item->_unknown_2 = 1700;

	// 	uint8_t *packet = malloc(1600);
	// 	item->packet = packet;
	// 	item->next = prev;
	// 	prev = item;
	// 	if (!rx_chain_last)
	// 	{
	// 		rx_chain_last = item;
	// 	}
	// }
	// First we create a contiguous area of RX_BUFFER_AMOUNT elements 
	// of size dma_list_item, then we create a contiguous area of 
	// RX_BUFFER_AMOUNT elements of size 1600, 
	// then we set the various fields and teh packet field of the dma_list_item to point to the
	// corresponding element of the second area

	// Printing the size of dma_list_item
	ESP_LOGI(TAG, "sizeof(dma_list_item) = %d", sizeof(dma_list_item));
	dma_list_item *items = malloc(RX_BUFFER_AMOUNT * sizeof(dma_list_item /*12*/) + 4 /*one word to store 0x6a8*/);
	uint8_t *packets = malloc(RX_BUFFER_AMOUNT * (1700+4+4));
	for (int i = 0; i < RX_BUFFER_AMOUNT; i++)
	{
		dma_list_item *item = items + i;
		item->has_data = 0;
		item->owner = 1;
		item->length = 1600;
		item->_unknown_1 = 0;
		item->_unknown_2 = 1700;
		uint8_t *packet = packets + i * (1700+4+4);
		item->packet = packet;
		item->next = prev;
		prev = item;
		if (!rx_chain_last)
		{
			rx_chain_last = item;
		}
	}
	// Get to the last word allocated for dma_list_item to write 0x6a8
	// by adding RX_BUFFER_AMOUNT * sizeof(dma_list_item) to items
	uint32_t *last_word = (uint32_t*)(items + RX_BUFFER_AMOUNT);
	*last_word = 0x6a8;
	set_rx_base_address(prev);
	rx_chain_begin = prev;
	ESP_LOGI(TAG, "rx_chain_begin = %p", rx_chain_begin);
	ESP_LOGI(TAG, "RX chain set up");
}

void update_rx_chain()
{
	ESP_LOGI(TAG, "Calling update_rx_chain");
	ESP_LOGI(TAG, "WIFI_MAC_INITMASK = 0x%08lx", read_register(WIFI_MAC_INITMASK));
	ESP_LOGI(TAG, "WIFI_MAC_BITMASK = 0x%08lx", read_register(WIFI_MAC_BITMASK));
	ESP_LOGI(TAG, "WIFI_BASE_RX_DSCR = 0x%08lx", read_register(WIFI_BASE_RX_DSCR));
	ESP_LOGI(TAG, "WIFI_NEXT_RX_DSCR = 0x%08lx", read_register(WIFI_NEXT_RX_DSCR));
	ESP_LOGI(TAG, "WIFI_LAST_RX_DSCR = 0x%08lx", read_register(WIFI_LAST_RX_DSCR));	
	write_register(WIFI_MAC_BITMASK, read_register(WIFI_MAC_BITMASK) | 0x1);
	// Wait for confirmation from hardware
	while (read_register(WIFI_MAC_BITMASK) & 0x1);
}

void handle_rx_messages(rx_callback rxcb)
{
	ESP_LOGI(TAG, "Calling handle_rx_messages");
	ESP_LOGI(TAG, "WIFI_MAC_INITMASK = 0x%08lx", read_register(WIFI_MAC_INITMASK));
	ESP_LOGI(TAG, "WIFI_MAC_BITMASK = 0x%08lx", read_register(WIFI_MAC_BITMASK));
	ESP_LOGI(TAG, "WIFI_BASE_RX_DSCR = 0x%08lx", read_register(WIFI_BASE_RX_DSCR));
	ESP_LOGI(TAG, "WIFI_NEXT_RX_DSCR = 0x%08lx", read_register(WIFI_NEXT_RX_DSCR));
	ESP_LOGI(TAG, "WIFI_LAST_RX_DSCR = 0x%08lx", read_register(WIFI_LAST_RX_DSCR));	

	ESP_LOGI(TAG, "printing the chain");
	print_rx_chain(rx_chain_begin);

	dma_list_item *current = rx_chain_begin;
	ESP_LOGI(TAG, "current ptr = %p", current);
	int received = 0;
	while(current)
	{
		dma_list_item *next = current->next;
		if (current->has_data)
		{
			received++;

			ESP_LOGI(TAG, "current->has_data = 1");
			wifi_promiscuous_pkt_t *packet = current->packet;

			ESP_LOGI(TAG, "Calling rxcb");
			rxcb(packet);

			rx_chain_begin = current->next;
			ESP_LOGI(TAG, "rx_chain_begin = current->next = %p", rx_chain_begin);
			current->next = NULL;
			current->has_data = 0;

			// Put the DMA buffer back in the linked list
			if(rx_chain_begin)
			{
				ESP_LOGI(TAG, "rx_chain_begin is not NULL");
				ESP_LOGI(TAG, "rx_chain_last = %p", rx_chain_last);
				rx_chain_last->next = current;
				ESP_LOGI(TAG, "rx_chain_last->next = current = %p", current);
				update_rx_chain();
				if(read_register(WIFI_NEXT_RX_DSCR) == 0x3ff00000)
				{
					ESP_LOGI(TAG, "read_register(WIFI_NEXT_RX_DSCR) == 0x3ff00000 -> 0x%08lx", read_register(WIFI_NEXT_RX_DSCR));
					dma_list_item *last_dscr = (dma_list_item *)read_register(WIFI_LAST_RX_DSCR);
					if (current == last_dscr)
					{
						ESP_LOGI(TAG, "current == last_dscr");
						rx_chain_last = current;
					}
					else
					{
						ESP_LOGI(TAG, "current != last_dscr");
						set_rx_base_address(last_dscr->next);
						rx_chain_last = current;
					}
				}
				else
				{
					ESP_LOGI(TAG, "read_register(WIFI_NEXT_RX_DSCR) != 0x3ff00000 -> 0x%08lx", read_register(WIFI_NEXT_RX_DSCR));
					rx_chain_last = current;
				}
			}
			else
			{
				ESP_LOGI(TAG, "rx_chain_begin is NULL");
				rx_chain_begin = current;
				set_rx_base_address(current);
				rx_chain_last = current;
			}
		}
		ESP_LOGI(TAG, "current->has_data = 0");
		current = next;
		if(received > MAX_RECEIVED_PACKETS)
		{
			goto out;
		}
	}
	out:
}

bool wifi_hardware_tx_func(uint8_t *packet, uint32_t len)
{
	if(!xSemaphoreTake(tx_queue_resources, 1))
	{
		ESP_LOGE(TAG, "TX semaphore full!");
		return false;
	}
	uint8_t *queue_copy = (uint8_t *)malloc(len);
	memcpy(queue_copy, packet, len);
	hardware_queue_entry_t queue_entry;
	queue_entry.type = TX_ENTRY;
	queue_entry.content.tx.len = len;
	queue_entry.content.tx.packet = queue_copy;
	xQueueSendToBack(hardware_event_queue, &queue_entry, 0);
	ESP_LOGI(TAG, "TX entry queued");
	return true;
}


void wifi_hardware_task(hardware_mac_args *pvParameter) 
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.static_rx_buf_num = 2; // we won't use these buffers, so reduce the amount from default 10, so we don't waste as much memory
	// Disable AMPDU and AMSDU for now, we don't support this (yet)
	cfg.ampdu_rx_enable = false;
	cfg.ampdu_tx_enable = false;
	cfg.amsdu_tx_enable = false;
	cfg.nvs_enable = false;	

	hardware_event_queue = xQueueCreate(RX_RESOURCE_SEMAPHORE+TX_RESOURCE_SEMAPHORE, sizeof(hardware_queue_entry_t));
	assert(hardware_event_queue);
	rx_queue_resources = xSemaphoreCreateCounting(RX_RESOURCE_SEMAPHORE, RX_RESOURCE_SEMAPHORE);
	assert(rx_queue_resources);
	tx_queue_resources = xSemaphoreCreateCounting(TX_RESOURCE_SEMAPHORE, TX_RESOURCE_SEMAPHORE);
	assert(tx_queue_resources);


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

	ESP_LOGW(TAG, "setting up interrupt");
	setup_interrupt();
	
	ESP_LOGW(TAG, "Killing proprietary wifi task (ppTask)");
	pp_post(0xf, 0);

	ESP_LOGW(TAG, "Setting up the rx chaing");
	setup_rx_chain();
	ESP_LOGW(TAG, "setting up buffers");
	setup_tx_buffers();

	pvParameter->_tx_func_callback(&wifi_hardware_tx_func);
	ESP_LOGW(TAG, "Starting to receive messages");

	// ESP_LOGW(TAG, "wifi_hw_start");
	// wifi_hw_start(1);
	// vTaskDelay(200 / portTICK_PERIOD_MS);



    ESP_LOGW(TAG, "switch channel to 0x96c");
    switch_channel(0x96c, 0);
    ESP_LOGW(TAG, "done switch channel to 0x96c");
    vTaskDelay(200 / portTICK_PERIOD_MS);
//
	//for (int i = 0; i < 2; i++) {
		//uint8_t mac[6] = {0};
		//if (esp_wifi_get_mac(i, mac) == ESP_OK) {
			//ESP_LOGW(TAG, "MAC %d = %02x:%02x:%02x:%02x:%02x:%02x", i, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		//}
	//}
//
	//for (int i = 0; i < 3; i++) {
		//ESP_LOGW(TAG, "going to transmit in %d", 3 - i);
		//vTaskDelay(1000 / portTICK_PERIOD_MS);
	//}
	//
	//ESP_LOGW(TAG, "transmitting now!");
	//for (int i = 0; ; i++) {
		//ESP_LOGW(TAG, "transmit iter %d", i);
		//int next_index = (i+1) % n_frequencies; // +1 is to start from the 0th element intead of the (n-1)th
		//int next_frequency = frequencies[next_index];
		//next_frequency = 0x96c;
		//ESP_LOGI(TAG, "Switching to frequency 0x%08x", next_frequency);
		//// Set the frequency
		//switch_channel(next_frequency, 0);		
		//vTaskDelay(200 / portTICK_PERIOD_MS);
        //// Increase the length of the payload by i, 
        //*(uint32_t*)(beacon_raw) = *(uint32_t*)(beacon_raw) + i;
		//transmit_one(i);
        //// send_sample_packets(true, false, false, false);
		//ESP_LOGW(TAG, "still alive");
		//vTaskDelay(500 / portTICK_PERIOD_MS);
	//}

	while(true)
	{
		hardware_queue_entry_t queue_entry;
		if(xQueueReceive(hardware_event_queue, &(queue_entry), 10))
		{
			if(queue_entry.type == RX_ENTRY)
			{
				uint32_t cause = queue_entry.content.rx.interrupt_received;
				ESP_LOGW(TAG, "interrupt received: 0x%08lx", cause);
				if(cause & 0x8000)
				{
					ESP_LOGW(TAG, "panic watchdog()");
				}
				if(cause & 0x1000024)
				{
					ESP_LOGW(TAG, "received message");
					handle_rx_messages(pvParameter->_rx_callback);
				}
				if(cause & 0x80)
				{
					ESP_LOGW(TAG, "lmacPostTxComplete");
				}
				if(cause & 0x80000)
				{
					ESP_LOGW(TAG, "lmacProcessAllTxTimeout");
				}
				if(cause & 0x100)
				{
					ESP_LOGW(TAG, "lmacProcessCollisions");
				}
				xSemaphoreGive(rx_queue_resources);
			}
			else if (queue_entry.type == TX_ENTRY)
			{
				transmit_one(0);
				// free
				xSemaphoreGive(tx_queue_resources);
			}
			else
			{
				ESP_LOGI(TAG, "unknown queue type");
			}
		}
	}
}
