// Smart Toybox Theme file reader
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

// set to remove unneeded functions on embedded systems
#define EMBEDDED 1


/***********************************************
STB file format:

-- Header (12b)
LONG id = 0xBABADEDA
CHAR numsounds
3 CHAR soundsize
CHAR nummusic
3 CHAR musicsize

-- SOUNDS INDEX for each numsounds
CHAR id
3 CHAR wvsize

-- MUSIC INDEX for each nummusic
CHAR type
CHAR plsize

-- SOUND DATA (soundsize) for each numsounds
CHAR wv[wvsize]

-- MUSIC DATA (musicsize) for each nummusic
CHAR playlist[plsize]

-- NAMES for each numsounds+nummusic
CHAR size
CHAR name[size]

************************************************/



#ifdef __cplusplus
extern "C" {
#endif

	enum {
		GAME_START = 0,
		GAME_WIN,
		GAME_LOSE,
		GAME_MUSIC,
		GAME_GIMME,
		GAME_CHEW,
		GAME_CHEW_GRAND,
		GAME_GULP,
		GAME_GULP_GRAND,
		GAME_COMPLAIN,
		GAME_GUARDIAN,
		GAME_GUARDIAN_OFF,
		GAME_GOOD_PROGRESS,
		GAME_BAD_PROGRESS,
		GUARDIAN,
		GUARDIAN_OFF,
	};

	int stb_open(const char *filename);
	void stb_close();
	int stb_get_sound(unsigned char c, int *pos, int *size);
	int stb_read(void *buf, int pos, int size);
	int stb_get_music_num(unsigned char type);
	int stb_get_music(char *buf, unsigned char type, int num);

#ifndef EMBEDDED
	#include "platform.h"
	#include <stdio.h>

	void stb_get_sound_name(char *buf, int namenum);
	void stb_get_music_name(char *buf, int namenum);
	int stb_get_num_sounds();
	int stb_get_num_musics();
	void stb_get_sound_bynum(int num, unsigned char *id, int *pos, int *size);
	void stb_get_music_bynum(int num, unsigned char *type, char *buf);

	#define INVALID_FILE_HANDLE (0)
	typedef FILE *filehandle;
	inline filehandle open_file(const char *fn) { return fopen(fn, "rb"); }

#else
	#include "platform.h"
	typedef _i32 filehandle;
	inline filehandle open_file(const char *fn) {
		return fs_open(fn, FS_OPEN_MODE_READ);
	}
	#define INVALID_FILE_HANDLE (-1)

#endif

#ifdef __cplusplus
};
#endif
