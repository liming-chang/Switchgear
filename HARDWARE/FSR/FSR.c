#include "stm32f4xx.h"                  // Device header      
#include "FSR.h"
#include "sys.h" 
#include "delay.h"
#include "usart.h"
/**
 *	压力传感器相关代码
 *
**/

								    
//按键初始化函数
void FSR_IO_Init(void) //IO初始化
{ 	
              
    // 1. 使能GPIOB和SYSCFG时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//使能GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    // 2. 配置PB0为输入模式，下拉电阻
    GPIO_InitTypeDef  GPIO_InitStructure; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOB,&GPIO_InitStructure);
    GPIO_ResetBits(GPIOB,GPIO_Pin_0);



    // 3. 将PB0映射到EXTI0中断线
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource0);

    // 4. 配置EXTI0中断参数
    EXTI_InitTypeDef EXTI_InitStruct;
    EXTI_InitStruct.EXTI_Line = EXTI_Line0;                 // 选择EXTI0
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;        // 中断模式
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;     // 上升沿触发
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;                  // 使能EXTI线
    EXTI_Init(&EXTI_InitStruct);    
    
    
    // 5. 配置NVIC优先级
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;               // EXTI0中断通道
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 5;      // 抢占优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 5;             // 子优先级
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;                // 使能中断
    NVIC_Init(&NVIC_InitStruct);
    
    

}

// 在stm32f4xx_it.c中实现以下函数：
void EXTI0_IRQHandler(void) {
    
    delay_ms(10);//去抖动 
    
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) { // 检查EXTI0中断标志
        // 用户自定义操作（如翻转LED、发送信号等）
        
        if(FSR_GPIO==1)  printf("大于阀值\r\n");
        
        EXTI_ClearITPendingBit(EXTI_Line0);      // 必须清除中断标志！
    }
}


