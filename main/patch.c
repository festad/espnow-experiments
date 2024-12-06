#include "patch.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
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

static const char *TAG = "patch";

int counter_patched_lmacTxFrame = 0;

int frequencies[] = {0x96c, 0x971, 0x976, 0x97b, 0x980, 0x985, 0x98a, 0x98f, 0x994, 0x999, 0x99e};
int n_frequencies = 11;

// IMPLEMENTATION OF PATCHED FUNCTIONS


// Function to initialize the Packet structure
void initialize_packet(struct Packet* packet, uint32_t base_address) {
    ESP_LOGI(TAG, "base_address 0x%"PRIx32"", base_address);
    ESP_LOGI(TAG, "packet 0x%"PRIx32"", (uint32_t)packet);
    packet->header1 = 0;
    packet->pointer_to_3c = base_address + 0x3C;
    packet->pointer_to_3c_dup = base_address + 0x3C;
    packet->one_value = 1;
    packet->pointer_to_90 = base_address + 0x90;

    // Initialize other fields based on offsets
    memset(packet->eight_words, 0, sizeof(packet->eight_words));
    packet->eight_words[0]=0x00190020;
    packet->eight_words[1]=0x00010000;
    packet->eight_words[4]=0x00002000;
    // packet->eight_words[7]=0xc04a4138;
    packet->pointer_to_48 = base_address + 0x48;
    packet->empty_word_1 = 0;
    packet->mysterious_value_1 = 0xc0464138;  // Example value
    packet->pointer_to_b0 = base_address + 0xB0;
    packet->empty_word_2 = 0;
    packet->mysterious_value_6412 = 0x00006412;
    packet->mysterious_value_7 = 7;
    packet->empty_word_3 = 0;
    packet->wifi_protocol = 0x00000017;  // Example protocol
    packet->mysterious_value_2 = 0x4002D44C; // Example value
    packet->mysterious_value_80 = 0x80;
    packet->maybe_timestamp = 0x1A8F0BF8;  // Example timestamp
    packet->pointer_to_4081621c = 0x4081621C;

    memset(packet->eighteen_words, 0, sizeof(packet->eighteen_words));
    packet->eighteen_words[0] = 0x00010001;
    packet->eighteen_words[4] = 0x00040000;

    packet->length = 0x45;//40*4=0xa0
    packet->zero_word = 0;

    memset(packet->packet_content, 0, sizeof(packet->packet_content));
    packet->packet_content[0] = 0xee9200b0;
    packet->packet_content[1] = 0xffffffff;
    packet->packet_content[2] = 0x4c40ffff;
    packet->packet_content[3] = 0xd85751ca;
    packet->packet_content[4] = 0xffffffff;
    packet->packet_content[5] = 0x0000ffff;
    packet->packet_content[6] = 0x34fe187f;
    packet->packet_content[7] = 0x7de59475;
    packet->packet_content[8] = 0xfe1817dd;
    packet->packet_content[9] = 0x00010434;
    packet->packet_content[10] = 0x57000000;
    packet->packet_content[11] = 0x3d2c4277;
    packet->packet_content[12] = 0xfebabeb5;
    packet->packet_content[13] = 0xfebabeca;
    packet->packet_content[14] = 0xd05fc9ca;

    packet->deadbeef = 0xDEADBEEF;
    packet->pad_word = 0x0004f1f1;
    packet->pointer_to_4081ca14 = 0x4081CA14;  // Fixed address
    packet->pointer_to_4081ca14_dup = 0x4081CA14;  // Fixed address
}

// Function to initialize the SubStruct
void initialize_substruct(struct SubStruct* substruct, uint32_t deadbeef_address) {
    substruct->field1 = 0xB6F35AFE;  // Example value
    substruct->field2 = 0xA760886D;  // Example value
    substruct->self_pointer = 0x4081CA14;  // Pointer to itself
    substruct->final_pointer = deadbeef_address; // Pointer to the address of "deadbeef"
    memset(substruct->reserved, 0, sizeof(substruct->reserved)); // Clear the padding
}


void switch_channel(uint32_t frequency, uint32_t param_2)
{
    // ESP_LOGI(TAG, "Before switching channel");
    // ESP_LOGI(TAG, "0x600a00c0: %"PRIu32" - %"PRIx32" ", *(uint32_t *)(0x600a00c0), *(uint32_t *)(0x600a00c0));
    // ESP_LOGI(TAG, "0x600a7848: %"PRIu32" - %"PRIx32" ", *(uint32_t *)(0x600a7848), *(uint32_t *)(0x600a7848));
    
    // pm_wake_up();
    // ic_mac_deinit();

    // phy_change_channel(param_1, 1, 0, param_2);

    int channel = patched_mhz2ieee(frequency);
    // chip_v7_set_chan(channel & 0xffff, param_2);

    // disable_agc();
    // force_txrx_off(1);
    // phy_bbpll_cal(1);

    // chip_v7_set_chan_misc(channel);
    // phy_i2c_master_mem_txcap();

    // set_channel_rfpll_freq(frequency, *(uint8_t *)((char *)&phy_param + 0x4f), 0); // It appears that this is the only function
    // that actually does the job of setting the frequency, after all, when setting a fixed frequency
    // here and leaving sliding channel on the previous one, the esp only sends at the fixed frequency.

    uint32_t reshaped_frequency = frequency - (0x960U & 0xffff);

    // Forcing a fixed frequency, here it's the middle point between 6 and 7
    // reshaped_frequency = 0x14;

    // ESP_LOGI(TAG, "========================================");
    // ESP_LOGI(TAG, "Switching channel");
    // ESP_LOGI(TAG, "---------------------------------");
    // ESP_LOGI(TAG, "channel: (%d)", channel);
    // ESP_LOGI(TAG, "frequency: (%"PRIu32") =  (%"PRIx32")", frequency, frequency);
    // ESP_LOGI(TAG, "reshaped frequency: (%"PRIu32") = (%"PRIx32")", reshaped_frequency, reshaped_frequency);
    // ESP_LOGI(TAG, "---------------------------------");

    if (reshaped_frequency < 0x55)
    {
        patched_freq_chan_en_sw(reshaped_frequency);
    }
    else
    {
        ESP_LOGI(TAG, "==> reshaped frequency above 85 = 0x55: (%"PRIu32") = (%"PRIx32")", reshaped_frequency, reshaped_frequency);
        void (*fun_ptr)(int a, int reshaped_frequency, int b) = (void (*)(int, int, int))0x40005c78;
        fun_ptr(param_2, reshaped_frequency, 0);
    }
    patched_nrx_freq_set(reshaped_frequency);

    // phy_bbpll_cal(0);
    // force_txrx_off(0);
    // enable_agc();

    // ic_mac_init();
    // pm_wake_done();

    return;
}


void patched_freq_chan_en_sw(uint32_t reshaped_frequency)
{
    *(uint32_t *)(0x600a00c0) = (reshaped_frequency & 0x7f) << 7 | ( *(uint32_t *)(0x600a00c0) & 0xffffc00f ) | 0x4000;
    // esp_rom_delay_us(1);
    // ESP_LOGI(TAG, "0x600a00c0: (%"PRIu32") - (%"PRIx32") ", *(uint32_t *)(0x600a00c0), *(uint32_t *)(0x600a00c0));

    *(uint32_t *)(0x600a00c0) = *(uint32_t *)(0x600a00c0) &  0xffffbfff;
    // ESP_LOGI(TAG, "0x600a00c0: (%"PRIu32") - (%"PRIx32") ", *(uint32_t *)(0x600a00c0), *(uint32_t *)(0x600a00c0));

    return;
}

void patched_nrx_freq_set(uint32_t reshaped_frequency)
{
    *(uint32_t *)(0x600a7848) = (((0x50 << ( (*(uint32_t *)(0x600a7848) >> 0x18) & 0x1f)) / reshaped_frequency) & 0xffffffU) | (*(uint32_t *)(0x600a7848) & 0xff000000);
    // ESP_LOGI(TAG, "0x600a7848: (%"PRIu32") - (%"PRIx32") ", *(uint32_t *)(0x600a7848), *(uint32_t *)(0x600a7848));

    return;
}


int patched_mhz2ieee(uint32_t frequency)
{
    if (frequency != 0x9b4)
    {
        if (frequency < 0x9b4)
        {
            return (int)((frequency - 0x967) / 5);
        }
        else
        {
            return (int)((frequency - 0x9d0) / 0x14) + 0x0f;
        }
    }
    return (int)0xe;
}

esp_err_t patched_esp_wifi_init_internal(const wifi_init_config_t *config)
{
    int iVar1;
    char *puVar2;
    char *pcVar3;

    if (config == NULL)
    {
        puts("esp_wifi_init: config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    iVar1 = wifi_osi_funcs_register(*(int *)config);
    if (iVar1 != 0)
    {
        return iVar1;
    }

    iVar1 = wifi_api_lock();

    if (iVar1 != 0)
    {
        return iVar1;
    }

    if (*(int *)(0x40815560) == 1)
    {
        iVar1 = wdev_funcs_init(*(char *)config + 0xc0);

        if ((iVar1 == 0) && (iVar1 = net80211_funcs_init(), iVar1 == 0))
        {
            *(int *)(0x40815560) = 2;
            wifi_api_unlock();
            iVar1 = wifi_init_in_caller_task(config);
            // iVar1 = patched_wifi_init_in_caller_task(config);
            if (iVar1 == 0)
            {
                puVar2 = (char *)wifi_zalloc_wrapper(0x18);
                if (puVar2 == NULL)
                {
                    iVar1 = 0x101;
                }
                else
                {
                    *puVar2 = 0x26;
                    *(void **)(puVar2 + 4) = wifi_init_process;
                    *(uint16_t *)(puVar2 + 0x8) = 0;
                    *(char *)(puVar2 + 0xa) = 0;
                    *(int **)(puVar2 + 0xc) = config;
                    iVar1 = ieee80211_ioctl(puVar2);
                    if (iVar1 == 0)
                    {
                        wifi_api_lock();
                        *(int *)(0x40815560) = 3;
                        goto LAB_42021DBE;
                    }
                }
            }
        }

        wifi_deinit_in_caller_task();
        wifi_api_lock();
        wdev_funcs_deinit();
        net80211_funcs_deinit();
        *(int *)(0x40815560) = 1;

    }
    else
    {
        iVar1 = 0;
        if (*(int *)(0x40815560) != 3)
        {
            if (*(int *)(0x40815560) == 2)
            {
                wifi_api_unlock();
                pcVar3 = "s_init: init is in progress 0x420a6660";
            }
            else
            {
                if (*(int *)(0x40815560) != 4)
                {
                    goto LAB_42021DBE;
                }
                wifi_api_unlock();
                pcVar3 = "s_init: deinit is in progress 0x420a667c";
            }
            wifi_log(3, 0, 0, "wifi_init", pcVar3);
            return 0x3013;
        }
    }

LAB_42021DBE:
    wifi_api_unlock();
    return iVar1;
}



// int patched_wifi_init_in_caller_task(const wifi_init_config_t *config)
// {
//     int iVar1;

//     if ((g_intr_lock_mux != 0) || (g_intr_lock_mux = esp_coex_common_spin_lock_create_wrapper(), g_intr_lock_mux != 0))
//     {
//         if ((g_wifi_global_lock != 0) || (g_wifi_global_lock = recursive_mutex_create_wrapper(), g_wifi_global_lock != 0))
//         {
//             iVar1 = wifi_menuconfig_init(config);
//             if ((((iVar1 == 0) && (iVar1 = misc_nvs_init(), iVar1 == 0)) &&
//                 (iVar1 = ic_create_wifi_task(), iVar1 == 0)) && 
//                 (iVar1 = ieee80211_ioctl_init(), iVar1 == 0))
//             {
//                 return 0;
//             }
//             goto LAB_4202190A;
//         }
//         wifi_log(1, 1, 2, "wifi_init", "s_init_failed_to_wifi_global_lock_420a6640");
//     }
//     iVar1 = 0x101;
//     LAB_4202190A:
//         wifi_deinit_in_caller_task();
//         return iVar1;
// }


void patched_lmacSetTxFrame(int *param_1, int param_2)
{
    uint32_t uVar1;
    char cVar2;
    uint32_t *puVar3;
    int *piVar4;
    uint8_t bVar5;
    uint32_t uVar6;
    int iVar7;
    int iVar8;
    uint32_t uStack_14;

    // int gp = 0x40815200; // Global pointer
    // int _our_wait_eb = 0x4087ff78;

    iVar8 = our_wait_eb;
    if (param_2 == 0)
    {
        iVar8 = *param_1;
    }

    puVar3 = *(uint32_t **)(iVar8 + 0x34);
    uVar6 = *puVar3;

    if (-1 < (int)(uVar6<<9))
    {
        if ((uVar6 & 0x80) == 0)
        {
            if (((((((param_1[3] & 0xff00ff00U) == 0) && (*(int16_t *)(param_1 + 6) != 0)) &&
                (*(char *)(param_1 + 7) == '\0')) &&
                ((*(char *) (( puVar3[4] >> 0x14 & 0xf ) * 0x34 + pTxRx + 0x30 ) == '\0' &&
                 (( uVar6 & 0x40480000) != 0x400000 )))) &&
                ((( uVar6 & 0x40402) == 0 && (( -1 < (int)uVar6 && ((uVar6 & 0x88) == 8)))))) && 
                (( *(int *)(iVar8 + 0x2c) != 0 && 
                   ((*(char *)(*(int *)(iVar8 + 0x2c) + 0x86) == '\x01' && 
                   (iVar7 = lmacRequestTxopQueue(puVar3[1] >> 4 & 0xf), iVar7 != 0))))))
            {

                cVar2 = ppCalTxopDur(iVar8, *(uint16_t *)(param_1+6));
                *(char *)(param_1 + 7) = cVar2;
                puVar3 = *(uint32_t **)(iVar8 + 0x34);
                if (cVar2 == '\0')
                {
                    lmacReleaseTxopQueue(puVar3[1] >> 4 & 0xf);
                }
                else
                {
                    *puVar3 = *puVar3 | 0x100;
                }
            }
        }
        else
        {
            *(uint8_t *)(param_1 + 7) = 0;
            if ((uVar6 & 0x40 ) == 0)
            {
                for (iVar7 = *(int *)(iVar8 + 0x30); iVar7 != 0; iVar7 = *(int *)(iVar7 + 0x30))
                {
                    *(char *)(param_1 + 7) = *(char *)(param_1 + 7) + '\x01';
                }
            }
        }
    }

    // 40815c08 -> lmacConfMib
    // 40815c08+0x2e = 0x40815c36
    piVar4 = *(int **)(iVar8 + 0x34);
    if (*(char *)(*(char *)&lmacConfMib+0x2e) != '\0')
    {
        cVar2 = *(char *)(piVar4 + 3);
        if (*(char *)(*(char *)&lmacConfMib+0x2d) == '\0')
        {
            if (2 < (uint8_t)(cVar2 - 1U)) goto LAB40810870;
            bVar5 = cVar2 + 4;
        }
        else
        {
            bVar5 = cVar2 - 4;
            if (3 < bVar5) goto LAB40810870;
        }
        *(uint8_t *)(piVar4 + 3) = bVar5;
    }

    LAB40810870:
        iVar7 = *(int *)0x600ad000;
        if (g_mesh_is_started == '\0')
        {
            uStack_14 = *(uint32_t *)((char *)&lmacConfMib + 0x8);
            if (*piVar4 << 4 < 0)
            {
                uStack_14 = *(uint32_t *)&lmacConfMib;
            }
            uStack_14 = uStack_14 << 10;
            ppProcessLifeTime(iVar8, iVar7);
            uVar6 = 10;
            uVar1 = (uStack_14 - iVar7) + *(int *)(*(int *)(iVar8 + 0x34) + 0x18);
            if ((uVar1 < uStack_14) && (uVar6 = uVar1 >> 10, uVar6 == 0)) 
            {
                uVar6 = 1;
            }
        }
        else
        {
            uVar6 = *(uint32_t *)((char *)&lmacConfMib + 0x8) - ( (uint32_t)(*(int *)0x600ad000 - piVar4[6]) >> 10 );
            if (*(uint32_t *)((char *)&lmacConfMib + 0x8) < uVar6)
            {   
                uVar6 = *(uint32_t *)((char *)&lmacConfMib + 0x8); 
            }
        }
	hal_mac_tx_config_timeout(param_1, uVar6);
	hal_mac_tx_set_ppdu(param_1, (int)pTxRx);
}

void patched_lmacTxFrame(int param_1, int param_2)
{
    uint8_t uVar1;
    uint16_t uVar2;
    int iVar3;
    uint32_t uVar4; 
    uint32_t *puVar5;
    int iVar6;
    int iVar7;
    uint32_t uVar8;
    int *piVar9;

    // param_1 is the base address of the packet structure
    ESP_LOGI(TAG, "patched_lmacTxFrame: address: 0x%"PRIx32"", (uint32_t)param_1);

    // gp assumed to be set somewhere else, not used in this scope
    piVar9 = (int *)(param_2 * 0x34 + (char *)our_instances_ptr); // Calculating the instance structure

    // Check a status byte in the instance
    if (*(char *)(param_2 * 0x34 + (char *)our_instances_ptr + 0x12) != '\x04') {
        puVar5 = *(uint32_t **)(param_1 + 0x34); // Pointer to frame control
        uVar8 = *puVar5;
        *piVar9 = param_1; // Save parameter into structure

        // Set a flag in the control word if a specific condition is met
        if ((uVar8 & 0x2102) == 0x2000) {
            *puVar5 |= 0x1000;
        }

        // Call an external function at address 0xXXXXXXXX, e.g., _pp_wdev_funcs + 0xe4
        iVar3 = patched_lmacIsLongFrame(param_1); // Replace with actual address

        puVar5 = *(uint32_t **)(param_1 + 0x34);
        if ((iVar3 != 0) && ((*puVar5 & 2) == 0)) {
            *puVar5 = (*puVar5 & 0xffffefff) | 0x100;
        }

        // Check another condition and modify the frame if needed
        if ((((int)(*puVar5 << 0x13) < 0) && (*(char *)(param_2 * 0x34 + (char *)our_instances_ptr + 0x12) == '\x03')) &&
           (*(uint8_t *)(lmacConfMib_ptr + 0x2a) <= *(uint8_t *)((int)puVar5 + 5))) {
            *puVar5 = (*puVar5 & 0xffffefff) | 0x100;

            uVar1 = *(uint8_t *)((int)puVar5 + 6);
            *(uint8_t *)((int)puVar5 + 6) = 0;
            *(uint8_t *)((int)puVar5 + 7) = uVar1;
        }

        // Call a function at address 0xYYYYYYYY, e.g., _pp_wdev_funcs + 0x2e0, based on a condition
        if (*(void **)(0x4087ff70 + 0x2e0) != NULL) {
            if ((int)(*puVar5 << 2) < 0) {
                ((void (*)(int))(0x4087ff70 + 0x2e0))(param_1); // Replace with actual address
            }
        }

        // Another function call with additional parameters, e.g., _pp_wdev_funcs + 0xe8
        // ((void (*)(void *, int, void *))(0x4087ff70 + 0xe8))(piVar9, 0, (void *)(0x4087ff70+0xe8)); // Replace with actual address
        patched_lmacSetTxFrame(piVar9, 0); // Replace with actual address
    }

    // Function call at address 0x4000a032, returning a 16-bit result
    // DO I NEED A CAST TO UINT_16 ???
    uVar2 = esp_random();

    param_2 = param_2 * 0x34;
    *(uint16_t *)(param_2 + (char *)our_instances_ptr + 0x6) =
        uVar2 & ~(uint16_t)(-1 << (*(uint8_t *)(param_2 + (char *)our_instances_ptr + 0x8) & 0x1f));

    puVar5 = (uint32_t *)((uint32_t)*(uint8_t *)(param_2 + (char *)our_instances_ptr + 0x4) * -0x10 + 0x600a4d68);
    *puVar5 = (*(uint8_t *)(param_2 + (char *)our_instances_ptr + 0x5) & 0xf) << 0x18 | (*puVar5 & 0xf0ffffff);
    puVar5 = (uint32_t *)((uint32_t)*(uint8_t *)(param_2 + (char *)our_instances_ptr + 0x4) * -0x10 + 0x600a4d68);
    *puVar5 = (*(uint16_t *)(param_2 + (char *)our_instances_ptr + 0x6) & 0x3ff) << 0xc | (*puVar5 & 0xffc00fff);
    puVar5 = (uint32_t *)((uint32_t)*(uint8_t *)(param_2 + (char *)our_instances_ptr + 0x4) * -0x10 + 0x600a4d68);
    *puVar5 = (*(uint32_t *)(*(int *)(*(int *)(param_2 + (char *)our_instances_ptr) + 0x34) + 0x10) >> 0x13 & 1) << 0x16
              | (*puVar5 & 0xff3fffff);

    uVar8 = (uint32_t)*(uint8_t *)(param_2 + (char *)our_instances_ptr + 0x4);
    *(uint8_t *)(param_2 + 0x4087f852) = 1;
    iVar3 = uVar8 * 0x34;
    puVar5 = (uint32_t *)(uVar8 * -0x10 + 0x600a4d6c);

    // Let's try to set here the data rate instead of in patched_ieee80211_send_action_vendor_spec

    // *(uint8_t *)(0x4081ec7c) = (uint8_t)0x17;
    // Doesn't work


    // // First, let's backup the 16 bytes following the first 15 on the PHY details (because
    // // we are going to substitute them with our own RTS frame, and then restore them)
    // unsigned char backup[16];
    // memcpy(backup, (char *)0x4081ecd1+15, 16);

    // esp_rom_delay_us(50000);

    // // Injection of custom packet
    // // From address 0x4081ecd1 (the start of the packet),
    // // we skip the 15 bytes of physical layer info,
    // // then we copy the following array (16 bytes)
    // unsigned char rts[] = {
    //     // 0x0, 0x0, 0xf, 0x0, 0x2e, 0x0, 0x0, 0x0, 0x10, 0x2, 0x85, 0x9, 0xa0, 0x0, 0xa8, // physical layer
    //     0xb4, // subtype
    //     0x0, // htc order flag
    //     0xce, 0x7, // duration
    //     0x08, 0x16, 0x05, 0xcc, 0xc6, 0x7c, // receiver address
    //     0x4a, 0xfe, 0xca, 0x52, 0xde, 0x63, // transmitter address
    //     // 0x90, 0x25, 0x75, 0x8a
    //     };

    // // base + Phy + MAC Action + ESPNOW_HEAD
    // // memcpy((0x4081ecd1+15+24+15), rts, 16);
    // memcpy((char *)0x4081ecd1+15, rts, 16);

    // esp_rom_delay_us(50000);

    // // // Now we send #n_rts RTS 
    // // int n_rts = 100;
    // // for (int i = 0; i < n_rts; i++) 
    // // {
    // //     esp_rom_delay_us(1000);
    // //     *puVar5 |= 0xc0000000;
    // // }

    // // *puVar5 |= 0xc0000000;

    // esp_rom_delay_us(50000);

    // // // And we restore the original packet
    // memcpy((char *)0x4081ecd1+15, backup, 16);

    
    *puVar5 |= 0xc0000000;

    iVar7 = *(int *)(iVar3 + (char *)our_instances_ptr);
    iVar6 = *(int *)(iVar7 + 0x34);
    *(uint8_t *)(iVar3 + (char *)our_instances_ptr + 0x28) &= 0xfd;

    // Condition checks and final function call based on calculated values
    if ((((*(uint8_t *)(iVar6 + 0x2f) & 0x78) - (0x28 & 0xf0)) == 0) &&
       ((0xa1U >> (*(uint32_t *)(iVar6 + 4) & 0xf) & 1) != 0)) {
        complete_ena_tb_seqno = *(uint32_t *)(iVar7 + 0x24) & 0xfff;
        complete_ena_tb_final = (uint32_t)*(uint16_t *)(iVar3 + (char *)our_instances_ptr + 0x32);
        complete_ena_tb_count = (uint32_t)*(uint8_t *)(iVar7 + 0x26);

        // Call function at 0x4000a040
        // uVar4 = ((uint32_t (*)(uint32_t))0x4000a040)(uVar8);
        uVar4 = is_use_muedca(uVar8); // Function in ROM, probably won't work

        *(uint8_t *)(iVar3 + (char *)our_instances_ptr + 0x28) =
            (*(uint8_t *)(iVar3 + (char *)our_instances_ptr + 0x28) & 0xfd) | ((uVar4 & 1) << 1);
    }

    // Last function call to 0xAAAAAAAA if non-zero
    if (*(void **)(0x4087ff70+0x334) != NULL) {
        // ((void (*)(uint32_t))(0x4087ff70+0x334))(uVar8); // Replace with actual address
        esp_test_tx_enab_statistics(uVar8); // Replace with actual address
    }
}


bool patched_lmacIsLongFrame(int param_1)
{
    uint32_t uVar1 = 0x0;

    if ((-1 < **(int **)(param_1 + 0x34)) || (uVar1 = (*(int **)(param_1 + 0x34))[0xd], uVar1 == 0))
    {
        uVar1 = (uint32_t)*(uint16_t *)(lmacConfMib_ptr+ 0x16);
    }

    return (int)uVar1 < (int)((uint32_t)*(uint16_t *)(param_1 + 0x14) + (uint32_t)*(uint16_t*)(param_1 + 0x16));
}



int patched_esp_now_send(int ptr_dest_mac, int ptr_data_buffer, uint32_t len)
{
    int iVar1;
    int uVar2;
    int iVar3;
    void* iVar4;
    // void (*pcVar5)(unsigned int level, const char *tag, const char *format, ...);
    // pcVar5 = &esp_log_write_wrapper; // It's the one with +0x14c

    if ((g_espnow_lock == 0) &&
        (g_espnow_lock = (int *)recursive_mutex_create_wrapper(), g_espnow_lock == 0))
    {
        // if _g_osi_funcs == 0 return 0x3067
        
        // if (pcVar5 == 0)
        // {
        //     return 0x3067;
        // }

        uVar2 = esp_log_timestamp();
        // (*pcVar5)(1, TAG, "esp_now_send: espnow lock is NULL");
        esp_log_write_wrapper(1, TAG, "esp_now_send: espnow lock is NULL");
        return 0x3067;
    }

    mutex_lock_wrapper(g_espnow_lock);
    iVar3 = ieee80211_espnow_get_init_flag();

    if (iVar3 == 0)
    {
        uVar2 = esp_log_timestamp();
        // (*pcVar5)(1, TAG, "esp_now_send: espnow is not initialized");
        esp_log_write_wrapper(1, TAG, "esp_now_send: espnow is not initialized");
        iVar1 = 0x3065;
        mutex_unlock_wrapper(g_espnow_lock);
    }
    else if (ptr_dest_mac == 0)
    {
        iVar1 = 0x3069;
        iVar4 = wifi_malloc(0x24);

        if (iVar4 == 0)
        {
            uVar2 = esp_log_timestamp();
            // (*pcVar5)(1, TAG, "esp_now_send: malloc failed");
            esp_log_write_wrapper(1, TAG, "esp_now_send: malloc failed");
            iVar1 = 0x3067;
            mutex_unlock_wrapper(g_espnow_lock);
        }
        else
        {
            do
            {
                iVar3 = mt_fetch_peer(iVar3, iVar4);
                if (iVar3 != 0)
                {
                    free(iVar4);
                    goto LAB_4201FBA6;
                }
                iVar1 = patched_mt_send(iVar4, ptr_data_buffer, len);
                iVar3 = 0;
            }
            while (iVar1 == 0);
            iVar1 = 0x306a;
            free(iVar4);
            mutex_unlock_wrapper(g_espnow_lock);
        }
    }
    else
    {
        iVar1 = patched_mt_send(ptr_dest_mac, ptr_data_buffer, len);
        LAB_4201FBA6:
            mutex_unlock_wrapper(g_espnow_lock);
    }
    return iVar1;
}


int patched_mt_send(void *ptr_dest_mac, int ptr_data_buffer, uint32_t len)
{
    int iVar1;
    int *piVar2;
    uint32_t uVar4;
    char *pcVar5;

    if (((ptr_dest_mac == 0) || (ptr_data_buffer == 0)) ||
        (piVar2 = *(int *)((char *)&g_mt + 0x20), 0xf9 < len - 1))
    {
        iVar1 = 0x3066;
        uVar4 = esp_log_timestamp();
        pcVar5 = "mt_send: invalid parameter";
        LAB_42020658:
            iVar1 = 0x3066;
            esp_log_write_wrapper(1, TAG, pcVar5);
    }
    else
    {
        for (; piVar2 != (int *)0x0; piVar2 = (int *)*piVar2)
        {
            iVar1 = memcmp((int)piVar2+0xa, ptr_dest_mac, 6);
            if (iVar1 == 0)
            {
                if ((*(char *)(piVar2 + 2) == '\0') || 
                    (*(char *)(g_chm + 0x50) == *(char *)(piVar2 + 2)))
                {
                    iVar1 = patched_ieee80211_send_action_vendor_spec(*(uint8_t *)((int)piVar2 + 9), ptr_dest_mac, ptr_data_buffer, (len & 0xff),
                                                              *(int *)((char *)&g_mt + 0x38) != 0, piVar2[6]);
                    if (iVar1 == 0)
                    {
                        return 0;
                    }
                    if (iVar1 < 1)
                    {
                        if (iVar1 != -1)
                        {
                            return iVar1;
                        }
                        return 0x306a;
                    }
                    if (iVar1 == 0x101)
                    {
                        return 0x3067;
                    }
                    if (iVar1 != 0x3004)
                    {
                        return iVar1;
                    }
                    return 0x306c;
                }
                uVar4 = esp_log_timestamp();
                pcVar5 = "mt_send: peer channel is not ...";
                goto LAB_42020658;
            }
        }
        iVar1 = 0x3069;
    }
    return iVar1;
}


int patched_ieee80211_send_action_vendor_spec(uint32_t param_1, uint8_t *param_2, 
                                      int param_3, int param_4, int param_5, int *param_6)
{
    int16_t sVar1;
    uint16_t uVar2;
    uint16_t uVar3;
    uint8_t *puVar4;
    int16_t sVar5;
    int uVar6;
    uint16_t *puVar7;
    uint8_t uVar8;
    uint8_t *pbVar9;
    int iVar10;
    int iVar11;
    uint32_t uVar12;
    uint32_t uVar13;
    uint8_t bVar14;
    uint32_t *puVar15;
    int iVar16;
    uint32_t uVar17;
    int iVar18;

    uint8_t mac_buffer[6];
    uint8_t buffer[6];

    wifi_get_macaddr(param_1, mac_buffer);

    pbVar9 = (uint8_t *)get_iav_key(param_2);
    mutex_lock_wrapper(g_wifi_global_lock);
    // uint32_t g_ic_addr = 0x40818bac;
    iVar16 = *(int *)((uint8_t *)(&g_ic) + 0x14);
    iVar11 = *(int *)((uint8_t *)(&g_ic) + 0x10);

    if (param_1 == 0)
    {
        if (iVar11 == 0)
        {
            LAB_42021160:
                mutex_unlock_wrapper(g_wifi_global_lock);
                return 0x3004;
        }
        iVar18 = *(int *)((uint8_t *)iVar11 + 0xe4);
        iVar16 = iVar11;
        if ((iVar18 == 0) || (iVar10 = memcmp(iVar18 + 4, param_2, 6), iVar10 != 0))
        {
            iVar18 = *(int *)((uint8_t *)iVar11 + 0xe8);
        }
    }
    else
    {
        if ((param_1 != 1) || (iVar16 == 0))
        {
            goto LAB_42021160;
        }
        iVar18 = cnx_node_search(param_2);
        if (iVar18 == 0)
        {
            iVar18 = *(int *)((uint8_t *)iVar16 + 0xec);
        }
    }
    iVar11 = ieee80211_alloc_action_vendor_spec(iVar16,param_2,param_3,param_4,pbVar9);
    if (iVar11 == 0)
    {
        uVar6 = 0x101;
        mutex_unlock_wrapper(g_wifi_global_lock);
    }
    else 
    {
        puVar15 = *(uint32_t **)(iVar11 + 4);
        if (-1 < (int)((uint32_t)*(uint16_t *)(iVar11 + 0x24) << 0x12))
        {
            sVar1 =  *(uint16_t *)(iVar11 + 0x14);
            // puVar15[1] = puVar15[1] - 8; Needs to be translated with pointer
            // arithmetics, and the [1] means adding 0x4 to the pointer
            // puVar15[1] = puVar15[1] - 8;
            *(uint32_t *)((uint8_t *)puVar15 + 0x4) = *(uint32_t *)((uint8_t *)puVar15 + 4) - 8;
            *(int16_t *)(iVar11 + 0x14) = sVar1 + 8;
            *(uint16_t *)(iVar11 + 0x24) = *(uint16_t *)(iVar11 + 0x24) | 0x2000;
            // *puVar15 = ((*puVar15 >> 0xe & 0x3fff) + 8 & 0x3fff) << 0xe | *puVar15 & 0xf0003fff;
            *(uint32_t *)((uint8_t *)puVar15 + 0x0) = 
                    (( (*(uint32_t *)((uint8_t *)puVar15+0x0) >> 0xe) & 0x3fff) + (8 & 0x3fff) ) << 0xe | 
                    (*(uint32_t *)((uint8_t *)puVar15 + 0x0) & 0xf0003fff);
        }
        *(uint8_t *)(*(int *)(iVar11 + 0x34) + 0x32) = *(uint8_t *)(*(int *)(iVar11 + 0x34) + 0x32) | 4;
        uVar2 = *(uint16_t *)(iVar11 + 0x14);
        uVar3 = *(uint16_t *)(iVar11 + 0x16);
        puVar7 = (uint16_t *)*(uint32_t *)((uint8_t *)puVar15 + 0x4);
        *(uint32_t *)puVar15 = *(uint32_t *)puVar15 | 0x80000000; 
        *(uint32_t *)puVar15 = *(uint32_t *)puVar15 | 0x40000000;
        *(uint32_t *)puVar15 = *(uint32_t *)puVar15 & 0xdfffffff;
        *(uint32_t *)puVar15 = ((uint32_t)uVar3 + ((uint32_t)uVar2 & 0x3fff)) << 0xe | (*(uint32_t *)puVar15 & 0xf0003fff);
        if ((int)((uint32_t)*(uint16_t *)(iVar11 + 0x24) << 0x12) < 0)
        {
            puVar7 = puVar7 + 4;
        }
        *(uint16_t *)puVar7 = (uint16_t)0xd0;
        *(uint16_t *)(*(uint8_t *)puVar7 + 0x2) = (uint16_t)0x0;

        memcpy(puVar7 + 2, param_2, 6);
        memcpy(puVar7 + 5, mac_buffer, 6);
        memcpy(puVar7 + 8, (uint8_t *)ieee80211_ethbroadcast, 6);

        if (pbVar9 != (uint8_t *)0x0)
        {
            *(uint8_t *)((uint8_t *)puVar7 + 0x1) = (uint8_t)0x40;
            iVar10 = iVar16;
            puVar15 = *(uint32_t **)(iVar11 + 0x34);
            // *puVar15 = *puVar15 | 1;
            *(uint32_t *)puVar15 = *(uint32_t *)puVar15 | 1;
            bVar14 = *pbVar9;
            if (iVar10 == iVar16)
            {
                bVar14 = bVar14 | 0x40;
            }
            *(uint8_t *)((uint8_t *)puVar15 + 0x10) = bVar14;
            // puVar15[4] = puVar15[4] & 0xfffff0ff | (**(uint32_t **)(pbVar9 + 0xa0) & 0xf) << 8;
            *(uint32_t *)((uint8_t *)puVar15 + 0x10) = 
                    (*(uint32_t *)((uint8_t *)puVar15 + 0x10) & 0xfffff0ff) |
                    (**(uint32_t **)(pbVar9 + 0xa0) & 0xf) << 8;
        }

        puVar15 = *(uint32_t **)(iVar11 + 0x34);

        if ((*param_2 & 1) != 0)
        {
            *(uint32_t *)((uint8_t *)puVar15 + 0x0) = *(uint32_t *)((uint8_t *)puVar15 + 0x0) | 0x402;
        }
        *(uint32_t *)((uint8_t *)puVar15 + 0x0) = *(uint32_t *)((uint8_t *)puVar15 + 0x0) | 0x4010;
        *(uint8_t *)((uint8_t *)puVar15 + 0x4) = (uint8_t)7;
        uVar12 = hal_now();
        iVar10 = *(int *)(iVar11 + 0x34);
        *(uint32_t *)((uint8_t *)puVar15 + 0x18) = uVar12;
        *(uint32_t *)(iVar10 + 0x10) = (*(uint32_t *)(iVar10 + 0x10) & 0xfff7ffff) | (param_1 & 1) << 0x13;
        if (param_5 != 0)
        {
            *(uint32_t *)(iVar10 + 0x14) = 0x80;
        }
        uVar6 = ic_get_default_sched();
        // MODIFYING THE VALUE OF THE SCHEDULE
        *(uint32_t *)(iVar10 + 0x1c) = uVar6;
        // *(uint32_t *)(iVar10 + 0x1c) = (uint32_t)0x408162dc;
        uVar8 = (uint8_t)ic_get_espnow_rate(param_1);
        puVar4 = (uint8_t *)g_wifi_nvs;
        puVar15 = *(uint32_t **)(iVar11 + 0x34);
        // MODIFYING THE VALUE OF THE RATE
        // *(uint8_t *)((uint8_t *)puVar15 + 0xc) = uVar8;
        *(uint8_t *)((uint8_t *)puVar15 + 0xc) = (uint8_t)0x17;
        uVar12 = ((uint8_t *)puVar15 + 0x10);

        if (param_1 == 0)
        {
            uVar13 = (uint32_t)( ( *(uint8_t *)((uint8_t *)puVar4 + 0x9d) ) == '\x02' ) << 0x12;
            uVar17 = (uVar12 & 0xfffbffff) | uVar13;
            *(uint32_t *)((uint8_t *)puVar15 + 0x10) = uVar17;
            if (*(char *)(iVar16 + 0x154) == '\x04')
            {
                uVar17 = uVar17 | 0x10000;
            }
            else
            {
                uVar17 = (uVar12 & 0xfffaffff) | uVar13;
            }
            *(uint32_t *)((uint8_t *)puVar15 + 0x10) = uVar17;
        }
        else
        {
            *(uint32_t *)((uint8_t *)puVar15 + 0x10) = 
                (uVar12 & 0xfffbffff) | 
                (uint32_t)( (*(uint8_t *)((uint8_t *)puVar4 + 0x3fb)) == '\x02') << 0x12;
        }

        if (param_6 != (int *)0x0)
        {
            iVar16 = *param_6;
            *(char *)((uint8_t *)puVar15 + 0xc) = (char)(*(uint8_t *)((uint8_t *)param_6 + 0x4));
            if (iVar16 == 5)
            {
                *(uint32_t *)((uint8_t *)puVar15 + 0x0) = *(uint32_t *)((uint8_t *)puVar15 + 0x0) |  0x80000000;
                bVar14 = *(uint8_t *)((uint8_t *)puVar15 + 0x2f) & 0x87;
                if (*(char *)((uint8_t *)param_6 + 0x8) == '\0')
                {
                    bVar14 = bVar14 | 0x28;
                }
                else
                {
                    bVar14 = bVar14 | 0x30;
                }
                *(uint8_t *)((uint8_t *)puVar15 + 0x2f) = bVar14;
            }
            if (  (*(uint8_t *)((uint8_t *)g_wifi_nvs + 0x9d)  == '\x02') || 
                  (*(uint8_t *)((uint8_t *)g_wifi_nvs + 0x3fb) == '\x02')    )
            {
                if (iVar16 == 4)
                {
                    uVar12 = (*(uint32_t *)((uint8_t *)puVar15 + 0x10) | 0x40000);
                }
                else
                {
                    uVar12 = (*(uint32_t *)((uint8_t *)puVar15 + 0x10) & 0xfffbffff);
                }
                *(uint32_t *)((uint8_t *)puVar15 + 0x10) = uVar12;
            } 
        }

        ieee80211_set_tx_pti(iVar11, 8);
        if (iVar18 == 0)
        {
            sVar5 = *(int16_t *)((uint8_t *)&g_mt_version + 0x4) + (int16_t)1;
            sVar1 = *(int16_t *)((uint8_t *)&g_mt_version + 0x4);
            // *(int16_t *)((uint8_t *)&g_mt_version + 0x4) = sVar5;
            // Actually, this is the same logic, either we write 
            // only the updated sVar5 value back here or,
            // after the else, we write  sVar15 anyway, just, if
            // the else branch is taken there will be no modification at all
            // in the address
        }
        else
        {
            sVar1 = *(int16_t *)(iVar18 + 0xce);
            *(int16_t *)(iVar18 + 0xce) = sVar1 + 1;
            sVar5 = *(int16_t *)((uint8_t *)&g_mt_version + 0x4);
        }
        // DAT_4081554c = sVar5; I suspect this is Ghidra's mistake:
        // this instruction to me is only executed in the if branch
        *(int16_t *)((uint8_t *)&g_mt_version + 0x4) = sVar5;

        *(int16_t *)((uint8_t *)puVar7 + 0x16) = sVar1 << 4;

        uVar6 = patched_ieee80211_post_hmac_tx(iVar11);
        mutex_unlock_wrapper(g_wifi_global_lock);
    }
    return uVar6;
}


int patched_ieee80211_post_hmac_tx(int param_1)
{
    int iVar1;
    int uVar2;

    if (*(char *)0x40818df8 == '\0')
    {
        *(uint32_t *)(param_1 + 0x30) = 0;
        uint32_t target_addr = *(uint32_t *)0x4081959c;
        *(int *)target_addr = param_1; 
        *(int *)0x4081959c = (int *)(param_1 + 0x30);
        iVar1 = patched_pp_post(5,0);
        uVar2 = 0;
        if (iVar1 != 0) 
        {
            uVar2 = 0x3012;
        }
    }
    else
    {
        ieee80211_recycle_cache_eb(param_1);
        uVar2 = 0x3002;
    }
    return uVar2;
}

bool patched_pp_post(uint32_t param_1, uint32_t param_2)
{
    char *pcVar1;
    uint32_t uVar2;
    int iVar3;
    uint32_t uVar4;
    uint32_t uVar5;
    void *pcVar6;
    int iStack_2c;
    uint32_t uStack_28;
    uint32_t uStack_24;

    uStack_28 = param_1;
    uStack_24 = param_2;

    uint8_t buffer[24];
    uint8_t snd_buffer[20];

    if (param_1 < 0x12)
    {
        uVar2 = esp_coex_common_int_disable_wrapper(g_intr_lock_mux);
        pcVar1 = (char *)((char *)pp_sig_cnt_ptr + param_1);
        if (*(char *)pcVar1 == '\0')
        {
            if (param_1 != 0xd)
            {
                goto LAB_4000d2f4;
            }
        }
        else 
        {
            if (2 < param_1 - 6)
            {
                    goto LAB_4000d262;
            }
            LAB_4000d2f4:
                *pcVar1 = *pcVar1 + '\x01';
        }

        esp_coex_common_int_restore_wrapper(g_intr_lock_mux, uVar2);

        if (param_1 == 0xd)
        {
            uVar5 = uxQueueMessagesWaiting(xphyQueue);
            if (0x95 < uVar5)
            {
                return true;
            }
            if (0x30 < *(uint32_t *)(pTxRx + 0x3fc))
            {
                return true;
            }
        }

        uVar2 = xphyQueue;
        uVar4 = task_ms_to_tick_wrapper(10);
        iVar3 = queue_send_wrapper(uVar2, &param_1, uVar4);

        if (iVar3 != 1)
        {
            // wifi_log(6, 0x800, 2, "pp_queue full %d-%x", param_1, param_2);
            return true;
        }
    }
    else
    {
        uVar2 = esp_coex_common_int_disable_wrapper(g_intr_lock_mux);
        pcVar1 = (char *)(param_1 + pp_sig_cnt_ptr);
        if (*(char *)pcVar1 == '\0')
        {
            *(char *)pcVar1 = *(char *)pcVar1 + '\x01';
            esp_coex_common_int_restore_wrapper(g_intr_lock_mux, uVar2);
            iVar3 = queue_send_from_isr_wrapper(xphyQueue, &param_1, &iStack_2c);
            if (iStack_2c != 0)
            {
                esp_coex_common_task_yield_from_isr_wrapper();
            }
            return iVar3 != 1;
        }
        LAB_4000d262:
            esp_coex_common_int_restore_wrapper(g_intr_lock_mux, uVar2);
    }
    return false;
}

// END OF CLOSED SOURCE FUNCTIONS


// Function to be called from all functions calling the unpatched lmacTxFrame function
void call_patched_lmacTxFrame(int param_1, int param_2) {
    counter_patched_lmacTxFrame++;
    ESP_LOGI(TAG, "lmacTxFrame called %d times", counter_patched_lmacTxFrame);
    // x_wrapper_function = (void (*)(int, int))0x40000c5c;
    // return x_wrapper_function(param_1, param_2);
    // Based on the counter, switch to the next element in the array "frequencies",
    // which is the channel to be used
    int next_index = (counter_patched_lmacTxFrame+6) % n_frequencies; // +6 to start from 6th channel
    int next_frequency = frequencies[next_index];
    next_frequency = 0x96c;
    ESP_LOGI(TAG, "Switching to frequency 0x%08x", next_frequency);
    // Set the frequency
    switch_channel(next_frequency, 0);

    return patched_lmacTxFrame(param_1, param_2);
    
    // uint32_t base_address = 0x4081ec28;
    // struct Packet* packet = (struct Packet*)base_address;
    // initialize_packet(packet, base_address);
    // uint32_t deadbeef_address = (uint32_t)&(packet->deadbeef);

    // struct SubStruct* substruct = (struct SubStruct*)0x4081CA14;
    // initialize_substruct(substruct, deadbeef_address);

    // // esp_rom_delay_us(1000000);
    // patched_lmacTxFrame(packet, 0);
    // // esp_rom_delay_us(1000000);
    // patched_lmacTxFrame(packet, 0);
    // // esp_rom_delay_us(1000000);
    // patched_lmacTxFrame(packet, 0);
    // // esp_rom_delay_us(1000000);
    // patched_lmacTxFrame(packet, 0);
    // // esp_rom_delay_us(1000000);

    // return;
}

// Insert cafebabe into the paylod (substitute for esp_fill_random)
void esp_fill_cafebabe(void *buf, size_t len)
{
    assert(buf != NULL);
    uint8_t *buf_bytes = (uint8_t *)buf;
    while (len > 0) {
        uint32_t word = 0xcafebabe;
        uint32_t to_copy = MIN(sizeof(word), len);
        memcpy(buf_bytes, &word, to_copy);
        buf_bytes += to_copy;
        len -= to_copy;
    }
}

void esp_fill_de_bruijn(void *buf)
{
    assert(buf != NULL);
    char de_bruijn[] = "aaaabaaacaaadaaaeaaafaaagaaahaaaiaaaj"
                        "aaakaaalaaamaaanaaaoaaapaaaqaaaraaasa"
                        "aataaauaaavaaawaaaxaaayaaazaabbaabca";
    uint8_t *buf_bytes = (uint8_t *)buf;
    size_t de_bruijn_len = strlen(de_bruijn);
    memcpy(buf_bytes, de_bruijn, de_bruijn_len);
}

// Dummy function to prevent optimization
void force_include_symbols() {
    lmacTxFrame(0, 0);
    // lmacSetTxFrame(0, 0);
    // lmacIsLongFrame(0);

    call_patched_lmacTxFrame(0, 0); // Dummy call to prevent optimization
    patched_lmacTxFrame(0, 0); // Dummy call to prevent optimization
    patched_lmacSetTxFrame(0, 0); // Dummy call to prevent optimization
    patched_lmacIsLongFrame(0);
    patched_esp_now_send(0, 0, 0);
    patched_mt_send(0, 0, 0);
    mt_send(0, 0, 0);
    ieee80211_send_action_vendor_spec(0, 0, 0, 0, 0, 0);
    pp_post(0, 0);
}