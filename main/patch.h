#ifndef PATCH_H
#define PATCH_H

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "espnow_example.h"


#include <stdint.h>

// Structure for the "mysterious" structure at 0x4081ca14
struct SubStruct {
    uint32_t field1;             // 0x00
    uint32_t field2;             // 0x04
    uint32_t self_pointer;       // 0x08 (pointer to this structure itself)
    uint32_t final_pointer;      // 0x0C (pointer to the "final" 0xdeadbeef in the main struct)
    uint32_t reserved[32];       // 0x10 (padding to align offsets for this example)
};

// Main structure for the packet
struct Packet {
    uint32_t header1;
    uint32_t pointer_to_3c;
    uint32_t pointer_to_3c_dup;
    uint32_t one_value;
    uint32_t pointer_to_90;
    uint32_t eight_words[8];
    uint32_t pointer_to_48;
    uint32_t empty_word_1;
    uint32_t mysterious_value_1;
    uint32_t pointer_to_b0;
    uint32_t empty_word_2;
    uint32_t mysterious_value_6412;
    uint32_t mysterious_value_7;
    uint32_t empty_word_3;
    uint32_t wifi_protocol;
    uint32_t mysterious_value_2;
    uint32_t mysterious_value_80;
    uint32_t maybe_timestamp;
    uint32_t pointer_to_4081621c;
    uint32_t eighteen_words[18];
    uint32_t length;
    uint32_t zero_word;
    uint32_t packet_content[40];
    uint32_t deadbeef;
    uint32_t pad_word;
    uint32_t pointer_to_4081ca14;
    uint32_t pointer_to_4081ca14_dup;
};

void initialize_packet(struct Packet* packet);
void initialize_substruct(struct SubStruct* substruct, uint32_t deadbeef_address);


extern int counter_patched_lmacTxFrame;
extern uint32_t g_espnow_lock;
extern uint32_t pTxRx;
extern uint32_t g_chm;
extern uint32_t g_mt;
extern uint32_t g_mt_version;
extern uint32_t g_ic;
extern uint32_t g_wifi_nvs;
extern uint8_t g_mesh_is_started;
extern uint32_t lmacConfMib_ptr;
extern uint32_t our_wait_eb;
extern uint32_t our_instances_ptr;
extern uint32_t our_instances;
extern uint32_t lmacConfMib;
extern uint32_t complete_ena_tb_seqno;
extern uint32_t complete_ena_tb_final;
extern uint32_t complete_ena_tb_count;
extern uint32_t g_wifi_global_lock;
extern uint32_t g_intr_lock_mux;
extern uint32_t pp_sig_cnt_ptr;
extern uint8_t ieee80211_ethbroadcast[6];
extern uint32_t xphyQueue;
extern uint32_t phy_param;

// CLOSED SOURCE FUNCTIONS

void lmacTxFrame(int param_1, int param_2);
bool lmacIsLongFrame(int param_1);
void lmacSetTxFrame(int *param_1, int param_2);
uint32_t esp_random(void);
int esp_test_tx_enab_statistics(uint32_t param_1);
void ppProcessLifeTime(int param_1, int *param_2);
int hal_mac_tx_config_timeout(int *param_1,uint32_t param_2);
int hal_mac_tx_set_ppdu(int *param_1,int param_2);
int  lmacRequestTxopQueue(int param_1);
short ppCalTxopDur(int param_1,int param_2);
void lmacReleaseTxopQueue(int param_1);
int mt_send(int param_1, int param_2, uint32_t param_3);
int mt_fetch_peer(int param_1, int param_2);
void *recursive_mutex_create_wrapper(void);
int32_t mutex_lock_wrapper(void *mutex);
int32_t mutex_unlock_wrapper(void *mutex);
int ieee80211_espnow_get_init_flag(void);
int ieee80211_send_action_vendor_spec(uint32_t param_1, uint8_t *param_2, int param_3, int param_4, int param_5, int *param_6);
void *wifi_malloc(uint32_t param_1);
// int esp_log_timestamp(void);
// void esp_log_write_wrapper(void);
void esp_log_write_wrapper(unsigned int level, const char *tag, const char *format, ...);
void free(void *param_1);
uint32_t roundup2(int param_1,int param_2);
int wifi_get_macaddr(uint32_t param_1, int param_2);
int get_iav_key(int param_1);
int cnx_node_search(uint8_t *param_1);
int ieee80211_alloc_action_vendor_spec(uint32_t param_1,int param_2,int param_3, int param_4, int param_5);
uint32_t hal_now(void);
uint32_t ic_get_default_sched(void);
uint32_t ic_get_espnow_rate(int param_1);
void ieee80211_set_tx_pti(int param_1, uint32_t param_2);
int ieee80211_post_hmac_tx(int param_1);
bool pp_post(uint32_t param_1, uint32_t param_2);
void ieee80211_recycle_cache_eb(int *param_1);
uint32_t esp_coex_common_int_disable_wrapper(void *wifi_int_mux);
void esp_coex_common_int_restore_wrapper(void *wifi_int_mux, uint32_t tmp);
void esp_coex_common_task_yield_from_isr_wrapper(void);
// uint32_t uxQueueMessagesWaiting(int param_1);
UBaseType_t uxQueueMessagesWaiting( const QueueHandle_t xQueue );
int32_t queue_send_wrapper(void *queue, void *item, uint32_t block_time_tick);
int32_t queue_send_from_isr_wrapper(void *queue, void *item, void *hptw);
int32_t task_ms_to_tick_wrapper(uint32_t ms);
void wifi_log(unsigned int level, int param_2, int param_3, const char *tag,const char *format, ...);
// -- in ROM
bool is_use_muedca(uint32_t param_1);

void *memcpy(void *param_1, const void *param_2, size_t param_3);
// at address 0x400004ac
void *memset(void *param_1, int param_2, size_t param_3);

void *wifi_zalloc_wrapper(size_t size);
int wifi_api_lock(void);
int wifi_osi_funcs_register(int *param_1);
int wdev_funcs_init(uint32_t param_1);
int net80211_funcs_init(void);
int wifi_api_unlock(void);
int wifi_init_in_caller_task(const wifi_init_config_t *config);
int wifi_init_process(int param_1);
int ieee80211_ioctl(char *param_1);
int wifi_deinit_in_caller_task(void);
int wdev_funcs_deinit(void);
int net80211_funcs_deinit(void);

void *esp_coex_common_spin_lock_create_wrapper(void);
int wifi_menuconfig_init(const wifi_init_config_t *config);
int misc_nvs_init(void);
int ic_create_wifi_task(void);
int ieee80211_ioctl_init(void);

void ppSetBarRate(uint32_t param_1);

void pm_wake_up(void);
void ic_mac_deinit(void);
int phy_change_channel(uint32_t param_1, uint32_t param_2, uint32_t param_3, uint32_t param_4);
void ic_mac_init(void);
void pm_wake_done(void);
void chip_v7_set_chan(int channel,int param_2);
void chip_v7_set_chan_misc(int channel);
void phy_i2c_master_mem_txcap(void);
void disable_agc(void);
void enable_agc(void);
void phy_bbpll_cal(int onoff);
void force_txrx_off(int onoff);
void set_channel_rfpll_freq(uint32_t frequency, int data_from_phy_rom_p79, int param_3);

void esp_rom_delay_us(long unsigned int param_1);


// END CLOSED SOURCE FUNCTIONS

// PATCHED FUNCTIONS

void patched_lmacSetTxFrame(int *param_1, int param_2);
void patched_lmacTxFrame(int param_1, int param_2);
bool patched_lmacIsLongFrame(int param_1);
int patched_esp_now_send(int param_1, int param_2, uint32_t param_3);
int patched_mt_send(void *param_1, int param_2, uint32_t param_3);
int patched_ieee80211_send_action_vendor_spec(uint32_t param_1, uint8_t *param_2, 
                                      int param_3, int param_4, int param_5, int *param_6);
int patched_ieee80211_post_hmac_tx(int param_1);
bool patched_pp_post(uint32_t param_1, uint32_t param_2);

int patched_esp_wifi_init_internal(const wifi_init_config_t *config);
int patched_wifi_init_in_caller_task(const wifi_init_config_t *config);

void switch_channel(uint32_t param_1, uint32_t param_2);
int patched_mhz2ieee(uint32_t frequency);

void patched_freq_chan_en_sw(uint32_t reshaped_frequency);
void patched_nrx_freq_set(uint32_t reshaped_frequency);



// END PATCHED FUNCTIONS

// Insert cafebabe into the paylod (substitute for esp_fill_random)
void esp_fill_cafebabe(void *buf, size_t len);
void esp_fill_de_bruijn(void *buf);

// counts how many times lmacTxFrame is called, redirecting
// towards the patched function
void call_patched_lmacTxFrame(int param_1, int param_2);

// Dummy function to prevent optimization
void force_include_symbols();

#endif // PATCH_H
