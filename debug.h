// Smart Toybox Debug interface
// TI PLATFORM implementation
//
// Copyright(C) 2016. Nebojsa Sumrak and Jelena (Petra) Markovic
//
//   This program is free software; you can redistribute it and / or modify
//	 it under the terms of the GNU General Public License as published by
//	 the Free Software Foundation; either version 2 of the License, or
//	 (at your option) any later version.
//
//	 This program is distributed in the hope that it will be useful,
//	 but WITHOUT ANY WARRANTY; without even the implied warranty of
//	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	 GNU General Public License for more details.
//
//	 You should have received a copy of the GNU General Public License along
//	 with this program; if not, write to the Free Software Foundation, Inc.,
//	 51 Franklin Street, Fifth Floor, Boston, MA 02110 - 1301 USA.

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef DEBUG
// TODO uncomment agaiin after inital debugging
//#define NOTERM
#endif

#ifdef NOTERM
#define UART_PRINT(x,...)
#define DBG_PRINT(x,...)
#define ERR_PRINT(x)
#define ASSERT(x)
#else
#define UART_PRINT Report
#define DBG_PRINT  Report
#define ERR_PRINT(x) Report("Error [%d] at line [%d] in function [%s]  \n\r",x,__LINE__,__FUNCTION__)
#define ASSERT(x) if (!(x)) { Report("Assert at line [%d] in function [%s]\n\r",__LINE__,__FUNCTION__); while (1) {} }
#endif

#define UART_BAUD_RATE  115200
#define SYSCLK          80000000
#define CONSOLE         UARTA0_BASE
#define CONSOLE_PERIPH  PRCM_UARTA0
//
// Define the size of UART IF buffer for RX
//
#define UART_IF_BUFFER           64

//
// Define the UART IF buffer
//
extern unsigned char g_ucUARTBuffer[];


/****************************************************************************/
/*								FUNCTION PROTOTYPES							*/
/****************************************************************************/
extern void DispatcherUARTConfigure(void);
extern void DispatcherUartSendPacket(unsigned char *inBuff, unsigned short usLength);
extern int GetCmd(char *pcBuffer, unsigned int uiBufLen);
extern void InitTerm(void);
extern void ClearTerm(void);
extern void Message(const char *format);
extern void Error(char *format,...);
extern int TrimSpace(char * pcInput);
extern int Report(const char *format, ...);
extern void Hexdump(unsigned char *data, unsigned len);

#ifdef __cplusplus
}
#endif
