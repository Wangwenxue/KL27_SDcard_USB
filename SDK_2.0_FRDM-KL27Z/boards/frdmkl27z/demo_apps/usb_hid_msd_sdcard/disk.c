/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "disk.h"

#include "fsl_device_registers.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_spi.h"


#include "pin_mux.h"
#include "clock_config.h"

#include "rl_usb.h"

#include <stdio.h>
#include <stdlib.h>
#if (defined(FSL_FEATURE_SOC_MPU_COUNT) && (FSL_FEATURE_SOC_MPU_COUNT > 0U))
#include "fsl_mpu.h"
#endif /* FSL_FEATURE_SOC_MPU_COUNT */
#if defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0)
#include "usb_phy.h"
#endif

#include "fsl_common.h"
#include "pin_mux.h"

#include "fsl_sdcard_spi.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*++++++++++++++++++++++++++++++++++++++sdspi+++++++++++++++++++++++++++++++++*/
#define BOARD_LED_GPIO BOARD_LED_RED_GPIO
#define BOARD_LED_GPIO_PIN BOARD_LED_RED_GPIO_PIN

#define BOARD_SDSPI_SPI_BASE SPI0_BASE          /*!< SPI base address for SDSPI */
#define BOARD_SDSPI_CD_GPIO_BASE GPIOB          /*!< Port related to card detect pin for SDSPI */
#define BOARD_SDSPI_CD_PIN 0U                   /*!< Card detect pin for SDSPI */



#define EXAMPLE_SPI_MASTER_SOURCE_CLOCK kCLOCK_BusClk
#define EXAMPLE_SPI_MASTER SPI0

extern sdspi_card_t g_card;
#define Block_Group     (15)   // wenxue
#define SECTER_SIZE     (512)
U8  BlockBuf[SECTER_SIZE*Block_Group]={0};


/* The block of the card to be accessed. */
//#define DATA_BLOCK_COUNT (3U)
//#define DATA_BLOCK_START (4U)

/* The value used to initialize the write buffer. */
//#define INITIAL_DATA_VALUE_WRITE (0x17U)
/* The value used to initialize the read buffer. */
//#define INITIAL_DATA_VALUE_READ (0U)

//+++++++++++++++++++++++++++
#define EXAMPLE_DMA DMA0
#define EXAMPLE_DMAMUX DMAMUX0
#define EXAMPLE_SPI_MASTER_TX_CHANNEL 0U
#define EXAMPLE_SPI_MASTER_RX_CHANNEL 1U

#define EXAMPLE_DMA_SPI_MASTER_TX_SOURCE 17U
#define EXAMPLE_DMA_SPI_MASTER_RX_SOURCE 16U
//---------------------------

typedef enum {
    USB_DISCONNECTED,
    USB_CONNECTING,
    USB_CONNECTED,
    USB_CHECK_CONNECTED,
    USB_CONFIGURED,
    USB_DISCONNECTING,
    USB_DISCONNECT_CONNECT
} USB_CONNECT;
// Global state of usb
USB_CONNECT usb_state;


/*--------------------------------------sdspi---------------------------------*/
/*******************************************************************************
  * Prototypes
  ******************************************************************************/


//---------------------------
/*******************************************************************************
 * Code
 ******************************************************************************/
//+++++++++++++++++++++++++++

//---------------------------

/*******************************************************************************/
/* init */
void usbd_msc_init (void)
{
    //USBD_MSC_MemorySize = SDGetCapacity();//SD_GetCapacity()*1024;//Transfer KBytes to Bytes
    USBD_MSC_BlockSize  = SECTER_SIZE;
    USBD_MSC_BlockGroup = Block_Group;
    USBD_MSC_BlockCount = SDGetCapacity();//USBD_MSC_MemorySize / USBD_MSC_BlockSize;
    USBD_MSC_BlockBuf   = BlockBuf;
    
    USBD_MSC_MediaReady = __TRUE;
}
/* read */
void usbd_msc_read_sect (U32 block, U8 *buf, U32 num_of_blocks)
{
    if (USBD_MSC_MediaReady)
    {
      //SD_ReadSector(block,buf);
      SDSPI_DRV_ReadBlocks(&g_card, buf, block, num_of_blocks);
    }
}
/* write */
void usbd_msc_write_sect (U32 block, U8 *buf, U32 num_of_blocks)
{
    if (USBD_MSC_MediaReady)
    {
      //SD_WriteSector(block, buf);
     SDSPI_DRV_WriteBlocks(&g_card, buf, block, num_of_blocks);
   //   SDSPI_DRV_WriteBlocks(&g_card, buf, block, num_of_blocks);
    }
}



void GetMouseInReport (S8 *report, U32 size)
{

    report[0] = 0;
    report[1] = 1; /* little move */
    report[2] = 0;
    report[3] = 0;
}

int usbd_hid_get_report (U8 rtype, U8 rid, U8 *buf, U8 req)
{
    U32 i;
    S8 report[4] = {0, 0, 0, 0};
    switch (rtype)
    {
        case HID_REPORT_INPUT:
        switch (rid)
        {
            case 0:
            switch (req)
            {
                case USBD_HID_REQ_EP_CTRL:
                case USBD_HID_REQ_PERIOD_UPDATE:
                    GetMouseInReport (report, 4);
                    for (i = 0; i < sizeof(report); i++)
                    {
                        *buf++ = (U8)report[i];
                    }
                    return (1);
                case USBD_HID_REQ_EP_INT:
                break;
            }
            break;
        }
    break;
    case HID_REPORT_FEATURE:
      return (1);
  }
  return (0);
}

void usbd_hid_set_report (U8 rtype, U8 rid, U8 *buf, int len, U8 req)
{
    static int i;
    switch (rtype)
    {
        case HID_REPORT_OUTPUT:
            printf("%d  %d\r\n", i++, len);
        break;
        case HID_REPORT_FEATURE:
        break;
    }
}



/*++++++++++++++++++++++++++++++++++++++sdspi+++++++++++++++++++++++++++++++++*/
/* Delay some time united in milliseconds. */
void Delay(uint32_t milliseconds)
{
    uint32_t i;
    uint32_t j;

    for (i = 0; i < milliseconds; i++)
    {
        for (j = 0; j < 3333U; j++)
        {
            __asm("NOP");
        }
    }
}


void main(void)

{
    spi_master_config_t masterConfig = {0};
     S8 report[4];
     uint32_t sourceClock = 0U;
   
     /* Define the init structure for the output LED pin*/
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput, 0,
    };
    
   /* Define the init structure for the SW3 pin*/
    gpio_pin_config_t sw3_config = {
        kGPIO_DigitalInput, 0,
    };
  
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    
         
    /* Init output LED GPIO. */
    GPIO_PinInit(BOARD_LED_GPIO, BOARD_LED_GPIO_PIN, &led_config);
    GPIO_WritePinOutput(BOARD_LED_GPIO,BOARD_LED_GPIO_PIN,0);
    
    /* Init SW3 */
    GPIO_PinInit(GPIOC, 1U, &sw3_config);
    
    PRINTF("SD card over SPI example start.\r\n\r\nPlease insert the SD card\r\n");
/* Wait the card to be inserted. */
#if defined BOARD_SDSPI_CD_LOGIC_RISING
    while (!(GPIO_ReadPinInput(BOARD_SDSPI_CD_GPIO_BASE, BOARD_SDSPI_CD_PIN)))
    {
    }
#else
    while (GPIO_ReadPinInput(BOARD_SDSPI_CD_GPIO_BASE, BOARD_SDSPI_CD_PIN))
    {
    }
#endif
    
    PRINTF("Detected SD card inserted.\r\n\r\n");
   
    /* Delat some time to make card stable. */
    Delay(1000U);

    // wenxue  SPI ³õÊ¼»¯
    /* Init SPI master */
    /*
     * masterConfig.enableStopInWaitMode = false;
     * masterConfig.polarity = kSPI_ClockPolarityActiveHigh;
     * masterConfig.phase = kSPI_ClockPhaseFirstEdge;
     * masterConfig.direction = kSPI_MsbFirst;
     * masterConfig.dataMode = kSPI_8BitMode;
     * masterConfig.txWatermark = kSPI_TxFifoOneHalfEmpty;
     * masterConfig.rxWatermark = kSPI_RxFifoOneHalfFull;
     * masterConfig.pinMode = kSPI_PinModeNormal;
     * masterConfig.outputMode = kSPI_SlaveSelectAutomaticOutput;
     * masterConfig.baudRate_Bps = 500000U;
     */
   // SPI_MasterGetDefaultConfig(&masterConfig);
   // sourceClock = CLOCK_GetFreq(EXAMPLE_SPI_MASTER_SOURCE_CLOCK);
   // SPI_MasterInit(EXAMPLE_SPI_MASTER, &masterConfig, sourceClock);
    
    SPI_Init();
    
    
    if (kStatus_SDSPI_NoError != SDSPI_DRV_Init(&g_card))
    {
      printf("Failed to initialize SD disk\r\n");
      while(1);
    }
    else
     printf("Success to initialize SD disk\r\n");

      
    // wenxue
    usbd_init();

    usbd_connect(__TRUE);

    usb_state = USB_CONNECTING;
      
    while (!usbd_configured ());          /* Wait for device to configure */
    

    while (1)
    {
       // GetMouseInReport(report, 4);
      //  usbd_hid_get_report_trigger(0, (U8 *)report, 4);
        
       Delay(50);

       if( GPIO_ReadPinInput(GPIOC,1) == 0)
       {
          USBD_WakeUp1 (); // wenxue Generate Resume Signal
          GetMouseInReport(report, 4);
          usbd_hid_get_report_trigger(0, (U8 *)report, 4);
       }
        
    }
}
