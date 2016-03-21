#include "SPI_kinetis.h"
#include "fsl_sdmmc_card.h"
#include "fsl_sdcard_spi.h"

/* rate unit is divided by 1000 */
static const uint32_t g_transpeedru[] =
{
  /* 100Kbps, 1Mbps, 10Mbps, 100Mbps*/
    100, 1000, 10000, 100000,
};

/* time value multiplied by 1000 */
static const uint32_t g_transpeedtv[] =
{
       0, 1000, 1200, 1300,
    1500, 2000, 2500, 3000,
    3500, 4000, 4500, 5000,
    5500, 6000, 7000, 8000,
};

sdspi_card_t g_card = {0};
/*FUNCTION****************************************************************
 *
 * Function Name: SDCardInit
 * Description: Initialize the SD card
 *
 *END*********************************************************************/
uint8_t SDCardInit(void)
{
  /* Begin test SD card. */
  memset(&g_card, 0, sizeof(g_card));
    
  if(kStatus_SDSPI_NoError==SDSPI_DRV_Init(&g_card))
    return 0;
  else
    return 1;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDGetCapacity
 * Description: check card capacity of the card in Bytes
 *
 *END*********************************************************************/
uint32_t SDGetCapacity(void)
{
  SDSPI_DRV_CheckCapacity(&g_card);
  
  return (g_card.blockCount);
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_GenerateCRC7
 * Description: calculate CRC7
 *
 *END*********************************************************************/
static uint32_t SDSPI_DRV_GenerateCRC7(uint8_t *buffer,
                                      uint32_t length,
                                      uint32_t crc)
{
    uint32_t index;

    static const uint8_t crcTable[] = {
        0x00, 0x09, 0x12, 0x1B, 0x24, 0x2D, 0x36, 0x3F,
        0x48, 0x41, 0x5A, 0x53, 0x6C, 0x65, 0x7E, 0x77
    };

    while (length)
    {
        index = ((crc >> 3) & 0x0F) ^ ((*buffer) >> 4);
        crc = (crc << 4) ^ crcTable[index];

        index = ((crc >> 3) & 0x0F) ^ ((*buffer) & 0x0F);
        crc = (crc << 4) ^ crcTable[index];

        buffer++;
        length--;
    }

    return (crc & 0x7F);
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_WaitReady
 * Description: wait ready 
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_WaitReady(void)
{
    uint8_t response;
    uint16_t timeout=0;
    
    do
    {
        response = SpiSendByte(0xFF);
    } while ((response != 0xFF) && timeout++ < 60000);

    if (response != 0xFF)
    {
        return kStatus_SDSPI_CardIsBusyError;
    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_SendCommand
 * Description: send command
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_SendCommand(sdspi_request_t *req,
                                            uint32_t timeout)
{
    uint8_t buffer[6];
    uint8_t response;
    uint8_t i;
    sdspi_status_t result = kStatus_SDSPI_NoError;

    SpiCsLow(); // Assert the CS to select the SD Card
    
    result = SDSPI_DRV_WaitReady();
    if ((result == kStatus_SDSPI_CardIsBusyError)
            && (req->cmdIndex != kGoIdleState))
    {
        SpiCsHigh();// Deassert the CS to deselect the SD Card
        return result;
    }

    buffer[0] = SDSPI_MAKE_CMD(req->cmdIndex);
    buffer[1] = req->argument >> 24 & 0xFF;
    buffer[2] = req->argument >> 16 & 0xFF;
    buffer[3] = req->argument >> 8 & 0xFF;
    buffer[4] = req->argument & 0xFF;
    buffer[5] = (SDSPI_DRV_GenerateCRC7(buffer, 5, 0) << 1) | 1;

    if (SpiSendFrame(buffer, NULL, sizeof(buffer)))
    {
        SpiCsHigh();// Deassert the CS to deselect the SD Card
        return kStatus_SDSPI_TransferFailed;
    }

    if (req->cmdIndex == kStopTransmission)
    {
        SpiSendByte(0xFF);
    }
    /* Wait for the response coming, the left most bit which is transfered first in response is 0 */
    for (i = 0; i < 9; i++)
    {
        response = SpiSendByte(0xFF);
        if (!(response & 0x80))
        {
            break;
        }
    }

    if ((response & 0x80))
    {
        SpiCsHigh();// Deassert the CS to deselect the SD Card
        return kStatus_SDSPI_Failed;
    }

    req->response[0] = response;
    switch(req->respType)
    {
        case kSdSpiRespTypeR1:
            break;
        case kSdSpiRespTypeR1b:
        {
            uint8_t busy = 0;
            uint32_t time=0;
            while (busy != 0xFF)
            {
                busy = SpiSendByte(0xFF);
                if (time++ > timeout)
                {
                    break;
                }
            }
            if (busy != 0xFF)
            {
                SpiCsHigh();// Deassert the CS to deselect the SD Card
                result = kStatus_SDSPI_CardIsBusyError;
            }
            break;
        }
        case kSdSpiRespTypeR2:
            req->response[1] = SpiSendByte(0xFF);
            break;
        case kSdSpiRespTypeR3:
        case kSdSpiRespTypeR7:
        default:
            for (i = 1; i <= 4; i++)/* R7 has total 5 bytes in SPI mode. */
            {
                req->response[i] = SpiSendByte(0xFF);
            }
            break;
    }

    SpiCsHigh();// Deassert the CS to deselect the SD Card

    return kStatus_SDSPI_NoError;
}

// wenxue add 20160218
/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_SendCommand
 * Description: send command
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_SendCommand_NoDeassert(sdspi_request_t *req,
                                            uint32_t timeout)
{
    uint8_t buffer[6];
    uint8_t response;
    uint8_t i;
    sdspi_status_t result = kStatus_SDSPI_NoError;

    SpiCsLow(); // Assert the CS to select the SD Card
    
    result = SDSPI_DRV_WaitReady();
    if ((result == kStatus_SDSPI_CardIsBusyError)
            && (req->cmdIndex != kGoIdleState))
    {
        SpiCsHigh();// Deassert the CS to deselect the SD Card
        return result;
    }

    buffer[0] = SDSPI_MAKE_CMD(req->cmdIndex);
    buffer[1] = req->argument >> 24 & 0xFF;
    buffer[2] = req->argument >> 16 & 0xFF;
    buffer[3] = req->argument >> 8 & 0xFF;
    buffer[4] = req->argument & 0xFF;
    buffer[5] = (SDSPI_DRV_GenerateCRC7(buffer, 5, 0) << 1) | 1;

    if (SpiSendFrame(buffer, NULL, sizeof(buffer)))
    {
        SpiCsHigh();// Deassert the CS to deselect the SD Card
        return kStatus_SDSPI_TransferFailed;
    }

    if (req->cmdIndex == kStopTransmission) //  è?1?ê?CMD12??á?￡??ù·￠?í0xFF ???
    {
        SpiSendByte(0xFF);
    }
    /* Wait for the response coming, the left most bit which is transfered first in response is 0 */
    for (i = 0; i < 9; i++)
    {
        response = SpiSendByte(0xFF);
        if (!(response & 0x80)) // ì?3?μ?ì??tê?ê?×????a0
        {
            break;
        }
    }

    if ((response & 0x80))
    {
        SpiCsHigh();// Deassert the CS to deselect the SD Card
        return kStatus_SDSPI_Failed;
    }

    req->response[0] = response;
    switch(req->respType)
    {
        case kSdSpiRespTypeR1:
            break; // R1 ê?ò???×??ú
        case kSdSpiRespTypeR1b:
        {
            uint8_t busy = 0;
            uint32_t time=0;
            while (busy != 0xFF) //?áê?μ?ì??tê?￡o1)3?ê± 2￡?busy=0xFF
            {
                busy = SpiSendByte(0xFF);
                if (time++ > timeout)
                {
                    break;
                }
            }
            if (busy != 0xFF)
            {
                SpiCsHigh();// Deassert the CS to deselect the SD Card
                result = kStatus_SDSPI_CardIsBusyError;
            }
            break;
        }
        case kSdSpiRespTypeR2:
            req->response[1] = SpiSendByte(0xFF);
            break;
        case kSdSpiRespTypeR3:
        case kSdSpiRespTypeR7:
        default:
            for (i = 1; i <= 4; i++)/* R7 has total 5 bytes in SPI mode. */
            {
                req->response[i] = SpiSendByte(0xFF);
            }
            break;
    }

  //  SpiCsHigh();// Deassert the CS to deselect the SD Card  // wenxue

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_GoIdle
 * Description: send CMD0
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_GoIdle(sdspi_card_t *card)
{
    uint32_t i, j;
    sdspi_request_t req;

    /*
     * SD card will enter SPI mode if the CS is asserted (negative) during the
     * reception of the reset command (CMD0) and the card is in IDLE state.
     */
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 10; j++)
        {
            SpiSendByte(0xFF);
        }

        req.cmdIndex = kGoIdleState;
        req.respType = kSdSpiRespTypeR1;
        if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT))
        {
            return kStatus_SDSPI_Failed;
        }

        if (req.response[0] == SDMMC_SPI_R1_IN_IDLE_STATE)
        {
            break;
        }
    }

    if (req.response[0] != SDMMC_SPI_R1_IN_IDLE_STATE)
    {
        return kStatus_SDSPI_Failed;
    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_SendApplicationCmd
 * Description: send application command to card
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_SendApplicationCmd(void)
{
    sdspi_request_t req;

    req.cmdIndex = kAppCmd;
    req.respType = kSdSpiRespTypeR1;
    if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT))
    {
        return kStatus_SDSPI_Failed;
    }

    if (req.response[0] && !(req.response[0] & SDMMC_SPI_R1_IN_IDLE_STATE))
    {

        return kStatus_SDSPI_Failed;
    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_AppSendOpCond
 * Description: Get the card to send its operating condition.
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_AppSendOpCond(sdspi_card_t *card,
                                              uint32_t argument,
                                              uint8_t *response)
{
    sdspi_request_t req;
    uint16_t timeout = 0;
    uint8_t i=0;


    req.cmdIndex = kSdAppSendOpCond;
    req.argument = argument;
    req.respType = kSdSpiRespTypeR1;

    timeout = 0;
    do
    {
        if (kStatus_SDSPI_NoError == SDSPI_DRV_SendApplicationCmd())
        {
            if (kStatus_SDSPI_NoError == SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT))
            {
                if (!req.response[0])
                {
                    break;
                }
            }
        }
        timeout++;
    } while (timeout < 2000);

    if (response)
    {
        //memcpy(response, req.response, sizeof(req.response));
      for(i=0;i<sizeof(req.response);i++)
      {
        response[i] = req.response[i];
      }
    }

    if (timeout < 2000)
    {
        return kStatus_SDSPI_NoError;
    }
    return kStatus_SDSPI_TimeoutError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_SendIfCond
 * Description: check card interface condition, which includes host supply
 * voltage information and asks the card whether card supports voltage.
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_SendIfCond(sdspi_card_t *card,
                                           uint8_t pattern,
                                           uint8_t *response1)
{
    sdspi_request_t req;
    uint8_t i=0;
    
    req.cmdIndex = kSdSendIfCond;
    req.argument = 0x100 | (pattern & 0xFF);
    req.respType = kSdSpiRespTypeR7;
    if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT))
    {
        return kStatus_SDSPI_Failed;
    }
    
    for(i=0;i<sizeof(req.response);i++)
    {
      response1[i] = req.response[i];
    }
    //memcpy(response1, (uint8_t *)req.response, sizeof(req.response));

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_ReadOcr
 * Description: Get OCR register from card
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_ReadOcr(sdspi_card_t *card)
{
    uint32_t i;
    sdspi_request_t req;

    req.cmdIndex = kReadOcr;
    req.respType = kSdSpiRespTypeR3;
    if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT))
    {
        return kStatus_SDSPI_Failed;
    }
    if (req.response[0])
    {
        return kStatus_SDSPI_Failed;
    }

    card->ocr = 0;
    for (i = 4; i > 0; i--)
    {
        card->ocr |= (uint32_t) req.response[i] << ((4 - i) * 8);
    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_Write
 * Description: write data to card
 *
 *END*********************************************************************/
static uint32_t SDSPI_DRV_Write(uint8_t *buffer, uint32_t size, uint8_t token)
{
    uint8_t     response = 0;
    uint16_t    timeout = 0;
    
    SpiCsLow(); // Assert the CS to select the SD Card
    
    if (SDSPI_DRV_WaitReady() != kStatus_SDSPI_NoError)
    {
        SpiCsHigh();// Deassert the CS to deselect the SD Card
        return 0;
    }

    SpiSendByte(token);  // first send token  wenxue

    if (token == SDMMC_SPI_DT_STOP_TRANSFER)
    {
        SpiCsHigh();// Deassert the CS to deselect the SD Card
        return size;
    }

    if (SpiSendFrame(buffer, NULL, size)) // Then send data wenxue
    {
        SpiCsHigh();// Deassert the CS to deselect the SD Card
        return 0;
    }

    /* Send CRC */  // Last Send CRC wenxue
    SpiSendByte(0xFF);
    SpiSendByte(0xFF);

    do{
        //read response  
        response = SpiSendByte(0xFF);  
        timeout ++;  
        //if time out,set CS high and return r1  
        if(timeout > 3000)  
        {  
            //set CS high and send 8 clocks  
            SpiCsHigh();  
            return 0;  
        }  
    }while((response & SDMMC_SPI_DR_MASK)!= SDMMC_SPI_DR_ACCEPTED);  // wenxue reponse = xxx 00101
    
   // SpiCsHigh(); // Deassert the CS to deselect the SD Card   // wenxue  no need to deasset CS
    
    return size;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_Read
 * Description: read data from card
 *
 *END*********************************************************************/
static uint32_t SDSPI_DRV_Read(uint8_t *buffer, uint32_t size)
{
    uint32_t timeout=0,i=0;
    uint8_t response;
    
    SpiCsLow();
    //continually read till get the start unsigned char 0xfe  
    do{  
        response = SpiSendByte(0xFF);  
        timeout ++;  
        //if time out,set CS high and return r1  
        if(timeout > 30000)  
        {  
            //set CS high and send 8 clocks  
            SpiCsHigh();  
            return response;  
        }  
    }while(response != SDMMC_SPI_DT_START_SINGLE_BLK);  
    
    for(i = 0;i < size;i ++)  
    *buffer++ = SpiSendByte(0xFF);
    
    SpiSendByte(0xFF);  
    SpiSendByte(0xFF); 
    
    SpiCsHigh();    
    
    return size;
}


// wenxue 
/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_Read_NoDeassert
 * Description: read data from card
 *
 *END*********************************************************************/
static uint32_t SDSPI_DRV_Read_NoDeassert(uint8_t *buffer, uint32_t size)
{
    uint32_t timeout=0,i=0;
    uint8_t response;
    
   // SpiCsLow();
    //continually read till get the start unsigned char 0xfe  
    do{  
        response = SpiSendByte(0xFF);  
        timeout ++;  
        //if time out,set CS high and return r1  
        if(timeout > 30000)  
        {  
            //set CS high and send 8 clocks  
            SpiCsHigh();  
            return response;  
        }  
    }while(response != SDMMC_SPI_DT_START_SINGLE_BLK);  
    
    for(i = 0;i < size;i ++)  
    *buffer++ = SpiSendByte(0xFF);
    
    SpiSendByte(0xFF);  
    SpiSendByte(0xFF); 
    
  //  SpiCsHigh();    
    
    return size;
}


/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_SendCsd
 * Description: get CSD register from card
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_SendCsd(sdspi_card_t *card)
{
    sdspi_request_t req;

    req.cmdIndex = kSendCsd;
    req.respType = kSdSpiRespTypeR1;
   // if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT))  // wenxue
    if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand_NoDeassert(&req, SDSPI_TIMEOUT))
    {

        return kStatus_SDSPI_Failed;
    }

    if (sizeof(card->rawCsd) !=
            (SDSPI_DRV_Read(card->rawCsd, sizeof(card->rawCsd))))
    {
        return kStatus_SDSPI_Failed;
    }

    /* No start single block token if found */
    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_SetBlockSize
 * Description:  set the block length in bytes for SDSC cards. For SDHC cards,
 * it does not affect memory read or write commands, always 512 bytes fixed
 * block length is used.
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_SetBlockSize(uint32_t blockSize)
{
    sdspi_request_t req;

    req.cmdIndex = kSetBlockLen;
    req.argument = blockSize;
    req.respType = kSdSpiRespTypeR1;

    if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT))
    {
        return kStatus_SDSPI_Failed;
    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_CheckCapacity
 * Description: check card capacity of the card
 *
 *END*********************************************************************/
void SDSPI_DRV_CheckCapacity(sdspi_card_t *card)
{
    uint32_t cSize, cSizeMult, readBlkLen;

    if (SDMMC_CSD_CSDSTRUCTURE_VERSION(card->rawCsd))
    {
        /* SD CSD structure v2.xx */
        cSize = SDV20_CSD_CSIZE(card->rawCsd);
        if (cSize >= 0xFFFF)
        {
            /* extended capacity */
            card->caps |= SDSPI_CAPS_SDXC;
        }
        else
        {
            card->caps |= SDSPI_CAPS_SDHC;
        }
        cSizeMult = 10;
        cSize += 1;
        readBlkLen = 9;
    }
    else
    {
        /* SD CSD structure v1.xx */
        cSize = SDMMC_CSD_CSIZE(card->rawCsd) + 1;
        cSizeMult = SDMMC_CSD_CSIZEMULT(card->rawCsd) + 2;
        readBlkLen = SDMMC_CSD_READBLK_LEN(card->rawCsd);
    }

    if (readBlkLen != 9)
    {
        /* Force to use 512-byte length block */
        cSizeMult += (readBlkLen - 9);
        readBlkLen = 9;
    }

    card->blockSize = 1 << readBlkLen;
    card->blockCount = cSize << cSizeMult;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_SendCid
 * Description: get CID information from card
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_SendCid(sdspi_card_t *card)
{
    sdspi_request_t req;

    
    req.cmdIndex = kSendCid;
    req.respType = kSdSpiRespTypeR1;

   // if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT))  // wenxue
    if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand_NoDeassert(&req, SDSPI_TIMEOUT)) 
    {
        return kStatus_SDSPI_Failed;
    }

    if (sizeof(card->rawCid) !=
            (SDSPI_DRV_Read(card->rawCid, sizeof(card->rawCid))))
    {
        return kStatus_SDSPI_Failed;
    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_InitSd
 * Description: initialize SD card
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_InitSd(sdspi_card_t *card)
{
    uint32_t maxFrequency;

    if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCsd(card))
    {
        return kStatus_SDSPI_Failed;  
    }

    /* Calculate frequency */
    maxFrequency = g_transpeedtv[SDMMC_CSD_TRANSPEED_TV(card->rawCsd)] *
                g_transpeedru[SDMMC_CSD_TRANSPEED_RU(card->rawCsd)];
    
    SpiHighRate();

    SDSPI_DRV_CheckCapacity(card);
    SDSPI_DRV_CheckReadOnly(card);

    if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCid(card))
    {
        return kStatus_SDSPI_Failed; 
    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_Init
 * Description: initialize card on the given host controller
 *
 *END*********************************************************************/
sdspi_status_t SDSPI_DRV_Init(sdspi_card_t *card)
{
    uint32_t timeout, acmd41Arg;
    uint8_t response[5], acmd41resp[5];
    unsigned int likelyMmc = false, likelySdV1 = false;

    card->cardType = kCardTypeUnknown;
    

    if (kStatus_SDSPI_NoError != SDSPI_DRV_GoIdle(card))
    {
        return kStatus_SDSPI_Failed;
    }

    acmd41Arg = 0;
    if (kStatus_SDSPI_NoError !=
            SDSPI_DRV_SendIfCond(card, 0xAA, response))
    {
        likelySdV1 = true;
    }
    else if ((response[3] == 0x1) || (response[4] == 0xAA))
    {
        acmd41Arg |= SD_OCR_HCS;
    }
    else
    {
        return kStatus_SDSPI_Failed;
    }

    timeout = 0;
    do
    {
        if (kStatus_SDSPI_NoError !=
                SDSPI_DRV_AppSendOpCond(card, acmd41Arg, acmd41resp))
        {
            if (likelySdV1)
            {
                likelyMmc = true;
                break;
            }
            return kStatus_SDSPI_Failed;
        }
        if (!acmd41resp[0])
        {
            break;
        }
        if (timeout++ > 500)
        {
            if (likelySdV1)
            {
                likelyMmc = true;
                break;
            }
        }
    } while(acmd41resp[0] == SDMMC_SPI_R1_IN_IDLE_STATE);

    if (likelyMmc)
    {
        card->cardType = kCardTypeMmc;
        return kStatus_SDSPI_NotSupportYet;
    }
    else
    {
        card->cardType = kCardTypeSd;
    }

    if (!likelySdV1)
    {
        card->version = kSdCardVersion_2_x;
        if (kStatus_SDSPI_NoError != SDSPI_DRV_ReadOcr(card))
        {
            return kStatus_SDSPI_Failed;
        }
        if (card->ocr & SD_OCR_CCS)
        {
            card->caps = SDSPI_CAPS_ACCESS_IN_BLOCK;
        }
    }
    else
    {
        card->version = kSdCardVersion_1_x;
    }

    /* Force to use 512-byte length block, no matter which version  */
    if (kStatus_SDSPI_NoError != SDSPI_DRV_SetBlockSize(512))
    {
        return kStatus_SDSPI_Failed;
    }

    if (kStatus_SDSPI_NoError != SDSPI_DRV_InitSd(card))
    {
        return kStatus_SDSPI_Failed;
    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_StopTransmission
 * Description:  Send stop transmission command to card to stop ongoing
 * data transferring.
 *
 *END*********************************************************************/
static sdspi_status_t SDSPI_DRV_StopTransmission(void)
{
    sdspi_request_t req;

    req.cmdIndex = kStopTransmission;
    req.respType = kSdSpiRespTypeR1b;
    if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT))
    {
        return kStatus_SDSPI_Failed;
    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_ReadBlocks
 * Description: read blocks from card 
 *
 *END*********************************************************************/
sdspi_status_t SDSPI_DRV_ReadBlocks(sdspi_card_t *card, uint8_t *buffer,
                                    uint32_t startBlock, uint32_t blockCount)
{
    uint32_t offset, i;
    sdspi_request_t req;

    offset = startBlock;
    if (!IS_BLOCK_ACCESS(card))
    {
        offset *= card->blockSize;
    }
    
    req.argument = offset;
    req.respType = kSdSpiRespTypeR1;
    if (blockCount == 1)
    {
        req.cmdIndex = kReadSingleBlock;

      //  if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT)) // 发送CMD17 
       if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand_NoDeassert(&req, SDSPI_TIMEOUT)) // 发送CMD17   // wenxue
        {
            return kStatus_SDSPI_Failed;
        }

        if (SDSPI_DRV_Read(buffer, card->blockSize) != card->blockSize)
        {
            return kStatus_SDSPI_Failed;
        }
    }
    else
    {
        req.cmdIndex = kReadMultipleBlock;

      //  if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT)) // wenxue
        if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand_NoDeassert(&req, SDSPI_TIMEOUT))
        {
            return kStatus_SDSPI_Failed;
        }

        for (i = 0; i < blockCount; i++)
        {
         //   if (SDSPI_DRV_Read(buffer, card->blockSize) != card->blockSize)
            if (SDSPI_DRV_Read_NoDeassert(buffer, card->blockSize) != card->blockSize)  // wenxue 
            {
                return kStatus_SDSPI_Failed;
            }
            buffer += card->blockSize;
        }
        SDSPI_DRV_StopTransmission();
        SpiCsHigh();  // wenxue  
    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_WriteBlocks
 * Description: write blocks to card 
 *
 *END*********************************************************************/
sdspi_status_t SDSPI_DRV_WriteBlocks(sdspi_card_t *card, uint8_t *buffer,
                                     uint32_t startBlock, uint32_t blockCount)
{
    uint32_t offset, i;
    sdspi_request_t req;

    if (card->state & SDSPI_STATE_WRITE_PROTECTED)
    {
        return kStatus_SDSPI_WriteProtected;
    }

    offset = startBlock;
    if (!IS_BLOCK_ACCESS(card))
    {
        offset *= card->blockSize;
    }

    if (blockCount == 1)
    {
        req.cmdIndex = kWriteBlock;  // CMD24
        req.argument = offset;
        req.respType = kSdSpiRespTypeR1;
        if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT)) // send CMD24
        {
            return kStatus_SDSPI_Failed;
        }
        if (req.response[0])
        {
            return kStatus_SDSPI_Failed;
        }

        if (SDSPI_DRV_Write(buffer, card->blockSize, SDMMC_SPI_DT_START_SINGLE_BLK) != card->blockSize) // send FE and data wenxue
        {
            return kStatus_SDSPI_Failed;
        }
        
        SpiCsHigh(); // Deassert the CS to deselect the SD Card   // wenxue add this 最后拉高片选  
    }
    else
    {
        memset(&req, 0, sizeof(sdspi_request_t));
        req.cmdIndex = kWriteMultipleBlock; // CMD25 wenxue 写多块
        req.argument = offset;
        req.respType = kSdSpiRespTypeR1;

        if (kStatus_SDSPI_NoError != SDSPI_DRV_SendCommand(&req, SDSPI_TIMEOUT)) // send CMD25 
        {
            return kStatus_SDSPI_Failed;
        }
        if (req.response[0])
        {
            return kStatus_SDSPI_Failed;
        }

        for (i = 0; i < blockCount; i++)
        {
            if (SDSPI_DRV_Write(buffer, card->blockSize, SDMMC_SPI_DT_START_MULTI_BLK) != card->blockSize) // send 0xFC and data wenxue
            {
                return kStatus_SDSPI_Failed;
            }
            buffer += card->blockSize;
        }

        SDSPI_DRV_Write(0, 0, SDMMC_SPI_DT_STOP_TRANSFER);
        
        SpiCsHigh(); // Deassert the CS to deselect the SD Card   // wenxue add this 最后拉高片选

    }

    return kStatus_SDSPI_NoError;
}

/*FUNCTION****************************************************************
 *
 * Function Name: SDSPI_DRV_CheckReadOnly
 * Description: check if card is read only
 *
 *END*********************************************************************/
unsigned int SDSPI_DRV_CheckReadOnly( sdspi_card_t *card)
{
    card->state &= ~SDSPI_STATE_WRITE_PROTECTED;
    if (card->cardType != kCardTypeSd)
    {
        return false;
    }

    if (SD_CSD_PERM_WRITEPROTECT(card->rawCsd) || SD_CSD_TEMP_WRITEPROTECT(card->rawCsd))
    {
        card->state |= SDSPI_STATE_WRITE_PROTECTED;
        return true;
    }

    return false;
}

void SDSPI_DRV_Shutdown(sdspi_card_t *card)
{

    memset(card, 0, sizeof(sdspi_card_t));
    return;
}
