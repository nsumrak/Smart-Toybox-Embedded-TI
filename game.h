// Smart Toybox game control interface
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
#include "evqueue.h"

extern BOOL started, pause;

BOOL game_start();
void game_stop();
void game_process_weight_start(unsigned short weight, unsigned short count);
void game_process_weight(unsigned short weight, unsigned short count);
void game_process_weight_stop(unsigned short weight, unsigned short count);
void game_process_timer_expired(unsigned short timer);
void game_process_accel_flat(BOOL flat);
void game_set_mood_led();
inline BOOL game_started() { return started; }
inline BOOL game_paused() { return pause; }
inline BOOL game_active() { return started && !pause; }
