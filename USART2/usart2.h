#ifndef __USART2_H
#define __USART2_H 
#include "sys.h"
#include "stdio.h"	  

#define USART2_TXBUF_LEN 16384
#define USART2_RXBUF_LEN  1024 //256 
#define USART3_RXBUF_LEN 128

#define UART5_TXBUF_LEN 128
#define UART5_RXBUF_LEN 128

extern uint16_t USART2_TxHead ;
extern uint16_t USART2_TxTail ;

// 帧接收状态跟踪
typedef enum {
    FRAME_IDLE,     // 空闲状态，等待数据
    FRAME_RECEIVING // 已收到\r，等待\n
} FrameState;

extern FrameState frame_state ;
extern uint8_t is_frame_ready;


extern uint16_t USART2_RxHead;
extern uint16_t USART2_RxTail;

void usart2_init(u32 bound); 
void usart3_init(u32 bound);
void uart5_init(u32 bound);
void USART2_sendstr(char *data);
void clear_buf_uart2(void);
void clear_buf_uart3(void);
void USART2_senddata(char *data,int len);
void UART2_SendChar(char ch);
void UART5_SendChar(char ch);
void clear_buf_uart5(void);
void USART2_SendChar(char ch);
#endif	   
















