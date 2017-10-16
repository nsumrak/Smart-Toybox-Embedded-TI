// Smart Toybox scale interface (HX711)
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

extern long SCALE_OFFSET;
extern long SCALE_FACTOR;
extern signed char SCALE_ORIENTATION;

static long previous = 0;

void hx711_init();

inline unsigned weight_in_grams(long raw)
{
	long val = raw - SCALE_OFFSET;
	val = SCALE_ORIENTATION * val;
	val = val > 0 ? val : 0;
	return (unsigned)(val / SCALE_FACTOR);
}

inline unsigned hx711_get_weight() { return weight_in_grams(previous); }
