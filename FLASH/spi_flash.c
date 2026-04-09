#include <string.h>
#include <stdio.h>
#include "spi_flash.h"
//#include "bitop.h"
#include "time.h"
#include "delay.h"
//#include "uart.h"
//#include "nvram.h"
//#include "isr.h"

#define CMD_WREN	0x06
#define CMD_WRDI	0x04
#define CMD_RDID	0x90
#define CMD_RDID2	0x9f
#define CMD_RDSR	0x05
#define CMD_WRSR	0x01
#define CMD_READ	0x03
#define CMD_PP		0x02
#define CMD_SE		0xd8
#define CMD_CE		0xc7

#define FLASH_SELECT GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define FLASH_RELEASE GPIO_SetBits(GPIOB, GPIO_Pin_12)
//#define FLASH_CS	PBout(12)
//#define FLASH_SELECT	do { FLASH_CS=0; } while(0)
//#define FLASH_RELEASE	do { FLASH_CS=1; } while(0)


//struct nvram_t nvram;

void spi_delay(void)
{
	delay_ms(100);
}

/*
 * addr < 0  表示24位地址字段不存在
 *      = *  表示24位地址A23..A0
 * dummy 表示需空等待周期个数
 * len  > 0  读
 *      < 0  写
 */
int __spi_cmd(char cmd, int addr, int dummy, char *data, int len)
{
	char buf[16], *p = buf, *d = data;
	int i,n;

	FLASH_SELECT;

	*p++ = cmd;
	if(addr >= 0) {
		*p++ = (addr>>16)&0xff;
		*p++ = (addr>> 8)&0xff;
		*p++ =  addr     &0xff;
	}

	for(i = 0; i < dummy; i++) *p++ = 0x00;
	n = p - buf;


	// write cmd
	for(i = 0; i < n; i++) {
		while(!(SPI2->SR & SPI_I2S_FLAG_TXE));
		SPI2->DR = buf[i];
		while(!(SPI2->SR & SPI_I2S_FLAG_RXNE));
		(void)SPI2->DR;
	}


	if(len > 0) {
		// read data
		for(i = 0; i < len; i++) {
			while(!(SPI2->SR & SPI_I2S_FLAG_TXE));
			SPI2->DR = 0x00;
			while(!(SPI2->SR & SPI_I2S_FLAG_RXNE));
			*d++ = SPI2->DR;
		}
	} else if(len < 0) {
		len = -len;
		// write data
//		xdump(addr, d, len, 1);
		for(i = 0; i < len; i++) {
			while(!(SPI2->SR & SPI_I2S_FLAG_TXE));
			SPI2->DR = *d++;
			while(!(SPI2->SR & SPI_I2S_FLAG_RXNE));
			(void)SPI2->DR;
		}
	}

	FLASH_RELEASE;
	
	return 0;	
}


// Status Register 1&2
//      S7   S6   S5   S4   S3   S2   S1   S0
// SR1: SRP0 SEC  TB   BP2  BP1  BP0  WEL  BUSY
// SR2: SUS  CMP  LB3  LB2  LB1  ---  QE   SRP1
int flash_status()
{
	char sr1,sr2;

	__spi_cmd(0x05, -1, -1, &sr1, 1);
	__spi_cmd(0x35, -1, -1, &sr2, 1);


	return ((sr2<<8|sr1)&0xffff);
}

int flash_isbusy()
{
	int sr = flash_status();

	return (sr&0x1);
}

int spi_cmd(char cmd, int addr, int dummy, char *data, int len)
{
  printf("spi_cmd 0x%02x, addr=0x%08x, dummy=%d, len=%d\r\n", cmd, addr, dummy, len);

	__spi_cmd(cmd, addr, dummy, data, len);

	// wait for completion
	while(flash_isbusy()) spi_delay();

 printf("spi_cmd done: SR=0x%04X\r\n", flash_status());

	return 0;
}

void flash_enablewrite()
{
	spi_cmd(0x06, -1, -1, 0, 0);
	spi_delay();
}

void flash_disablewrite()
{
	spi_cmd(0x04, -1, -1, 0, 0);
	spi_delay();
}

// READ ID
// Byte1    B2    B3    B4    B5    B6
//  90h     --    --    00    VID   PID
int flash_readid()
{
	int id = 0;
	char buf[2];

	spi_cmd(0x90, 0, -1, buf, 2);
	id = (buf[0]<<8) | buf[1];
	id &= 0xffff;

	return id;
}

// JEDEC ID
// Byte1    B2    B3    B4    B5    B6
//  9Fh     VID   PIDH  PIDL
int flash_readid2()
{
	int id = 0;
	char buf[3];

	spi_cmd(0x9F, -1, -1, buf, 3);
	id = (buf[0]<<16) | (buf[1]<<8) | buf[2];
	id &= 0xffffff;

	return id;
}

// UNIQUE ID
// Byte1    B2    B3    B4    B5    B6
//  4Bh     --    --    --    --    ID63..ID0
int flash_readuid(char *uid)
{
	spi_cmd(0x4B, 0, 1, uid, 8);
	return 0;
}

int flash_read(int addr, char *buf, unsigned int nbytes)
{
	spi_cmd(0x03, addr, -1, buf, nbytes);
	return nbytes;
}

// security registers
// 0x001000 reg1
// 0x002000 reg2
// 0x003000 reg3
int flash_read_sreg(int id, int off, char *buf, int nbytes)
{
	if(id > 2 || (off+nbytes) > SREG_SIZE) return 0;
	id++;

	// A7..0: byte address
	spi_cmd(0x48, id<<12|off, 1, buf, nbytes);

	return nbytes;
}

int flash_erase_sreg(int id)
{
	if(id > 2) return 0;
	id++;

	flash_enablewrite();
	spi_cmd(0x44, id<<12, -1, 0, 0);

	return 0;
}

int flash_write_sreg(int id, int off, char *buf, int nbytes)
{
	if(id > 2 || (off+nbytes) > SREG_SIZE) return 0;
	id++;

	flash_enablewrite();
	// A7..0: byte address
	spi_cmd(0x42, id<<12|off, -1, buf, -nbytes);

	return nbytes;
}

int flash_write_sreg2(int id, int off, char *buf, int nbytes)
{
	char tmpbuf[SREG_SIZE];
	char *p, *s = buf;
	int i;

	if(id > 2 || (off+nbytes) > SREG_SIZE) return 0;

	if(!off || nbytes != SREG_SIZE) {
		/* partial write, read back and merge */
		flash_read_sreg(id, 0, tmpbuf, SREG_SIZE);
		p = tmpbuf+off;
		i = nbytes;
		while(i--) *p++ = *s++;
		s = tmpbuf;
	} else {
		/* full write, no need to copy buffer */
//		s = buf;
	}

	flash_erase_sreg(id);
	flash_write_sreg(id, off, s, SREG_SIZE);

	return nbytes;
}

int flash_write(int addr, char *buf, unsigned int nbytes)
{
	int rem = nbytes;	// remaining bytes to be written
	int len;

	do {
		// align on page boundaries
		if(addr&PAGE_MASK)
			len = PAGE_SIZE - (addr & PAGE_MASK);
		else
			len = PAGE_SIZE;

		if(len > rem)
			len = rem;

		while(flash_isbusy()) spi_delay();
		flash_enablewrite();
		spi_cmd(0x02, addr, 0, buf, -len);

		rem  -= len;
		addr += len;
		buf  += len;
	} while(rem > 0);

	// bytes written
	return (nbytes-rem);
}

void flash_erase_sector(int addr)
{
	flash_enablewrite();
	spi_cmd(0x20, addr, -1, 0, 0);
}

void flash_erase_block32(int addr)
{
	flash_enablewrite();
	spi_cmd(0x52, addr, -1, 0, 0);
}

void flash_erase_block(int addr)
{
	flash_enablewrite();
	spi_cmd(0xd8, addr, -1, 0, 0);
}

void flash_erase_all()
{
	//extern void watchdog_clear(void);

	// ensure watchdog feed before erase
	//watchdog_clear();

	flash_enablewrite();
	spi_cmd(0xc7, -1, -1, 0, 0);
}

/* xor all bytes */
int flash_verify_config(struct flash_config *pconf)
{
	int i;
	unsigned char *p = (unsigned char *)pconf;
	unsigned char sum = 0;

	for(i = 0; i < sizeof(struct flash_config); i++, p++)
		sum ^= *p;

	return sum;
}

int flash_read_config(struct flash_config *pconf)
{
	int sum, magic;

	/* 1st: try backup */
	flash_read_sreg(1, 0, (char *)pconf, sizeof(struct flash_config));
	//magic = _be16p(pconf->magic);
	sum = flash_verify_config(pconf);

	if(magic == 0x55AA && !sum) {
		/* cleanup: possible power shut during flash_write_config() */
		flash_write_sreg2(0, 0, (char *)pconf, sizeof(struct flash_config));
		/* erase backup */
		flash_erase_sreg(1);
	} else {
		/* 2nd: normal */
		flash_read_sreg(0, 0, (char *)pconf, sizeof(struct flash_config));
	//	magic = _be16p(pconf->magic);
		sum = flash_verify_config(pconf);

		if(magic != 0x55AA || sum) {
			//printf("no valid config on sreg, dump:\r\n");
			//xdump(0, (char *)pconf, sizeof(struct flash_config), 1);
	
			/* 3rd: try old scheme */
			flash_read(0, (char *)pconf, sizeof(struct flash_config));
			//if(_be24p(pconf->magic) != 0x112233) {
				//memset(pconf, 0, sizeof(struct flash_config));
				//memcpy(pconf, &default_config, sizeof(struct flash_config));
				//printf("no valid config, using default\r\n");
			//} else {
				/* config migaration */
				//flash_write_config(pconf);
			//}
		}
	}

	return 0;
}

/*
 * Policy: write to backup to avoid power shut
 *
 */
int flash_write_config(struct flash_config *pconf)
{
	pconf->magic[0] = 0x55;
	pconf->magic[1] = 0xaa;
	pconf->magic[2] = 0x00;
	pconf->magic[2] = flash_verify_config(pconf);

	/* write to backup first */
	flash_write_sreg2(1, 0, (char *)pconf, sizeof(struct flash_config));
	/* then normal block */
	flash_write_sreg2(0, 0, (char *)pconf, sizeof(struct flash_config));
	/* if ready, erase backup */
	flash_erase_sreg(1);

	return 0;
}

unsigned int flash_rp, flash_wp;
int flash_init_record()
{
	int i;
	int cp,rp,wp;
	time_t j0, j1;
	int t;
	char buf[32];
	rec_t *pr = (rec_t *)buf;
	struct {
		unsigned int t;
		int cp;
	} _00, _f00, _f01;

	printf("search flash...\r\n");
	/* start profiling */
//	j0 = jiffies;

	_00.t = _f01.t = 0;
	_f00.t = 0xffffffff;
	_00.cp = _f00.cp = _f01.cp = 0;

	printf("111111111\r\n");

	/* feed dog */
	//watchdog_clear();

	printf("222222222222\r\n");

	for(i = 0; i < FL_SIZE/SECTOR_SIZE; i++) 
	{
		cp = i<<SECTOR_SHIFT;
		flash_read(cp, buf, 6);

		/* find rp */
		if(buf[0] == (char)0xff)
			t = 0;
		else
			t = pr->time;
		if(buf[0] == 0x00)
		{
			if(t > _00.t) 
			{
				_00.t = t; _00.cp = cp; 
			}
		} 
		else if(buf[0] == (char)0xf0) 
		{
			if(t < _f00.t) 
			{
				_f00.t = t; _f00.cp = cp; 
			}
			if(t >= _f01.t) 
			{
				_f01.t = t; 
				_f01.cp = cp; 
			}
		}
//		printf("%3d: [%02X] t0:%08x t1:%08x t:%08x\r\n", i, (unsigned char)buf[0], _f00.t, _f01.t, t);
	}

	printf("333333333333\r\n");

	//watchdog_clear();


	printf("4444444444444444\r\n");
	
	if(_f01.t != 0) 
	{
		/* 0xf0 found, found rp backward and wp forwardly
		 *    xx f0 f0 f0 f0 xx
		 *    <--rp       wp-->
		 */
		printf("0xf0 found.\r\n");
		rp = _f00.cp;
// we erase a sector if no space for new records.
// so rp must be on sector boundary. no need to try backwardly.
//		 for(cp = _f00.cp; cp > _f00.cp-0x1000; cp -= RR_SIZE) {
//		 	flash_read(cp, buf, 6);
//			if(buf[0] == (char)0xf0 && pr->time < _f00.t) rp = cp;
//		 }

		wp = _f01.cp;
		for(cp = _f01.cp; cp < _f01.cp+SECTOR_SIZE; cp += RR_SIZE) 
		{
			flash_read(cp, buf, 6);
			// within sector, record time must be contiguous, no need to cmp.
			// && pr->time > _f01.t
			if(buf[0] == (char)0xf0) 
				wp = cp+RR_SIZE;
		 }
	} 
	else if(_00.t != 0) 
	{
		// no 0xf0, but 0x00 found, rp&wp both in this block
		printf("0xf0 not found, use 0x00.\r\n");
		rp = _00.cp;
		for(cp = _00.cp; cp < _00.cp+SECTOR_SIZE; cp += RR_SIZE)
		{
			flash_read(cp, buf, 1);
			if(buf[0] == (char)0xf0) 
			{
				rp = cp; 
				break;
			}
			else if(buf[0] == (char)0x00) 
			{
				rp = cp+RR_SIZE;
			}
//			printf("--cp:%08x [%02x]\r\n", cp, (unsigned)buf[0]);
		}

		//watchdog_clear();
		wp = _00.cp;
		// *NOTE* cp and _00.cp must be `signed int` to make `>=` condition valid
		// mix signed and unsigned op will result in endless loop.
		for(cp = _00.cp+SECTOR_SIZE; cp >= _00.cp; cp -= RR_SIZE) 
		{
			flash_read(cp, buf, 1);
			if(buf[0] == (char)0xf0) { wp = cp+RR_SIZE; break; }
			else if(buf[0] == (char)0xff) wp = cp;	/* backward to 1st 0xff */
//			printf("++cp:%08x [%02x]\r\n", cp, (unsigned)buf[0]);
		}
		// no 0xf0 or 0xff
		if(wp == _00.cp)
			wp = _00.cp+SECTOR_SIZE;

	} 
	else {
		// empty flash
		rp = wp = 0;
	}

	//watchdog_clear();
	// wrap around
	if(wp < rp) wp += FL_SIZE;

	// stop profiling
	//j1 = jiffies; 
	t = (j1-j0)*10;
	printf("_00:%04x %04x  _f00:%04x %04x  _f01:%04x %04x\r\n",
				_00.t, _00.cp, _f00.t, _f00.cp, _f01.t, _f01.cp);
	printf("rp:%04x  wp:%04x\r\n  %d.%03d seconds used\r\n", rp, wp, t/1000,t%1000);

	flash_rp = rp;
	flash_wp = wp;

	return 0;
}

int flash_add_record(rec_t *rr)
{
	int rp = flash_rp&FL_MASK;
	int wp = flash_wp&FL_MASK;

	// erase if we reach a new sector
	if((wp & SECTOR_MASK) == 0) {
		flash_erase_sector(wp);
		/* buffer not empty */
		if(flash_rp != flash_wp) {
			/* rp in sector, buffer overrun, drop old records */
			if((rp&~SECTOR_MASK) == wp) {
				flash_rp &= ~SECTOR_MASK;
				flash_rp += SECTOR_SIZE;
			}
		}
	}

	flash_write(wp, (char *)rr, sizeof(rec_t));
	flash_wp += RR_SIZE;

//	nvram.rr[0]++;

	return 0;
}

int flash_del_record(int n)
{
	char c = 0;
	int done = 0, i;

	for(i = 0; i < n; i++) {
		if(flash_rp == flash_wp) break;

		flash_write(flash_rp&FL_MASK, &c, 1);
		flash_rp += RR_SIZE;
		done++;
	}

//	nvram.rr[1] += done;

	return done;
}

/*
 * *rr holds record data and return with rp pointer
 * n: nItem index since current position
 */
int flash_get_record(rec_t *rr, int n)
{
	int off = n*RR_SIZE;
	int rp = flash_rp + off;

	if(flash_rp == flash_wp || rp >= flash_wp) return -1;

	rp &= FL_MASK;

	flash_read(rp, (char *)rr, sizeof(rec_t));
	rr->useq = rp/RR_SIZE;

	return rp;
}

/*
 * *rr holds record data and return with rp pointer
 * n: nItem index of absolute position
 */
int flash_get_record_abs(rec_t *rr, int n)
{
	int rp = n*RR_SIZE;
	char *p = (char *)rr;

	if(rp >= FL_SIZE) return -1;

	flash_read(rp, (char *)rr, sizeof(rec_t));

	/* legal check */
	if(*p == (char)0xff)
		rp = -1;
	else
		rr->useq = rp/RR_SIZE;

	return rp;
}

// backward find latest odo
int flash_get_odo(void)
{
	rec_t rr;
	unsigned char *p = (unsigned char *)&rr;
	int i;

	unsigned int rp = flash_wp;
	int odo = 0;

	for(i = 1; i < 5; i++) {
		rp = (flash_wp - RR_SIZE*i)&FL_MASK;
		flash_read(rp, (char *)&rr, sizeof(rr));
		if((p[0] == 0xf0 || p[0] == 0x00) && rr.odo && rr.odo != 0xffffffff)
			odo = rr.odo;
	}

	return odo;
}

void spiflash_init(void)
{
	int ret = 0;
	SPI_InitTypeDef SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE); //使能GPIOB时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);//使能SPI2时钟

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; //复用功能
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //100MZ
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	GPIO_PinAFConfig(GPIOB,GPIO_PinSource13,GPIO_AF_SPI2);//PB13复用为spi2
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource14,GPIO_AF_SPI2);//PB14复用为spi2
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource15,GPIO_AF_SPI2);//PB15复用为spi2

	//RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2,ENABLE); //复位SPI2
	//RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2,DISABLE); //停止复位SPI2


	//spi2
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,  ENABLE);
	//spi3
	//RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;  //SPI CS   
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; //输出
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //100MHZ
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化  
	GPIO_SetBits(GPIOB, GPIO_Pin_12); //拉高不选中


	
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //双线双向全双工 
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;    //主机模式
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;  //8数据位  
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;  //时钟悬空高
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge; //数据捕获于第二个时钟沿 
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;  // 
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;  //波特率预分频为256  
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; //数据从MSB位开始  
	SPI_InitStructure.SPI_CRCPolynomial = 7;   
	SPI_Init(SPI2, &SPI_InitStructure);
	SPI_Cmd(SPI2, ENABLE); //使能SPI


	printf("flash_readid 1111\r\n");

	// scan for existing resource records
	if((ret = (flash_readid()&0xff00)) == 0xef00)
	{
		printf("flash_init_record 4444\r\n");
		flash_init_record();
	}
	else
	{
		printf("ret=0x%x\r\n",ret);
	}
}
