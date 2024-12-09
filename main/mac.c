#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "hardware.h"
#include "80211.h"
#include "mac.h"

#include <string.h>

static char *TAG = "mac.c";
static tx_func *tx = NULL;
static uint8_t recv_mac_addr[6] = {0};
static QueueHandle_t reception_queue = NULL;

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

void open_mac_rx_callback(wifi_promiscuous_pkt_t *packet)
{
    ESP_LOGI(TAG, "Calling open_mac_rx_callback");
    mac80211_frame *p = (mac80211_frame *)packet->payload;

    if ((memcmp(recv_mac_addr, p->receiver_address, 6)) 
    && (memcmp(BROADCAST_MAC, p->receiver_address, 6)))
    {
        ESP_LOGI(TAG, "Discarding packet from "MACSTR" to "MACSTR"", MAC2STR(p->transmitter_address), MAC2STR(p->receiver_address));
        return;
    }
    ESP_LOGI(TAG, "Accepted: from "MACSTR" to "MACSTR" type=%d, subtype=%d from_ds=%d to_ds=%d",MAC2STR(p->transmitter_address), MAC2STR(p->receiver_address), p->frame_control.type, p->frame_control.sub_type, p->frame_control.from_ds, p->frame_control.to_ds);  

    if(!reception_queue) 
    {
        ESP_LOGI(TAG, "Received, but queue does not exist yet");
        return;
    }

    wifi_promiscuous_pkt_t *packet_queue_copy = malloc(packet->rx_ctrl.sig_len + 28 - 4);
    memcpy(packet_queue_copy, packet, packet->rx_ctrl.sig_len + 28-4);

    if (!(xQueueSendToBack(reception_queue, &packet_queue_copy, 0)))
    {
        ESP_LOGW(TAG, "MAC RX queue full!");
    }
    else
    {
        ESP_LOGI(TAG, "MAC RX queue entry added");
    }
}

void open_mac_tx_func_callback(tx_func *t)
{
    tx = t;
}

void mac_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting mac_task");

    reception_queue = xQueueCreate(10, sizeof(wifi_promiscuous_pkt_t *));
    assert(reception_queue);

    openmac_sta_state_t sta_state = IDLE;
    uint64_t last_transmission_us = esp_timer_get_time();

    uint8_t my_mac[6] = {0x00, 0x23, 0x45, 0x67, 0x89, 0xab};
    memcpy(recv_mac_addr, my_mac, 6);

    while(true) 
    {
        wifi_promiscuous_pkt_t *packet;
        if(xQueueReceive(reception_queue, &packet, 10)) 
        {
            ESP_LOGI(TAG, "xQueueReceive correctly received from reception_queue");
            mac80211_frame *p = (mac80211_frame *) packet->payload;

            // ESP_LOG_BUFFER_HEXDUMP("packet-content", packet->payload, packet->rx_ctrl.sig_len - 4, ESP_LOG_INFO);

            switch(sta_state)
            {
            case IDLE: //idle, wait for authenticate packet
                if (p->frame_control.type == IEEE80211_TYPE_MGT && p->frame_control.sub_type == IEEE80211_TYPE_MGT_SUBTYPE_AUTHENTICATION)
                {
                    ESP_LOGW(TAG, "Authentication received");
                    sta_state = AUTHENTICATED;
                    last_transmission_us = 0;
                }
                break;
            case AUTHENTICATED:
                if(p->frame_control.type == IEEE80211_TYPE_MGT && p->frame_control.sub_type == IEEE80211_TYPE_MGT_SUBTYPE_ASSOCIATION_RESP)
                {
                    ESP_LOGI(TAG, "Association response received");
                    sta_state = ASSOCIATED;
                    last_transmission_us = 0;
                }
                break;
            case ASSOCIATED:
                break;
            default:
                break;
            }
            free(packet);
        }
        else
        {
            ESP_LOGI(TAG, "xQueueReceive did not receive from reception_queue");
        }

        if (!tx) continue;

        if(esp_timer_get_time() - last_transmission_us < 1000*1000) continue;

        switch (sta_state)
        {
            case IDLE:
                ESP_LOGI(TAG, "Sending authentication frame!");
                tx(beacon_raw, sizeof(beacon_raw));
                break;
            case AUTHENTICATED:
                ESP_LOGI(TAG, "TODO: sending association request frame");
                break;
            case ASSOCIATED:
                ESP_LOGI(TAG, "TODO: sending data frame");
                break;
            default:
                break;
        }
        last_transmission_us = esp_timer_get_time();
    }
}