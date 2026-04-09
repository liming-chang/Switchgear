#ifndef _JPEGSEND_H_
#define _JPEGSEND_H_
#include <string.h>
#include <stdio.h>
#include "sys.h"
#include "delay.h"
#include "usart.h"

uint32_t send_big_block_data(char * send_data_buf ,uint32_t data_len);

#endif

