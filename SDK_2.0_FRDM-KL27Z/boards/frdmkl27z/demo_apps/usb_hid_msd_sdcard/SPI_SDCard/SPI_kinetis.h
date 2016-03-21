/******************************************************************************
*
* Freescale Semiconductor Inc.
* (c) Copyright 2004-2009 Freescale Semiconductor, Inc.
* ALL RIGHTS RESERVED.
*
**************************************************************************//*!
*
* @file SPI_kinetis.h
*
* @author
*
* @version
*
* @date May-28-2009
*
* @brief  This file is SPI Driver Header File
*****************************************************************************/
#ifndef __SPI__
#define __SPI__


/* Includes */
#include "fsl_common.h"


/* Defines */

/* Prototypes */
void   SPI_Init(void);
unsigned char SpiSendByte(unsigned char u8Data);
unsigned short SpiSendWord(unsigned short u16Data);
unsigned int SpiSendFrame(unsigned char *in, unsigned char *out, unsigned int size);
void   SpiHighRate(void);
void   SpiCsLow(void);
void   SpiCsHigh(void);

#endif /* __SPI__ */
