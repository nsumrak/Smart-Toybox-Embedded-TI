// Smart Toybox HTTP client/server
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


#define MAX_CONN	4

struct ConnDesc;
typedef void (*processfn_t)(struct ConnDesc*,char*,int);

typedef struct ConnDesc {
	_i16 sock;
	_u8 incomming:1;
	_u8 phase:1;
	_u8 active:1;
	char name;
	_i32 fd; // write file desc or response code in client
	_i32 woffs;
	_i32 contlen;
	processfn_t fn;
} ConnDesc;


void http_process();
void http_close(ConnDesc *);
void http_deinit_server();
int http_init_server(unsigned short port);
int http_connect(_u32 addr, const char *method, const char *host, const char *object, char *optbuf, int buflen, processfn_t fn);
_u32 http_resolve_host(const char *host);
