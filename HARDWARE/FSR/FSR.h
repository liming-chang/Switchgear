#ifndef __FSR_H
#define __FSR_H	 
#include "sys.h"


#define FSR_GPIO  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0)//读取



#define KEY_PRESS		1


void FSR_IO_Init(void);//IO初始化
u8 FSR_Scan(u8);  	//按键扫描函数
int FSR_Test(void);
#endif
