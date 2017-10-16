// Smart Toybox File wrapper 
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

#define THEME_FILE "www/theme.stb"

// simplelink include
#include "platform.h"
#include "STBreader.h"

#define THEME_FILE_BIT	0x80000000

inline int file_init() {
	return !stb_open(THEME_FILE);
}

inline int file_open(const char name, int *size) {
	_i32 fh;
	if(!stb_get_sound(name, (int*)&fh, size)) return (fh|THEME_FILE_BIT);
	char fn[2] = {name,0};
	*size = (int)fs_size(fn);
	if((fh = fs_open(fn, FS_OPEN_MODE_READ)) == -1) return -1;
	return fh;
}

inline int file_read(int fh, int offset, void *buffer, int count) {
	if(fh&THEME_FILE_BIT)
		return stb_read(buffer, offset+(fh & (~THEME_FILE_BIT)), count);
	else return fs_read(fh,offset,buffer,count);
}

inline void file_close(int fh) {
	if(!(fh&THEME_FILE_BIT)) fs_close(fh);
}
