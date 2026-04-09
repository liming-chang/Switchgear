#include "stm32f4xx.h"                  // Device header
#include "Motor.h"
#include "delay.h"
#include "usart.h"

/**
 * 步进电机相关代码
 * 优化点：
 * 1. 增加详细调试信息，追踪各阶段状态和步数
 * 2. 修复初始化阶段步数计算逻辑
 * 3. 强化阶段切换的电机控制时序
 * 4. 增加边界条件保护
**/

int32_t step_count = 0;          // 当前步数计数
uint8_t current_dir = 0;         // 当前方向(0:反向, 1:正向)
uint8_t is_resetting = 0;        // 普通复位标志
uint8_t reset_dir = 0;           // 普通复位方向
uint8_t is_init_move_1dir = 0;   // 初始化第一阶段标志(向1方向移动)
uint8_t is_init_move_0dir = 0;   // 初始化第二阶段标志(向0方向移动)
uint32_t init_1dir_steps = 0;    // 第一阶段已走步数
uint32_t init_0dir_steps = 0;    // 第二阶段已走步数
#define MAX_STEP_0DIR 2500        // 0方向最大步数限制(从原点向0方向最多4000步)
#define INIT_1DIR_TIME_MS 10000    // 第一阶段(1方向)移动时间(ms)
#define INIT_0DIR_TIME_MS 5000    // 第二阶段(0方向)移动时间(ms)

// 定时器参数
static uint32_t step_interval_ms = 0;       // 每步时间(ms)
static uint32_t init_1dir_total_steps = 0;  // 第一阶段总步数
static uint32_t init_0dir_total_steps = 0;  // 第二阶段总步数

void Motor_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;

    // 使能GPIOB时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    // 配置PB1为TIM3通道4复用功能(输出PWM)
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource1, GPIO_AF_TIM3);  // PB1对应TIM3_CH4

    // 配置PB2为方向控制输出(初始化为低电平)
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_WriteBit(GPIOB, GPIO_Pin_2, Bit_RESET);
}

void TIM3_Init(uint32_t prescaler, uint32_t period) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_OCInitTypeDef TIM_OCInitStruct;

    // 使能TIM3时钟(APB1时钟为84MHz)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 配置时基
    TIM_TimeBaseStruct.TIM_Prescaler = prescaler;
    TIM_TimeBaseStruct.TIM_Period = period;
    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStruct);

    // 配置PWM模式 - 通道4
    TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStruct.TIM_Pulse = period / 2;  // 50%占空比
    TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC4Init(TIM3, &TIM_OCInitStruct);
    TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);

    // 使能自动重装载预装载
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    
    // 启用更新中断
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);    

    // 配置NVIC(确保中断优先级合理)
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;  // 非最高优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    // 计算每步时间和各阶段总步数(关键参数验证)
    step_interval_ms = ((prescaler + 1) * (period + 1)) / 84000;  // 84MHz/1000=84000
    init_1dir_total_steps = INIT_1DIR_TIME_MS / step_interval_ms;
    init_0dir_total_steps = INIT_0DIR_TIME_MS / step_interval_ms;

    // 打印定时器参数(调试关键)
    printf("TIM3配置: 每步时间=%dms, 1方向总步数=%lu, 0方向总步数=%lu\r\n",
           step_interval_ms, init_1dir_total_steps, init_0dir_total_steps);

    // 启动定时器(暂时不使能PWM输出)
    TIM_Cmd(TIM3, ENABLE);
}

// TIM3更新中断服务函数(核心逻辑)
void TIM3_IRQHandler(void) {
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        
        // 1. 初始化第一阶段：向1方向移动5秒
        if (is_init_move_1dir) {
            init_1dir_steps++;
            step_count++;  // 1方向步数递增

            // 打印阶段进度(每100步打印一次，避免刷屏)
            if (init_1dir_steps % 100 == 0) {
//                printf("1方向移动中: 已走%lu/%lu步, 当前位置=%d\r\n",
//                       init_1dir_steps, init_1dir_total_steps, step_count);
            }

            // 第一阶段完成条件
            if (init_1dir_steps >= init_1dir_total_steps) {
                Motor_Stop();
                is_init_move_1dir = 0;
//                printf("第一阶段完成: 总移动%d步, 位置=%d\r\n", 
//                       init_1dir_steps, step_count);

                // 切换到第二阶段：向0方向移动2秒(延迟100ms确保电机停稳)
                delay_ms(100);
                is_init_move_0dir = 1;
                init_0dir_steps = 0;
                current_dir = 0;
                GPIO_WriteBit(GPIOB, GPIO_Pin_2, Bit_RESET);  // 设置0方向
                Motor_Start();
                printf("开始第二阶段: 向0方向移动2秒\r\n");
                return;
            }
            return;
        }

        // 2. 初始化第二阶段：向0方向移动2秒
        if (is_init_move_0dir) {
            init_0dir_steps++;
            step_count--;  // 0方向步数递减

            // 打印阶段进度
//            if (init_0dir_steps % 100 == 0) {
//                printf("0方向移动中: 已走%lu/%lu步, 当前位置=%d\r\n",
//                       init_0dir_steps, init_0dir_total_steps, step_count);
//            }

            // 第二阶段完成条件
            if (init_0dir_steps >= init_0dir_total_steps) {
                Motor_Stop();
                is_init_move_0dir = 0;
                // 最终位置设为原点
//                int32_t final_pos = step_count;
                step_count = 0;
//                printf("第二阶段完成: 总移动%d步, 原点确立(原位置=%d → 新位置=0)\r\n",
//                       init_0dir_steps, final_pos);
                return;
            }
            return;
        }

        // 3. 正常运行时更新步数
        if (current_dir) {
            // 1方向运动：无限制
            step_count++;
        } else {
            // 0方向运动：限制最大5000步(从原点开始计算)
            step_count--;
            if (step_count <= -MAX_STEP_0DIR) {
                Motor_Stop();
                step_count = -MAX_STEP_0DIR;
                printf("0方向已达最大限制(%d步), 已停止\r\n", -MAX_STEP_0DIR);
                return;
            }
        }

        // 4. 处理普通复位逻辑
        if (is_resetting) {
            if ((reset_dir == 0 && step_count <= 0) || 
                (reset_dir == 1 && step_count >= 0)) {
                Motor_Stop();
                step_count = 0;
                is_resetting = 0;
                printf("普通复位完成: 回到原点\r\n");
            }
        }
    }
}

/**
 * 普通复位函数：回到原点(0位置)
 */
void Motor_Reset(void) {
    // 初始化阶段或已在原点时不执行复位
    if (step_count == 0 || is_resetting || is_init_move_1dir || is_init_move_0dir) {
        printf("复位失败: 无效状态(step_count=%d, 初始化中=%d)\r\n",
               step_count, (is_init_move_1dir || is_init_move_0dir));
        return;
    }
    
    reset_dir = (step_count > 0) ? 0 : 1;  // 向原点方向运动
    SetDirection(reset_dir);
    is_resetting = 1;
    Motor_Start();
    printf("开始普通复位: 从位置%d向%d方向移动\r\n", step_count, reset_dir);
}

/**
 * 设置运动方向
 * @param dir: 0-向0方向, 1-向1方向
 */
void SetDirection(uint8_t dir) {
    // 初始化或复位阶段禁止改变方向
    if (is_resetting || is_init_move_1dir || is_init_move_0dir) {
        printf("方向设置失败: 初始化或复位中\r\n");
        return;
    }
    
    current_dir = dir;
    GPIO_WriteBit(GPIOB, GPIO_Pin_2, dir ? Bit_SET : Bit_RESET);
    printf("方向已设置为: %d(引脚电平=%d)\r\n", 
           dir, GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_2));
}

void Motor_Start(void) {
    // 检查启动条件
    if (!is_init_move_1dir && !is_init_move_0dir && !is_resetting) {
        // 非初始化/复位阶段，检查0方向限制
        if (step_count <= -MAX_STEP_0DIR) {
            printf("启动失败: 0方向已达最大限制\r\n");
            return;
        }
    }
    
    TIM_CtrlPWMOutputs(TIM3, ENABLE);  // 先使能PWM输出
    TIM_Cmd(TIM3, ENABLE);             // 再启动定时器
    printf("电机已启动(当前方向=%d, 位置=%d)\r\n", current_dir, step_count);
}

void Motor_Stop(void) {
    TIM_Cmd(TIM3, DISABLE);            // 先停止定时器
    TIM_CtrlPWMOutputs(TIM3, DISABLE); // 再关闭PWM输出
    printf("电机已停止(当前位置=%d)\r\n", step_count);
}

/**
 * 电机初始化完整流程：
 * 1. 向1方向移动5秒
 * 2. 向0方向移动2秒
 * 3. 停止位置设为原点(step_count = 0)
 */
void Motor_Init(void) {
    // 初始化前重置所有状态
    step_count = 0;
    current_dir = 0;
    is_resetting = 0;
    is_init_move_1dir = 0;
    is_init_move_0dir = 0;
    
    Motor_GPIO_Init();
    // 初始化定时器(168分频+1000周期=每步2ms，500步/秒)
    TIM3_Init(167, 999);  
    Motor_Stop();
    
    // 启动第一阶段：向1方向移动5秒
    printf("===== 开始初始化流程 =====\r\n");
    is_init_move_1dir = 1;
    init_1dir_steps = 0;
    current_dir = 1;
    GPIO_WriteBit(GPIOB, GPIO_Pin_2, Bit_SET);  // 设置1方向
    printf("初始化第一阶段: 向1方向移动5秒(引脚电平=%d)\r\n",
           GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_2));
    Motor_Start();
}

/**
 * 电机测试函数：验证完整流程
 */
void Motor_Test(void){
    Motor_Init();
    
    // 等待初始化两个阶段完成
    printf("等待初始化完成...\r\n");
    while(is_init_move_1dir || is_init_move_0dir);
    printf("===== 初始化全部完成，开始测试 =====\r\n");
	
	
	
	  SetDirection(0);
    Motor_Start();
    delay_ms(10000);
    Motor_Stop();
	
		Motor_Reset();
    
    // 测试流程：1方向移动→0方向移动→复位
    while(1){
        // 向1方向运动3秒
//        SetDirection(1);
//        Motor_Start();
//        delay_ms(3000);
//        Motor_Stop();
//        delay_ms(1000);
//        
//        // 向0方向运动2秒
//        SetDirection(0);
//        Motor_Start();
//        delay_ms(2000);
//        Motor_Stop();
//        delay_ms(1000);
//        
//        // 复位到原点
//        Motor_Reset();
//        delay_ms(3000);
    }
}
