#ifndef __IAP_H__
#define __IAP_H__
//#include "sys.h"
#include "stm32f4xx.h"  

#define version "1.0.0.170708_release"
#define p_name "V Gps"//产品名称
#define p_type "V Gps"//产品类型
//#define p_sn "2017070800"//产品sn号 id


//升级服务器IP和端口
#define GPRS_UPGRADE_IP		"106.15.33.228"
#define GPRS_UPGRADE_PORT	"14370"

#define DEVICE_ID_LEN	32	//设备ID长度
#define VERSION_LEN	32	  //消息版本长度
#define PRODUCT_NAME	10	//产品名称
#define PRODUCT_TYPE	10	//产品型号 缺少产品id


#define FILE_NAME_LEN	64 //程序名

#define MAX_PACKET_LEN	512 //包最大长度

#define DEVICE_COMMAND_HEAD_FLAG		0xD5D5D5D5//包头

#define EEPROM_FILED_LENGTH 32		//段长度

#define UART_FRAME_LEN 24 //串口通信帧长度



 


void __set_PSP(u32 addr);	//设置堆栈地址


typedef  void (*iapfun)(void);				//定义一个函数类型的参数. 

//参数区各参数偏移量
#define UPGRADE_DATA_OFFSET			0  //升级参数信息偏移量
#define UPGRADE_SERVER_OFFSET		0x400
#define BUSINESS_SERVER_OFFSET		0x800
#define DEVICE_PARAM_OFFSET			0xC00



#define FLASH_BOOT_ADDR				0x08000000	//启动区 256KB
#define FLASH_APP1_ADDR				0x08040000	//app区 256KB
#define FLASH_PARAMS_ADDR 			0x08080000 //参数区 128KB
#define FLASH_PARAMS_ADDR_BEIFEN 	0x080A0000 //参数备份区 128KB
//#define FLASH_APP1_ADDR		0x08032000  //第一个应用程序起始地址(存放在FLASH) 
											//保留0X08000000~0X08032000的空间为Bootloader使用(共200KB)



 

void iap_load_app(u32 appxaddr);			//跳转到APP程序执行
u8 connect_server(u16 try_num);//连接服务器
void update_logic(void);//更新程序主逻辑
void read_deviceparam_save_flash_beifen(void);
void read_deviceparam_save_flash(void);
void write_deviceparam_save_flash(void);
void write_deviceparam_save_flash_beifen(void);
void set_lightopenperiod(void);
void set_snapperiod(void);
void set_snapparam(void);
void set_rtctime(unsigned int currenttime);
void send_shutdowncmd(void);
void set_updatetime(void);
void run(void);

//keil 下1字节对齐
#pragma pack(push,1)


//数据包头
typedef struct device_command_header
{
	u32   headFlag;      			// 包头标识4个字节
	unsigned char  commandType;   			// 命令类型
	unsigned char deviceid[DEVICE_ID_LEN]; 		//设备id
	unsigned char commandVersion[VERSION_LEN];	// 版本号
	unsigned char productname[PRODUCT_NAME];		//产品名称
	unsigned char producttype[PRODUCT_TYPE];		//产品类型
	u16 dataLength;				// 数据长度
	u16 crc_check;//整个数据包的校验码 数据区 校验码 
}DEVICE_CMD_HEADER;


//设备参数结构体
typedef struct _DEVICE_PARAM_DATA
{
	u32 starttime1; //开始抓拍时间(时间段1)
	u32 endtime1; //抓拍结束时间(时间段1)
	u32 starttime2; //开始抓拍时间(时间段2)
	u32 endtime2; //抓拍结束时间(时间段2)
	u32 snap_timeval; //白天抓拍间隔
	u32 snap_nighttimeval;//晚上抓拍间隔
	u32 min_snaptimeval; //最小抓拍间隔
	u32 snap_resolution; //抓拍分辨率
	u32 lightopen_starttime1; //红外灯开启开始时间(时间段1)
	u32 lightopen_endtime1; //红外灯开启结束时间(时间段1)
	u32 lightopen_starttime2; //红外灯开启开始时间(时间段2)
	u32 lightopen_endtime2; //红外灯开启结束时间(时间段2)
	char upgrade_system_addr[EEPROM_FILED_LENGTH]; //升级服务器域名
	u32 upgrade_system_port; //升级服务器端口
	char operation_system_addr[EEPROM_FILED_LENGTH]; //运营系统域名
	u32 operation_system_port; //运营系统端口
	char internal_system_addr[EEPROM_FILED_LENGTH]; //内置系统域名
	u32 internal_system_port; //内置系统端口
	char wifi_ssid[EEPROM_FILED_LENGTH];  //wifi的ssid
 	char wifi_mode[EEPROM_FILED_LENGTH];  //wifi的加密模式
 	char wifi_password[EEPROM_FILED_LENGTH]; //wifi的密码
	char mideware_version[EEPROM_FILED_LENGTH]; //固件版本号
	char longitude[EEPROM_FILED_LENGTH]; //经度
	char latitude[EEPROM_FILED_LENGTH]; //维度
	u32 update_flag;     //升级标志
	u32 getgps_flag; //是否获取gps的标志 0--不获取gps  1--获取gps
	u32 write_flag; //是否写过flash的标志
	u32 update_result; //升级结果 0--未升级或升级失败 1--升级成功，可以跳转
}DEVICE_PARAM_DATA;




typedef struct _EEPROM_CONFIG_DATA
{
    char alivetime[EEPROM_FILED_LENGTH+1];          //激活
    char protocoltype[EEPROM_FILED_LENGTH+1];       //协议类型
    char ipaddr[EEPROM_FILED_LENGTH+1];             //IP地址
    char portnum[EEPROM_FILED_LENGTH+1];            //端口号
    char domain[EEPROM_FILED_LENGTH+1];             //域名
    char telnum[EEPROM_FILED_LENGTH+1];             //电话号码
}EEPROM_CONFIG_DATA;


//命令
typedef enum
{
        DEVICE_COMMAND_CHECKVERSION 	    = 0x10,     //询问服务器 是否有自己更新包 命令
        DEVICE_COMMAND_REQUESTFILEINFO      = 0x11,     //获取升级包 信息 命令
        DEVICE_COMMAND_REQUESTFILEDATA      = 0x12,     //获取升级包数据命令
        DEVICE_COMMAND_FILEDATA 	    = 0x13,
        DEVICE_COMMAND_RESP_ERROR 	    = 0x14      //升级是否成功 告知服务器的命令
}TRANS_COMMAND_TYPE;
//回复服务器 升级情况
typedef enum
{
	UPDATE_SUCCEED_FLAG = 0x05, //回复服务器---升级成功
	NO_UPDATE_FLAG      = 0x03, //回复服务器---无需升级
	UPDATE_FAILD_FLAG   = 0x02  //回复服务器---升级失败
}UPDATE_MESSAGE_TYPE;



//向服务器询问  是否存在更新命令包
typedef struct device_checkversion
{
	DEVICE_CMD_HEADER header;
}DEVICE_CHECKVERSION;

//服务器向终端回复的 结构体
typedef struct server2device_checkversion
{
	DEVICE_CMD_HEADER header;
	//0--不存在更新 1--存在更新 可以进入更新流程 2--不用更新 而且不要跳转到app区执行
	unsigned char check_result; 
}SERVER2DEVICE_CHECKVERSION;

//向服务器发送获取  文件 概要信息的 结构体
typedef struct device_requestfileinfo
{
	DEVICE_CMD_HEADER header;
}DEVICE_REQUESTFILE;


//服务器回复的文件概要信息
typedef struct server2device_fileinfo
{
	DEVICE_CMD_HEADER header;
	char fileinfo[FILE_NAME_LEN];
	u32 filesize;			//文件大小
	unsigned char Version[VERSION_LEN];	//当前要更新的版本号
	u16 crc;   //crc校验码 用于获取完毕后的校验
	u16 packetcount; //分包数量
}SERVER2DEVICE_FILEINFO;

//按照包编号向服务器获取  数据包
typedef struct device_requestfiledata
{
	DEVICE_CMD_HEADER header;
	u16 packetnum;//包编号 从0开始
}DEVICE_REQUESTFILEDATA;

//服务器发的程序文件数据包
typedef struct server2device_filedata
{
	DEVICE_CMD_HEADER header;
	u16 packetnum; //包编号
	u16 packetlen; //包长度
	unsigned char filedata[MAX_PACKET_LEN];//包数据
}SERVER2DEVICE_FILEDATA;

//参数保存结构体
/*
该结构体用于把要保存的 iap程序相关参数写入flash
*/
typedef struct params_save
{
        u16 run_flag;           //跳转标志位 0--服务器有更新包  1--没有更新包 跳转到app区 执行  2--不更新 执行iap区业务逻辑
        u16 cur_file_packnum;   //当前已经下载完成的包数量
        unsigned char cur_version[VERSION_LEN];     //当前程序版本号
        unsigned char update_version[VERSION_LEN];  //要更新到的目标版本号
	
}PARAMS_SAVE;

//向服务器返回 升级情况 
typedef struct update_flag
{
	unsigned char  _update_flag;   			// 命令类型

}UPDATE_FLAG;
#pragma pack(pop)//回复keil原来的数据对齐方式



#endif







































