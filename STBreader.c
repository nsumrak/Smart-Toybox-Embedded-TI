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

#include <stdlib.h>
#include "STBreader.h"


static filehandle stbfilet = INVALID_FILE_HANDLE;
static unsigned char numsounds, nummusics;
static int soundoffs, musicoffs;
static unsigned char soundheader[384], *musicheader;
#ifndef EMBEDDED
static int namesoffs;
#endif


static int read3Blen(const unsigned char *buf)
{
	int res = (*buf++) << 16;
	res |= (*buf++) << 8;
	res |= *buf;
	return res;
}
int stb_open(const char *filename)
{
	stbfilet = open_file(filename);
	if (INVALID_FILE_HANDLE == stbfilet) return 1;

	unsigned char buf[12];
	if (stb_read(buf, 0, 12) < 12 || buf[0] != 0xba || buf[1] != 0xba || buf[2] != 0xde || buf[3] != 0xda) {
		stb_close();
		return 2;
	}

	numsounds = buf[4];
	nummusics = buf[8];

	musicoffs = read3Blen(buf + 5);
#ifndef EMBEDDED
	namesoffs = read3Blen(buf + 9);
#endif

	int shsize = (int)numsounds << 2;
	int hdrsize = shsize + ((int)nummusics << 1);
	stb_read(soundheader, 12, hdrsize);

	musicheader = soundheader + shsize;
	soundoffs = hdrsize + 12;
	musicoffs += soundoffs;
#ifndef EMBEDDED
	namesoffs += musicoffs;
#endif

	return 0;
}

void stb_close()
{
	if(stbfilet == INVALID_FILE_HANDLE) return;
#ifdef EMBEDDED
	fs_close(stbfilet);
#else
	fclose(stbfilet);
#endif
	stbfilet = INVALID_FILE_HANDLE;
}

// find pos and size of sound data with id 'c'
int stb_get_sound(unsigned char c, int *pos, int *size)
{
	if(stbfilet == INVALID_FILE_HANDLE) return 1;
	unsigned char i;
	unsigned char *sh = soundheader;
	*pos = soundoffs;

	for (i = 0; i < numsounds; i++) {
		int sz = read3Blen(sh + 1);
		if (*sh == c) {
			if(size) *size = sz;
			return 0;
		}
		*pos += sz;
		sh += 4;
	}
	return 1;
}

// read theme data from pos in size
int stb_read(void *buf, int pos, int size)
{
	if(stbfilet == INVALID_FILE_HANDLE) return 0;
#ifdef EMBEDDED
	return fs_read(stbfilet,pos,buf,size);
#else
	fseek(stbfilet, pos, SEEK_SET);
	return fread(buf, 1, size, stbfilet);
#endif
}

// get count of musics od given type
int stb_get_music_num(unsigned char type)
{
	if(stbfilet == INVALID_FILE_HANDLE) return 0;
	unsigned char i;
	unsigned char *mh = musicheader;
	int cnt = 0;

	for (i = 0; i < nummusics; i++) {
		if (*mh == type) cnt++;
		mh += 2;
	}
	return cnt;
}

// copy music number num of given type to the buffer
int stb_get_music(char *buf, unsigned char type, int num)
{
	if(stbfilet == INVALID_FILE_HANDLE) return 2;
	unsigned char i;
	unsigned char *mh = musicheader;
	int cnt = 0;
	int pos = musicoffs;

	for (i = 0; i < nummusics; i++) {
		if (*mh == type) {
			if (cnt == num) {
				stb_read(buf, pos, mh[1]);
				buf[mh[1]] = 0;
				return 0;
			}
			cnt++;
		}
		pos += (int)mh[1];
		mh += 2;
	}
	buf[0] = 0;
	return 1;
}

#ifndef EMBEDDED

// get sound name
void stb_get_sound_name(char *buf, int namenum)
{
	int i, npos = namesoffs;
	unsigned char sz;

	for (i = 0; i < namenum; i++) {
		stb_read(&sz, npos, 1);
		npos += (int)sz+1;
	}
	stb_read(&sz, npos, 1);
	if(sz)
		stb_read(buf, npos+1, sz);
	buf[sz] = 0;
}

void stb_get_music_name(char *buf, int namenum)
{
	stb_get_sound_name(buf, namenum + numsounds);
}

int stb_get_num_sounds()
{
	return (int)numsounds;
}

int stb_get_num_musics()
{
	return (int)nummusics;
}

// get sound no# num
void stb_get_sound_bynum(int num, unsigned char *id, int *pos, int *size)
{
	unsigned char i;
	unsigned char *sh = soundheader;
	*pos = soundoffs;

	for (i = 0; i <= num; i++) {
		*id = *sh;
		*size = read3Blen(sh + 1);
		if (i != num) {
			*pos += *size;
			sh += 4;
		}
	}
}

// get music no# num
void stb_get_music_bynum(int num, unsigned char *type, char *buf)
{
	unsigned char i;
	unsigned char *mh = musicheader+1;
	int pos = musicoffs;

	for (i = 0; i < num; i++) {
		pos += (int)*mh;
		mh += 2;
	}
	stb_read(buf, pos, *mh);
	buf[*mh] = 0;
	*type = *(mh - 1);
}


#endif
