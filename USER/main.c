#include <string.h>
#include <stdio.h>
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "usart2.h"
#include "timer.h"
#include "ov2640.h"
#include "dcmi.h"
#include "iap.h"
#include "adc.h"
#include "4g.h"
#include "sram.h"
#include "jpeg.h"
#include "Motor.h"
#include "FSR.h"
#include "MQTT.h"

#define IMAGES_IP "images.bemfa.com" // 修改服务器的ip地址		img.bemfa.com
#define IMAGES_PORT 8347             // 修改服务器的的端口		8347
#define SERVER_IP "119.91.109.180"   // bemfa.com				119.91.109.180
#define SERVER_PORT 8344

u8 ov2640_mode = 1; // 工作模式:0,RGB565模式;1,JPEG模式
#define jpeg_buf_tmp 250 * 1000
int jpeg_buf_size = jpeg_buf_tmp;                                      // 定义JPEG数据缓存jpeg_buf的大小
__align(4) u32 jpeg_buf[jpeg_buf_tmp] __attribute__((at(0X68000000))); // 测试用数组

volatile u32 jpeg_data_len = 0; // buf中的JPEG有效数据长度
volatile u8 jpeg_data_ok = 0;   // JPEG数据采集完成标志
// 0,数据没有采集完;
// 1,数据采集完了,但是还没处理;
// 2,数据已经处理完成了,可以开始下一帧接收
// JPEG尺寸支持列表
const u16 jpeg_img_size_tbl[][2] =
    {
        176,
        144, // QCIF
        160,
        120, // QQVGA
        352,
        288, // CIF
        320,
        240, // QVGA
        640,
        480, // VGA
        800,
        600, // SVGA
        1024,
        768, // XGA
        1280,
        960, // SXGA
        1600,
        1200, // UXGA
};
const u8 *EFFECTS_TBL[7] = {"Normal", "Negative", "B&W", "Redish", "Greenish", "Bluish", "Antique"}; // 7种特效
const u8 *JPEG_SIZE_TBL[9] = {"QCIF", "QQVGA", "CIF", "QVGA", "VGA", "SVGA", "XGA", "SXGA", "UXGA"}; // JPEG图片 分别和上面对应

extern uint8_t USART2_RxBuf[USART2_RXBUF_LEN]; // 串口2接收缓冲
extern uint8_t UART5_RxBuf[UART5_RXBUF_LEN];   // 串口5接收缓冲
extern uint16_t UART5_RxTail;

extern char p_sn[DEVICE_ID_LEN]; // 设备序列号

#define MAX_SEND_PACKETCOUNT 15

// 电池电量
float g_voltage;

// 当前时间
uint32_t g_currenttime = 0;

// 是否是夜视模式
int g_nightmode = 0;

extern uint16_t USART2_RxTail;

// 抓拍间隔时间计数
int snap_timecount = 0;
// 是否是第一次抓拍
int first_snap = 1;
// 注册统计计数，最慢1分钟抓拍一张上传
int register_count = 0;

// 设备参数结构体
extern DEVICE_PARAM_DATA_4G g_deviceparam_data;

char parse_data[1024] = {0};

// 当采集完一帧JPEG数据后,调用此函数,切换JPEG BUF.开始下一帧采集.
void jpeg_data_process(void)
{
    if (ov2640_mode) // 只有在JPEG格式下,才需要做处理.
    {
        if (jpeg_data_ok == 0) // jpeg数据还未采集完?
        {
            DMA_Cmd(DMA2_Stream1, DISABLE); // 停止当前传输
            while (DMA_GetCmdStatus(DMA2_Stream1) != DISABLE)
            {
            } // 等待DMA2_Stream1可配置
            jpeg_data_len = jpeg_buf_size - DMA_GetCurrDataCounter(DMA2_Stream1); // 得到此次数据传输的长度

            jpeg_data_ok = 1; // 标记JPEG数据采集完按成,等待其他函数处理
        }
        if (jpeg_data_ok == 2) // 上一次的jpeg数据已经被处理了
        {
            DMA2_Stream1->NDTR = jpeg_buf_size;
            DMA_SetCurrDataCounter(DMA2_Stream1, jpeg_buf_size); // 传输长度为jpeg_buf_size*4字节
            DMA_Cmd(DMA2_Stream1, ENABLE);                       // 重新传输
            jpeg_data_ok = 0;                                    // 标记数据未采集
        }
    }
}

// 查找FFD8
static u8 *find_ffd8(int xi, u8 *buf, int size)
{
    int i = xi;
    u8 x8[] = {0xFF, 0xD8};
    u16 *x16 = (u16 *)&x8;
    for (; i < size; i++)
    {
        if (i == size - 1)
            return 0;
        // printf("[%d][%d]\r\n",*x16,*(u16*)(buf+i));
        if (*x16 == *(u16 *)(buf + i))
        {
            // printf("i=%d\r\n",i);
            return buf + i;
        }
    }
    return 0;
}

// 查找FFD9
static u8 *find_ffd9(int xi, u8 *buf, int size)
{
    int i = xi;
    u8 x8[] = {0xFF, 0xD9};
    u16 *x16 = (u16 *)&x8;
    for (; i < size; i++)
    {
        if (i == size - 1)
            return 0;
        // printf("[%d][%d]\r\n",*x16,*(u16*)(buf+i));
        if (*x16 == *(u16 *)(buf + i))
            return buf + i;
    }
    return 0;
}

// 获取图片数据
static char *get_img_data(uint32_t *len)
{
    u8 *_res = find_ffd8(0, (u8 *)jpeg_buf, sizeof(jpeg_buf)); // start jpeg ff d8
    u8 *_res2;
    if (_res > 0)
    {
        _res2 = find_ffd9(_res - (u8 *)jpeg_buf, (u8 *)jpeg_buf, sizeof(jpeg_buf)); // end jpeg ff d9
    }

    if (_res2 > 0)
    {
        *len = (_res2 - _res) + 2; // len
        return (char *)_res;       // start
    }
    return 0;
}


#define MAX_SEND_BUF_SIZE (1024 * 32) // 32KB，根据最大图片尺寸调整
static char send_buf[MAX_SEND_BUF_SIZE] = {0};

int jpeg_send(int jpeg_size)
{
    u32 jpeglength = 0;
    char *p;
    char response[HTTP_RESPONSE_MAX_LEN] = {0};
    int ret;

    // 1. 配置HTTP参数
    config_http_param(HTTP_CFG_REQUEST_HEADER, "1");

    // 2. 设置HTTP URL（示例URL，需替换为实际服务器地址）
    char server_url[128];
    sprintf(server_url, "http://%s/upload/v1/upimages.php", IMAGES_IP);
    if (set_http_url(server_url) != 0)
    {
        printf("HTTP URL设置失败\r\n");
        return -1;
    }

    // 3. 初始化摄像头并获取JPEG数据
    DCMI_Start();

    // 等待1帧图像数据
    while (jpeg_data_ok != 1)
        ;             // 等待第一帧数据准备好
    jpeg_data_ok = 2; // 标记为已处理，准备下一次

    DCMI_Stop(); // 停止采集

    // 4. 获取JPEG数据
    p = get_img_data(&jpeglength);
    if (!p)
    {
        printf("JPEG数据获取失败\r\n");
        return -1;
    }

    // 5. 构建HTTP POST请求头部
    char http_header[512] = {0};
    sprintf(http_header, "POST /upload/v1/upimages.php HTTP/1.1\r\n"
                         "Host:%s\r\n"
                         "Content-Type: image/jpeg\r\n"
                         "Connection: keep-alive\r\n"
                         "Authorization:759960d096494f47b96e70151c5ba421\r\n"
                         "Authtopic:picture\r\n"
                         "wechatmsg:\r\n"
                         "wecommsg:\r\n"
                         "picpath:\r\n"
                         "Content-Length: %d\r\n"
                         "\r\n",
            IMAGES_IP, jpeglength); // strlen(data)
    printf("%s\r\n", http_header);
    // 6. 发送HTTP POST请求（头部+数据）

//    char send_buf[jpeglength + 512];
    memcpy(send_buf, http_header, strlen(http_header));
    memcpy(send_buf + strlen(http_header), p, jpeglength);

    ret = send_http_post(send_buf, strlen(http_header) + jpeglength);

    if (ret != 0)
    {
        printf("HTTP POST发送失败\r\n");
        return -1;
    }

    // 7. 读取HTTP响应
    int resp_len = read_http_response(response, HTTP_RESPONSE_MAX_LEN);

    if (resp_len > 0)
    {
        printf("HTTP响应:\r\n%s\r\n", response);
        // 解析响应状态码，例如"OK"表示成功
        if (strstr(response, "OK"))
        {
            // 提取响应信息中url对应的值
            char url[128] = {0};
            // 1. 查找JSON对象的起始位置
            const char *start_market = "{\"url\":\"";
            const char *json_start = strstr(response, start_market);

            if (json_start != NULL)
            {

                // 2. 定位到URL值的起始位置
                const char *url_start = json_start + strlen(start_market); // 跳过起始标记
                // 3. 查找URL结束的双引号
                const char *url_end = strchr(url_start, '"');

                if (url_end != NULL)
                {

                    // 4. 计算URL长度并分配内存
                    int url_len = url_end - url_start;
                    strncpy(url, url_start, url_len);

                    // 将图片的url值上传到云平台
                    MQTT_Publish(UID, PICTURE_TOPIC, url);
                }
            }

            printf("图片上传成功\r\n");

            return 0;
        }
    }

    printf("HTTP响应读取失败或上传失败\r\n");
    return -1;
}

// 检查4G模块的信号质量，并将结果上传到巴法云
int handle_signal(void)
{

    // 更新4G模块信号  +CSQ: 28,99
    char result[128] = {0};

    USART2_sendstr("AT+CSQ\r\n");

    int ms = 200;
    while (ms > 0)
    {
        ms -= 1;
        delay_ms(1);

        if (strstr((char *)USART2_RxBuf, "+CSQ"))
        {

            memcpy(result, (char *)USART2_RxBuf, strlen((char *)USART2_RxBuf));
            printf("%s", result);

            char *token;

            // 第一次分割：跳过前缀
            token = strtok(result, ":");
            if (token != NULL)
            {

                // 第二次分割：获取数值部分
                token = strtok(NULL, ",");

                if (token != NULL)
                {
                    // 去除首尾空格（如果有）
                    while (*token == ' ')
                        token++;

                    printf("信号强度: %s\n", token); // 输出: 28

                    MQTT_Publish(UID, SIGNAL_TOPIC, token);
                }
            }
        }
    }
    return 0;
}

// 获取硬件信息 硬件序列号 手动设置
void get_chipinfo(void)
{
    unsigned char deviceid[12];

    memset(p_sn, 0, sizeof(p_sn));

    // f407
    printf("Get Device Info \r\n");
    deviceid[0] = *(unsigned char *)(0x1FFF7A10);
    deviceid[1] = *(unsigned char *)(0x1FFF7A11);
    deviceid[2] = *(unsigned char *)(0x1FFF7A12);
    deviceid[3] = *(unsigned char *)(0x1FFF7A13);
    deviceid[4] = *(unsigned char *)(0x1FFF7A14);
    deviceid[5] = *(unsigned char *)(0x1FFF7A15);
    deviceid[6] = *(unsigned char *)(0x1FFF7A16);
    deviceid[7] = *(unsigned char *)(0x1FFF7A17);
    deviceid[8] = *(unsigned char *)(0x1FFF7A18);
    deviceid[9] = *(unsigned char *)(0x1FFF7A19);
    deviceid[10] = *(unsigned char *)(0x1FFF7A1A);
    deviceid[11] = *(unsigned char *)(0x1FFF7A1B);

    sprintf(p_sn, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            deviceid[0], deviceid[1], deviceid[2], deviceid[3], deviceid[4], deviceid[5], deviceid[6],
            deviceid[7], deviceid[8], deviceid[9], deviceid[10], deviceid[11]);
    printf("Sn:%s\r\n", p_sn);
}

// 获取adc
void getbatvoltage(float *vol)
{
    u16 adcx;
    adcx = Get_Adc_Average(ADC_Channel_5, 20); // 获取通道5的转换值，20次取平均
    *vol = (float)adcx * (3.3 / 4096);         // 获取计算后的带小数的实际电压值，比如3.1111
    printf("ADC VOL: %.3f\r\n", *vol);
    *vol = (*vol - 7.831f) * 38.46f;
    if (*vol < 0)
    {
        *vol = 0;
    }
    else if (*vol > 100)
    {
        *vol = 100;
    }
}

// 获取图片大小
void get_jpegsize(u32 *jpeg_size, int index)
{
    // 1--160*120 2--352*288 3--320*240
    // 4--640*480 5--800*600
    switch (index)
    {
    case 1:
        *jpeg_size = 1; // 4
        break;

    case 2:
        *jpeg_size = 2;
        break;

    case 3:
        *jpeg_size = 3;
        break;

    case 4:
        *jpeg_size = 4;
        break;
    case 5:
        *jpeg_size = 5;
        break;
    default:
        *jpeg_size = 3;
        break;
    }
}

void connect_net(void)
{
    int ret = 0;
    Write_AT_CMD("AT+CFUN=1,1\x00D\x00A", "OK", 2000);
    delay_ms(5000);
    start_4g(1);

    ret = connect_tcpserver(SERVER_IP, SERVER_PORT); // 连接到服务器

    while (ret != 0)
    {
        printf("TCP连接失败，再次尝试...\r\n");
        ret = connect_tcpserver(SERVER_IP, SERVER_PORT);
    }

    if (ret == 0)
    {
        printf("TCP服务器连接成功\r\n");
    }
    else
    {
        printf("TCP服务器重连失败，系统重启\r\n");
        __set_FAULTMASK(1);
        NVIC_SystemReset();
    }

    MQTT_Subscribe(UID, Motor_TOPIC);
    MQTT_Subscribe(UID, SIGNAL_TOPIC);
    MQTT_Subscribe(UID, PICTURE_TOPIC);
    MQTT_Subscribe(UID, TAKE_TOPIC);
    MQTT_Subscribe(UID, MESSAGE_TOPIC);
}

int main(void)
{

    u32 jpeg_size;                                  // 设置图片大小序号
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 设置系统中断优先级分组2
    delay_init(168);                                // 初始化延时函数

    uart_init(115200); // 初始化串口波特率为115200   串口1和电脑通信用

    usart2_init(115200); // 初始化串口2波特率为115200  串口2和4G模块通信用
    //    usart2_init(921600);	//初始化串口2波特率为921600
    //    usart3_init(115200);        //初始化串口3
    //    uart5_init(9600);           //初始化串口5
    printf("\r\n ############ http://www.csgsm.com/ ############\r\n ############(" __DATE__ " - " __TIME__ ")############\r\n");

    //    Adc_Init();                 //初始化ADC
    //    LED_Init();			//初始化gpio
    TIM4_Int_Init(10000 - 1, 8400 - 1); // 定时器4的10Khz计数,1秒钟中断一次

    FSMC_SRAM_Init(); // 初始化外部SRAM

    // gpio初始化
    gpio_init();

    // 获取设备唯一ID
    get_chipinfo();

    // 获取抓拍尺寸大小
    get_jpegsize(&jpeg_size, 4);
    printf("jpeg_size=%d\r\n", jpeg_size);

    // OV2640初始化

    int retry = 30;
    while (retry) // 初始化OV2640
    {
        if (OV2640_Init() == 0)
            break;

        delay_ms(200);
        printf("OV2640 Init ERROR\r\n");
        retry--;
    }
    printf("OV2640 Success Start\r\n");

    // 摄像头相关初始化
    OV2640_JPEG_Mode(); // 切换为JPEG模式（一次即可）
    My_DCMI_Init();     // DCMI初始化（一次即可）
    // 初始化DMA（缓冲区地址和大小固定，一次即可）
    DCMI_DMA_Init((u32)&jpeg_buf, jpeg_buf_size, DMA_MemoryDataSize_Word, DMA_MemoryInc_Enable);
    // 设置图像尺寸（如果尺寸固定，一次即可）
    OV2640_OutSize_Set(jpeg_img_size_tbl[jpeg_size][0], jpeg_img_size_tbl[jpeg_size][1]);

    // 电机初始化
		Motor_Init();

    if (!(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5))) // 表示白天
    {
        printf("Strong light\r\n");
        // 白天模式
        g_nightmode = 0;
        ircut_today();
        delay_ms(2000);
    }

    else if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5)) // 表示是晚上
    {
        printf("Weak light\r\n");
        // 夜间模式
        g_nightmode = 1;
        ircut_tonight();
        delay_ms(2000);
        // 图像设置成夜视模式
        SCCB_WR_Reg(0XFF, 0X01);
        SCCB_WR_Reg(0X11, 0X07);
    }

    do
    {
        // 连接4G网络，订阅巴法云主题
        connect_net();
    } while (jpeg_send(jpeg_size) != 0); // 测试网络是否能正常发送图片

    Timer_Init(); // 定时向巴法云发送心跳信息

    while (1)
    {

        if (PING_FLAG == 1)
        {

            MQTT_Ping();
            PING_FLAG = 0;
        }

        if (is_frame_ready == 1)
        {
            memcpy(parse_data, (char *)USART2_RxBuf, strlen((char *)USART2_RxBuf));
            clear_buf_uart2();
            printf("-------------------------------------\n");
            printf("%s\n", parse_data);
            printf("-------------------------------------\n");
            if (strstr(parse_data, "uid="))
            {

                if (strstr(parse_data, Motor_TOPIC))
                {

                    if (strstr(parse_data, "msg=right"))
                    {

                        //printf("Motor right\n");
                        Motor_Start();
                        SetDirection(0);
                        delay_ms(250);
                        Motor_Stop();
                    }
                    else if (strstr(parse_data, "msg=left"))
                    {

                        //printf("Motor left\n");
                        Motor_Start();
                        SetDirection(1);
                        delay_ms(250);
                        Motor_Stop();
                    }
                    else if (strstr(parse_data, "msg=on"))
                    {

                        //printf("Motor on\n");
                        Motor_Start();
                        SetDirection(0);
                        delay_ms(1500);

                        SetDirection(1);
                        delay_ms(1500);

                        Motor_Stop();
                    }
                    else if (strstr(parse_data, "msg=off"))
                    {
                        Motor_Start();
                        SetDirection(0);

                        //printf("Motor Close!");
                        delay_ms(1500);

                        Motor_Stop();
                        Motor_Reset();
                    }
                    else if (strstr(parse_data, "msg=reset"))
                    {

                        //printf("Motor reset\n");

                        Motor_Reset();
                    }
                    else
                    {
                        // 指令错误
                    }
                }
                else if (strstr(parse_data, SIGNAL_TOPIC))
                {

                    if (strstr(parse_data, "msg=update"))
                    {

                        handle_signal();
                    }
                }
                else if (strstr(parse_data, PICTURE_TOPIC))
                {
                }
                else if (strstr(parse_data, TAKE_TOPIC))
                {

                    if (strstr(parse_data, "msg=take"))
                    {
                        // 图片发送失败，复位4G模块，重新建立网络连接
                        while (jpeg_send(jpeg_size) != 0)
                        {

                            connect_net();
                        }
                    }
                }
                else
                {
                }
            }
            memset(parse_data, 0, 1024);
            is_frame_ready = 0;
        }
    }
}
