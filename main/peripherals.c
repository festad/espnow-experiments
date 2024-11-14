#include "peripherals.h"

void my_hal_he_set_power_save(int param_1)
{
    if (param_1 == 0) 
    {
        *(uint32_t *)0x600a40a0 = *(uint32_t *)0x600a40a0 | 0x30000000;
    }
    else 
    {
        *(uint32_t *)0x600a40a0 = *(uint32_t *)0x600a40a0 & 0xcfffffff;
    }
    return;
}

void macchanger(char mac[])
{
    // We need to copy the 6 bytes of the MAC address
    // to the address 0x40818dbc and to the address 0x40818dc2

    for (int i = 0; i < 6; i++)
    {
        *(uint8_t *)(0x40818dbc + i) = mac[6-i-1];
        *(uint8_t *)(0x40818dc2 + i) = mac[6-i-1];
    }
}

void hw_macchanger(uint8_t mac[])
{
    // Ensure we write the MAC address in reverse byte order
    // to 0x600b0844 (for the first 4 bytes) and 0x600b0848 (for the next 4 bytes).

    // Write the first 4 bytes of the MAC address to 0x600b0844.
    *(uint32_t *)(0x600b0844) = (mac[3] << 24) | (mac[2] << 16) | (mac[1] << 8) | mac[0];

    // Write the next 2 bytes of the MAC address to 0x600b0848.
    *(uint32_t *)(0x600b0848) = (mac[5] << 8) | mac[4];
}

void write_to_memory(uint32_t address, uint8_t buffer[], int size)
{
    for (int i = 0; i < size; i++)
    {
        *(uint8_t *)(address + i) = buffer[i];
    }
}