#include "stm32f4xx.h"                  // Device header
#include "SERVO.h"
#include "usart.h"


/**
 *	舵机相关代码
 *
**/

void PWM_Init(void) {
    // 1. 使能GPIOE和TIM1时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 2. 配置PE9为复用推挽输出
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;        // 复用模式
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;      // 推挽输出
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOE, &GPIO_InitStruct);

    // 3. 映射PE9到TIM1_CH1
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource9, GPIO_AF_TIM1); // 关键步骤！
    
    
    // 4. 配置TIM1时基和PWM通道
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_OCInitTypeDef TIM_OCInitStruct;

    // 时基配置（20ms周期）
    TIM_TimeBaseStruct.TIM_Prescaler = 84 - 1;       // 84MHz / 84 = 1MHz → 1μs/计数
    TIM_TimeBaseStruct.TIM_Period = 20000 - 1;       // 20000计数 → 20ms
    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStruct);

    // PWM通道1配置（模式1）
    TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStruct.TIM_Pulse = 1500;               // 初始脉宽1.5ms（中位）
    TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &TIM_OCInitStruct);            // 初始化通道1

    // 5. 使能预装载和主输出
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);                           // 启动定时器
    TIM_CtrlPWMOutputs(TIM1, ENABLE);                // 关键！高级定时器需使能主输出
    
    
}


// 设置舵机角度（0°~180°）
void Servo_SetAngle(float angle) {
    if (angle < 0) angle = 0;
    if (angle > 360) angle = 360;
    
    // 计算脉宽（0.5ms~2.5ms → 500~2500个计数）
    uint16_t pulse = 500 + (angle / 180.0f) * 2000;  // 500 + (angle * 11.111)
    TIM_SetCompare1(TIM1, pulse);                    // 更新PWM脉宽
}


int SERVO_Test(void) {
    PWM_Init();          // 初始化PWM
    Servo_SetAngle(90);  // 初始位置设为90°
    
    
    while (1) {
        // 示例：舵机往复转动
        for (int angle = 0; angle <= 360; angle += 10) {
            
            printf("angle is %d \r\n",angle);
            Servo_SetAngle(angle);
            delay_ms(30);
        }
        delay_ms(1000);
        
        for (int angle = 360; angle >= 0; angle -= 10) {
            printf("angle is %d \r\n",angle);            
            Servo_SetAngle(angle);
            delay_ms(30);
        }
        delay_ms(1000);
        
//        Servo_SetAngle(0);
//        delay_ms(5000);
//        Servo_SetAngle(180);
//        delay_ms(5000);
        
        
    }
}






