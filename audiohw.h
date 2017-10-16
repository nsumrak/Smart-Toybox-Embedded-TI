// Smart Toybox Audio hardware interface (TI 3254)
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

#ifdef __cplusplus
extern "C" {
#endif

// define to provide hw mono support
#define AUDIO_HARDWARE_MONO

#define AUDIO_SPEAKER_GAIN	0

#define CB_TRANSFER_SZ	        256
#define HALF_WORD_SIZE          2

int audiohw_init();

// Configure volume level for specific audio out on a codec device
// volumeLevel: Volume level. 0-100
void audiohw_set_master_volume(unsigned char volumeLevel);
void audiohw_set_channel_volume(unsigned char channel, unsigned char volumeLevel);

// Mute/unmute Audio line out
void audiohw_speaker_mute(BOOL mute);

#ifdef __cplusplus
}
#endif
