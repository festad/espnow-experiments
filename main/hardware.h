#pragma once

#define _MMIO_DWORD(mem_addr) (*(volatile uint32_t *)(mem_addr))
#define _MMIO_ADDR(mem_addr) ((volatile uint32_t*)(mem_addr))

void transmit_one(uint8_t index);
void setup_tx_buffers();
void set_datarate(uint8_t datarate, uint32_t length);
