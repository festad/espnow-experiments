#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>

// extern uint32_t g_ic

void my_hal_he_set_power_save(int param_1);

void macchanger(char mac[]);
void hw_macchanger(uint8_t mac[]);
void write_to_memory(uint32_t address, uint8_t buffer[], int size);

#endif