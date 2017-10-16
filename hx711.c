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

#include "platform.h"
#include "board.h"
#include "evqueue.h"
#include "debug.h"
#include "hx711.h"
#include "settings.h"

// gain: 128 or 64 for channel A; channel B works with 32 gain factor only
// channel selection is made by passing the appropriate gain
#define GAIN 128
#define GAIN_BITS 1 // 1 bit for gain 128, 3 bits for 64, 2 bits for 32

#define CALIBRATION

#define CALIB_STEPS	80
#define CALIB_WEIGHT 1510

long WEIGHT_THRESHOLD = 96;
long SCALE_OFFSET = 0xffff00d5;
long SCALE_FACTOR = 110;
signed char SCALE_ORIENTATION = -1;

// writes value to clock gpio
inline void write_clock(BOOL val)
{
	gpio_write(GPIO_HX_CLK, val);
}

// reads value from data gpio
inline BOOL read_data()
{
	return gpio_read(GPIO_HX_DATA);
}

static void power_down() {
	write_clock(0);
	delay_us(1);
	write_clock(1);
	delay_us(70); // must be > 60us
}

static void power_up() {
	write_clock(0);
}

long hx711_read() {
	unsigned i, data = 0;

//	MAP_UtilsDelay(80);

	// pulse the clock pin 24 times to read the data
	for (i = 0; i < 24; i++) {
		write_clock(1);
		delay_us(1);
		data <<= 1;
		data |= read_data();
		write_clock(0);
		delay_us(1);
	}

	// set the channel and the gain factor for the next reading using the clock pin
	for (i = 0; i < GAIN_BITS; i++) {
		write_clock(1);
		delay_us(1);
		write_clock(0);
		delay_us(1);
	}

	if (data & 0x800000) data |= 0xff000000;
	return (long)data;
}

static int first = 0;

#ifdef CALIBRATION

long scalemin, scalemax;
static void ScaleCalibIntReq()
{
	if (first < CALIB_STEPS) {
		if (!first) {
			UART_PRINT("hx711 calibration\n\r");
			scalemin = scalemax = hx711_read();
			first = 1;
			return;
		}
		long val = hx711_read();
		if (val < scalemin) scalemin = val;
		else if (val > scalemax) scalemax = val;
		first++;
	}
	else {
		int cnt = first - CALIB_STEPS;
		if (!cnt) {
			long diff = (scalemax - scalemin) / 2;
			WEIGHT_THRESHOLD = labs(diff);
			SCALE_OFFSET = scalemax - diff;
			first++;
			UART_PRINT("Please put %d gram weight on scale...\n\r", CALIB_WEIGHT);
			first++;
			return;
		}
		long val = hx711_read();
		if (cnt == 1) {
			// waiting for weight
			if (labs(val - SCALE_OFFSET) < 2 * WEIGHT_THRESHOLD) return;
			scalemin = scalemax = hx711_read();
			first++;
			return;
		}
		first++;
		if (cnt < 20) return; //wait to stabilize
		cnt -= 20;
		if (cnt < CALIB_STEPS) {
			if (val < scalemin) scalemin = val;
			else if (val > scalemax) scalemax = val;
			return;
		}
		if (cnt == CALIB_STEPS) {
			long scalehigh = scalemax - ((scalemax - scalemin) / 2);
			long diff = scalehigh - SCALE_OFFSET;
			if (diff < 0) {
				SCALE_ORIENTATION = -1;
				diff = -diff;
			}
			else SCALE_ORIENTATION = 1;
			SCALE_FACTOR = diff / CALIB_WEIGHT;
			UART_PRINT("calibrated scale settings:\n\rscaleoffset=%d\n\rscalefactor=%d\n\rscaleorientation=%d\n\rscalethreshold=%d\n\r------\n\rProcess finished\n\r", SCALE_OFFSET, SCALE_FACTOR, SCALE_ORIENTATION, WEIGHT_THRESHOLD);
		}
		// ended
	}
}

#else //calibration

static void ScaleIntReq()
{
	if (!first) UART_PRINT("hx711 normal operation\n\r");
	first = 1;
	static unsigned char cnt = 0;

	long val = hx711_read();
	// int w = weight_in_grams(val); if (w) UART_PRINT("HX711: raw=%ld grams = %d count = %d\n\r", val, w, cnt);
	if (labs(val - previous) > WEIGHT_THRESHOLD) {
		evqueue_write(cnt ? EV_WEIGHT : EV_WEIGHT_START, weight_in_grams(val), cnt);
		if (cnt < 255) cnt++;
	}
	else if (cnt) {
		evqueue_write(EV_WEIGHT_STOP, weight_in_grams(val), cnt);
		cnt = 0;
	}
	previous = val;
	gpio_clear_irq(GPIO_HX_DATA);
}

#endif

void hx711_init()
{
	gpio_mode(GPIO_HX_DATA, GPIO_MODE_INPUT);
	gpio_mode(GPIO_HX_CLK, GPIO_MODE_OUTPUT);

	power_down();
	power_up();

    // set interrupt for data pin
#ifdef CALIBRATION
	gpio_attach_interrupt(GPIO_HX_DATA, ScaleCalibIntReq, GPIO_IRQMODE_FALLING_EDGE);
#else
	// read offset, factor & orientation from settings
	SCALE_OFFSET = settings_get_int("scaleoffset", SCALE_OFFSET);
	SCALE_FACTOR = settings_get_int("scalefactor", SCALE_FACTOR);
	SCALE_ORIENTATION = settings_get_int("scaleorientation", SCALE_ORIENTATION);
	WEIGHT_THRESHOLD = settings_get_int("scalethreshold", WEIGHT_THRESHOLD);
	UART_PRINT("read hx711 settings:\n\rscaleoffset=%d\n\rscalefactor=%d\n\rscaleorientation=%d\n\rscalethreshold=%d\n\r", SCALE_OFFSET, SCALE_FACTOR, SCALE_ORIENTATION, WEIGHT_THRESHOLD);

	gpio_attach_interrupt(GPIO_HX_DATA, ScaleIntReq, GPIO_IRQMODE_FALLING_EDGE);
#endif
}
