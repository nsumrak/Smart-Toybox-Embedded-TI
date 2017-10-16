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

#pragma once

#include "common.h"

#define PACKET_SIZE             1024
#define PLAY_SIZE				8
#define PLAY_BUFFER_SIZE        (PLAY_SIZE*PACKET_SIZE)

extern unsigned char *circbuff_readptr, *circbuff_writeptr;
extern long circbuff_fill;

BOOL circbuff_init();
unsigned char *circbuff_get_read_ptr();
void circbuff_update_read_ptr(int size);
unsigned char *circbuff_get_write_ptr();
void circbuff_update_write_ptr(int size);
unsigned int circbuff_get_filled();
unsigned char *circbuff_get_read_ptr_offset(int offset);

inline unsigned char *circbuff_get_read_ptr()
{
	return circbuff_readptr;
}

inline unsigned char *circbuff_get_write_ptr()
{
	return circbuff_writeptr;
}

inline unsigned int circbuff_get_filled()
{
	return circbuff_fill;
}

inline BOOL circbuff_is_filled(int size)
{
	return (circbuff_get_filled() >= size);
}

inline BOOL circbuff_is_empty()
{
	return circbuff_get_filled()==0;
}
