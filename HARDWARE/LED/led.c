#include "led.h" 
#include "delay.h"
//初始化PF9和PF10为输出口.并使能这两个口的时钟		    

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
// 定义LED引脚（根据实际硬件修改）
#define LED0_PORT    GPIOF
#define LED0_PIN     GPIO_Pin_9
#define LED1_PORT    GPIOF
#define LED1_PIN     GPIO_Pin_10

//LED IO初始化
void LED_Init(void)
{    	 
    // 1. 使能GPIO端口时钟（GPIOF属于AHB1总线）
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
    
    // 2. 配置GPIO结构体
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = LED0_PIN | LED1_PIN; // 同时配置LED0和LED1的引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;      // 输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     // 推挽输出（适合驱动LED）
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // 输出速度50MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;   // 无上下拉（LED通常通过外部电阻上拉/下拉）
    
    // 3. 初始化GPIO端口
    GPIO_Init(LED0_PORT, &GPIO_InitStructure);
    
    // 初始状态：熄灭LED（假设高电平熄灭，低电平点亮，根据硬件调整）
    GPIO_SetBits(LED0_PORT, LED0_PIN);
    GPIO_SetBits(LED1_PORT, LED1_PIN);

}








