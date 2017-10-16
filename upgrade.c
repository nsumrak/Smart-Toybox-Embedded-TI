// upgrade interface
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

#include "simplelink.h"
#include "http.h"
#include "evqueue.h"
#include "debug.h"
#include "led.h"
#include "flc.h"
#include "upgrade.h"
#include "power.h"

#define FWUPGRADE_URL "/fw.bin"
#define STB_HOST "smarttoybox.com"

BOOL upgrading = 0;
static _i32 fh = -1;
static int alen = 0;

// bootloader: /sys/mcuimg
// factory default: /sys/mcuimg1
// user1: /sys/mcuimg2
// user2: /sys/mcuimg3
static char img_path[] = "/sys/mcuimgA.bin"; // #define FWIMG_TMP_PATH "/tmp/newimg.bin"
static sBootInfo_t bi;

// copied from simplelink_ext/flc/flc.c
static _i32 _WriteBootInfo(sBootInfo_t *psBootInfo)
{
    _i32 lFileHandle;
    _u32 ulToken;
    _i32 status = -1;

    if( 0 == sl_FsOpen((_u8 *)IMG_BOOT_INFO, FS_MODE_OPEN_WRITE, &ulToken, &lFileHandle) )
    {
        if( 0 < sl_FsWrite(lFileHandle, 0, (_u8 *)psBootInfo, sizeof(sBootInfo_t)) )
        {
        	UART_PRINT("WriteBootInfo: ucActiveImg=%d, ulImgStatus=0x%x\n\r", psBootInfo->ucActiveImg, psBootInfo->ulImgStatus);
            status = 0;
        }
        sl_FsClose(lFileHandle, 0, 0, 0);
    }

    return status;
}
static _i32 _ReadBootInfo(sBootInfo_t *psBootInfo)
{
    _i32 lFileHandle;
    _u32 ulToken;
    _i32 status = -1;

    if( 0 == sl_FsOpen((_u8 *)IMG_BOOT_INFO, FS_MODE_OPEN_READ, &ulToken, &lFileHandle) )
    {
        if( 0 < sl_FsRead(lFileHandle, 0, (_u8 *)psBootInfo, sizeof(sBootInfo_t)) )
        {
            status = 0;
            UART_PRINT("ReadBootInfo: ucActiveImg=%d, ulImgStatus=0x%x\n\r", psBootInfo->ucActiveImg, psBootInfo->ulImgStatus);
        }
        sl_FsClose(lFileHandle, 0, 0, 0);
    }

    return status;
}
//

static void on_upgrade_error(struct ConnDesc *cd)
{
	if (cd) http_close(cd);
	if (fh != -1) sl_FsClose(fh, 0, 0, 0);
	fh = -1;
	alen = 0;
	upgrading = 0;
	led_blink_cnt(LED_RED, 4);
	evqueue_write(EV_FWUPGRADE_FINISHED, 0, 0);
}

static void upgrade_cb(struct ConnDesc* cd, char* buf, int len)
{
	UART_PRINT("upgrade_cb, resp code = %d, upgrading = %d\n\r", cd->fd, upgrading);
	if (!upgrading) return;

	if (fh == -1) {
		if (cd->fd != 200 || cd->contlen <= 0) {
			UART_PRINT("fw upgrade: first packet problem, resp code = %d, contlen = %d\n\r", img_path, cd->fd, cd->contlen);
			on_upgrade_error(cd);
			return;
		}
		sl_FsDel((const _u8 *)img_path, 0);
		if(sl_FsOpen((const _u8 *)img_path, FS_MODE_OPEN_CREATE(cd->contlen, 0), 0, &fh)) {
			UART_PRINT("opening fw img file %s for upgrade failed!\n\r", img_path);
			on_upgrade_error(cd);
			return;
		}
	}

	int res = sl_FsWrite(fh, cd->woffs, (_u8*)buf, len);
	if(res < len) {
		UART_PRINT("writing to fw img file %s failed!\n\r", img_path);
		on_upgrade_error(cd);
		return;
	}

	alen += len;
	UART_PRINT("upgrade_cb, len = %d, resp code = %d\n\r", alen, cd->fd);

	if (alen == cd->contlen) {
		UART_PRINT("DONE!!!\n\r");
		sl_FsClose(fh, 0, 0, 0);
		leds_off();
		fh = -1;
		upgrading = 0;
	    bi.ulImgStatus = IMG_STATUS_NOTEST;
		_WriteBootInfo(&bi);
		// delete old image
		if (bi.ucActiveImg != IMG_ACT_FACTORY) {
			img_path[11] =  (bi.ucActiveImg == IMG_ACT_USER1 ? IMG_ACT_USER2 : IMG_ACT_USER1) + '1';
			sl_FsDel((const _u8 *)img_path, 0);
		}
		evqueue_write(EV_FWUPGRADE_FINISHED, 1, 0);
	}
}

void upgrade()
{
	if (upgrading) {
		UART_PRINT("upgrade already in progress\n\r");
		return;
	}

	UART_PRINT("Upgrading software...\n\r");

	upgrading = 1;
	alen = 0;
	fh = -1;

	upgrade_set_led();

	// find no. of currently active image; default is factory default
	// (boot info must always exist, since bootloader creates it)
	if (_ReadBootInfo(&bi)) {
		UART_PRINT("read boot info failed\n\r");
		on_upgrade_error(0);
		return;
	}
	// if active image was mcuimg2 (user1), new image should be mcuimg3 (user2), and vice versa;
	// if active image was mcuimg1 (factory default), new should be mcuimg2 (although it could also be mcuimg3)
	bi.ucActiveImg = bi.ucActiveImg == IMG_ACT_USER1 ? IMG_ACT_USER2 : IMG_ACT_USER1;
	img_path[11] =  bi.ucActiveImg + '1';
	UART_PRINT("new image is %s\n\r", img_path);

	_u32 addr = http_resolve_host(STB_HOST);
	if (!addr || http_connect(addr, "GET", STB_HOST, FWUPGRADE_URL, 0, 0, upgrade_cb) < 0) {
		UART_PRINT("http connect to %d.%d.%d.%d failed\n\r", addr & 0x000000ff, (addr >> 8) & 0x000000ff, (addr >> 16) & 0x000000ff, (addr >> 24) & 0x000000ff);
		on_upgrade_error(0);
	}
}

void restore_to_factory_img()
{
	if (upgrading) {
		UART_PRINT("upgrade already in progress\n\r");
		return;
	}

	bi.ucActiveImg = IMG_ACT_FACTORY;
    bi.ulImgStatus = IMG_STATUS_NOTEST;
	_WriteBootInfo(&bi);
	reboot();
}
