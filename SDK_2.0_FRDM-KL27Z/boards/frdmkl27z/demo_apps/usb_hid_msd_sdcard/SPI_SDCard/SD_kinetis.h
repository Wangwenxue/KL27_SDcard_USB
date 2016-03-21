#ifndef __SD__
#define __SD__

#include "fsl_common.h"

#define CMD0    0   /* GO_IDLE_STATE */  
#define CMD55   55  /* APP_CMD */  
#define ACMD41  41  /* SEND_OP_COND (ACMD) */  
#define	ACMD23	23/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD1    1   /* SEND_OP_COND */
#define CMD7    7   /* Set SDCard */
#define CMD16   16  /* Set Block size */ 
#define CMD17   17  /* READ_SINGLE_BLOCK */  
#define CMD8    8   /* SEND_IF_COND */  
#define CMD18   18  /* READ_MULTIPLE_BLOCK */  
#define CMD12   12  /* STOP_TRANSMISSION */  
#define CMD24   24  /* WRITE_BLOCK */  
#define CMD25   25  /* WRITE_MULTIPLE_BLOCK */  
#define CMD13   13  /* SEND_STATUS */  
#define CMD9    9   /* SEND_CSD */  
#define CMD10   10  /* SEND_CID */

#define CSD     9  
#define CID     10  

#define SD_Detect       GPIOD_PDIR & GPIO_PDIR_PDI(0)

// A pull-up resistor reside in SD card detection port
typedef enum{
  SD_Desert,
  SD_Insert
} SD_Detect_State;

extern SD_Detect_State SD_Flag;

extern void Delayms(unsigned int cnt);

extern void SD_Detect_Init(void);

extern unsigned char SD_SendCmd(unsigned char cmd, unsigned long arg, unsigned char crc);

extern unsigned char SD_Reset(void);

extern unsigned char SD_Init(void);

extern unsigned char SD_ReadSector(unsigned long addr,unsigned char * buffer) ;

extern unsigned char SD_ReadMultiSector(unsigned long  addr, unsigned char * buffer, unsigned char sector_num);

extern unsigned char SD_WriteSector(unsigned long  addr, unsigned char * buffer);

extern unsigned char SD_WriteMultiSector(unsigned long  addr, unsigned char * buffer, unsigned char sector_num);  

extern unsigned char SD_GetCIDCSD(unsigned char cid_csd, unsigned char * buffer);

extern unsigned long  SD_GetCapacity(void);

#endif
