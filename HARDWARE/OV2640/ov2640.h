#ifndef _OV2640_H
#define _OV2640_H
#include "sys.h"
#include "sccb.h"

#define OV2640_PWDN  	PGout(9)			//POWER DOWN控制信号 
#define OV2640_RST  	PGout(15)			//复位控制信号 
////////////////////////////////////////////////////////////////////////////////// 
#define OV2640_MID				0X7FA2
#define OV2640_PID				0X2642
 

//当选择DSP地址(0XFF=0X00)时,OV2640的DSP寄存器地址映射表
#define OV2640_DSP_R_BYPASS     0x05
#define OV2640_DSP_Qs           0x44
#define OV2640_DSP_CTRL         0x50
#define OV2640_DSP_HSIZE1       0x51
#define OV2640_DSP_VSIZE1       0x52
#define OV2640_DSP_XOFFL        0x53
#define OV2640_DSP_YOFFL        0x54
#define OV2640_DSP_VHYX         0x55
#define OV2640_DSP_DPRP         0x56
#define OV2640_DSP_TEST         0x57
#define OV2640_DSP_ZMOW         0x5A
#define OV2640_DSP_ZMOH         0x5B
#define OV2640_DSP_ZMHH         0x5C
#define OV2640_DSP_BPADDR       0x7C
#define OV2640_DSP_BPDATA       0x7D
#define OV2640_DSP_CTRL2        0x86
#define OV2640_DSP_CTRL3        0x87
#define OV2640_DSP_SIZEL        0x8C
#define OV2640_DSP_HSIZE2       0xC0
#define OV2640_DSP_VSIZE2       0xC1
#define OV2640_DSP_CTRL0        0xC2
#define OV2640_DSP_CTRL1        0xC3
#define OV2640_DSP_R_DVP_SP     0xD3
#define OV2640_DSP_IMAGE_MODE   0xDA
#define OV2640_DSP_RESET        0xE0
#define OV2640_DSP_MS_SP        0xF0
#define OV2640_DSP_SS_ID        0x7F
#define OV2640_DSP_SS_CTRL      0xF8
#define OV2640_DSP_MC_BIST      0xF9
#define OV2640_DSP_MC_AL        0xFA
#define OV2640_DSP_MC_AH        0xFB
#define OV2640_DSP_MC_D         0xFC
#define OV2640_DSP_P_STATUS     0xFE
#define OV2640_DSP_RA_DLMT      0xFF 

//当选择传感器地址(0XFF=0X01)时,OV2640的DSP寄存器地址映射表
#define OV2640_SENSOR_GAIN       0x00
#define OV2640_SENSOR_COM1       0x03
#define OV2640_SENSOR_REG04      0x04
#define OV2640_SENSOR_REG08      0x08
#define OV2640_SENSOR_COM2       0x09
#define OV2640_SENSOR_PIDH       0x0A
#define OV2640_SENSOR_PIDL       0x0B
#define OV2640_SENSOR_COM3       0x0C
#define OV2640_SENSOR_COM4       0x0D
#define OV2640_SENSOR_AEC        0x10
#define OV2640_SENSOR_CLKRC      0x11
#define OV2640_SENSOR_COM7       0x12
#define OV2640_SENSOR_COM8       0x13
#define OV2640_SENSOR_COM9       0x14
#define OV2640_SENSOR_COM10      0x15
#define OV2640_SENSOR_HREFST     0x17
#define OV2640_SENSOR_HREFEND    0x18
#define OV2640_SENSOR_VSTART     0x19
#define OV2640_SENSOR_VEND       0x1A
#define OV2640_SENSOR_MIDH       0x1C
#define OV2640_SENSOR_MIDL       0x1D
#define OV2640_SENSOR_AEW        0x24
#define OV2640_SENSOR_AEB        0x25
#define OV2640_SENSOR_W          0x26
#define OV2640_SENSOR_REG2A      0x2A
#define OV2640_SENSOR_FRARL      0x2B
#define OV2640_SENSOR_ADDVSL     0x2D
#define OV2640_SENSOR_ADDVHS     0x2E
#define OV2640_SENSOR_YAVG       0x2F
#define OV2640_SENSOR_REG32      0x32
#define OV2640_SENSOR_ARCOM2     0x34
#define OV2640_SENSOR_REG45      0x45
#define OV2640_SENSOR_FLL        0x46
#define OV2640_SENSOR_FLH        0x47
#define OV2640_SENSOR_COM19      0x48
#define OV2640_SENSOR_ZOOMS      0x49
#define OV2640_SENSOR_COM22      0x4B
#define OV2640_SENSOR_COM25      0x4E
#define OV2640_SENSOR_BD50       0x4F
#define OV2640_SENSOR_BD60       0x50
#define OV2640_SENSOR_REG5D      0x5D
#define OV2640_SENSOR_REG5E      0x5E
#define OV2640_SENSOR_REG5F      0x5F
#define OV2640_SENSOR_REG60      0x60
#define OV2640_SENSOR_HISTO_LOW  0x61
#define OV2640_SENSOR_HISTO_HIGH 0x62


								
void OV2640_PowerOn(void);
void OV2640_PowerOff(void);
u8 OV2640_Init(void);  
void OV2640_JPEG_Mode(void);
void OV2640_RGB565_Mode(void);
void OV2640_Auto_Exposure(u8 level);
void OV2640_Light_Mode(u8 mode);
void OV2640_Color_Saturation(u8 sat);
void OV2640_Brightness(u8 bright);
void OV2640_Contrast(u8 contrast);
void OV2640_Special_Effects(u8 eft);
void OV2640_Color_Bar(u8 sw);
void OV2640_Window_Set(u16 sx,u16 sy,u16 width,u16 height);
u8 OV2640_OutSize_Set(u16 width,u16 height);
u8 OV2640_ImageWin_Set(u16 offx,u16 offy,u16 width,u16 height);
u8 OV2640_ImageSize_Set(u16 width,u16 height);

#define MAGICCODE 			0xD5D5D5D5
#define DEVICE_CMD_JPG			0x30
#define DEVICE_CMD_REGISTER		0x40
#define DEVICE_TYPE			0x10 ////2018.8.11升级4G设备，包序号和包总数都改成2个字节

#pragma pack(1)

typedef struct device_comm_header_t
{
	 uint32_t magicode;
	 uint8_t cmdtype;
	 uint8_t device_id[32];
	 uint8_t devicetype; //设备类型 0-gprs设备 1-wifi设备
	 uint32_t datalen;	      
	 uint16_t packetseq;	      
	 uint16_t packetlen;		  
	 uint16_t packnum;		  
}device_comm_header;

//设置注册结构体
typedef struct device_register_t
{
	device_comm_header device_header;
	uint8_t mideware_version[32]; //固件版本号
	uint32_t battery_capacity; //电池容量
	uint8_t longitude[32]; //经度
	uint8_t latitude[32]; //维度
}device_register;


typedef struct device_reg_resp_t
{
	 uint32_t starttime1;//抓拍开始时间(时间段1)		  
	 uint32_t endtime1;//抓拍结束时间(时间段1)
	 uint32_t starttime2; //抓拍开始时间(时间段2)
	 uint32_t endtime2; //抓拍结束时间(时间段2)
	 uint32_t snap_timeval; //白天抓拍间隔
	 uint32_t snap_nighttimeval;//晚上抓拍间隔
	 uint32_t min_snaptimeval; //最小抓拍间隔
	 uint32_t  updateflag; //是否要进行升级 0--不升级 1--升级 设备只要注册一次就恢复成不升级
	 uint32_t snap_resolution;//抓拍分辨率 		 
	 uint32_t is_havlight;//是否有红外灯，该设备是否有红外灯			 
	 uint32_t light_status;//0--关闭 1--打开 2--自动(根据时间段判断是否开灯)	
	 uint32_t lightopen_starttime1;//红外灯抓拍开始时间(时间段1)  
	 uint32_t lightopen_endtime1;//红外灯抓拍结束时间(时间段1)
	 uint32_t lightopen_starttime2;//红外灯抓拍开始时间(时间段2)  
	 uint32_t lightopen_endtime2;//红外灯抓拍结束时间(时间段2)
	 uint8_t upgrade_system_addr[32]; //升级服务器域名
	 uint32_t upgrade_system_port; //升级服务器端口
	 uint8_t operation_system_addr[32]; //运营系统域名
 	 uint32_t operation_system_port; //运营系统端口
 	 uint8_t  wifi_ssid[32];  //wifi的ssid
 	 uint8_t  wifi_mode[32];  //wifi的加密模式
 	 uint8_t  wifi_password[32]; //wifi的密码
 	 uint32_t  updateaddr_flag;  //是否进行网络相关设置更新 0--默认不更新(因为参数很重要，一旦更新失败，设备就要全部坏掉了)1--更新地址(地址只更新一次，更新完就置0)
	 uint32_t  getgps_flag;  //是否获取gps信息
	 uint32_t currenttime; //当前系统时间 UTC时间
}device_reg_resp;


#pragma pack()

//delay_ms(10);
typedef enum   
{
  BMP_QQVGA             =   0x00,	    /* BMP Image QQVGA 160x120 Size */
  BMP_QVGA              =   0x01,           /* BMP Image QVGA 320x240 Size */
  JPEG_160x120          =   0x02,	    /* JPEG Image 160x120 Size */
  JPEG_176x144          =   0x03,	    /* JPEG Image 176x144 Size */
  JPEG_320x240          =   0x04,	    /* JPEG Image 320x240 Size */	
  JPEG_640x480          =   0x05,	    /* JPEG Image 320x240 Size */
  JPEG_800x600          =   0x06,	    /* JPEG Image 352x288 Size */
  JPEG_1024x768         =   0x07,	    /* JPEG Image 352x288 Size */
  JPEG_1280x960        =   0x08
}ImageFormat_TypeDef;
void OV2640_JPEGConfig(ImageFormat_TypeDef ImageFormat);
void OV2640_BrightnessConfig(uint8_t Brightness);
void OV2640_AutoExposure(u8 level);
//#endif
#endif





















