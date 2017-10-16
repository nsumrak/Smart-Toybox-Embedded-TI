// Smart Toybox accelerator services (BMA222 and BMA222e)
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

#include "platform.h"
#include "board.h"
#include "common.h"
#include <stdio.h>
#include "debug.h"
#include "evqueue.h"

#define BMA222_DEV_ADDR          0x18

#define BMA222_CHID_ID_NUM       0x00
#define BMA222_INT_STATUS		 0x09
#define BMA222_INTMAP_1			 0x19
#define BMA222_INTMAP_2			 0x1A
#define BMA222_INTMAP_3			 0x1B
#define BMA222_INT_MODE_RESET	 0x21
#define BMA222_INT_EN_1			 0x16
#define BMA222_INT_EN_2			 0x17
#define BMA222_INT_SRC			 0x1E
#define BMA222_POWER_MODE		 0x11
#define BMA222_TAP_LP_TH_CONFIG	 0x2B
#define BMA222_TAP_DUR			 0x2A
#define BMA222_FLAT_ORIENT_STAT	 0x0C
#define BMA222_TAP_SLOPE_STAT	 0x0B
#define BMA222_FLAT_TH_ANGLE	 0x2E
#define BMA222_Z_AXIS			 0x07

inline unsigned char GetRegisterValue(unsigned char reg)
{
	return i2c_read(BMA222_DEV_ADDR, reg);
}

inline BOOL SetRegisterValue(unsigned char reg, unsigned char value)
{
	return i2c_write(BMA222_DEV_ADDR, reg, value);
}


char accel_isflat = 1;

static void AccelIntReq()
{
	//, lastorient = 0;
	unsigned char flat, status = GetRegisterValue(BMA222_FLAT_ORIENT_STAT);//, orient;

	flat = status&0x80;
	//orient = (fostatus>>4) & 7;
	//UART_PRINT("acc orient=%d",orient);
	if((flat && !accel_isflat) || (!flat && accel_isflat)) { // flat change
		accel_isflat = !accel_isflat;
		evqueue_write(EV_ACCEL_FLAT, accel_isflat, 0);
//	} else if(orient != lastorient) {
//		lastorient = orient;
//		UART_PRINT("acc orient int, new orient = %d\n\r", orient);
	} else {
		evqueue_write(EV_ACCEL_TAP, 0, 0);
	}

	//SetRegisterValue(BMA222_INT_MODE_RESET,0x8f); // reset latched
	gpio_clear_irq(GPIO_ACCEL);
}

unsigned char accel_get_zax()
{
	return GetRegisterValue(BMA222_Z_AXIS);
}

int accel_init()
{
	if(!SetRegisterValue(BMA222_INTMAP_1,0xF7)) return 2; // E7 - flat, orient, s_tap, d_tap, slope, high, low -> map to int 1
	if(!SetRegisterValue(BMA222_INTMAP_2,0x01)) return 3; // data -> map to int 1
	if(!SetRegisterValue(BMA222_INTMAP_3,0x00)) return 4; // no mapping to int2

	//if(!SetRegisterValue(BMA222_INT_MODE_RESET,0x8F)) return 5; // latched mode, reset int
	if(!SetRegisterValue(BMA222_INT_MODE_RESET,0x0)) return 5; // non-latched mode
	//if(!SetRegisterValue(BMA222_TAP_LP_TH_CONFIG,0x0A)) return; // 625ms for th, 2 samples in LP mode
	if(!SetRegisterValue(BMA222_FLAT_TH_ANGLE,0x01)) return 4; // 0-44.8 deg (bits 0-5), default is 8

	// set up interrupt for accelerometer
	gpio_attach_interrupt(GPIO_ACCEL, AccelIntReq, GPIO_IRQMODE_FALLING_EDGE);

	if(!SetRegisterValue(BMA222_INT_EN_1,0xA0)) return 5; // enable flat and single tap int (no orientation)
	if(!SetRegisterValue(BMA222_POWER_MODE,0x5C)) return 6; // lowpower mode with 500ms period
	return 0;
}
