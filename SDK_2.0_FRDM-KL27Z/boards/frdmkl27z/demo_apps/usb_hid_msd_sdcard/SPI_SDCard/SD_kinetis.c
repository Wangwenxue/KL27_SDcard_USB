
/* Includes */
#include "SD_kinetis.h"
#include "SPI_kinetis.h"


SD_Detect_State SD_Flag = SD_Desert;

void Delayms(unsigned int cnt)
{
  unsigned int i;
  for(;cnt>0;--cnt)
    for(i=500;i>0;--i)
    {
      asm("nop");
      asm("nop");
    }
}

void SD_Detect_Init(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
    
    PORTD->PCR[0] = PORT_PCR_MUX(1) | PORT_PCR_IRQC(0x0A) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;//Falling edge interrupt
    
    PORTD->PCR[0] |= PORT_PCR_ISF_MASK;    // Clear the IO interrupt flag
    
  //  enable_irq(INT_PORTC_PORTD-16);
}

void Isr_PORTD(void)
{
    PORTD->PCR[0] |= PORT_PCR_ISF_MASK;    // Clear the IO interrupt flag
    
    SD_Flag = SD_Desert; 
}

//send a command and send back the response  
unsigned char SD_SendCmd(unsigned char cmd, unsigned long arg, unsigned char crc)  
{  
    unsigned char r1,time = 0;  
  
    //send the command,arguments and CRC  
    SpiSendByte((cmd & 0x3f) | 0x40);  
    SpiSendByte(arg >> 24);  
    SpiSendByte(arg >> 16);  
    SpiSendByte(arg >> 8);  
    SpiSendByte(arg);  
    SpiSendByte(crc);  
  
    //read the respond until responds is not '0xff' or timeout  
    do{  
        r1 = SpiSendByte(0xFF);  
        time ++;  
        //if time out,return  
        if(time > 254) break;  
    }while(r1 == 0xff);  
  
    return r1;  
}  

//reset SD card  
unsigned char SD_Reset(void)  
{  
    unsigned char i,r1;
    unsigned short time = 0;  
  
    do{ 
        //set CS high  
        SpiCsHigh();  
  
        //send 128 clocks  
        for(i = 0;i < 16;i ++)  
        {  
            SpiSendByte(0xFF);  
        }  
  
        //set CS low  
        SpiCsLow();  
  
        Delayms(10);
        
        //send CMD0 till the response is 0x01  
        SpiSendByte(0xFF);
        SpiSendByte(0xFF);
        SpiSendByte(0xFF);
        
        r1 = SD_SendCmd(CMD0,0,0x95);  
        time ++;  
        //if time out,set CS high and return r1  
        if(time > 30000)  
        {  
            //set CS high and send 8 clocks  
            SpiCsHigh();  
            return r1;  
        }  
    }while(r1 != 0x01);  
  
    //set CS high and send 8 clocks  
    SpiCsHigh();  
  
    return 0;  
} 
    
//initial SD card(send CMD55+ACMD41 or CMD1)  
unsigned char SD_Init(void)  
{  
    unsigned char r1;
    unsigned short time = 0;  
  
    SPI_Init();

    SD_Reset();
    
    //set CS low  
    SpiCsLow(); 
    
    Delayms(10);
  
    //check interface operating condition  
    r1 = SD_SendCmd(CMD8, 0x000001aa, 0x87);  
    
    //if support Ver1.x,but do not support Ver2.0,set CS high and return r1  
    if(r1 == 0x05)  
    {  
        //set CS high and send 8 clocks  
        SpiCsHigh();  
        return r1;  
    }  
    //read the other 4 bytes of response(the response of CMD8 is 5 bytes)  
    r1=SpiSendByte(0xFF);  
    r1=SpiSendByte(0xFF);  
    r1=SpiSendByte(0xFF);  
    r1=SpiSendByte(0xFF);  
  
    do{  
        //send CMD55+ACMD41 to initial SD card  
        do{  
            r1 = SD_SendCmd(CMD55,0,0x01);  
            time ++;  
            //if time out,set CS high and return r1  
            if(time > 3000)  
            {  
                //set CS high and send 8 clocks  
                SpiCsHigh();  
                return r1;  
            }  
        }while(r1 != 0x01);  
  
        r1 = SD_SendCmd(ACMD41,0x40000000,0x01);  
  
        //send CMD1 to initial SD card  
        //r1 = SDSendCmd(CMD1,0x00ffc000,0xff);  
        time ++;  
  
        //if time out,set CS high and return r1  
        if(time > 3000)  
        {  
            //set CS high and send 8 clocks  
            SpiCsHigh();  
            return r1;  
        }  
    }while(r1 != 0x00);  
  
    r1 = SD_SendCmd(CMD16,512,0x01);   
    
    //set CS high and send 8 clocks  
    SpiCsHigh();  
  
    SpiHighRate();
    
    return 0;  
}  

//read a single sector  
unsigned char SD_ReadSector(unsigned long addr,unsigned char * buffer)  
{  
    unsigned char r1, temp;  
    unsigned short i,time = 0;  
    
    //set CS low  
    SpiCsLow();  
    //send CMD17 for single block read  
    //r1 = SD_SendCmd(CMD17, addr << 9, 0x55); // For SDv1.0 Sector*512 for address
    r1 = SD_SendCmd(CMD17, addr, 0x01);        // For SDV2.0 Sector is address
    //if CMD17 fail,return  
    if(r1 != 0x00)  
    {  
        //set CS high and send 8 clocks  
        SpiCsHigh();  
        return r1;  
    } 
//    do{  
//        //send CMD17 for multiple blocks read  
//        // r1 = SD_SendCmd(CMD17, addr << 9, 0xff);  // For SDv1.0 Sector*512 for address
//        r1 = SD_SendCmd(CMD17, addr, 0x01);         // For SDV2.0 Sector is address
//        time ++;  
//        //if time out,set CS high and return r1  
//        if(time > 254)  
//        {  
//            //set CS high and send 8 clocks  
//            SpiCsHigh();  
//            return r1;  
//        }  
//      }while(r1 != 0x00);  
//    time = 0; 
  
    //continually read till get the start unsigned char 0xfe  
    do{  
        r1 = SpiSendByte(0xFF);  
        time ++;  
        //if time out,set CS high and return r1  
        if(time > 30000)  
        {  
            //set CS high and send 8 clocks  
            SpiCsHigh();  
            return r1;  
        }  
    }while(r1 != 0xfe);  
    
    //read 512 Bits of data  
#ifndef SPI_DMA
    for(i = 0;i < 512;i ++)  
        *buffer++ = SpiSendByte(0xFF);  
#else  
    DMA_SAR1 = (unsigned long)&SPI0_DL;
    DMA_DAR1 = (unsigned long)&buffer;
    DMA_DSR_BCR1 = DMA_DSR_BCR_BCR(512);    //Set BCR to know how many bytes to transfer   
    DMA_DCR1 |= DMA_DCR_ERQ_MASK; //start the transmit by DMA
    
    temp = 0xFF;
    DMA_SAR2 = (unsigned long)&temp;
    DMA_DAR2 = (unsigned long)&SPI0_DL;
    DMA_DSR_BCR2 = DMA_DSR_BCR_BCR(512);    //Set BCR to know how many bytes to transfer
    DMA_DCR2 |= DMA_DCR_ERQ_MASK; //start the transmit by DMA
   
    SPI0_C2 |= SPI_C2_RXDMAE_MASK | SPI_C2_TXDMAE_MASK;
    while(!(DMA_DSR_BCR1&DMA_DSR_BCR_DONE_MASK));
    DMA_DSR_BCR1 |= DMA_DSR_BCR_DONE_MASK;
    DMA_DSR_BCR2 |= DMA_DSR_BCR_DONE_MASK;
    //SPI0_C2 &=~ (SPI_C2_RXDMAE_MASK | SPI_C2_TXDMAE_MASK);
    DMA_DCR1 &= ~DMA_DCR_ERQ_MASK;
    DMA_DCR2 &= ~DMA_DCR_ERQ_MASK;
    
#endif  
    //read two bits of CRC  

    SpiSendByte(0xFF);  
    SpiSendByte(0xFF);  

  
    //set CS high and send 8 clocks  
    SpiCsHigh();  
  
    return 0;  
}  

  //read multiple sectors  
unsigned char SD_ReadMultiSector(unsigned long addr, unsigned char * buffer, unsigned char sector_num)  
{  
    unsigned short i,time = 0;  
    unsigned char r1;  
  
    //set CS low  
    SpiCsLow();  
    
    if(sector_num==1)
    {
      SD_ReadSector(addr, buffer);
    }
    else{
            //send CMD18 for multiple blocks read  
           // r1 = SD_SendCmd(CMD18, addr << 9, 0xff);  // For SDv1.0 Sector*512 for address
            r1 = SD_SendCmd(CMD18, addr, 0x01);         // For SDV2.0 Sector is address        
            //if CMD18 fail,return  
            if(r1 != 0x00)  
            {  
                //set CS high and send 8 clocks  
                SpiCsHigh();  
                return r1;  
            }  
//            do{  
//                  //send CMD18 for multiple blocks read  
//                  // r1 = SD_SendCmd(CMD18, addr << 9, 0xff);  // For SDv1.0 Sector*512 for address
//                  r1 = SD_SendCmd(CMD18, addr, 0x01);         // For SDV2.0 Sector is address
//                  time ++;  
//                  //if time out,set CS high and return r1  
//                  if(time > 254)  
//                  {  
//                      //set CS high and send 8 clocks  
//                      SpiCsHigh();  
//                      return r1;  
//                  }  
//              }while(r1 != 0x00);  
//            time = 0; 
//          
            //read sector_num sector  
            do{  
                //continually read till get start unsigned char  
                do{  
                    r1 = SpiSendByte(0xFF);  
                    time ++;  
                    //if time out,set CS high and return r1  
                    //if(time > 30000 || ((r1 & 0xf0) == 0x00 && (r1 & 0x0f)))  
                    if(time > 30000) 
                    {  
                        //set CS high and send 8 clocks  
                        SpiCsHigh();  
                        return r1;  
                    }  
                }while(r1 != 0xfe);  
                time = 0;  
          
                //read 512 Bits of data  
                for(i = 0;i < 512;i ++)   
                    *buffer++ = SpiSendByte(0xFF);    
          
                //read two bits of CRC  
                SpiSendByte(0xFF);  
                SpiSendByte(0xFF);  
            }while( -- sector_num);  
            time = 0;  
          
            //stop multiple reading  
            r1 = SD_SendCmd(CMD12,0,0x01);  
          
            //set CS high and send 8 clocks  
            SpiCsHigh();  
    }
    return 0;  
}
    
//write a single sector  
unsigned char SD_WriteSector(unsigned long addr, unsigned char * buffer)  
{  
    unsigned short i,time = 0;  
    unsigned char r1;
  
    //set CS low  
    SpiCsLow();
     
    r1 = SD_SendCmd(CMD24, addr, 0x01);  
 
    if(r1 != 0x00)  
    {  
        //set CS high and send 8 clocks  
        SpiCsHigh();  
        return r1;  
    }       
//    do{  
//        //send CMD24 for single block write  
//       // r1 = SD_SendCmd(CMD24, addr << 9, 0xff);  // For SDv1.0 Sector*512 for address
//        r1 = SD_SendCmd(CMD24, addr, 0x01);         // For SDV2.0 Sector is address
//        time ++;  
//        //if time out,set CS high and return r1  
//        if(time > 254)  
//        {  
//            //set CS high and send 8 clocks  
//            SpiCsHigh();  
//            return r1;  
//        }  
//    }while(r1 != 0x00);  
//    time = 0;  

    //send some dummy clocks  
    for(i = 0;i < 5;i ++)   
        SpiSendByte(0xff);    

    //write start unsigned char  
    SpiSendByte(0xFE);  

    //write 512 bytes of data  
#ifndef SPI_DMA
    for(i = 0;i < 512;i ++)    
        SpiSendByte(*buffer++);
#else   
    DMA_SAR0 = (unsigned long)&buffer;
    DMA_DAR0 = (unsigned long)&SPI0_DL;
    DMA_DSR_BCR0 = DMA_DSR_BCR_BCR(512);    //Set BCR to know how many bytes to transfer
    DMA_DCR0 |= DMA_DCR_ERQ_MASK; //start the transmit by DMA
    SPI0_C2 |= SPI_C2_TXDMAE_MASK;
    while(!(DMA_DSR_BCR0&DMA_DSR_BCR_DONE_MASK));
    DMA_DSR_BCR0 |= DMA_DSR_BCR_DONE_MASK;
    DMA_DCR0 &= ~DMA_DCR_ERQ_MASK;
    SPI0_C2 &=~ SPI_C2_TXDMAE_MASK;
#endif
    
    //write 2 bytes of CRC  
    SpiSendByte(0xff);  
    SpiSendByte(0xff);  

    
    do{
        //read response  
        r1 = SpiSendByte(0xFF);  
        time ++;  
        //if time out,set CS high and return r1  
        if(time > 60000)  
        {  
            //set CS high and send 8 clocks  
            SpiCsHigh();  
            return r1;  
        }  
    }while((r1 & 0x1f)!= 0x05);  
    time = 0;  
  
    //check busy  
    do{  
        r1 = SpiSendByte(0xFF);  
        time ++;  
        //if time out,set CS high and return r1  
        if(time > 60000)  
        {  
            //set CS high and send 8 clocks  
            SpiCsHigh();  
            return r1;  
        }  
    }while(r1 != 0xff);  
  
    //set CS high and send 8 clocks  
    SpiCsHigh();  
  
    return 0;  
}  
    
//write several blocks  
unsigned char SD_WriteMultiSector(unsigned long addr, unsigned char * buffer, unsigned char sector_num)  
{  
    unsigned short i,time = 0;  
    unsigned char r1;  
  
    //set CS low  
    SpiCsLow();  
    
    //send CMD25 for multiple block read  
    //r1 = SD_SendCmd(CMD25, addr << 9, 0xff);  
    if(sector_num==1)
    {
      SD_WriteSector(addr, buffer);
    }
    else{
          //SD_SendCmd(CMD55, 0, 0x01);
          SD_SendCmd(ACMD23, sector_num, 0x01);
          
          r1 = SD_SendCmd(CMD25, addr, 0x01);
          //if CMD25 fail,return  
          if(r1 != 0x00)  
          {  
              //set CS high and send 8 clocks  
              SpiCsHigh();  
              return r1;  
          }  
        
          do{  
              do{  
                  //send several dummy clocks  
                  for(i = 0;i < 5;i ++)    
                      SpiSendByte(0xFF);   
        
                  //write start unsigned char  
                  SpiSendByte(0xFC);  
        
                  //write 512 unsigned char of data  
                  for(i = 0;i < 512;i ++)    
                      SpiSendByte(*buffer++);  
         
                  //write 2 unsigned char of CRC  
                  SpiSendByte(0xFF);  
                  SpiSendByte(0xFF);  
        
                  //read response  
                  r1 = SpiSendByte(0xFF);  
                  time ++;  
                  //if time out,set CS high and return r1  
                  if(time > 254)  
                  {  
                      //set CS high and send 8 clocks  
                      SpiCsHigh();  
                      return r1;  
                  }  
              }while((r1 & 0x1f)!= 0x05);  
              time = 0;  
        
              //check busy  
              do{  
                  r1 = SpiSendByte(0xFF);
                  time ++;  
                  //if time out,set CS high and return r1  
                  if(time > 30000)  
                  {  
                      //set CS high and send 8 clocks  
                      SpiCsHigh();  
                      return r1;  
                  }  
              }while(r1 != 0xFF);  
              time = 0;  
          }while(-- sector_num);  
        
          //send stop unsigned char  
          SpiSendByte(0xFD);  
        
          //check busy  
          do{  
              r1 = SpiSendByte(0xFF);  
              time ++;  
              //if time out,set CS high and return r1  
              if(time > 30000)  
              {  
                  //set CS high and send 8 clocks  
                  SpiCsHigh();  
                  return r1;  
              }  
          }while(r1 != 0xff);  
        
          //set CS high and send 8 clocks  
          SpiCsHigh();  
   }
    return 0;  
}  

//get CID or CSD  
unsigned char SD_GetCIDCSD(unsigned char cid_csd, unsigned char * buffer)  
{  
    unsigned char r1;  
    unsigned short i,time = 0;  
  
    //set CS low  
    SpiCsLow();  
  
    //send CMD10 for CID read or CMD9 for CSD  
    do{  
        if(cid_csd == CID)  
            r1 = SD_SendCmd(CMD10, 0, 0xff);  
        else  
            r1 = SD_SendCmd(CMD9, 0, 0xff);  
        time ++;  
        //if time out,set CS high and return r1  
        if(time > 254)  
        {  
            //set CS high and send 8 clocks  
            SpiCsHigh();  
            return r1;  
        }  
    }while(r1 != 0x00);  
    time = 0;  
  
    //continually read till get 0xfe  
    do{  
        r1 = SpiSendByte(0xFF);  
        time ++;  
        //if time out,set CS high and return r1  
        if(time > 30000)  
        {  
            //set CS high and send 8 clocks  
            SpiCsHigh();  
            return r1;  
        }  
    }while(r1 != 0xFE);  
  
    //read 16 Bytes of data  
    for(i = 0;i < 16;i ++)   
        *buffer ++ = SpiSendByte(0xFF);   
  
    //read two bits of CRC  
    SpiSendByte(0xFF);  
    SpiSendByte(0xFF);  
  
    //set CS high and send 8 clocks  
    SpiCsHigh();  
  
    return 0;  
}

unsigned long SD_GetCapacity(void)
{
  unsigned char CSD_Buff[16]={0}; 
  unsigned long c_size=0, c_size_mult, read_bl_en;
  
  SD_GetCIDCSD(CSD, CSD_Buff);
  
  if((CSD_Buff[0]&0xC0)==0x40) /* bit 126 = 1 SDC version 2.00 */
  {
    c_size = (unsigned long)CSD_Buff[9] + ((unsigned long)CSD_Buff[8] << 8) + ((unsigned long)(CSD_Buff[7] & 0x3F) << 16) + 1;
    
    return (c_size << 9);//Mult 512KBytes
  }
  else                    /* bit 126 = 0 SDC version 1.00 */
  {
    c_size = (unsigned long)(CSD_Buff[8] >> 6) + ((unsigned long)CSD_Buff[7] << 2) + ((unsigned long)(CSD_Buff[6] & 0x03) << 10) ;
    c_size_mult = ((CSD_Buff[10] & 128) >> 7) + ((CSD_Buff[9] & 3) << 1);
    read_bl_en = CSD_Buff[5] & 0x0F;
    return ((c_size+1)<<(c_size_mult+2+read_bl_en-10));//KBytes
  }
}