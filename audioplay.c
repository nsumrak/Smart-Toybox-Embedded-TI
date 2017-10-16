// Smart Toybox Audio playback
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

// Hardware & driverlib library includes
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "hw_ints.h"

// Demo app includes
#include "network.h"
#include "common.h"
#include "debug.h"
#include "files.h"
#include "audioplay.h"
#include "circ_buff.h"
#include "audiohw.h"
#include "wp/wavpack.h"
#include "STBreader.h"


#define MAX_MUSIC_SIZE	63


struct audio_fileinput {
	int infile;
	unsigned offset;
	int filelen;
	char music[MAX_MUSIC_SIZE];
	uchar inmusic;
	uchar pause;
	uchar repeat[3];
	WavpackContext wpc;
	read_stream rsf;
};

static struct audio_fileinput fileleft;
static struct audio_fileinput fileright;


inline int32_t read_bytes(struct audio_fileinput *fi, void *buff, int32_t bcount)
{
	int32_t fleft = fi->filelen - fi->offset;
	if(fleft<=0) {
		file_close(fi->infile);
		fi->infile = -1;
		return -1;
	}
	if(fleft<bcount) bcount = fleft;
	if (fi->infile == -1) return 0;
	int32_t read = file_read(fi->infile, fi->offset, buff, bcount);
	if (read < bcount) {
		file_close(fi->infile);
		fi->infile = -1;
	} else {
		fi->offset += read;
		// UART_PRINT("read = %d new offset = %d\n\r", read, fi->offset);
	}
	return read;
}

static int32_t read_bytes_left(void *buff, int32_t bcount)
{
	return read_bytes(&fileleft, buff, bcount);
}

static int32_t read_bytes_right(void *buff, int32_t bcount)
{
	return read_bytes(&fileright, buff, bcount);
}


void audioplay_init()
{
	CLEAR(fileleft);
	CLEAR(fileright);
	fileleft.infile = fileright.infile = -1;
	fileleft.rsf = read_bytes_left;
	fileright.rsf = read_bytes_right;
}


inline void do_rewind(char cto, struct audio_fileinput *fi, int type)
{
	if(fi->music[fi->inmusic+1] && (++fi->repeat[type]) == fi->music[fi->inmusic+1]-'0') {
		fi->inmusic += 2;
		fi->repeat[type] = 0;
		return;
	}
	while(fi->inmusic && fi->music[--fi->inmusic] != cto) ;
	fi->inmusic++;
}


static int play_next_file(struct audio_fileinput *fi)
{
	while(1)
		switch(fi->music[fi->inmusic]) {

		case 0:
			fi->music[(fi->inmusic = 0)] = 0;
			return -1;

		// rewind
		case '{':
		case '[':
		case '(':
			fi->inmusic++;
			continue;

		case '}':
			do_rewind('{',fi,0);
			continue;

		case ']':
			do_rewind('[',fi,1);
			continue;

		case ')':
			do_rewind('(',fi,2);
			continue;

		// pause
		case ',':
			fi->pause = 2;
			fi->inmusic++;
			return 0;

		case ';':
			fi->pause = 4;
			fi->inmusic++;
			return 0;

		case '.':
			fi->pause = 8;
			fi->inmusic++;
			return 0;

		case '\'':
			fi->pause = 16;
			fi->inmusic++;
			return 0;

		case '"':
			fi->pause = 31;
			fi->inmusic++;
			return 0;

		default:
			fi->infile = file_open(fi->music[fi->inmusic++], &fi->filelen);
			if (fi->infile == -1) return 1;
			fi->offset = 0;
			if (WavpackOpenFileInput(&fi->wpc, fi->rsf)) return 2;
			return 0;
		}
}


// channels - 1 (left), 2 (right) or 3 (both)
void audioplay_fadeout(unsigned channels)
{
	ASSERT(channels>0 && channels<4);
	int filled = circbuff_get_filled();
	if(filled < (HALF_WORD_SIZE * CB_TRANSFER_SZ)*2) {
		UART_PRINT("fadeout: buff filled %d\n\r",filled);
		return;
	}
	int toprocess = (filled - (HALF_WORD_SIZE * CB_TRANSFER_SZ)) >> 2; // get number of samples of left/right channel (minus i2s current transfer)
	UART_PRINT("num samples to fade out: %d\n\r",toprocess);
	int inpass = toprocess / 98;
	int now = inpass;
	int curval = 99;
	int buffer = CB_TRANSFER_SZ / 2; // read i2s chunks (size is long)
	int boffset = (HALF_WORD_SIZE * CB_TRANSFER_SZ); // set to not disturb i2s
	unsigned char *buff = circbuff_get_read_ptr_offset(boffset);
	for(; toprocess; toprocess--)
	{
		short el;
		if(channels & 1) {
			el = ((signed)buff[1] << 8) + buff[0];
			el = (((int)el) * curval) / 100;
			*buff++ = el;
			*buff++ = el >> 8;
		} else buff += 2;

		if(channels & 2) {
			el = ((signed)buff[1] << 8) + buff[0];
			el = (((int)el) * curval) / 100;
			*buff++ = el;
			*buff++ = el >> 8;
		} else buff += 2;

		if(!(--now)) {
			now = inpass;
			if(curval) curval--;
		}

		if(!(--buffer)) {
			buffer = CB_TRANSFER_SZ / 2;
			buff = circbuff_get_read_ptr_offset((boffset += CB_TRANSFER_SZ * 2));
		}
	}
}


void audioplay_process()
{
	while(circbuff_get_filled() <= PACKET_SIZE * (PLAY_SIZE-2)) {
//		unsigned long now = time_get();
		audioplay_getbuffer(circbuff_get_write_ptr());
		circbuff_update_write_ptr(PACKET_SIZE);
//		unsigned long elapse = time_get()-now;
//		if(elapse>10) UART_PRINT("el %d\n\r", elapse);
	}
}


inline void audioplay_dostop(struct audio_fileinput *fi)
{
	if (fi->infile != -1) {
		file_close(fi->infile);
		fi->infile = -1;
	}
	if(fi->music[fi->inmusic]) audioplay_fadeout(fi==&fileleft ? 1 : 2);
	fi->pause = 0;
	CLEAR(fi->repeat);
	fi->music[(fi->inmusic = 0)] = 0;
}


void audioplay_stop_all()
{
	audioplay_fadeout(3);
	fileleft.music[(fileleft.inmusic = 0)] = 0;
	fileright.music[(fileright.inmusic = 0)] = 0;
	audioplay_dostop(&fileleft);
	audioplay_dostop(&fileright);
}


void audioplay_stop(int channel)
{
	audioplay_dostop(channel ? &fileright : &fileleft);
}


int audioplay_playfile(int channel, const char *music)
{
	struct audio_fileinput *fi = channel ? &fileright : &fileleft;
	audioplay_dostop(fi);
	strcpy(fi->music, music);
	return play_next_file(fi);
}


int audioplay_appendmusic(int channel, unsigned char type, int num)
{
	struct audio_fileinput *fi = channel ? &fileright : &fileleft;
	int len = strlen(fi->music);
	if(!len) return audioplay_playmusic(channel, type, num);
	char c;
	if((c=fi->music[len-1])=='}' || c==']' || c==')')
		fi->music[len++] = '1'; // break infinite loop
	stb_get_music(fi->music+len, type, num);
	return 0;
}


int audioplay_playmusic(int channel, unsigned char type, int num)
{
	struct audio_fileinput *fi = channel ? &fileright : &fileleft;
	audioplay_dostop(fi);
	stb_get_music(fi->music, type, num);
	return play_next_file(fi);
}

// Reformat samples from longs in processor's native endian mode to
// little-endian data with 2-pass channel muxing.
static void convert2stereo(int32_t *input, uchar *output, int scount)
{
	for (; scount; scount--)
	{
		int32_t temp = *(input++);
		*(output++) = (uchar)temp;
		*(output++) = (uchar)(temp >> 8);
		output += 2; // skip channel, will be filled in next pass (or is already filled)
	}
}


#ifdef AUDIO_SOFTWARE_MONO
// Reformat samples from longs in processor's native endian mode to
// little-endian data muxing with with 2-pass channel muxing.
static void mix2mono(int32_t *input1, int32_t *input2, uchar *output, int scount)
{
	for (; scount; scount--)
	{
		int32_t a = *(input1++);
		int32_t b = *(input2++);//((signed)*((unsigned short*)output++)) | (*((signed short *)output++) << 8);
		int32_t m = a+b;

#if (AUDIO_SOFTWARE_MONO == 1)
		a += 32768;
		b += 32768;
		m = ((a < 32768) && (b < 32768)) ? ((a * b) >> 15) : (((a + b) << 1) - ((a * b) >> 15) - 65536);
		if (m == 65536) m = 32767;
		else m -= 32768;
#else
		if(m>32767) m = 32767;
		else if(m<-32768) m = -32768;
#endif
		*(output++) = 0;
		*(output++) = 0;
		*(output++) = (uchar)m;
		*(output++) = (uchar)(m >> 8); // put mix into left channel
	}
}

#else

static void fill_pause2stereo(uchar *output, int scount)
{
	for (; scount; scount--)
	{
		*(output++) = *(output++) = 0;
		output += 2; // skip channel, will be filled in next pass (or is already filled)
	}
}

#endif


static int filecheck(struct audio_fileinput *fi)
{
	if(fi->infile != -1) return 1;
	if(play_next_file(fi)) return 0;
	if(fi->pause) {
		fi->pause--;
		return 0;
	}
	return 1;
}


void audioplay_getbuffer(uchar *buffer) // sample count = 256*16bit*stereo = 1024 bytes
{
	uint32_t samples_unpacked=0;

#ifdef AUDIO_SOFTWARE_MONO
	static int32_t temp_buffer1[256], temp_buffer2[256];
#else
	static int32_t temp_buffer[256];
	#define temp_buffer1 temp_buffer
	#define temp_buffer2 temp_buffer
#endif

leftagain:
	if(fileleft.pause) {
#ifdef AUDIO_SOFTWARE_MONO
		memset(temp_buffer2, 0, 1024);
#else
		fill_pause2stereo(buffer, 256);
#endif
		fileleft.pause--;
	} else {
		if (filecheck(&fileleft)) {
			samples_unpacked += WavpackUnpackSamples(&fileleft.wpc, temp_buffer1+samples_unpacked, 256-samples_unpacked);
			if (!samples_unpacked && fileleft.infile != -1) goto leftagain;
			if (samples_unpacked < 256) goto leftagain;
		} else
			memset(temp_buffer1 + samples_unpacked, 0, (256 - samples_unpacked) << 2);
		convert2stereo(temp_buffer1, buffer, 256);
	}

	samples_unpacked = 0;
rightagain:
	if(fileright.pause) {
#ifdef AUDIO_SOFTWARE_MONO
		memset(temp_buffer2, 0, 1024);
		mix2mono(temp_buffer1, temp_buffer2, buffer, 256);
#else
		fill_pause2stereo(buffer+2, 256);
#endif
		fileright.pause--;
	} else {
		if (filecheck(&fileright)) {
			samples_unpacked += WavpackUnpackSamples(&fileright.wpc, temp_buffer2+samples_unpacked, 256-samples_unpacked);
			if (!samples_unpacked && fileright.infile != -1) goto rightagain;
			if (samples_unpacked < 256) goto rightagain;
		} else
			memset(temp_buffer2 + samples_unpacked, 0, (256 - samples_unpacked) << 2);
#ifdef AUDIO_SOFTWARE_MONO
		mix2mono(temp_buffer1, temp_buffer2, buffer, 256);
#else
		convert2stereo(temp_buffer2, buffer+2, 256);
#endif
	}
}
