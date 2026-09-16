#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <stdint.h>

void WINDOW_Init(void);

void WINDOW_Render(void);

void WINDOW_OnInput(uint16_t code);

#endif