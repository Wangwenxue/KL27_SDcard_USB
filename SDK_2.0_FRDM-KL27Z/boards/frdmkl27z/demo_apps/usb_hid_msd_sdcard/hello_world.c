/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
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

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"

#include "pin_mux.h"
#include "clock_config.h"

#include "rl_usb.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

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

/*******************************************************************************
 * Code
 ******************************************************************************/

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




/*!
 * @brief Main function
 */
int main(void)
{
    char ch;

    /* Init board hardware. */
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    PRINTF("hello world.\r\n");

    
      usbd_init();

      usbd_connect(__TRUE);

      usb_state = USB_CONNECTING;
      
      while (!usbd_configured ());          /* Wait for device to configure */
    
    while (1)
    {
        ch = GETCHAR();
        PUTCHAR(ch);
       
    }
}
