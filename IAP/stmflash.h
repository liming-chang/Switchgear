#ifndef __STMFLASH_H__
#define __STMFLASH_H__
#include "stm32f4xx.h" 
#include "stm32f4xx_flash.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////
//用户根据自己的需要设置
#define STM32_FLASH_SIZE 512 	 		//所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 1              //使能FLASH写入(0，不是能;1，使能)
//////////////////////////////////////////////////////////////////////////////////////////////////////

//FLASH起始地址
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH的起始地址

/* Base address of the Flash sectors */
 #define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Sector 0, 16 Kbytes */
 #define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Sector 1, 16 Kbytes */
 #define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Sector 2, 16 Kbytes */
 #define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Sector 3, 16 Kbytes */
 #define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Sector 4, 64 Kbytes */
 #define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Sector 5, 128 Kbytes */
 #define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Sector 6, 128 Kbytes */
 #define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Sector 7, 128 Kbytes */
 #define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Sector 8, 128 Kbytes */
 #define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Sector 9, 128 Kbytes */
 #define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Sector 10, 128 Kbytes */
 #define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Sector 11, 128 Kbytes */


//FLASH解锁键值
 

u16 STMFLASH_ReadHalfWord(u32 faddr);		  //读出半字  
void STMFLASH_WriteLenByte(u32 WriteAddr,u32 DataToWrite,u16 Len);	//指定地址开始写入指定长度的数据
u32 STMFLASH_ReadLenByte(u32 ReadAddr,u16 Len);						//指定地址开始读取指定长度数据
void STMFLASH_Write(u32 WriteAddr,u32 *pBuffer,u32 NumToWrite);		//从指定地址开始写入指定长度的数据
void STMFLASH_Read(u32 ReadAddr,u32 *pBuffer,u32 NumToRead);   		//从指定地址开始读出指定长度的数据
void STMFLASH_Erase(u32 AppAddr);	//按照扇区擦除flash	


uint32_t GetSector(uint32_t Address);


//测试写入
void Test_Write(u32 WriteAddr,u16 WriteData);								   
#endif

















