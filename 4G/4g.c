#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "usart2.h"  
#include "4g.h"
#include "delay.h"
#include "iap.h"

extern uint8_t  USART2_RxBuf[USART2_RXBUF_LEN];
extern uint8_t  USART3_RxBuf[USART2_RXBUF_LEN];

//设备参数信息
extern DEVICE_PARAM_DATA_4G g_deviceparam_data;

#define GNSS_CREG         "AT+CREG?;+CGREG?\r\n" // 检测网络状态
#define GNSS_QGNSSTS      "AT+QGNSSTS?\r\n"      // 查询时间同步状态
#define GNSS_QEPOSTATUS   "AT+QGNSSEPO?\r\n"
#define GNSS_QGNSSEPO     "AT+QGNSSEPO=1\r\n"     // 使能EPO功能
#define GNSS_QGEPOAID     "AT+QGEPOAID\r\n"
#define GNSS_QGNSSRD      "AT+QGNSSRD?\r\n"
#define GPRS_SIGNAL_STRENGTH "AT+CSQ\r\n"          // 获取信号强度
#define GPRS_BAUDRATE     "AT+IPR=921600\r\n"     // 设置波特率（需与USART2一致）
#define SIMCARD_CMD       "AT+CPIN?\r\n"         // 查询SIM卡状态
#define GPRS_PDP_CONTEXT  "AT+QIFGCNT=0\r\n"      // 配置PDP上下文计数
#define GPRS_CREG_CMD     "AT+CREG?\r\n"         // 查询网络注册
#define GPRS_NETWORK_CHECK "AT+CGATT?\r\n"        // 查询GPRS附着
#define GPRS_ADD_IP_HEAD  "AT+QIHEAD=1\r\n"       // 增加IP头信息
#define GPRS_DISCONNECT   "AT+QICLOSE=0\r\n"      // 断开TCP连接
#define GPRS_CMD_MODE     "AT+QIMODE=0\r\n"       // 命令模式
#define GPRS_DATA_MODE    "AT+QIMODE=1\r\n"       // 透传模式
#define GPRS_SET_SMSMODE  "AT+CMGF=1\r\n"         // 短信文本模式
#define GPRS_SET_GSMMODE  "AT+CSCS=\"GSM\"\r\n"  // GSM编码
#define GPRS_SET_HEXMODE  "AT+CSCS=\"HEX\"\r\n"   // HEX编码
#define SMS_DELETE_ALL_SMS "AT+CMGD=1,4\r\n"      // 删除所有短信
#define CALL_NUM_DISPLAY  "AT+CLIP=1\r\n"        // 显示来电号码

// 新增：PDP上下文配置命令（带APN）
#define QICSGP_CMD_FORMAT "AT+QICSGP=1,1,\"%s\",\"\",\"\",1\r\n"  // cid=1,auth=1,APN,user=空,pwd=空,dns=1
// 新增：PDP激活命令
#define QIACT_CMD        "AT+QIACT=1\r\n"       // 激活PDP上下文（cid=1）



/**
 * 发送AT命令到板子并等待响应
 * @param str AT命令字符串
 * @param resq 期望的响应关键字
 * @param timeout_seconds 超时时间（毫秒）
 * @return 0-成功，1-超时，2-达到最大重传次数，3-其他错误
 */
/**
 * 发送AT命令并等待响应（原有函数保留，优化缓冲区清空）
 */
int Write_AT_CMD(char *str, char *resq, uint32_t timeout) {
    uint32_t ms = timeout;
    const int MAX_RETRIES = 2;
    int retry_count = 0;
    
    do {
        clear_buf_uart2();  // 清空接收缓冲区
        ms = timeout;
        
        printf("ATCMD --> %s", str);  // 打印命令（str已包含\r\n）
        if (strlen(str) > 0) USART2_sendstr(str);
        
        while (ms > 0) {
            ms--;
            delay_ms(1);
            // 检查是否收到期望响应
            if (strstr((char*)USART2_RxBuf, resq)) {
                printf("ATREV <-- %s\n", USART2_RxBuf);
                clear_buf_uart2();
                return 0;  // 成功
            }			
        }
        // 超时重试
        printf("AT超时: %s, 重试%d次\n", str, retry_count+1);
        retry_count++;
        clear_buf_uart2();
    } while (retry_count < MAX_RETRIES);
    
    return 1;  // 失败
}
void start_4g(int mode) {
    char cmd[64] = {0};
    g_deviceparam_data.sim_ready = 0;
    g_deviceparam_data.net_reg = 0;
    g_deviceparam_data.gprs_attach = 0;
    g_deviceparam_data.pdp_active = 0;

    // 1. 关闭回显（避免串口干扰）
    Write_AT_CMD("ATE0\r\n", "OK", 2000);
    // 2. 查询模块信息（确认模块正常）
    Write_AT_CMD("ATI\r\n", "Quectel", 2000);  // 移远模块返回"Quectel"

    // 3. 检查SIM卡状态（必须返回+CPIN: READY）
    if (Write_AT_CMD(SIMCARD_CMD, "READY", 2000) == 0) {
        g_deviceparam_data.sim_ready = 1;
        printf("SIM卡就绪\n");
    } else {
        printf("SIM卡异常（检查插卡/ PIN码）\n");
        return;
    }

    // 4. 检查网络注册（必须返回+CREG: 0,1或0,5）
    if (Write_AT_CMD(GPRS_CREG_CMD, "+CREG: 0,1", 2000) == 0 || 
        Write_AT_CMD(GPRS_CREG_CMD, "+CREG: 0,5", 2000) == 0) {
        g_deviceparam_data.net_reg = 1;
        printf("网络注册成功\n");
    } else {
        printf("网络注册失败（检查信号/运营商）\n");
        return;
    }

    // 5. 检查GPRS附着（必须返回+CGATT:1）
    if (Write_AT_CMD("AT+CGATT=1\r\n", "OK", 2000) == 0) {
        g_deviceparam_data.gprs_attach = 1;
        printf("GPRS附着成功\n");
    } else {
        printf("GPRS附着失败\n");
//        Write_AT_CMD("AT+CGATT=1\r\n", "OK", 2000);  // 主动附着
        return;
    }

    // 6. 配置PDP上下文（补充APN，关键修复！）
    sprintf(cmd, QICSGP_CMD_FORMAT, CURRENT_APN);  // 填入当前APN
    if (Write_AT_CMD(cmd, "OK", 2000) == 0) {
        printf("PDP上下文配置成功（APN: %s）\n", CURRENT_APN);
    } else {
        printf("PDP配置失败（检查APN正确性）\n");
        return;
    }

    // 7. 激活PDP上下文（关键修复！原有代码注释，需取消）
    if (Write_AT_CMD(QIACT_CMD, "OK", 2000) == 0) {
        g_deviceparam_data.pdp_active = 1;
        printf("PDP上下文激活成功\n");
    } else {
        printf("PDP激活失败（检查APN/网络）\n");
        return;
    }

    // 8. 根据模式配置（连接巴法云用透传模式）
    if (mode == 0) {
        Write_AT_CMD(GPRS_CMD_MODE, "OK", 200);
        Write_AT_CMD(GPRS_ADD_IP_HEAD, "OK", 200);
        Write_AT_CMD(GPRS_SET_SMSMODE, "OK", 200);
        Write_AT_CMD(GPRS_SET_HEXMODE, "OK", 200);
        printf("已进入命令模式\n");
    } else if (mode == 1) {
        Write_AT_CMD(GPRS_DATA_MODE, "OK", 200);  // 透传模式（TCP数据直接转发）
        Write_AT_CMD(GPRS_SET_HEXMODE, "OK", 200);
        printf("已进入透传模式（准备连接巴法云）\n");
    }
}



/**************************************************************************************************
函数名:  CLOSE_IP
功能描述：退出连接
输入参数：无
函数返回值：退出成功返回1 失败返回0
关闭数据 上传服务器连接
***************************************************************************************************/
void CLOSE_IP(void)
{
    Write_AT_CMD(GPRS_DISCONNECT,"OK",200);
    delay_ms(2000);
}


//返回值
//0--获取成功
//1--超时
int connect_tcpserver(char * ipaddr,unsigned int port)
{
    char tmpbuf[128] = {0x0};
    char port_str[16];
    //尝试次数
    int try_count = 0;

    char* ret_val = NULL;
    char* bret_val = NULL;
    char* dret_val = NULL;
    char* eret_val = NULL;

    memset(port_str,0,sizeof(port_str));

    clear_buf_uart2();

    sprintf(tmpbuf,"AT+QIOPEN=1,0,\"TCP\",\"%s\",%d,0,1\r\n",ipaddr,port);
    printf("tmpbuf=%s\r\n",tmpbuf);
    USART2_sendstr(tmpbuf);
    while(1)
    {
        delay_ms(3000);
        printf("USART2_RxBuf = %s\n",USART2_RxBuf);
        bret_val = (strstr((char*)USART2_RxBuf,"CONNECT FAIL"));
        dret_val = (strstr((char*)USART2_RxBuf,"ERROR"));
        ret_val = (strstr((char*)USART2_RxBuf,"OK"));
        eret_val = (strstr((char*)USART2_RxBuf,"PDP DEACT"));

        try_count++;
        if(try_count > 30)  //超时失败
        {
            printf("TCP Connect Overtime Failure.\r\n");
            clear_buf_uart2();
            return 1;
        }
        if(bret_val)    //连接失败
        {
            printf("TCP Connect link failure.\r\n");
            clear_buf_uart2();
            CLOSE_IP();
            clear_buf_uart2();
            USART2_sendstr(tmpbuf);
        }
        else if(eret_val)   //PDP激活失败
        {
            printf("PDP is not activated.\r\n");
            clear_buf_uart2();
            CLOSE_IP();
            clear_buf_uart2();
            USART2_sendstr(tmpbuf);

        }
        else if(ret_val)    //连接成功
        {
            printf("==========CONNECT OK=============\r\n");
            break;
        }
        else if(dret_val)   //莫名失败
        {
            printf("Please set up the server!!! \r\n");
            clear_buf_uart2();
            CLOSE_IP();
            clear_buf_uart2();
            USART2_sendstr(tmpbuf);
        }
        else    //没有connect
        {
            printf("Please set up the server first!!! \r\n");
            clear_buf_uart2();
            CLOSE_IP();
            clear_buf_uart2();
            USART2_sendstr(tmpbuf);
            printf("tmpbuf=%s\r\n",tmpbuf);
        }

    }
    return 0;
}

//获取信号强度
void get_signal_strength()
{
    char tempbuf[128];
    char strength_str[16];

    clear_buf_uart2();
    memset(tempbuf,0,sizeof(tempbuf));
    memset(strength_str,0,sizeof(strength_str));
    Write_AT_CMD(GPRS_SIGNAL_STRENGTH,"OK",200);
}
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
/**
 * 配置HTTP服务器参数
 * @param param 参数名
 * @param value 参数值
 * @return 0成功，非0失败
 */
int config_http_param(const char* param, const char* value) {
    char cmd[128] = {0};
    sprintf(cmd, "AT+QHTTPCFG=\"%s\",%s\r\n", param, value);
    return Write_AT_CMD(cmd, "OK",50);
}

/**
 * 设置HTTP服务器URL
 * @param url 服务器URL
 * @return 0成功，非0失败
 */
int set_http_url(const char* url) {
    char cmd[128] = {0};
    sprintf(cmd, "AT+QHTTPURL=%d,20\r\n", strlen(url));
		Write_AT_CMD(cmd, "CONNECT",500);
	
    return Write_AT_CMD((char*)url, "OK",200);
}



/**
 * 发送HTTP POST请求
 * @param data 发送数据
 * @param len 数据长度
 * @return 0成功，非0失败
 */
int send_http_post(const char* data, uint32_t len) {
    char cmd[64] = {0};
    sprintf(cmd, "AT+QHTTPPOST=%d,60,60\r\n", len);
    if (Write_AT_CMD(cmd, "CONNECT",200) != 0) return 1;
    
		printf("开始发送数据\r\n");
    // 发送数据
    USART2_senddata((char*)data, len);
		
    // 等待缓冲区数据全部发送完成
    while (USART2_TxHead != USART2_TxTail);
		
		printf("数据发送结束\r\n");
	
		int ms = 5000;
		while(ms > 0){
		ms-=1;
		delay_ms(1);
			
        // 检查是否收到期望响应
        if (strstr((char*)USART2_RxBuf, "OK")) {
            printf("ATREV <-- %s\r\n", USART2_RxBuf);
            return 0; // 成功收到响应
        }			
		
	}
	
    
    // 等待响应

    return 1;
}

/**
 * 读取HTTP响应
 * @param buf 响应缓冲区
 * @param len 缓冲区长度
 * @return 实际读取长度
 */
int read_http_response(char* buf, uint32_t len) {
	
	char str[128] = {0};
	sprintf(str,"AT+QHTTPREAD=1\r\n");
	
	int timeout = 1000;
	uint32_t ms = timeout;
	const int MAX_RETRIES = 2;
	int retry_count = 0;
	
	do{
        // 清空接收缓冲区
        clear_buf_uart2();
		
				ms = timeout;
        
        // 发送AT命令
        printf("ATCMD --> %s\r\n", str);
        USART2_sendstr(str);
		
				while(ms > 0){
					ms-=1;
					delay_ms(1);
					
					// 检查是否收到期望响应
					if (strstr((char*)USART2_RxBuf, "OK")) {
							printf("ATREV <-- %s\r\n", USART2_RxBuf);
							memcpy(buf,(char*)USART2_RxBuf,len);
							return strlen(buf); // 成功收到响应
					}			
				
		}
        // 超时处理
    printf("AT命令超时: %s, 已重试 %d 次\r\n", str, retry_count+1);
		retry_count++;
		
	}while(retry_count < MAX_RETRIES);
	
	return 0;


}



