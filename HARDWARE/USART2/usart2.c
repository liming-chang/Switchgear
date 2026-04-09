#include "sys.h"
#include "usart2.h"
#include "string.h"
#include "MQTT.h"

uint8_t USART2_TxBuf[USART2_TXBUF_LEN];
uint16_t USART2_TxHead = 0;
uint16_t USART2_TxTail = 0;
uint8_t USART2_RxBuf[USART2_RXBUF_LEN];
uint16_t USART2_RxHead = 0;
uint16_t USART2_RxTail = 0;

uint8_t USART3_RxBuf[USART3_RXBUF_LEN];
uint16_t USART3_RxHead = 0;
uint16_t USART3_RxTail = 0;

uint8_t UART5_TxBuf[UART5_TXBUF_LEN];
uint16_t UART5_TxHead = 0;
uint16_t UART5_TxTail = 0;

uint8_t UART5_RxBuf[UART5_RXBUF_LEN];
uint16_t UART5_RxHead = 0;
uint16_t UART5_RxTail = 0;

FrameState frame_state = FRAME_IDLE;
uint8_t is_frame_ready = 0;

void USART2_SendStart(void)
{
    USART_ITConfig(USART2, USART_IT_TC, ENABLE);
}

void USART2_SendChar(char ch)
{
    USART2_TxBuf[USART2_TxTail++] = (uint8_t)ch;
    USART2_TxTail &= USART2_TXBUF_LEN - 1;
    USART2_SendStart();
}

void USART2_sendstr(char *data)
{
    while (*data)
        USART2_SendChar(*data++);
}

void USART2_senddata(char *data, int len)
{
    int i = 0;
    for (i = 0; i < len; i++)
    {

        uint16_t next_tail = (USART2_TxTail + 1) & (USART2_TXBUF_LEN - 1);

        while (next_tail == USART2_TxHead)
            ;

        USART2_TxBuf[USART2_TxTail] = (uint8_t)*data++;
        USART2_TxTail = next_tail;

        USART2_SendStart();
    }
}

void UART5_SendStart(void)
{
    USART_ITConfig(UART5, USART_IT_TC, ENABLE);
}

void UART5_SendChar(char ch)
{
    UART5_TxBuf[UART5_TxTail++] = (uint8_t)ch;
    UART5_TxTail &= UART5_TXBUF_LEN - 1;
    UART5_SendStart();
}

void UART5_sendstr(char *data)
{
    while (*data)
        UART5_SendChar(*data++);
}

void UART5_senddata(char *data, int len)
{
    int i = 0;
    for (i = 0; i < len; i++)
    {
        UART5_SendChar(*data++);
    }
}

void usart3_init(u32 bound)
{

    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    // USART1_TX   PA.2 PA.3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);
    USART_Cmd(USART3, ENABLE);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void uart5_init(u32 bound)
{

    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_UART5);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_UART5);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource1, GPIO_AF_UART5);

    USART_DeInit(UART5);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(UART5, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
    USART_Cmd(UART5, ENABLE);
}

void usart2_init(u32 bound)
{

    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // USART1_TX   PA.2 PA.3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_SetBits(GPIOB, GPIO_Pin_12);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void clear_buf_uart2(void)
{
    unsigned int i = 0, c;
    c = USART2_RXBUF_LEN;
    USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);

    for (i = 0; i < c; i++)
    {
        USART2_RxBuf[i] = 0x0;
    }

    USART2_RxTail = 0;

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}

void clear_buf_uart3(void)
{
    unsigned int i = 0, c;
    c = USART3_RXBUF_LEN;
    USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);

    for (i = 0; i < c; i++)
    {
        USART3_RxBuf[i] = 0x0;
    }

    USART3_RxTail = 0;

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
}

void clear_buf_uart5(void)
{
    unsigned int i = 0, c;
    c = UART5_RXBUF_LEN;
    USART_ITConfig(UART5, USART_IT_RXNE, DISABLE);

    for (i = 0; i < c; i++)
    {
        UART5_RxBuf[i] = 0x0;
    }

    UART5_RxTail = 0;

    USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint8_t data = USART_ReceiveData(USART2);

        USART2_RxBuf[USART2_RxTail++] = USART_ReceiveData(USART2);
        USART2_RxTail &= USART2_RXBUF_LEN - 1;
    }

    if (USART_GetITStatus(USART2, USART_IT_TC) != RESET)
    {

        if (USART2_TxHead == USART2_TxTail)
        {
            USART_ITConfig(USART2, USART_IT_TC, DISABLE);
        }
        else
        {
            USART_SendData(USART2, USART2_TxBuf[USART2_TxHead++]);
            USART2_TxHead &= USART2_TXBUF_LEN - 1;
        }
    }

    if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
    {

        uint32_t temp = USART2->SR;
        temp = USART2->DR;

        is_frame_ready = 1;
    }
}

void UART5_IRQHandler(void)
{
    if (USART_GetITStatus(UART5, USART_IT_RXNE) != RESET)
    {
        UART5_RxBuf[UART5_RxTail++] = USART_ReceiveData(UART5);
        // printf("recv data=%d\r\n",UART5_RxBuf[UART5_RxTail-1]);
        UART5_RxTail &= UART5_RXBUF_LEN - 1;
    }

    if (USART_GetITStatus(UART5, USART_IT_TC) != RESET)
    {

        if (UART5_TxHead == UART5_TxTail)
        {
            USART_ITConfig(UART5, USART_IT_TC, DISABLE);
        }
        else
        {
            USART_SendData(UART5, UART5_TxBuf[UART5_TxHead++]);
            UART5_TxHead &= UART5_TXBUF_LEN - 1;
        }
    }
}

void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        USART3_RxBuf[USART3_RxTail++] = USART_ReceiveData(USART3);
        USART3_RxTail &= USART3_RXBUF_LEN - 1;
    }
}
