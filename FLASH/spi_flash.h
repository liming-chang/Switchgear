#ifndef _SPI_FALSH_H
#define _SPI_FALSH_H

#include "stm32f4xx.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_gpio.h"

#define PAGE_SIZE	256
#define PAGE_MASK	(PAGE_SIZE-1)

#define SREG_SIZE	256
#define SREG_MASK	(SREG_SIZE-1)

#define SECTOR_SIZE	0x1000
#define SECTOR_MASK	(SECTOR_SIZE-1)
#define SECTOR_SHIFT 12

/* reserve half of flash capacity
 * FL_SIZE must be power of 2 to use rp/wp wrap around feature of int type
 */
#define FL_SIZE	0x080000
#define FL_MASK	(FL_SIZE-1)

struct flash_config {
	char magic[3];		//  00..02
	char cnum[11];		//  03..13
	char imei[15];		//  14..28
	char smsc[3][13];		//  29..41 china mobile
//	char smsc1[13];		//  42..54 china unicom
//	char smsc2[13];		//  55..67 admin
	char svcip[26];		//  68..93 len:42.96.200.118:90090000000
	char softversion[16];	//  94-109 ppzc141019v9.bin
	char od_pol;		// 110-111 OD,delay(in 0.1s)
	char od_delay;
	char cd_pol;		// 112-113 CD,delay(in 0.1s)
	char cd_delay;
	char __unused1[4];	// 114-117 ???
	char sn[8];			// 118-125
	char __unused2[1];	// 126-126
	char rr_int1[2];	// 127-128 report interval 1
	char rr_int2[3];	// 129-131 report interval 2
	char milepost;		// 132-132 P/N
	char __unused5[8];	// 133-140
	char updip[26];		// 141-166 update server 54.254.103.96:64201000000
};

typedef struct flash_record {
#ifndef __BIG_ENDIAN
/* B7 6 5 4  3 2 1 0
 *  `--read  written
 */
		int written:4;		// 0:written
		int read:4;			// 0:uploaded
#else
		int read:4;			// 0:uploaded
		int written:4;		// 0:written
#endif
	char type;			// 16bit aligned

	int time;			// utc seconds
	__packed union {
	__packed	struct {
			int lat;			// in 1/10000min
			int lon;			// in 1/10000min
			unsigned short sog;	// in 0.01m/s
		
			unsigned short cog;	// in 0.01deg
		} gps;
	__packed	struct {
			int cell[3];
		} lbs;
	} u;
	unsigned int odo;		// in 0.1m/s
	unsigned char flags;	//7..0:io_bits
	unsigned short useq;
	int year;	//年-UTC时间
	int month;  //月-UTC时间
	int day;    //日-UTC时间
	int hour;   //时-UTC时间
	int min;   //分-UTC时间
	int sec;   //秒-UTC时间
} __attribute__((packed)) rec_t;

// Resource-Record aligned to 16-byte boundary
#define RR_SIZE	((sizeof(rec_t)+ 0xf)&(~0xf))



extern unsigned int flash_rp, flash_wp;
extern struct flash_config *pconfig;
extern const struct flash_config default_config;

void flash_enablewrite(void);
void flash_disablewrite(void);

void spiflash_init(void);
int flash_status(void);
int flash_isbusy(void);

int flash_readid(void);
int flash_readid2(void);
int flash_readuid(char *buf);

int flash_read_sreg(int id, int off, char *buf, int nbytes);
int flash_erase_sreg(int id);
int flash_write_sreg(int id, int off, char *buf, int nbytes);

void flash_erase_all(void);
void flash_erase_block(int addr);
void flash_erase_sector(int addr);

//int flash_programpage(int addr, char * buff, unsigned int nbytes);
int flash_read(int addr, char * buff, unsigned int nbytes);
int flash_write(int addr, char *buff, unsigned int nbytes);

int flash_read_config(struct flash_config *);
int flash_write_config(struct flash_config *);

int flash_add_record(rec_t *);
int flash_get_record(rec_t *, int n);
int flash_get_record_abs(rec_t *rr, int n);
int flash_del_record(int n);
int flash_get_odo(void);
int encode_spos(char *buf, rec_t *rr);
static __inline unsigned int rr_len(void)
{
	return ((flash_wp-flash_rp)%FL_SIZE)/RR_SIZE;
}

#endif
