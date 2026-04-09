#include "stmflash.h"
#include "iap.h"



/******************************************************
函数名称: uint32_t GetSector(uint32_t Address)
功    能：判断地址所在的Sector
输    入：uint32_t Address ：存储空间首地址
输    出：uint32_t ：Sector编号
******************************************************/
uint32_t GetSector(uint32_t Address)
{
    uint32_t sector = 0;

    if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
    {
        sector = FLASH_Sector_0;
    }
    else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
    {
        sector = FLASH_Sector_1;
    }
    else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
    {
        sector = FLASH_Sector_2;
    }
    else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
    {
        sector = FLASH_Sector_3;
    }
    else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
    {
        sector = FLASH_Sector_4;
    }
    else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
    {
        sector = FLASH_Sector_5;
    }
    else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
    {
        sector = FLASH_Sector_6;
    }
    else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
    {
        sector = FLASH_Sector_7;
    }
    else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
    {
        sector = FLASH_Sector_8;
    }
    else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
    {
        sector = FLASH_Sector_9;
    }
    else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
    {
        sector = FLASH_Sector_10;
    }
    else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
    {
        sector = FLASH_Sector_11;
    }

    return sector;
}




//读取指定地址的半字(16位数据)
//faddr:读地址(此地址必须为2的倍数!!)
//返回值:对应数据.
u16 STMFLASH_ReadHalfWord(u32 faddr)
{
    return *(vu16*)faddr;
}

//读取指定地址的字(32位数据)
//faddr:读地址(此地址必须为4的倍数!!)
//返回值:对应数据.
u32 STMFLASH_ReadWord(u32 faddr)
{
    return *(vu32*)faddr;
}



#if STM32_FLASH_WREN	//如果使能了写   
//不检查的写入
//WriteAddr:起始地址
//pBuffer:数据指针
//NumToWrite:半字(16位)数   
void STMFLASH_Write_NoCheck(u32 WriteAddr,u32 *pBuffer,u32 NumToWrite)   
{ 			 		 
    u32 i;
    for(i=0;i<NumToWrite;i++)
    {
        if(FLASH_ProgramWord(WriteAddr,pBuffer[i]) != FLASH_COMPLETE)
        {
            //printf("write failed\r\n");
        }
        WriteAddr += 4;
    }
} 

//写数据之前先擦出之前的程序区sector
void STMFLASH_Erase(u32 AppAddr)
{
    FLASH_Unlock();//解除FLASH后才能向FLASH中写数据

    FLASH_DataCacheCmd(DISABLE);

    if(FLASH_EraseSector(GetSector(AppAddr),VoltageRange_3) != FLASH_COMPLETE)
    {
        //printf("Erase failed\r\n");
    }
    else
    {
        //printf("Erase Success\r\n");
    }

    FLASH_DataCacheCmd(ENABLE);

    //上锁
    FLASH_Lock();
}


//从指定地址开始写入指定长度的数据
//WriteAddr:起始地址(此地址必须为2的倍数!!)
//pBuffer:数据指针
//NumToWrite:半字(16位)数(就是要写入的16位数据的个数.)
#if STM32_FLASH_SIZE<256
#define STM_SECTOR_SIZE 1024 //字节
#else 
#define STM_SECTOR_SIZE	2048
#endif		 
u16 STMFLASH_BUF[STM_SECTOR_SIZE/2];//最多是2K字节
void STMFLASH_Write(u32 WriteAddr,u32 *pBuffer,u32 NumToWrite)	
{


    FLASH_Unlock();//解除FLASH后才能向FLASH中写数据

    FLASH_DataCacheCmd(DISABLE);

    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    STMFLASH_Write_NoCheck(WriteAddr,pBuffer,NumToWrite);

    FLASH_DataCacheCmd(ENABLE);

    //上锁
    FLASH_Lock();
}
#endif

//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToWrite:字(32位)数
void STMFLASH_Read(u32 ReadAddr,u32 *pBuffer,u32 NumToRead)   	
{
    u32 i;
    for(i = 0;i < NumToRead;i++)
    {
        pBuffer[i]=STMFLASH_ReadWord(ReadAddr);//读取4个字节.
        ReadAddr += 4;//偏移4个字节.
    }

}


















