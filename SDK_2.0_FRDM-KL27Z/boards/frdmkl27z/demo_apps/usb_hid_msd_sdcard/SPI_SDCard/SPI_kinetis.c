/******************************************************************************
 *  (c) copyright Freescale Semiconductor China Ltd. 2008
 *  ALL RIGHTS RESERVED
 *  File Name: SPI.C
 *  Description: This file is to handle SPI communication
 *  Assembler:  Codewarrior for HC(S)08 V6.1
 *  Version: 2.0
 *  Author: Patrick Yang
 *  Location: Shanghai, P.R.China
 *
 * UPDATED HISTORY:
 *
 * REV   YYYY.MM.DD  AUTHOR        DESCRIPTION OF CHANGE
 * ---   ----------  ------        ---------------------
 * 1.0   2008.01.02  Patrick Yang  Initial version
 * 2.0   2008.08.12  Jose Ruiz     Speed Change (x2)
 ******************************************************************************/
/* Freescale  is  not  obligated  to  provide  any  support, upgrades or new */
/* releases  of  the Software. Freescale may make changes to the Software at */
/* any time, without any obligation to notify or provide updated versions of */
/* the  Software  to you. Freescale expressly disclaims any warranty for the */
/* Software.  The  Software is provided as is, without warranty of any kind, */
/* either  express  or  implied,  including, without limitation, the implied */
/* warranties  of  merchantability,  fitness  for  a  particular purpose, or */
/* non-infringement.  You  assume  the entire risk arising out of the use or */
/* performance of the Software, or any systems you design using the software */
/* (if  any).  Nothing  may  be construed as a warranty or representation by */
/* Freescale  that  the  Software  or  any derivative work developed with or */
/* incorporating  the  Software  will  be  free  from  infringement  of  the */
/* intellectual property rights of third parties. In no event will Freescale */
/* be  liable,  whether in contract, tort, or otherwise, for any incidental, */
/* special,  indirect, consequential or punitive damages, including, but not */
/* limited  to,  damages  for  any loss of use, loss of time, inconvenience, */
/* commercial loss, or lost profits, savings, or revenues to the full extent */
/* such  may be disclaimed by law. The Software is not fault tolerant and is */
/* not  designed,  manufactured  or  intended by Freescale for incorporation */
/* into  products intended for use or resale in on-line control equipment in */
/* hazardous, dangerous to life or potentially life-threatening environments */
/* requiring  fail-safe  performance,  such  as  in the operation of nuclear */
/* facilities,  aircraft  navigation  or  communication systems, air traffic */
/* control,  direct  life  support machines or weapons systems, in which the */
/* failure  of  products  could  lead  directly to death, personal injury or */
/* severe  physical  or  environmental  damage  (High  Risk Activities). You */
/* specifically  represent and warrant that you will not use the Software or */
/* any  derivative  work of the Software for High Risk Activities.           */
/* Freescale  and the Freescale logos are registered trademarks of Freescale */
/* Semiconductor Inc.                                                        */
/*****************************************************************************/

/* Includes */
#include "SPI_kinetis.h"
#include "fsl_spi.h"


/* Global variable defining the baud rate; this variable can be touched by
   SPI_Init() and SPI_HighRate() to change the baud rate from 375KHz (normal
   baud rate) to 6 MHz (high rate) */
void SPI_Init(void)
{
	/*Body*/	
       // SIM->SCGC4 |= SIM_SCGC4_SPI0_MASK;
        SIM->SCGC4 |= SIM_SCGC4_SPI1_MASK;
        
        //SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
        SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
        
        
            /* SPI1 */
  //  PORT_SetPinMux(PORTD, 4U, kPORT_MuxAlt2); /* PCS0 J1-9  */
  //  PORT_SetPinMux(PORTD, 5U, kPORT_MuxAlt2); /* SCK  J1-11 */
  //  PORT_SetPinMux(PORTD, 6U, kPORT_MuxAlt2); /* MOSI J2_18 */
  //  PORT_SetPinMux(PORTD, 7U, kPORT_MuxAlt2); /* MISO J2-20 */
        
        
        //MISO
	PORTD->PCR[7] =  PORT_PCR_MUX(2);
        //MOSI
	PORTD->PCR[6] =  PORT_PCR_MUX(2);
        //SCK
	PORTD->PCR[5] =  PORT_PCR_MUX(2);
        //CS
	PORTD->PCR[4] =  PORT_PCR_MUX(1);

	GPIOD->PDDR |= (1<<4);
        
	//GPIOC->PDOR |= (1<<4);
	
	SPI1->C2 = SPI_C2_SPISWAI_MASK;     
	SPI1->C1 = SPI_C1_SPE_MASK | SPI_C1_MSTR_MASK | SPI_C1_CPHA_MASK | SPI_C1_CPOL_MASK | SPI_C1_SSOE_MASK;
        
        
	/* 24Mhz SPI clock for SD Card Initilization */  // wenxue
	SPI1->BR = SPI_BR_SPPR(0) | SPI_BR_SPR(0);             /* SPI Baud Rate = Busclock/((SPPR+1)<<(SPR+1)) */
}/*EndBody*/


/*********************************************************
 * Name: SpiSendByte
 * Desc: The byte received by SPI
 * Parameter: None
 * Return: Received byte
 **********************************************************/
unsigned char SpiSendByte(unsigned char u8Data)
{
  //SPI_ReadWriteByte(HW_SPI0, HW_CTAR0, u8Data, 0, kSPI_PCS_ReturnInactive);	
  
  	/*Body*/
    while(!(SPI1->S & SPI_S_SPTEF_MASK));	
    SPI1->DL = u8Data;
    while(!(SPI1->S & SPI_S_SPRF_MASK));
    return(SPI1->DL);
  
  
  
}/*EndBody*/

/*********************************************************
 * Name: SpiSendWord
 * Desc: The word received by SPI
 * Parameter: None
 * Return: Received byte
 **********************************************************/
unsigned short SpiSendWord(unsigned short u16Data)
{

		
}/*EndBody*/

unsigned int SpiSendFrame(unsigned char *in, unsigned char *out, unsigned int size)
{
  unsigned char * destination = out;
  unsigned char * source = in;
  if(out!=NULL)
    do
    {
      *destination++ = SpiSendByte(*source++);
    }while(--size);
  else
    do
    {
      SpiSendByte(*source++);
    }while(--size);
  
    return 0;
}

/*********************************************************
 * Name: SpiHighRate
 * Desc: Change SPI baud rate to high speed
 * Parameter: None
 * Return: None
 **********************************************************/
void SpiHighRate(void)
{


}/*EndBody*/


/*********************************************************
 * Name: SPI_set_SS
 * Desc: Change SPI_CS pin state to Low
 * Parameter: expected state
 * Return: None
 **********************************************************/
void SpiCsLow(void)
{
	GPIOC->PDOR &= ~(1<<4);
}
/*********************************************************
 * Name: SPI_clr_SS
 * Desc: Change SPI_CS pin state to High
 * Parameter: expected state
 * Return: None
 **********************************************************/
void SpiCsHigh(void)
{
	GPIOC->PDOR |= (1<<4);     
        SpiSendByte(0xFF);
}
/*EOF*/
