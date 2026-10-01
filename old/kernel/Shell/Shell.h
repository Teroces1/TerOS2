#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

void SHELL_Init();
void SHELL_Render();

void SHELL_OnInput(uint16_t key);

void SHELL_putChar(char c);
void SHELL_Print(char *str);

#endif