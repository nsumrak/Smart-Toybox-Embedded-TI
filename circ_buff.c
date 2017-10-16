// Smart Toybox Circular Buffer implementation
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
#include "circ_buff.h"
#include "debug.h"
#include <stdlib.h>

static unsigned char *circbuff_buffer;
unsigned char *circbuff_readptr, *circbuff_writeptr;
long circbuff_fill;


BOOL circbuff_init()
{
	circbuff_fill = 0;
	return ((circbuff_readptr = circbuff_writeptr = circbuff_buffer = (unsigned char *)malloc(PLAY_BUFFER_SIZE)) != 0);
}

unsigned char *circbuff_get_read_ptr_offset(int offset)
{
	unsigned char *buf = circbuff_get_read_ptr() + offset;
	if(buf >= circbuff_buffer + PLAY_BUFFER_SIZE)
		buf -= PLAY_BUFFER_SIZE;
	return buf;
}

void circbuff_update_read_ptr(int size)
{
	if(circbuff_fill<size) {
		UART_PRINT("circbuff update read ptr size: %d, fill was: %d\n\r",size, circbuff_fill);
		size = circbuff_fill;
	}
	circbuff_readptr += size;
	circbuff_fill -= size;
//	if(circbuff_readptr > circbuff_buffer + PLAY_BUFFER_SIZE)
//		UART_PRINT("sjeb\n\r");
	if(circbuff_readptr >= circbuff_buffer + PLAY_BUFFER_SIZE)
		circbuff_readptr = circbuff_buffer;
}

void circbuff_update_write_ptr(int size)
{
	int_disable_all();
	circbuff_writeptr += size;
	circbuff_fill += size;
//	if(circbuff_writeptr > circbuff_buffer + PLAY_BUFFER_SIZE)
//		UART_PRINT("fuck\n\r");
	if(circbuff_writeptr >= circbuff_buffer + PLAY_BUFFER_SIZE)
		circbuff_writeptr = circbuff_buffer;
	int_enable_all();
}
