#ifndef __Motor_H
#define __Motor_H
#include "sys.h"

void Motor_GPIO_Init(void);//≥ı ºªØ
void TIM1_Init(uint32_t prescaler, uint32_t period);
void SetDirection(uint8_t dir);
void SetSpeed(uint32_t frequency);
void Motor_Start(void);
void Motor_Stop(void);
void Motor_Reset(void);
void Motor_Init(void);

void Motor_Test(void);




#endif
