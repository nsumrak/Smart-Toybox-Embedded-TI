// Smart Toybox platform dependent routiners
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

#ifdef TI_PLATFORM

#include "platform.h"
#include "hw_mcasp.h"
#include "hw_i2c.h"
#include "hw_memmap.h"
#include "udma.h"
#include "prcm.h"
#include "i2s.h"
#include "rom_map.h"
#include <stdlib.h>
#include "i2c.h"
#include "string.h"
#include "debug.h"


#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif


extern void PinMuxConfig();


void platform_init()
{
	// Set vector table base
#if defined(ccs)
	MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
	MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif

	// Enable Processor
	MAP_IntMasterEnable();
	MAP_IntEnable(FAULT_SYSTICK);

	PRCMCC3200MCUInit();

	PinMuxConfig();

	// Initialising the UART terminal
#ifndef NOTERM
	InitTerm();
#endif
}


void gpio_attach_interrupt(unsigned char gpio, gpio_irq irq, int mode)
{
	static unsigned long intbase[] = { INT_GPIOA0, INT_GPIOA1, INT_GPIOA2, INT_GPIOA3 };
	unsigned long port = ti_gpio_get_port(gpio);
	unsigned char pin = ti_gpio_get_pin(gpio);
	unsigned long intp = intbase[gpio >> 3];

	MAP_GPIOIntTypeSet(port, pin, mode);

	MAP_IntRegister(intp, irq);
	MAP_IntPrioritySet(intp, INT_PRIORITY_LVL_1);
	MAP_IntEnable(intp);

	MAP_GPIOIntClear(port, pin);
	MAP_GPIOIntEnable(port, pin);
}


// I2C

#define CRYSTAL_FREQ               40000000

#define I2C_BASE                I2CA0_BASE
#define I2C_MRIS_CLKTOUT        0x2
#define I2C_TIMEOUT_VAL         0x7D

#define RETERR_IF_TRUE(condition) {if(condition) return -1;}

#define RET_IF_ERR(Func)          {int err = (Func); \
                                   if (err) \
                                     return  err;}




void i2c_init(bool fastmode)
{
    MAP_PRCMPeripheralClkEnable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralReset(PRCM_I2CA0);
    MAP_I2CMasterInitExpClk(I2C_BASE,CRYSTAL_FREQ,fastmode);
}


static int I2CTransact(unsigned long ulCmd)
{
    MAP_I2CMasterIntClearEx(I2C_BASE,MAP_I2CMasterIntStatusEx(I2C_BASE,false));
    MAP_I2CMasterTimeoutSet(I2C_BASE, I2C_TIMEOUT_VAL);
    MAP_I2CMasterControl(I2C_BASE, ulCmd);
    while((MAP_I2CMasterIntStatusEx(I2C_BASE, false) & (I2C_INT_MASTER | I2C_MRIS_CLKTOUT)) == 0) ;
    if(MAP_I2CMasterErr(I2C_BASE) != I2C_MASTER_ERR_NONE)
    {
        switch(ulCmd)
        {
        case I2C_MASTER_CMD_BURST_SEND_START:
        case I2C_MASTER_CMD_BURST_SEND_CONT:
        case I2C_MASTER_CMD_BURST_SEND_STOP:
            MAP_I2CMasterControl(I2C_BASE, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
            break;
        case I2C_MASTER_CMD_BURST_RECEIVE_START:
        case I2C_MASTER_CMD_BURST_RECEIVE_CONT:
        case I2C_MASTER_CMD_BURST_RECEIVE_FINISH:
            MAP_I2CMasterControl(I2C_BASE, I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP);
            break;
        default:
            break;
        }
        return -1;
    }

    return 0;
}


int ti_i2c_write(unsigned char ucDevAddr, unsigned char *pucData, unsigned char ucLen, unsigned char ucStop)
{
    RETERR_IF_TRUE(pucData == 0);
    RETERR_IF_TRUE(ucLen == 0);

    MAP_I2CMasterSlaveAddrSet(I2C_BASE, ucDevAddr, false);

    MAP_I2CMasterDataPut(I2C_BASE, *pucData);
    RET_IF_ERR(I2CTransact(I2C_MASTER_CMD_BURST_SEND_START));

    ucLen--;
    pucData++;
    while(ucLen)
    {
        MAP_I2CMasterDataPut(I2C_BASE, *pucData);
        RET_IF_ERR(I2CTransact(I2C_MASTER_CMD_BURST_SEND_CONT));
        ucLen--;
        pucData++;
    }
    if(ucStop)
    {
        RET_IF_ERR(I2CTransact(I2C_MASTER_CMD_BURST_SEND_STOP));
    }

    return 0;
}


int ti_i2c_read(unsigned char ucDevAddr, unsigned char *pucData, unsigned char ucLen)
{
    RETERR_IF_TRUE(pucData == 0);
    RETERR_IF_TRUE(ucLen == 0);

    MAP_I2CMasterSlaveAddrSet(I2C_BASE, ucDevAddr, true);
    if(ucLen == 1) {
        RET_IF_ERR(I2CTransact(I2C_MASTER_CMD_SINGLE_RECEIVE));
    } else {
        RET_IF_ERR(I2CTransact(I2C_MASTER_CMD_BURST_RECEIVE_START));
    }

    ucLen--;
    while(ucLen)
    {
        *pucData = MAP_I2CMasterDataGet(I2C_BASE);
        ucLen--;
        pucData++;
        if(ucLen) {
            RET_IF_ERR(I2CTransact(I2C_MASTER_CMD_BURST_RECEIVE_CONT));
        } else {
            RET_IF_ERR(I2CTransact(I2C_MASTER_CMD_BURST_RECEIVE_FINISH));
        }
    }
    *pucData = MAP_I2CMasterDataGet(I2C_BASE);

    return 0;
}


// I2S


#define CB_TRANSFER_SZ	        256
#define HALF_WORD_SIZE          2
#define BITCLK ((AUDIO_CODEC_16_BIT) * 16000 * (AUDIO_CODEC_STEREO)) //512000

#define MAX_NUM_CH              64  //32*2 entries
#define CTL_TBL_SIZE            64  //32*2 entries
#define UDMA_CH5_BITID          (1<<5)

// Bits per sample Macro
#define AUDIO_CODEC_16_BIT		16
// Number of Channels Macro
#define AUDIO_CODEC_STEREO		2



// The size of the channel control table depends on the number of uDMA channels
// and the transfer modes that are used. Refer to the introductory text
// and the microcontroller datasheet for more information about the channel control table.
#if defined(gcc)
static tDMAControlTable gpCtlTbl[CTL_TBL_SIZE] __attribute__(( aligned(1024)));
#endif
#if defined(ccs)
#pragma DATA_ALIGN(gpCtlTbl, 1024)
static tDMAControlTable gpCtlTbl[CTL_TBL_SIZE];
#endif
#if defined(ewarm)
#pragma data_alignment=1024
static tDMAControlTable gpCtlTbl[CTL_TBL_SIZE];
#endif

// Invoked when DMA operation is complete
static void DmaSwIntHandler(void)
{
	unsigned long uiIntStatus = MAP_uDMAIntStatus();
	MAP_uDMAIntClear(uiIntStatus);
}

// Invoked when DMA operation is in error
static void DmaErrorIntHandler(void)
{
	MAP_uDMAIntClear(MAP_uDMAIntStatus());
}

static unsigned char gaucZeroBuffer[(CB_TRANSFER_SZ * HALF_WORD_SIZE)] = {0};

// Init UDMA transfer
void UDMASetupTransferInit(unsigned long ulChannel)
{
	MAP_uDMAChannelControlSet(ulChannel, UDMA_SIZE_16 | UDMA_CHCTL_SRCINC_16 | UDMA_DST_INC_NONE | UDMA_ARB_8);
	MAP_uDMAChannelAttributeEnable(ulChannel, UDMA_ATTR_USEBURST);
	MAP_uDMAChannelTransferSet(ulChannel, UDMA_MODE_PINGPONG, gaucZeroBuffer, (void *)I2S_TX_DMA_PORT, CB_TRANSFER_SZ);
	MAP_uDMAChannelEnable(ulChannel);
}

// Sets up the uDMA registers to perform the actual transfer
// ulChannel - DMA Channel to be used
// pvSrcBuf - Pointer to the source Buffer
inline void UDMASetupTransfer(unsigned long ulChannel, void *pvSrcBuf)
{
	MAP_uDMAChannelTransferSet(ulChannel, UDMA_MODE_PINGPONG, pvSrcBuf, (void *)I2S_TX_DMA_PORT, CB_TRANSFER_SZ);
}

// Callback function implementing ping pong mode DMA transfer
static void DMAPingPongCompleteAppCB_opt()
{
	const unsigned long ulPrimaryIndexRx = 0x5;
	const unsigned long ulAltIndexRx = 0x25;
	unsigned long ulChannel;

	//static unsigned int playWaterMark = 0;

	if (MAP_uDMAIntStatus() & 0x00000020) { // 1 << channel_no
		// Clear the MCASP write interrupt
		I2SIntClear(I2S_BASE, I2S_INT_XDMA);

		tDMAControlTable *pControlTable = MAP_uDMAControlBaseGet();

		if((pControlTable[ulPrimaryIndexRx].ulControl & UDMA_CHCTL_XFERMODE_M) == 0)
			ulChannel = UDMA_CH5_I2S_TX;
		else if((pControlTable[ulAltIndexRx].ulControl & UDMA_CHCTL_XFERMODE_M) == 0)
			ulChannel = UDMA_CH5_I2S_TX | UDMA_ALT_SELECT;
		else
			return;

		if (circbuff_is_empty()) {// || !playWaterMark) {
			//playWaterMark = IsBufferSizeFilled(pPlayBuffer, PLAY_WATERMARK);
			UDMASetupTransfer(ulChannel, (void *)gaucZeroBuffer);
			//UART_PRINT("zero\n\r");
		} else {
			UDMASetupTransfer(ulChannel, (void *)circbuff_get_read_ptr());
			circbuff_update_read_ptr(HALF_WORD_SIZE * CB_TRANSFER_SZ);
		}
		MAP_uDMAChannelEnable(ulChannel);

	}
}


void i2s_init()
{
	// Enable McASP at the PRCM module
	MAP_PRCMPeripheralClkEnable(PRCM_I2S, PRCM_RUN_MODE_CLK);
	MAP_PRCMPeripheralClkEnable(PRCM_UDMA, PRCM_RUN_MODE_CLK);
	MAP_PRCMPeripheralReset(PRCM_UDMA);

	// Register interrupt handlers
	MAP_IntPrioritySet(INT_UDMA, INT_PRIORITY_LVL_1);
	MAP_uDMAIntRegister(UDMA_INT_SW, DmaSwIntHandler);

	MAP_IntPrioritySet(INT_UDMAERR, INT_PRIORITY_LVL_1);
	MAP_uDMAIntRegister(UDMA_INT_ERR, DmaErrorIntHandler);


	// Set Control Table
	memset(gpCtlTbl, 0, sizeof(tDMAControlTable) * CTL_TBL_SIZE);
	MAP_uDMAControlBaseSet(gpCtlTbl);

	// Enable uDMA using master enable
	MAP_uDMAEnable();

	// configure dma select
	MAP_uDMAChannelAssign(UDMA_CH5_I2S_TX);
	MAP_uDMAChannelAttributeDisable(UDMA_CH5_I2S_TX, UDMA_ATTR_ALTSELECT); // why?

	// setup dma transfer
	UDMASetupTransferInit(UDMA_CH5_I2S_TX);
	UDMASetupTransferInit(UDMA_CH5_I2S_TX | UDMA_ALT_SELECT);

	// Initialize the interrupt callback routine
	// Set up the AFIFO to interrupt at the configured interval
	MAP_I2SIntEnable(I2S_BASE, I2S_INT_XDATA);
	MAP_I2SIntRegister(I2S_BASE, DMAPingPongCompleteAppCB_opt);
	MAP_I2SRxFIFOEnable(I2S_BASE, 8, 1);
	MAP_I2STxFIFOEnable(I2S_BASE, 8, 1);

	// Configure audio renderer
	MAP_PRCMI2SClockFreqSet(512000);
	MAP_I2SConfigSetExpClk(I2S_BASE, 512000, BITCLK, I2S_SLOT_SIZE_16 | I2S_PORT_DMA);
	MAP_I2SSerializerConfig(I2S_BASE, I2S_DATA_LINE_1, I2S_SER_MODE_RX, I2S_INACT_LOW_LEVEL);
	MAP_I2SSerializerConfig(I2S_BASE, I2S_DATA_LINE_0, I2S_SER_MODE_TX, I2S_INACT_LOW_LEVEL);

	// Start Audio
	MAP_I2SEnable(I2S_BASE, I2S_MODE_TX_RX_SYNC);
}



#endif
