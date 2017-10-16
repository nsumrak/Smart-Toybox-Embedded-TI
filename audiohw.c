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

#include "platform.h"
#include <string.h>

#include "audiohw.h"
#include "audioplay.h"
#include "circ_buff.h"
#include "debug.h"


//
// Audio codec definitions:
//

// Define 8-bit codec id to identify codec device
#define AUDIO_CODEC_TI_3254		0

// Audio out Macro
// Note: Max 8 line out per codec device supported
#define AUDIO_CODEC_SPEAKER_NONE		(0x00)
#define AUDIO_CODEC_SPEAKER_HP			(0x01)		// Headphone line
#define AUDIO_CODEC_SPEAKER_LO			(0x02)		// Line out
#define AUDIO_CODEC_SPEAKER_RESERVED1	(0x04)		// Onboard mic device
#define AUDIO_CODEC_SPEAKER_RESERVED2	(0x08)		// Onboard mic device
#define AUDIO_CODEC_SPEAKER_RESERVED3	(0x10)		// Onboard mic device
#define AUDIO_CODEC_SPEAKER_RESERVED4	(0x20)		// Onboard mic device
#define AUDIO_CODEC_SPEAKER_RESERVED5	(0x40)		// Onboard mic device
#define AUDIO_CODEC_SPEAKER_RESERVED6	(0x80)		// Onboard mic device
#define AUDIO_CODEC_SPEAKER_ALL			(0xFF)

// TI3254 codec defs:
#define CODEC_I2C_SLAVE_ADDR      ((0x30 >> 1))

#define TI3254_PAGE_0					0x00
#define TI3254_PAGE_1					0x01
#define TI3254_PAGE_8					0x08
#define TI3254_PAGE_44					0x2C

#define PAGE_CTRL_REG					0x00

// Page 0
#define TI3254_PAGE_SEL_REG				0x00
#define TI3254_SW_RESET_REG				0x01
#define TI3254_CLK_MUX_REG				0x04
#define TI3254_CLK_PLL_P_R_REG			0x05
#define TI3254_CLK_PLL_J_REG			0x06
#define TI3254_CLK_PLL_D_MSB_REG		0x07
#define TI3254_CLK_PLL_D_LSB_REG		0x08
#define TI3254_CLK_NDAC_REG				0x0B
#define TI3254_CLK_MDAC_REG				0x0C
#define TI3254_DAC_OSR_MSB_REG			0x0D
#define TI3254_DAC_OSR_LSB_REG			0x0E
#define TI3254_DSP_D_CTRL_1_REG			0x0F
#define TI3254_DSP_D_CTRL_2_REG			0x10
#define TI3254_DSP_D_INTERPOL_REG		0x11
#define TI3254_CLK_NADC_REG				0x12
#define TI3254_CLK_MADC_REG				0x13
#define TI3254_ADC_OSR_REG				0x14
#define TI3254_DSP_A_CTRL_1_REG			0x15
#define TI3254_DSP_A_CTRL_2_REG			0x16
#define TI3254_DSP_A_DEC_FACT_REG		0x17
#define TI3254_AUDIO_IF_1_REG			0x1B

#define TI3254_DAC_SIG_P_BLK_CTRL_REG	0x3C
#define TI3254_ADC_SIG_P_BLK_CTRL_REG	0x3D

#define TI3254_DAC_CHANNEL_SETUP_1_REG	0x3F
#define TI3254_DAC_CHANNEL_SETUP_2_REG	0x40

#define TI3254_LEFT_DAC_VOL_CTRL_REG	0x41
#define TI3254_RIGHT_DAC_VOL_CTRL_REG	0x42

#define TI3254_ADC_CHANNEL_SETUP_REG	0x51
#define TI3254_ADC_FINE_GAIN_ADJ_REG	0x52

#define TI3254_LEFT_ADC_VOL_CTRL_REG	0x53
#define TI3254_RIGHT_ADC_VOL_CTRL_REG	0x54

//Page 1

#define TI3254_PWR_CTRL_REG				0x01
#define TI3254_LDO_CTRL_REG				0x02
#define TI3254_OP_DRV_PWR_CTRL_REG		0x09
#define TI3254_HPL_ROUTING_SEL_REG		0x0C
#define TI3254_HPR_ROUTING_SEL_REG		0x0D
#define TI3254_LOL_ROUTING_SEL_REG		0x0E
#define TI3254_LOR_ROUTING_SEL_REG		0x0F
#define TI3254_HPL_DRV_GAIN_CTRL_REG	0x10
#define TI3254_HPR_DRV_GAIN_CTRL_REG	0x11
#define TI3254_LOL_DRV_GAIN_CTRL_REG	0x12
#define TI3254_LOR_DRV_GAIN_CTRL_REG	0x13
#define TI3254_HP_DRV_START_UP_CTRL_REG	0x14

#define TI3254_MICBIAS_CTRL_REG			0x33
#define TI3254_LEFT_MICPGA_P_CTRL_REG	0x34
#define TI3254_LEFT_MICPGA_N_CTRL_REG	0x36
#define TI3254_RIGHT_MICPGA_P_CTRL_REG	0x37
#define TI3254_RIGHT_MICPGA_N_CTRL_REG	0x39
#define TI3254_FLOAT_IP_CTRL_REG		0x3a
#define TI3254_LEFT_MICPGA_VOL_CTRL_REG	0x3B
#define TI3254_RIGHT_MICPGA_VOL_CTRL_REG 0x3C

#define TI3254_ANALOG_IP_QCHRG_CTRL_REG	0x47
#define TI3254_REF_PWR_UP_CTRL_REG		0x7B

// Page 8
#define TI3254_ADC_ADP_FILTER_CTRL_REG	0x01

// Page 44
#define TI3254_DAC_ADP_FILTER_CTRL_REG	0x01

// end of TI3254 codec defs



inline bool AudioCodecRegWrite(unsigned char reg,unsigned char value)
{
	return i2c_write(CODEC_I2C_SLAVE_ADDR, reg, value);
}

inline unsigned long AudioCodecPageSelect(unsigned char pageno)
{
    return AudioCodecRegWrite(TI3254_PAGE_SEL_REG,pageno);
}


// Configure volume level for audio out on a codec device
// volumeLevel 	-  Volume level. 0-100

unsigned char g_masterVolume = 100, g_chVolume[2] = { 90, 90 };

void audiohw_set_channel_volume(unsigned char channel, unsigned char volumeLevel)
{
    unsigned vol = volumeLevel;
    g_chVolume[channel] = volumeLevel;
    vol *= g_masterVolume;
    vol /= 100;

    if(vol < 4) vol = 128;
    else if (vol > 97) vol = 316;
    else {
    	vol <<= 1;
    	vol += 122;
    }

    UART_PRINT("Volume: %d reg: %d %d\n\r", volumeLevel, vol, (unsigned char )(vol&0x00FF));

    AudioCodecPageSelect(TI3254_PAGE_0);
    AudioCodecRegWrite(channel ? TI3254_RIGHT_DAC_VOL_CTRL_REG : TI3254_LEFT_DAC_VOL_CTRL_REG, (unsigned char )(vol&0x00FF));
}

void audiohw_set_master_volume(unsigned char volumeLevel)
{
	if(g_masterVolume == volumeLevel) return;
	g_masterVolume = volumeLevel;
	audiohw_set_channel_volume(0, g_chVolume[0]);
	audiohw_set_channel_volume(1, g_chVolume[1]);
}

// Mute Audio line out
void audiohw_speaker_mute(BOOL mute)
{
    AudioCodecPageSelect(TI3254_PAGE_0);

    //Mute/unmute LDAC/RDAC
    AudioCodecRegWrite(TI3254_DAC_CHANNEL_SETUP_2_REG, mute ? 0x0C : 0x00);	// Left and Right DAC Channel muted/unmuted
}

// Configure audio codec for smaple rate, bits and number of channels
static void AudioCodecConfig(unsigned char speaker)
{
	AudioCodecPageSelect(TI3254_PAGE_0);

	// Set I2S Mode and Word Length
	AudioCodecRegWrite(TI3254_AUDIO_IF_1_REG, 0x00); 	// 0x00 	16bit, I2S, BCLK is input to the device

	AudioCodecPageSelect(TI3254_PAGE_0);

	AudioCodecRegWrite(TI3254_CLK_MUX_REG, 0x03);		// PLL Clock is CODEC_CLKIN
	AudioCodecRegWrite(TI3254_CLK_PLL_P_R_REG, 0x94);	// PLL is powered up, P=1, R=4
	AudioCodecRegWrite(TI3254_CLK_PLL_J_REG, 0x2A);		// J=42
	AudioCodecRegWrite(TI3254_CLK_PLL_D_MSB_REG, 0x00);	// D = 0

	AudioCodecRegWrite(TI3254_CLK_NDAC_REG, 0x8E);		// NDAC divider powered up, NDAC = 14
	AudioCodecRegWrite(TI3254_CLK_MDAC_REG, 0x81);		// MDAC divider powered up, MDAC = 1
	AudioCodecRegWrite(TI3254_DAC_OSR_MSB_REG, 0x01);	// DOSR = 0x0180 = 384
	AudioCodecRegWrite(TI3254_DAC_OSR_LSB_REG, 0x80);	// DOSR = 0x0180 = 384

	// Configure Power Supplies
	AudioCodecPageSelect(TI3254_PAGE_1);		//Select Page 1

	AudioCodecRegWrite(TI3254_PWR_CTRL_REG, 0x08);	// Disabled weak connection of AVDD with DVDD
	AudioCodecRegWrite(TI3254_LDO_CTRL_REG, 0x01);	// Over Current detected for AVDD LDO
	AudioCodecRegWrite(TI3254_ANALOG_IP_QCHRG_CTRL_REG, 0x32); // Analog inputs power up time is 6.4 ms
	AudioCodecRegWrite(TI3254_REF_PWR_UP_CTRL_REG, 0x01);	// Reference will power up in 40ms when analog blocks are powered up

	unsigned char reg1;

	AudioCodecPageSelect(TI3254_PAGE_0);	//Select Page 0

	// ##Configure Processing Blocks
	AudioCodecRegWrite(TI3254_DAC_SIG_P_BLK_CTRL_REG, 0x2);  // DAC Signal Processing Block PRB_P2

	AudioCodecPageSelect(TI3254_PAGE_44);	// Select Page 44

	AudioCodecRegWrite(TI3254_DAC_ADP_FILTER_CTRL_REG, 0x04);   // Adaptive Filtering enabled for DAC

	AudioCodecPageSelect(TI3254_PAGE_1);	// Select Page 1

	reg1 = 0x00;

	if (speaker & AUDIO_CODEC_SPEAKER_HP) {
		//De-pop: 5 time constants, 6k resistance
		AudioCodecRegWrite(TI3254_HP_DRV_START_UP_CTRL_REG, 0x25);	// Headphone ramps power up time is determined with 6k resistance,
																	// Headphone ramps power up slowly in 5.0 time constants

		//Route LDAC/RDAC to HPL/HPR
		AudioCodecRegWrite(TI3254_HPL_ROUTING_SEL_REG, 0x08);// Left Channel DAC reconstruction filter's positive terminal is routed to HPL
		AudioCodecRegWrite(TI3254_HPR_ROUTING_SEL_REG, 0x08);// Left Channel DAC reconstruction filter's negative terminal is routed to HPR

		reg1 |= 0x30;	// HPL and HPR is powered up
	}

	if (speaker & AUDIO_CODEC_SPEAKER_LO) {
		//Route LDAC/RDAC to LOL/LOR
		AudioCodecRegWrite(TI3254_LOL_ROUTING_SEL_REG, 0x08);	// Left Channel DAC reconstruction filter output is routed to LOL
		AudioCodecRegWrite(TI3254_LOR_ROUTING_SEL_REG, 0x08);	// Right Channel DAC reconstruction filter output is routed to LOR

		reg1 |= 0x0C;	// LOL and LOR is powered up
	}

	//Power up HPL/HPR and LOL/LOR drivers
	AudioCodecRegWrite(TI3254_OP_DRV_PWR_CTRL_REG, reg1);

	if (speaker & AUDIO_CODEC_SPEAKER_HP) {
		//Unmute HPL/HPR driver, 0dB Gain
		AudioCodecRegWrite(TI3254_HPL_DRV_GAIN_CTRL_REG, 0x00);		// HPL driver is not muted, HPL driver gain is 0dB
		AudioCodecRegWrite(TI3254_HPR_DRV_GAIN_CTRL_REG, 0x00);		// HPR driver is not muted, HPL driver gain is 0dB
	}

	if (speaker & AUDIO_CODEC_SPEAKER_LO) {
		//Unmute LOL/LOR driver (bit d6 = 0), 0dB Gain
		AudioCodecRegWrite(TI3254_LOL_DRV_GAIN_CTRL_REG, AUDIO_SPEAKER_GAIN);	// LOL driver gain is 0dB (bits d0-d5 in 2's complement)
		AudioCodecRegWrite(TI3254_LOR_DRV_GAIN_CTRL_REG, AUDIO_SPEAKER_GAIN);	// LOL driver gain is 0dB (bits d0-d5 in 2's complement)
	}

	AudioCodecPageSelect(TI3254_PAGE_0);		//Select Page 0

	//DAC => 64dB
	AudioCodecRegWrite(TI3254_LEFT_DAC_VOL_CTRL_REG, 0x2E);	// 2E = 90 as set by defaul on channel
	AudioCodecRegWrite(TI3254_RIGHT_DAC_VOL_CTRL_REG, 0x2E);// 2E = 90 as set by defaul on channel

	AudioCodecPageSelect(TI3254_PAGE_0);		//Select Page 0

#ifdef AUDIO_HARDWARE_MONO
	//Power up LDAC/RDAC
	AudioCodecRegWrite(TI3254_DAC_CHANNEL_SETUP_1_REG, 0xFE);	// Left and Right DAC Channel Powered Up
	// Left DAC data is mix of Left and Right Channel Audio Interface Data
	// Right DAC data is disabled
	// Soft-Stepping is disabled

#else
	//Power up LDAC/RDAC
	AudioCodecRegWrite(TI3254_DAC_CHANNEL_SETUP_1_REG, 0xD6);	// Left and Right DAC Channel Powered Up
	// Left DAC data Left Channel Audio Interface Data
	// Right DAC data is Left Channel Audio Interface Data
	// Soft-Stepping is disabled
#endif
	//Unmute LDAC/RDAC
	AudioCodecRegWrite(TI3254_DAC_CHANNEL_SETUP_2_REG, 0x00);	// When Right DAC Channel is powered down, the data is zero.
	// Auto Mute disabled
	// Left and Right DAC Channel not muted
	// Left and Right Channel have independent volume control
}


int audiohw_init()
{
	// Configure Audio Codec
	// rest audio codec
    AudioCodecPageSelect(TI3254_PAGE_0);
    AudioCodecRegWrite(TI3254_SW_RESET_REG, 0x01);
    delay_us(1000);

#ifdef DEBUG
    AudioCodecConfig(AUDIO_CODEC_SPEAKER_LO | AUDIO_CODEC_SPEAKER_HP);
#else
    AudioCodecConfig(AUDIO_CODEC_SPEAKER_LO);
#endif

    if(!circbuff_init()) return -1;
	i2s_init();
	return 0;
}
