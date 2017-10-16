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

#pragma once


// TI IMPLEMENTATION
#ifdef TI_PLATFORM
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "gpio.h"
#include "interrupt.h"
#include "rom.h"
#include "rom_map.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
#include "circ_buff.h"
#include "simplelink.h"

void platform_init();

// gpio
typedef void (*gpio_irq)();

inline unsigned long ti_gpio_get_port(unsigned char gpio)
{
	static unsigned long bases[] = { GPIOA0_BASE, GPIOA1_BASE, GPIOA2_BASE, GPIOA3_BASE };
	return bases[gpio >> 3];
}

inline unsigned char ti_gpio_get_pin(unsigned char gpio)
{
	return (1 << (gpio & 7));
}

inline void gpio_mode(unsigned char gpio, int mode)
{
	MAP_GPIODirModeSet(ti_gpio_get_port(gpio), ti_gpio_get_pin(gpio), mode);
}

inline void gpio_write(unsigned char gpio, bool val)
{
	MAP_GPIOPinWrite(ti_gpio_get_port(gpio), ti_gpio_get_pin(gpio), val ? 0xff : 0);
}

inline bool gpio_read(unsigned char gpio)
{
	unsigned char pin = ti_gpio_get_pin(gpio);
	return (MAP_GPIOPinRead(ti_gpio_get_port(gpio), pin) & pin) ? true : false;
}

void gpio_attach_interrupt(unsigned char gpio, gpio_irq irq, int mode);

inline void gpio_clear_irq(unsigned char gpio)
{
	MAP_GPIOIntClear(ti_gpio_get_port(gpio), ti_gpio_get_pin(gpio));
}

// gpio modes
#define GPIO_MODE_INPUT		GPIO_DIR_MODE_IN
#define GPIO_MODE_OUTPUT	GPIO_DIR_MODE_OUT

// gpio irq modes
#define GPIO_IRQMODE_RISING_EDGE	GPIO_RISING_EDGE	
#define GPIO_IRQMODE_FALLING_EDGE	GPIO_FALLING_EDGE
#define GPIO_IRQMODE_BOTH_EDGES		GPIO_BOTH_EDGES
#define GPIO_IRQMODE_LOW_LEVEL		GPIO_LOW_LEVEL
#define GPIO_IRQMODE_HIGH_LEVEL		GPIO_HIGH_LEVEL


// interrupts, delay
inline void int_disable_all()
{
	IntMasterDisable();
}

inline void int_enable_all()
{
	IntMasterEnable();
}

inline delay_us(unsigned long us)
{
    MAP_UtilsDelay(us*27);
}


// i2c
int ti_i2c_write(unsigned char ucDevAddr, unsigned char *pucData, unsigned char ucLen, unsigned char ucStop);
int ti_i2c_read(unsigned char ucDevAddr, unsigned char *pucData, unsigned char ucLen);

void i2c_init(bool fastmode);

inline bool i2c_write(unsigned char address, unsigned char reg, unsigned char value)
{
    unsigned char ucData[2] = { reg, value };
	return (ti_i2c_write(address,ucData,2,1) == 0);
}

inline unsigned char i2c_read(unsigned char address, unsigned char reg)
{
	unsigned char readbuf;
    if(!ti_i2c_write(address,&reg,1,0))
    	if(!ti_i2c_read(address, &readbuf, 1))
			return readbuf;

	return 0;
}


// i2s-DMA-audio_buffers
void i2s_init();
//void i2s_exit();

// files
typedef int fs_file;

inline long fs_size(const char *fn)
{
	SlFsFileInfo_t fsfi;
	if(sl_FsGetInfo((const _u8*)fn, 0, &fsfi)) return -1;
	return (long)fsfi.FileLen;
}

inline fs_file fs_open(const char *fn, unsigned long mode)
{
	_i32 fh;
	if(sl_FsOpen((const _u8*)fn, mode, 0, &fh)) return -1;
	return (fs_file)fh;
}

#define FS_OPEN_MODE_READ	FS_MODE_OPEN_READ
#define FS_OPEN_MODE_WRITE	FS_MODE_OPEN_WRITE
#define FS_OPEN_MODE_RW		(FS_MODE_OPEN_READ|FS_MODE_OPEN_WRITE)
#define FS_OPEN_MODE_CREATE	FS_MODE_OPEN_CREATE(0, FS_MODE_OPEN_WRITE)


inline int fs_read(fs_file fh, long offset, const char *buf, int count)
{
	return sl_FsRead(fh,offset,(_u8*)buf,count);
}

inline int fs_write(fs_file fh, long offset, char *buf, int count)
{
	return sl_FsRead(fh,offset,(_u8*)buf,count);
}

inline void fs_close(fs_file fh)
{
	sl_FsClose(fh, 0, 0, 0);
}


#else
// ESP implementation

// gpio
typedef void (*gpio_irq)();

void gpio_mode(unsigned char gpio);
void gpio_write(unsigned char gpio, bool val);
bool gpio_read(unsigned char gpio);
void gpio_attach_interrupt(unsigned char gpio, gpio_irq irq, int mode);
void gpio_clear_irq(unsigned char gpio);

// gpio irq modes
#define GPIO_IRQMODE_RISING_EDGE	GPIO_RISING_EDGE	
#define GPIO_IRQMODE_FALLING_EDGE	GPIO_FALLING_EDGE
#define GPIO_IRQMODE_BOTH_EDGES		GPIO_BOTH_EDGES
#define GPIO_IRQMODE_LOW_LEVEL		GPIO_LOW_LEVEL
#define GPIO_IRQMODE_HIGH_LEVEL		GPIO_HIGH_LEVEL

// interrupt
void int_disable_all();
void int_enable_all();

// i2c
void i2c_init();
void i2c_write(unsigned char address, unsigned char reg, const char *writebuf, int bufsize);
void i2c_read(unsigned char address, unsigned char reg, char *readbuf, int bufsize);

// i2s-DMA-audio_buffers
void i2s_init();
void i2s_exit();

// files
typedef int fs_file;
long fs_size(const char *fn);
fs_file fs_open(const char *fn, int mode);
int fs_read(fs_file, long offset, const char *buf, int bufsize);
int fs_write(fs_file, long offset, char *buf, int bufsize);
void fs_close(fs_file);

#endif
