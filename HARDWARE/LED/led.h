#ifndef __LED_H
#define __LED_H
#include "sys.h"

//LED端口定义
#define LED0 PCout(4)	// DS0
#define LED1 PAout(7)	// DS1

void LED_Init(void);//初始化
void Play_LED(void);//初始化
#endif
