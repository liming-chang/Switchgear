#include "stm32f4xx.h"                  // Device header 
#include "stdio.h"
#include "string.h"
#include "MQTT.h" 
#include "usart.h"
#include "usart2.h"
#include "4g.h"
#include "delay.h"
#include "Motor.h"
#include "FSR.h"

extern uint8_t   USART2_RxBuf[USART2_RXBUF_LEN];

/***
    *   函数名称：Timer_Init
    *   函数功能：初始化定时器TIM5，每过30s向云服务器发送一个心跳
    *   返回参数：无
*/
void Timer_Init(void)   {
    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    // 1. 使能时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

    // 2. 配置时基
    TIM_InitStruct.TIM_Prescaler = 8400 - 1;         // 84MHz/8400 = 10kHz
    TIM_InitStruct.TIM_Period = 10000 - 1;           // 10kHz/10000 = 1Hz (1秒)
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM5, &TIM_InitStruct);

    // 3. 使能更新中断
    TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);

    // 4. 配置NVIC
    NVIC_InitStruct.NVIC_IRQChannel = TIM5_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    // 5. 启动定时器
    TIM_Cmd(TIM5, ENABLE);
}
uint8_t PING_FLAG = 0;
uint32_t heartbeat_counter = 0;
/***
*   定时器TIM5的中断处理函数
*/
void TIM5_IRQHandler(void) {
    if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
        
        heartbeat_counter++;
        if(heartbeat_counter >= 30) {  // 30秒触发
            heartbeat_counter = 0;
            PING_FLAG = 1 ;
            
        }
    }
}





/***
*函数名称：StartPing
*函数功能：定期向云平台发送心跳信息
*输入参数：无
*返回参数：无
*/
void StartPing(uint32_t arr, uint32_t psc){
    
     Timer_Init();     //初始化定时器TIM5，定时向云平台发送心跳信息
}
    

/***
*函数名称：MQTT_Ping
*函数功能：向云服务器发送心跳信息
*输入参数：无
*返回参数：0 ：成功，1 ： 失败
*/
uint8_t MQTT_Ping(void){

    char  cmdbuff[256] = { 0 };
		sprintf(cmdbuff,"cmd=0&msg=ping\r\n");
    
	  char ATCommand[128] = { 0 };
		sprintf(ATCommand,"AT+QISEND=0,%d\r\n",strlen(cmdbuff ));
		
//		Write_AT_CMD(ATCommand,">",200);
//		Write_AT_CMD(cmdbuff,"OK",200);
		
	
		
		int timeout = 25;
		clear_buf_uart2();
		
		USART2_sendstr(ATCommand);
		
		int ms = timeout;
		while(ms > 0){
			ms-=1;
			delay_ms(1);
			
      // 检查是否收到期望响应
      if (strstr((char*)USART2_RxBuf, ">")) {
           // 成功收到响应
				USART2_sendstr(cmdbuff);
				
				ms = timeout;
				while(ms > 0){
					
					ms -= 1;
					delay_ms(1);
					if (strstr((char*)USART2_RxBuf, "cmd=0&res=1")){
//							printf("Ping发送成功！\n");
						return 0;
					}
					
					
				}		
				
				
      }			
		
		}
		
//		printf("Ping发送失败！\n");
		
    return 0;
}
/***
*函数名称：MQTT_Subscribe
*函数功能：订阅主题
*输入参数：topic ： 订阅的主题
*返回参数：0 ：成功，1 ： 失败
*/
uint8_t MQTT_Subscribe(char * uid,char *topic){

    printf("订阅主题[%s]！\r\n", topic);
    
    char  cmdbuff[256] = { 0 };
		sprintf(cmdbuff,"cmd=1&uid=%s&topic=%s\r\n",uid,topic);
    
	  char ATCommand[128] = { 0 };
		sprintf(ATCommand,"AT+QISEND=0,%d\r\n",strlen(cmdbuff ));
		
		
		Write_AT_CMD(ATCommand,">",200);    
		
		if(Write_AT_CMD(cmdbuff,"cmd=1&res=1",1000) != 0 ){
        
        printf("订阅主题[%s]失败！\r\n",topic);
        return 1;
    }  
    return 0;                
}   

/***
*函数名称：MQTT_Publish
*函数功能：向指定主题发送数据
*输入参数：uid ： 用户标识符 ,topic ：订阅的主题，data : 发送的数据
*返回参数：0 ：成功，1 ： 失败
*/
uint8_t MQTT_Publish(char * uid,char *topic, char *data){
    
    printf("推送数据[msg = %s]！\r\n",data);

    char  cmdbuff[256] = { 0 };    
    sprintf(cmdbuff,"cmd=2&uid=%s&topic=%s/set&msg=%s\r\n",uid,topic,data); 
		
	  char ATCommand[128] = { 0 };    
		sprintf(ATCommand,"AT+QISEND=0,%d\r\n",strlen(cmdbuff ));
		
		Write_AT_CMD(ATCommand,">",200);
		
    
    if(Write_AT_CMD(cmdbuff,"cmd=2&res=1",1000) != 0 ) {
        
        printf("推送数据[msg = %s]失败！\r\n",data);
        return 1;
    }                   
    return 0;         
}


void Handle_Data(char * data){
    
//    int counter;
    
            printf("%s\r\n",data);
						
	if(strstr(data,Motor_TOPIC)){
		
		        if(strstr(data,"msg=right")){
                Motor_Start();
                SetDirection(0);
                delay_ms(250);
                Motor_Stop();  
                
            }else if(strstr(data,"msg=left")){
                Motor_Start();
                SetDirection(1);
                delay_ms(250);
                Motor_Stop();             
                                            
            }else if(strstr(data,"msg=on")){
                Motor_Start();
                SetDirection(0);
                
                delay_ms(1500);
                                 
                                    
                Motor_Stop();               
                Motor_Reset();   
                
            }else if(strstr(data,"msg=off")){
                Motor_Start();
                SetDirection(0);
                
                printf("Motor Close!");
                delay_ms(1500);
                
                

                Motor_Stop();
                Motor_Reset();
            
            }else if(strstr(data,"msg=reset")){
                Motor_Reset();
            
            }else {
                 // 指令错误
            } 
	
	}else if(strstr(data,SIGNAL_TOPIC)){
		
			
	
	}else if(strstr(data,PICTURE_TOPIC)){
		
		
		
	}else if(strstr(data,TAKE_TOPIC)){
		
			
		
	}else{
	
	
	}

           
}

void MQTT_Test(void){
    
    
// 
//    ESP8266_Init();
//    Timer_Init(10000 - 1, 3600 - 1);     
//    
//    LED_Init();
//    DHT11_Init();
//                            

//    uint8_t temperature = 0;
//    uint8_t humidity = 0;    
//    char  buff[256] = { 0 };
//    if(MQTT_Subscribe(UID,LED_TOPIC) != 0 )  Serial_SendString("LED订阅成功！\r\n"); 
//    if(MQTT_Subscribe(UID,DHT11_TOPIC) != 0 )  Serial_SendString("DHT11订阅成功！\r\n"); 
       
    while(1){
        
        delay_ms(5000);   
        
//        
//        // 每过一段时间提交外设当前的状态
//        if(GPIO_ReadOutputDataBit(LED0_GPIO,LED0_Pin) == RESET )
//            MQTT_Publish(UID,LED_TOPIC,"on#");
//        else
//            MQTT_Publish(UID,LED_TOPIC,"off#");                
//        
//        DHT11_ReadData(&temperature,&humidity);
//        sprintf(buff,"%d#%d#", temperature,humidity);
//        MQTT_Publish(UID,DHT11_TOPIC,buff);        
        
        
    }    
    
    
}

