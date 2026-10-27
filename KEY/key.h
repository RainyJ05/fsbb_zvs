#ifndef __KEY_H
#define __KEY_H

#include "board.h"

void Key_Open();
void Key_Close();
void Key_Tick(void);
uint8_t Key_GetNum(void);
uint8_t Key_GetState(void);

#endif
