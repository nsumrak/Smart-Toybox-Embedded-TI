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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// define AUDIO_SOFTWARE_MONO for software mono mixing
// set to 0 for quick and dirty mixing
// set to 1 for nice mono mixing
//#define AUDIO_SOFTWARE_MONO	1

#include "simplelink.h"

void audioplay_init();
int audioplay_playfile(int channel, const char *music);
void audioplay_getbuffer(unsigned char *buffer);
int audioplay_appendmusic(int channel, unsigned char type, int num);
int audioplay_playmusic(int channel, unsigned char type, int num);
void audioplay_stop(int channel);
void audioplay_stop_all();
// main loop audio processing
void audioplay_process();

#ifdef __cplusplus
}
#endif

