#ifndef _GPRS_H_
#define _GPRS_H_

#include "sys.h"

// HTTP相关AT命令
#define HTTP_CFG "AT+QHTTPCFG="
#define HTTP_URL "AT+QHTTPURL="
#define HTTP_GET "AT+QHTTPGET"
#define HTTP_POST "AT+QHTTPPOST="
#define HTTP_PUT "AT+QHTTPPUT="
#define HTTP_READ "AT+QHTTPREAD="
#define HTTP_STOP "AT+QHTTPSTOP"

// HTTP配置参数
#define HTTP_CFG_CUSTOM_HEADER "custom_header"
#define HTTP_CFG_CONTENT_TYPE "contenttype"
#define HTTP_CFG_AUTH "auth"
#define HTTP_CFG_CLOSEWAIT "closewaittime"
#define HTTP_CFG_REQUEST_HEADER "requestheader"
#define HTTP_CFG_RESPONSE_HEADER "responseheader"

// 内容类型
#define CONTENT_TYPE_JPEG "image/jpeg"
#define CONTENT_TYPE_FORM "application/x-www-form-urlencoded"

// 最大HTTP响应长度
#define HTTP_RESPONSE_MAX_LEN 1024

// 运营商APN配置（根据SIM卡选择，三选一）
#define APN_CMNET        "CMNET"         // 中国移动
#define APN_UNINET       "UNINET"        // 中国联通
#define APN_CTNET        "CTNET"         // 中国电信
#define CURRENT_APN      APN_CMNET       // 当前使用的APN

// 设备参数结构体（已有则无需重复定义）
typedef struct {
    uint8_t sim_ready;    // SIM卡状态：0-未就绪，1-就绪
    uint8_t net_reg;      // 网络注册：0-未注册，1-注册成功
    uint8_t gprs_attach;  // GPRS附着：0-未附着，1-附着成功
    uint8_t pdp_active;   // PDP激活：0-未激活，1-激活成功
} DEVICE_PARAM_DATA_4G;



int  Write_AT_CMD(char *str,char *resq,uint32_t timeout);
void send_gprs_data(char * buf , unsigned int send_count);

//开始4g拨号
void start_4g(int mode);

//连接服务器
int connect_tcpserver(char * ipaddr,unsigned int port);

void senddata_toserver(char* serverdata,int datalen);



int config_http_param(const char* param, const char* value);
int set_http_url(const char* url);
int send_http_post(const char* data, uint32_t len) ;
int read_http_response(char* buf, uint32_t len);















#endif
