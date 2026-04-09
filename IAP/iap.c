#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "4g.h"
#include "usart2.h"
#include "stmflash.h"
#include "iap.h" 
#include "crc16.h"
#include "delay.h"

u8 UPDATE_IPADDR[48]= "\"TCP\",\"101.200.42.61\",2000" ;      //升级服务器域名或IP 以及端口号
//u8 UPDATE_IPADDR[48]= "\"TCP\",\"59.110.160.128\",1501" ;      //升级服务器域名或IP 以及端口号
//u8 UPDATE_IPADDR[48]= "\"TCP\",\"101.200.42.61\",1503" ;      //升级服务器域名或IP 以及端口号
iapfun jump2app; 
u16 iapbuf[256]; 	//2K字节缓存  
PARAMS_SAVE params_save_stru;//当前 更新状态信息 结构体



//升级服务器信息
EEPROM_CONFIG_DATA g_upgradeserver_data;
//业务服务器信息
EEPROM_CONFIG_DATA g_businessserver_data;
//设备参数信息
DEVICE_PARAM_DATA g_deviceparam_data;

//目前接收的升级包数
int g_curfilepacknum = 0;


extern uint8_t  USART2_RxBuf[USART2_RXBUF_LEN];

extern uint16_t USART2_RxTail;
extern uint16_t USART2_RxHead;

char p_sn[DEVICE_ID_LEN];


//设置抓拍时间段
void set_snapperiod()
{
    int i = 0;
    unsigned char senddata[UART_FRAME_LEN];
    memset(senddata,0,sizeof(senddata));

    printf("set_snapperiod\r\n");


    //协议头
    senddata[0] = 0x5F;
    senddata[1] = 0xF5;
    //命令字
    senddata[2] = 0x10;
    //源设备地址
    senddata[3] = 0x02;
    //目的设备地址
    senddata[4] = 0x01;
    //数据共16个字节
    //默认单位是秒，需要转换为分钟
    memcpy(senddata + 5,&(g_deviceparam_data.starttime1),sizeof(u32));
    memcpy(senddata + 5 + sizeof(u32),&(g_deviceparam_data.endtime1),sizeof(u32));
    memcpy(senddata + 5 + 2*sizeof(u32),&(g_deviceparam_data.starttime2),sizeof(u32));
    memcpy(senddata + 5 + 3*sizeof(u32),&(g_deviceparam_data.endtime2),sizeof(u32));

    //校验位
    senddata[21] = 0x00;
    //协议尾
    senddata[22] = 0x0D;
    senddata[23] = 0x0A;

    for(i = 0;i < UART_FRAME_LEN;i++)
    {
       	UART5_SendChar((u8)senddata[i]);
    }
    delay_ms(500);

}


//设置红外灯打开时间段
void set_lightopenperiod()
{
    int i = 0;
    unsigned char senddata[UART_FRAME_LEN];

    memset(senddata,0,sizeof(senddata));

    printf("set_lightopenperiod\r\n");

    //协议头
    senddata[0] = 0x5F;
    senddata[1] = 0xF5;
    //命令字
    senddata[2] = 0x11;
    //源设备地址
    senddata[3] = 0x02;
    //目的设备地址
    senddata[4] = 0x01;
    //数据共16个字节
    memcpy(senddata + 5,&(g_deviceparam_data.lightopen_starttime1),sizeof(u32));
    memcpy(senddata + 5 + sizeof(u32),&(g_deviceparam_data.lightopen_endtime1),sizeof(u32));
    memcpy(senddata + 5 + 2*sizeof(u32),&(g_deviceparam_data.lightopen_starttime2),sizeof(u32));
    memcpy(senddata + 5 + 3*sizeof(u32),&(g_deviceparam_data.lightopen_endtime2),sizeof(u32));



    //校验位
    senddata[21] = 0x00;
    //协议尾
    senddata[22] = 0x0D;
    senddata[23] = 0x0A;

    for(i = 0;i < UART_FRAME_LEN;i++)
    {
       	UART5_SendChar((u8)senddata[i]);
    }
    delay_ms(500);


}

//设置白天抓拍间隔，晚上抓拍间隔，分辨率，最小抓拍时间
void set_snapparam()
{
    int i = 0;
    unsigned char senddata[UART_FRAME_LEN];
    u32 snap_timeval = 0;
    u32 snap_nighttimeval = 0;
    u32 min_snaptimeval = 0;
    memset(senddata,0,sizeof(senddata));

    printf("set_snapparam\r\n");

    //协议头
    senddata[0] = 0x5F;
    senddata[1] = 0xF5;
    //命令字
    senddata[2] = 0x12;
    //源设备地址
    senddata[3] = 0x02;
    //目的设备地址
    senddata[4] = 0x01;
    //数据共16个字节

    //如果白天抓拍间隔小于5分钟，为了防止单片机重启，直接甚至最大超时时间60分钟
    if(g_deviceparam_data.snap_timeval < 300)
    {
        snap_timeval = 0;
    }
    else
    {
        snap_timeval = (g_deviceparam_data.snap_timeval) / 60;
    }


    if(g_deviceparam_data.snap_nighttimeval < 300)
    {
        snap_nighttimeval = 0;
    }
    else
    {
        snap_nighttimeval = (g_deviceparam_data.snap_nighttimeval) / 60;
    }

    if(g_deviceparam_data.min_snaptimeval < 300)
    {
        min_snaptimeval = 5;
    }
    else
    {
        min_snaptimeval = g_deviceparam_data.min_snaptimeval / 60;
    }



    memcpy(senddata + 5,&snap_timeval,sizeof(u32));
    memcpy(senddata + 5 + sizeof(u32),&snap_nighttimeval,sizeof(u32));
    memcpy(senddata + 5 + 2*sizeof(u32),&(g_deviceparam_data.snap_resolution),sizeof(u32));
    memcpy(senddata + 5 + 3*sizeof(u32),&min_snaptimeval,sizeof(u32));

    //校验位
    senddata[21] = 0x00;
    //协议尾
    senddata[22] = 0x0D;
    senddata[23] = 0x0A;

    for(i = 0;i < UART_FRAME_LEN;i++)
    {
       	UART5_SendChar((u8)senddata[i]);
    }
    delay_ms(500);


}


//设置升级时间,在升级时间内407不断开电源
void set_updatetime()
{
    int i = 0;
    unsigned char senddata[UART_FRAME_LEN];
    memset(senddata,0,sizeof(senddata));

    printf("set_updatetime\r\n");

    //协议头
    senddata[0] = 0x5F;
    senddata[1] = 0xF5;
    //命令字
    senddata[2] = 0x13;
    //源设备地址
    senddata[3] = 0x02;
    //目的设备地址
    senddata[4] = 0x01;
    //升级时间
    senddata[5] = 10;
    //校验位
    senddata[21] = 0x00;
    //协议尾
    senddata[22] = 0x0D;
    senddata[23] = 0x0A;

    for(i = 0;i < UART_FRAME_LEN;i++)
    {
       	UART5_SendChar((u8)senddata[i]);
    }
    delay_ms(500);

}

//关闭407电源
//上传图片完毕，向msp430发送关机命令
void send_shutdowncmd()
{
    int i = 0;
    unsigned char senddata[UART_FRAME_LEN];
    memset(senddata,0,sizeof(senddata));

    printf("send_shutdowncmd\r\n");

    //协议头
    senddata[0] = 0x5F;
    senddata[1] = 0xF5;
    //命令字
    senddata[2] = 0x14;
    //源设备地址
    senddata[3] = 0x02;
    //目的设备地址
    senddata[4] = 0x01;
    //数据共16个字节
    //预留位默认置0
    //校验位
    senddata[21] = 0x00;
    //协议尾
    senddata[22] = 0x0D;
    senddata[23] = 0x0A;

    for(i = 0;i < UART_FRAME_LEN;i++)
    {
       	UART5_SendChar((u8)senddata[i]);
    }
    delay_ms(50);

}



//设置RTC时间
void set_rtctime(unsigned int currenttime)
{
    int i = 0;
    unsigned char senddata[UART_FRAME_LEN];
    memset(senddata,0,sizeof(senddata));

    printf("set_rtctime\r\n");

    //协议头
    senddata[0] = 0x5F;
    senddata[1] = 0xF5;
    //命令字
    senddata[2] = 0x15;
    //源设备地址
    senddata[3] = 0x02;
    //目的设备地址
    senddata[4] = 0x01;
    //数据共16个字节
    //年
    senddata[5] = 18;
    //月
    senddata[6] = 5;
    //日
    senddata[7] = 1;

    //目前只考虑时分秒即可
    //时
    senddata[8] = currenttime/3600;
    //分
    senddata[9] = (currenttime - (currenttime/3600)*3600)/60;
    //秒
    senddata[10] = currenttime - (currenttime/3600)*3600 - ((currenttime - (currenttime/3600)*3600)/60)*60;

    printf("hour=%d\r\n",senddata[8]);
    printf("min=%d\r\n",senddata[9]);
    printf("sec=%d\r\n",senddata[10]);
    //预留10字节
    //校验位
    senddata[21] = 0x00;
    //协议尾
    senddata[22] = 0x0D;
    senddata[23] = 0x0A;

    for(i = 0;i < UART_FRAME_LEN;i++)
    {
       	UART5_SendChar((u8)senddata[i]);
    }
    delay_ms(500);

}


//向430获取抓拍模式
void get_snapmode()
{
    int i = 0;
    unsigned char senddata[UART_FRAME_LEN];
    memset(senddata,0,sizeof(senddata));

    printf("get_snapmode\r\n");

    //协议头
    senddata[0] = 0x5F;
    senddata[1] = 0xF5;
    //命令字
    senddata[2] = 0x16;
    //源设备地址
    senddata[3] = 0x02;
    //目的设备地址
    senddata[4] = 0x01;
    //数据共16个字节
    //预留位默认置0
    //校验位
    senddata[21] = 0x00;
    //协议尾
    senddata[22] = 0x0D;
    senddata[23] = 0x0A;

    for(i = 0;i < UART_FRAME_LEN;i++)
    {
       	UART5_SendChar((u8)senddata[i]);
    }
    delay_ms(500);

}






/*
将接收到的 app 包写入制定的flash地址中
appdir--要写入的地址
appbuf--要写入的数据
len--吸入的长度  2个字节
*/
void write_app2flash(u32 appdir,u32* appbuf,u32 len)
{
    STMFLASH_Write(appdir,appbuf,len);//这个需要修改
}

//跳转到应用程序段
//appxaddr:用户代码起始地址.
void iap_load_app(u32 appxaddr)
{
    if(((*(vu32*)appxaddr)&0x2FFE0000)==0x20000000)	//检查栈顶地址是否合法.
    {
        printf("jumpxxxxxxxxxxxxxxxxxxxxxxxxxxxx\r\n");
        jump2app=(iapfun)*(vu32*)(appxaddr+4);		//用户代码区第二个字为程序开始地址(复位地址)
        // MSR_MSP(*(vu32*)appxaddr);			//初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
        __set_PSP(*(vu32*)appxaddr);			//初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
        jump2app();					//跳转到APP.
    }
}	


void write_deviceparam_save_flash(void)
{	
    //先擦除参数区
    STMFLASH_Erase(FLASH_PARAMS_ADDR);
    //然后再写
    STMFLASH_Write(FLASH_PARAMS_ADDR,(u32*)(&g_deviceparam_data),sizeof(DEVICE_PARAM_DATA)/4);
}

void write_deviceparam_save_flash_beifen(void)
{	
    printf("write_deviceparam_save_flash_beifen\r\n");
    //先擦除参数区
    STMFLASH_Erase(FLASH_PARAMS_ADDR_BEIFEN);
    //然后再写
    STMFLASH_Write(FLASH_PARAMS_ADDR_BEIFEN,(u32*)(&g_deviceparam_data),sizeof(DEVICE_PARAM_DATA)/4);
}


void read_deviceparam_save_flash(void)
{
    int i = 0;
    u32 data[sizeof(DEVICE_PARAM_DATA)/4]={0};
    STMFLASH_Read(FLASH_PARAMS_ADDR,data,sizeof(DEVICE_PARAM_DATA)/4);
    memset(&g_deviceparam_data,0,sizeof(g_deviceparam_data));
    memcpy(&g_deviceparam_data,(u8*)data,sizeof(DEVICE_PARAM_DATA));

    for(i =0;i < sizeof(DEVICE_PARAM_DATA)/4;i++)
    {
        if(data[i] != 0xFFFF)
        {
            printf("deviceparam !=0xFFFF\r\n");
            break;
        }
    }

}


void read_deviceparam_save_flash_beifen(void)
{
    int i = 0;
    u32 data[sizeof(DEVICE_PARAM_DATA)/4]={0};

    printf("read_deviceparam_save_flash_beifen\r\n");
    STMFLASH_Read(FLASH_PARAMS_ADDR_BEIFEN,data,sizeof(DEVICE_PARAM_DATA)/4);
    memcpy(&g_deviceparam_data,(u8*)data,sizeof(DEVICE_PARAM_DATA));

    for(i =0;i < sizeof(DEVICE_PARAM_DATA)/4;i++)
    {
        if(data[i] != 0xFFFF)
        {
            printf("deviceparam !=0xFFFF\r\n");
            break;
        }
    }

}

/*
该函数用来判断 服务器返回数据的有效性 和完整性
这个函数不做数据逻辑分析功能 只是校验
1--没有找到包头 0--成功
*/
u8 check_response_data(TRANS_COMMAND_TYPE cmd,u8* response_data,u16 response_data_len)
{
    u8 res=0;
    DEVICE_CMD_HEADER _header;
    SERVER2DEVICE_CHECKVERSION _resp_stru;
    SERVER2DEVICE_FILEINFO _file_stru;
    SERVER2DEVICE_FILEDATA _data_stru;

    u8 crc_buf[1200]={0};//用于放置crc教研的临时数据
    u16 crc_check_m=0;//用于调试

    u8 temp[200];

    memset(&_header,0,sizeof(DEVICE_CMD_HEADER));
    memset(&_resp_stru,0,sizeof(SERVER2DEVICE_CHECKVERSION));
    memset(&_file_stru,0,sizeof(SERVER2DEVICE_FILEINFO));
    memset(&_data_stru,0,sizeof(SERVER2DEVICE_FILEDATA));
    memset(temp,0,sizeof(temp));


    switch(cmd)
    {
    case DEVICE_COMMAND_CHECKVERSION ://检查版本号明星
        if(response_data_len<sizeof(SERVER2DEVICE_CHECKVERSION))
        {
            sprintf((char *)temp,"response_data_len:%d,sizeof(SERVER2DEVICE_CHECKVERSION):%d\r\n",
                    response_data_len,sizeof(SERVER2DEVICE_CHECKVERSION));
            printf("temp=%s\r\n",temp);
            return 1;//回复数据长度不足
        }
        memcpy(&_resp_stru,response_data,response_data_len);
        _header=_resp_stru.header;
        if(_header.headFlag!=DEVICE_COMMAND_HEAD_FLAG)
        {
            printf("DEVICE_COMMAND_CHECKVERSION _header.headFlag error\r\n");
            return 1;//命令头错误
        }
        if(_header.commandType!=cmd)
        {
            printf("DEVICE_COMMAND_CHECKVERSION _header.commandType error\r\n");
            return 1;//命令错误
        }

        break;


		case DEVICE_COMMAND_REQUESTFILEINFO ://获取升级文件信息
                    if(response_data_len<sizeof(SERVER2DEVICE_FILEINFO))
                    {
			sprintf((char *)temp,"response_data_len:%d,sizeof(SERVER2DEVICE_FILEINFO):%d\r\n",
                                response_data_len,sizeof(SERVER2DEVICE_FILEINFO));
			printf("temp=%s\r\n",temp);
			printf(" DEVICE_COMMAND_REQUESTFILEINFO length error\r\n");
			return 1;//回复数据长度不足
                    }
                    memcpy(&_file_stru,response_data,response_data_len);
                    _header=_file_stru.header;
                    if(_header.headFlag!=DEVICE_COMMAND_HEAD_FLAG)
                    {
		  	printf(" DEVICE_COMMAND_REQUESTFILEINFO _header.headFlag error\r\n");
			return 1;//命令头错误
                    }
                    if(_header.commandType!=cmd)
                    {
			printf(" DEVICE_COMMAND_REQUESTFILEINFO _header.commandType error\r\n");
			return 1;//命令错误
                    }
                    //校验数据
                    memcpy(crc_buf,&_file_stru.fileinfo,sizeof(SERVER2DEVICE_FILEINFO)-sizeof(DEVICE_CMD_HEADER));
                    crc_check_m=CalcCrc16(crc_buf,sizeof(SERVER2DEVICE_FILEINFO)-sizeof(DEVICE_CMD_HEADER));
                    if(_header.crc_check!=crc_check_m)
                    {
			printf("_header.crc_check error\r\n");
			return 1;//数据校验错误	
                    }
                    break;


		case DEVICE_COMMAND_REQUESTFILEDATA://获取升级文件
                    if(response_data_len<sizeof(SERVER2DEVICE_FILEDATA))
                    {
			sprintf((char *)temp,"response_data_len:%d,sizeof(SERVER2DEVICE_FILEDATA):%d\r\n",
                                response_data_len,sizeof(SERVER2DEVICE_FILEDATA));
			printf("temp=%s\r\n",temp);
			printf(" DEVICE_COMMAND_REQUESTFILEDATA length error\r\n");
			return 1;//回复数据长度不足
                    }

                    memcpy(&_data_stru,response_data,response_data_len);
                    _header=_data_stru.header;

                    if(_header.headFlag!=DEVICE_COMMAND_HEAD_FLAG)
                    {
			printf(" DEVICE_COMMAND_REQUESTFILEDATA _header.headFlag error\r\n");
			return 1;//命令头错误
                    }

                    if(_header.commandType!=cmd)
                    {
			printf(" DEVICE_COMMAND_REQUESTFILEDATA _header.commandType error\r\n");
			return 1;//命令错误
                    }

                    //crc校验

                    memcpy(crc_buf,&_data_stru.packetnum,sizeof(SERVER2DEVICE_FILEDATA)-sizeof(DEVICE_CMD_HEADER));
                    crc_check_m=CalcCrc16(crc_buf,sizeof(SERVER2DEVICE_FILEDATA)-sizeof(DEVICE_CMD_HEADER));

                    if(_header.crc_check!=crc_check_m)
                    {
			printf("wen jian shu ju _header.crc_check error\r\n");
			return 1;//数据校验错误	
                    }

                    break;

		case DEVICE_COMMAND_RESP_ERROR ://向服务器报告  升级情况
                    break;

		case DEVICE_COMMAND_FILEDATA ://不知道这个干什么用

                    break;



                }

    return res;
}

/**************************************************************************************************
函数名:  SENDCHAR_IP_len
功能描述：退出连接
输入参数：无
函数返回值：退出成功返回1 失败返回0
关闭数据 上传服务器连接
***************************************************************************************************/
unsigned char SENDCHAR_IP_len(unsigned char *CHARdata,unsigned short len)
{
    char datalen_str[16];
    char temp[256];

    //send_gprs_data(CHARdata,len);
    

    memset(temp,0,sizeof(temp));
    memset(datalen_str,0,sizeof(datalen_str));

    delay_ms(4000);
    return 0; //发送成功
}



/*
递归发送组合好的数据给服务器 并等待服务器的返回信息
TRANS_COMMAND_TYPE cmd 发送的命令
u8* data_packet 发送的数据包
,u16 data_len 发送的数据包长度
,u8* response_data 服务器返回的数据
,u16 response_data_len 服务器返回的数据长度
,u8 try_num 尝试发送的次数
0--成功  1--尝试次数超时 发送失败
*/
u8 send_cmd(TRANS_COMMAND_TYPE cmd, u8* data_packet,u16 data_len,u8 try_num)
{
    u8 res=0;
    u8 temp[256];

    memset(temp,0,sizeof(temp));

    /*1--未连接 2--模块关机 0--成功*/
    if(try_num > 4)
    {
        return 1;//发送失败
    }
    //发送数据之前清空接收缓冲区
    clear_buf_uart2();


    printf("send start\r\n");
    res = SENDCHAR_IP_len(data_packet,data_len);

    //喂狗
    IWDG_ReloadCounter();

    switch(res)
    {
    case 0:
        //等待服务器接收 返回结果 1--超时
        printf("recv data from server\r\n");
        //res= gprs_getdata_quickly(3);//3s
        if(res==1)
        {
            printf("servers return no data TRY AGAIN\r\n");
            //3s了还没有接收到服务器返回的任何信息 估计有问题了 递归重新发送
            try_num++;
            res= send_cmd(cmd,data_packet,data_len,try_num);
        }
        else
        {
            //+IPD,1,67:2017042001 去掉服务器返回的字节头
            if(USART2_RxTail - USART2_RxHead > 0)
            {
                //收到的数据在串口2的接收缓冲区里
                res = check_response_data(cmd,USART2_RxBuf,USART2_RxTail - USART2_RxHead);
            }
            else
            {
                res = 1;
            }
            if(res == 1)
            {
                //服务器返回的数据错误 递归重新发送命令
                try_num++;
                res = send_cmd(cmd,data_packet,data_len,try_num);
            }

        }
        break;//成功
		case 1:
        //服务器返回的数据错误 递归重新发送命令
        try_num++;
        res = send_cmd(cmd,data_packet,data_len,try_num);
        //连接服务器
        //CLOSE_IP();
        //current_status = TCPIP_CONNECT;
        //g_gprs_conn_ok = 0;


        break;//未连接
		case 2://模块关机
                    //重新开机模块
                    //if(reset_gprs()==0){
                    //重新初始化连接服务器成功
                    //递归发送
                    //try_num++;
                    //res=send_cmd(cmd,data_packet,data_len,try_num);
                    //}else{
                    //不做其他努力 返回
                    //return 1;
                    //}
                    break;
                }

    return res;
}

/*
发送命令给服务器
cmd--要发送的命令码
返回值：0--发送成功 接收成功；1--gprs无响应；2--gprs 服务器断开；3--接收服务器回复信息 超时
这个函数写成递归函数 一个命令超过5次 都不成功那么  说明失败了 退出更新模式
*/
u8 packet_cmd(TRANS_COMMAND_TYPE cmd)
{
    u8 flag=0;//标示 结果
    u8 i=0;
    DEVICE_CMD_HEADER header;
    DEVICE_CHECKVERSION check_version;
    DEVICE_REQUESTFILE file;
    DEVICE_REQUESTFILEDATA file_data;
    UPDATE_FLAG  update_message;
    u8 crc_buf[2]={0};//crc 缓存

    u8 temp[256];

    memset(&header,0,sizeof(DEVICE_CMD_HEADER));
    memset(&check_version,0,sizeof(DEVICE_CHECKVERSION));
    memset(&file,0,sizeof(DEVICE_REQUESTFILE));
    memset(&file_data,0,sizeof(DEVICE_REQUESTFILEDATA));
    memset(&update_message,0,sizeof(update_message));

    memset(temp,0,sizeof(temp));

    header.headFlag=DEVICE_COMMAND_HEAD_FLAG;
    header.commandType=cmd;
    sprintf((char*)header.commandVersion,"%s",g_deviceparam_data.mideware_version);

    sprintf((char*)header.productname,"%s",p_name);
    sprintf((char*)header.producttype,"%s",p_type);
    for(i=0;i<DEVICE_ID_LEN;i++)
    {
        header.deviceid[i]=0;
    }
    sprintf((char*)header.deviceid,"%s",p_sn);

    //header.deviceid[10]=0;
    printf("p_sn 5=%s\r\n",p_sn);



    switch(cmd)
    {
    case DEVICE_COMMAND_CHECKVERSION ://检查版本号信息
        header.dataLength=0;
        header.crc_check=0;
        check_version.header=header;
        //发送命令并等待回复
        printf("DEVICE_COMMAND_CHECKVERSION\r\n");
        flag= send_cmd(cmd,(u8*)&header,sizeof(header),1);
        break;

    case DEVICE_COMMAND_REQUESTFILEINFO ://获取升级文件信息
        header.dataLength=0;
        header.crc_check=0;
        file.header=header;
        //发送命令并等待回复
        flag= send_cmd(cmd,(u8*)&file,sizeof(file),1);
        break;

    case DEVICE_COMMAND_REQUESTFILEDATA ://获取升级文件

        printf("DEVICE_COMMAND_REQUESTFILEDATA\r\n");
        //计算crc
        file_data.packetnum = g_curfilepacknum;
        memcpy(crc_buf,&file_data.packetnum,2);
        header.dataLength=2;
        header.crc_check=CalcCrc16(crc_buf,2);
        file_data.header=header;

        sprintf((char *)temp,"file_data.packetnum:%d\r\n",file_data.packetnum);
        printf("temp=%s\r\n",temp);

        //发送命令并等待回复
        flag= send_cmd(cmd,(u8*)&file_data,sizeof(file_data),1);
        break;

    case DEVICE_COMMAND_RESP_ERROR :
        update_message._update_flag=UPDATE_SUCCEED_FLAG;
        flag= send_cmd(cmd,(u8*)&update_message,sizeof(update_message),1);   //向服务器报告  升级情况
        break;

    case DEVICE_COMMAND_FILEDATA ://不知道这个干什么用

        break;
    }


    //sprintf(temp,"index 22:%d\r\n",buf_uart2.index);
    //	PUT((char*)temp);
    return flag;
}

/*
退出更新模式 开始根据情况执行相应程序
读取flash中的 标志位决定是执行 app 还是执行 iap程序的业务逻辑区

*/
void run()
{
    //根据标志位决定执行哪段程序
    //if(params_save_stru.run_flag==1||params_save_stru.run_flag==0)
    //{
    //#ifdef __debug
    //_debug("run app\r\n");
    //#endif
    //切换到app之前关掉gprs
    iap_load_app(FLASH_APP1_ADDR);//执行FLASH APP代码
    //}
}
/*
升级主程序逻辑
1 尝试连接升级服务器
2 

*/
void update_logic()
{
    u8 r=0;//发送命令函数返回的值 该值标示 是否发送成功
    u8 temp[256];//用于输出临时信息 用于调试

    SERVER2DEVICE_CHECKVERSION resp_stru_checkversion;//服务器回复的 询问是否有更新版本的 指令结构体
    SERVER2DEVICE_FILEINFO resp_stru_fileinfo;//服务器回复的更新包信息结构体
    SERVER2DEVICE_FILEDATA resp_stru_filedata;//服务器回复的更新数据结构体


    memset(temp,0,sizeof(temp));
    memset(&resp_stru_checkversion,0,sizeof(SERVER2DEVICE_CHECKVERSION));
    memset(&resp_stru_fileinfo,0,sizeof(SERVER2DEVICE_FILEINFO));
    memset(&resp_stru_filedata,0,sizeof(SERVER2DEVICE_FILEDATA));



    //iap_load_app(FLASH_BOOT_ADDR);
    //iap_load_app(FLASH_APP1_ADDR);

    printf("update_logic\r\n");
    //read_upgradeparams_save_flash();//读取iap参数

    printf("p_sn 4=%s\r\n",p_sn);


    /*
		从flash 读取 参数保存结构体
		连接服务器
		询问更新包 同时 更新 param_save_stru.run_flag 并保存 到flash
		通过目标版本号，与服务返回的更新文件信息比对，如果版本号一致，那么从 params_save_stru.cur_file_packnum开始 索要文件包；
		如果不一致，那么重新赋值 param_save_stru.update_version,然后从第0个开始索要  升级文件数据包
		
		在保存 更新文件数据的时候  每 10 次，保存在  param_save_stru.cur_file_packnum
		更新完毕  修改 param_save_stru.cur_version 并保存 
		
		根据升级情况 向服务器 发送 DEVICE_COMMAND_RESP_ERROR
	*/	
    //检查gprs是否开手机

    //向升级服务器发起连接
    connect_tcpserver(g_deviceparam_data.upgrade_system_addr,g_deviceparam_data.upgrade_system_port);

    //到此为止已经连接上了服务器  可以开始和服务器沟通了
    printf("update_logic connect ok\r\n");

    //首先发送 指令  询问服务器是否 自己 是跳转 还是升级  还是运行iap区程序
    r=0;
    printf("packet_cmd before\r\n");
    r= packet_cmd(DEVICE_COMMAND_CHECKVERSION);

    //sprintf(temp,"index 33:%d\r\n",buf_uart2.index);
    //PUT((char*)temp);

    if(r!=0)
    {
        printf("r != 0 \r\n");
        //run();//命令发送出错
        return;
    }
    //+IPD,1,67:2017042001 去掉服务器返回的字节头*///???可不可以定义一个check_result全局变量

    //处理回复结果  回复结果在 USART_RX_BUF 中 本次接收的长度 在USART_RX_Len 中
    memcpy(&resp_stru_fileinfo,USART2_RxBuf,USART2_RxTail);

    switch(resp_stru_checkversion.check_result){
    case 0:    //0--需要升级  升级成功后跳转到APP执行

        //需要升级，先擦除升级区

        r=0;   //向服务器索要更新包信息
        printf("DEVICE_COMMAND_REQUESTFILEINFO before\r\n");
        r= packet_cmd(DEVICE_COMMAND_REQUESTFILEINFO);

        if(r!=0)
        {
            printf("send DEVICE_COMMAND_REQUESTFILEINFO error\r\n");
            //run();//命令发送出错
            return;
        }else//收到了更新包信息
        {
            memcpy(&resp_stru_fileinfo,USART2_RxBuf,USART2_RxTail);

            printf("recv file info\r\n");

            printf("resp_stru_fileinfo.Version=%s\r\n",resp_stru_fileinfo.Version);
            printf("g_deviceparam_data.mideware_version=%s\r\n",g_deviceparam_data.mideware_version);
            printf("resp_stru_fileinfo.packetcount=%d\r\n",resp_stru_fileinfo.packetcount);
            printf("strcmp((char*)resp_stru_fileinfo.Version,(char*)g_deviceparam_data.mideware_version)=%d\r\n",strcmp((char*)resp_stru_fileinfo.Version,(char*)g_deviceparam_data.mideware_version));

            //服务器的版本号只有大于目前的版本号才能更新
            if(strcmp((char*)resp_stru_fileinfo.Version,(char*)g_deviceparam_data.mideware_version) <= 0)
            {
                printf("version nochange\r\n");
                run();//命令发送出错
                return;
            }
            //升级之前先把升级标志位置0，表示不可用
            g_deviceparam_data.update_result = 0;
            if(g_deviceparam_data.write_flag)
            {
                write_deviceparam_save_flash();
                write_deviceparam_save_flash_beifen();
            }
            delay_ms(5000);
            STMFLASH_Erase(FLASH_APP1_ADDR);

            while(g_curfilepacknum < resp_stru_fileinfo.packetcount)//获取到的数据包 小于分包数量
            {
                r=0;//向服务器索要更新包信息
                printf("DEVICE_COMMAND_REQUESTFILEDATA before\r\n");

                r= packet_cmd(DEVICE_COMMAND_REQUESTFILEDATA);


                sprintf((char*)temp,"cur_file_packnum:%d,resp_stru_fileinfo.packetcount:%d\r\n",g_curfilepacknum,resp_stru_fileinfo.packetcount);//打印iap参数
                printf("temp=%s\r\n",temp);

                if(r!=0)
                {
                    //给服务器发送 更新不成功指令
                    //run();//命令发送出错
                    printf("send DEVICE_COMMAND_REQUESTFILEDATA error\r\n");
                    return;
                }
                else//收到了更新包信息
                {
                    printf("USART2_RxBuf = %s\r\n",USART2_RxBuf);
                    //	sprintf((char*)temp,"buf_uart3.index:%d,i:%d\r\n",buf_uart3.index,i);//打印iap参数
                    printf("temp=%s\r\n",temp);

                    //处理回复结果  回复结果在 USART_RX_BUF 中 本次接收的长度 在USART_RX_Len 中
                    memcpy(&resp_stru_filedata,USART2_RxBuf,USART2_RxTail);
                    //将收到的app代码写入flash   //测试注销  需要恢复
                    printf("cur_file_packnum=%d\r\n",g_curfilepacknum);
                    write_app2flash(FLASH_APP1_ADDR+512*g_curfilepacknum,(u32*)(resp_stru_filedata.filedata),128);

                    g_curfilepacknum++;
                }
            }

            if(g_curfilepacknum == resp_stru_fileinfo.packetcount) //只有写入数据包的数量与总报数相同 才执行
            {
                printf("params_save_stru.cur_file_packnum==resp_stru_fileinfo.packetcount\r\n");

                //升级完毕，将最新的版本号写到flash中
                memset(g_deviceparam_data.mideware_version,0,sizeof(g_deviceparam_data.mideware_version));
                strcpy((char*)(g_deviceparam_data.mideware_version),(char *)resp_stru_fileinfo.Version);
                g_deviceparam_data.update_result = 1;

                if(g_deviceparam_data.write_flag)
                {
                    write_deviceparam_save_flash();
                    write_deviceparam_save_flash_beifen();
                }
                delay_ms(2000);
                read_deviceparam_save_flash();

                if(g_deviceparam_data.write_flag != 1)
                {
                    read_deviceparam_save_flash_beifen();
                    delay_ms(2000);
                    write_deviceparam_save_flash();
                }
                printf("write new version to flash\r\n");

                run();
            }
            //回到IAP主逻辑区
        }
        break;
		case 1: // 1--不更新  跳转到app区 执行
#ifdef __debug
                    _debug("no updata jump app\r\n");
#endif//不更新
                    run();
                    break;
		case 2:  //2--不更新 继续执行iap区业务逻辑	
#ifdef __debug
                    _debug("no updata run iap\r\n");
#endif//不更新

                    return;
                }
}













