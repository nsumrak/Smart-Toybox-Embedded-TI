// Smart Toybox HTTP client/server (file) implementation
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
#include <stdio.h>
#include "simplelink.h"
#include "debug.h"
#include "http.h"
#include "evqueue.h"
#include "STBreader.h"
#include "files.h"


_i16 listen_sock = -1;
static ConnDesc g_conn[MAX_CONN] = { 0 };


// initialise web server
int http_init_server(unsigned short port)
{
	SlSockAddrIn_t saddr;
	SlSockNonblocking_t nb;
	_i16 sock;

	if(listen_sock >= 0) {
		sl_Close(listen_sock);
		listen_sock = -1;
	}

	// filling the TCP server socket address
	saddr.sin_family = SL_AF_INET;
	saddr.sin_port = sl_Htons(port);
	saddr.sin_addr.s_addr = 0;

	sock = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
	if(sock<0) {
		UART_PRINT("server sock err\r\n");
		return -1;
	}

	// binding the TCP socket to the TCP server address
	if(sl_Bind(sock, (SlSockAddr_t *)&saddr, sizeof(saddr)) < 0)
	{
		// error
		sl_Close(sock);
		UART_PRINT("bind error\r\n");
		return -2;
	}

	// putting the socket for listening to the incoming TCP connection
	if(sl_Listen(sock, 0) < 0)
	{
		sl_Close(sock);
		UART_PRINT("Listen failed\r\n");
		return -3;
	}

	// setting socket option to make the socket as non blocking
	nb.NonblockingEnabled = 1;
	if(sl_SetSockOpt(sock, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &nb, sizeof(nb)) < 0)
	{
		sl_Close(sock);
		UART_PRINT("set sock opt non block fail\r\n");
		return -4;
	}

	listen_sock = sock;
	return 0;
}


void http_deinit_server()
{
	int i;
	if(listen_sock == -1) return;
	sl_Close(listen_sock);
	listen_sock = -1;
	for(i=0; i<MAX_CONN; i++)
		if(g_conn[i].active) http_close(&g_conn[i]);
}

// adds sock to connection pool so process could send/receive
static ConnDesc *addConnection(_i16 sock, int incomming)
{
	int i;
	for(i=0; i<MAX_CONN; i++)
		if(!g_conn[i].active) {
			g_conn[i].sock = sock;
			g_conn[i].incomming = incomming;
			g_conn[i].phase = 0;
			g_conn[i].active = 1;
			g_conn[i].fd = -1;
			g_conn[i].name = 0;
			g_conn[i].fn = 0;
			UART_PRINT("add connection sock=%d, i=%d\r\n",sock, i);
			return &g_conn[i];
		}

	UART_PRINT("addconn: No free connections\r\n");
	sl_Close(sock);
	return 0;
}


static void sendErrorResponse(ConnDesc *cd, int code)
{
	static struct {
		_u16 code;
		const char *text;
	} responses[] = {
			{ 200, "OK" },
			{ 404, "Not Found" },
			{ 400, "Bad Request" },
			{ 500, "Error" },
			{ 501, "Not Implemented" },
			{ 507, "Insufficient Storage" },
			{ 0, 0 }
	};
	char buf[48];
	const char *str = "";
	int len;

	for(len=0; responses[len].text; len++)
		if(responses[len].code == code) {
			str = responses[len].text;
			break;
		}

	len = sprintf(buf, "HTTP/1.0 %d %s\r\n\r\n", code, str);
	sl_Send(cd->sock, buf, len, 0);
	UART_PRINT("error response %d (%s)\r\n", code, str);
	http_close(cd);
}


// process http headers and incomming strings
static void http_incomming_process(ConnDesc *cd, char *buf, int len)
{
	UART_PRINT("incomming on sock %d, len %d\r\n",cd->sock,len);
	if(cd->phase) {
		UART_PRINT("write file len %d\r\n", len);
		int res = sl_FsWrite(cd->fd, cd->woffs, (_u8*)buf, len);
		if(res < len) {
			sendErrorResponse(cd, 507);
			return;
		}
		cd->woffs += res;
		if(cd->woffs >= cd->contlen) {
			UART_PRINT("should hang out\r\n");
			evqueue_write(EV_NEW_HTTP_FILE, cd->name, 0);
			sendErrorResponse(cd, 200); // not error
		}
	} else {
		int contentlength = 0;
		char status = 0;
		// server connection
		char isget = !strncmp(buf,"GET",3);
		char ispost = !strncmp(buf,"POST",4);

		if(isget) {
			if(strncmp(buf+4,"/ ",2)) {
				// not found
				sendErrorResponse(cd, 404);
				return;
			}
		} else if(ispost) { // not get request and not post
			if(strncmp(buf+5, "/snd/", 5) || *(buf+11)!=' ') {
				// not found
				sendErrorResponse(cd, 400);
				return;
			}
			cd->name = *(buf+10);
		} else {
			// bad method
			sendErrorResponse(cd, 501);
			return;
		}

		while(len) {
			if(*buf == '\r' && (!status || status==2)) status++;
			else if(*buf == '\n') {
				if(status == 3) {
					// end of header
					if(isget) {
						const char *str = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-length: 44\r\n\r\n<html><body><h1>It works!</h1></body></html>";
						sl_Send(cd->sock, str, strlen(str), 0);
						http_close(cd);
						return;
					} else {
						// incomming file
						_u8 fn[2] = { cd->name, 0 };
						UART_PRINT("file to be created name='%s' size=%d\r\n",fn,contentlength);
						buf++; len--;
						short err = 0;
						cd->woffs = 0;
						if (*fn == 't') {
							stb_close(THEME_FILE);
							err = sl_FsDel(THEME_FILE, 0);
						} else {
							err = sl_FsDel(fn, 0);
						}
						if (err) UART_PRINT("file delete error: %d\n\r", err);
						if(!contentlength || (err = sl_FsOpen((const _u8*)(*fn == 't' ? (_u8*)THEME_FILE : (_u8*)fn), FS_MODE_OPEN_CREATE(contentlength, _FS_FILE_OPEN_FLAG_NO_SIGNATURE_TEST), 0, &(cd->fd)))
										  || (len && (cd->woffs = sl_FsWrite(cd->fd, 0, (_u8*)buf, len)) < len)) {
							UART_PRINT("file fail! err=%d fd=%d, wrlen=%d len=%d\r\n",err,cd->fd,cd->woffs, len);
							// fail, send no storage
							sendErrorResponse(cd, 507);
							return;
						}
						cd->contlen = contentlength;
						cd->phase = 1;
						return;
					}
				} else if(status == 1) {
					// new header
					++status;
					if(!strncmp(buf+1, "Content-Length:", 15)) contentlength = atoi(buf+16);
				} else status = 0;
			}
			else status = 0;
			buf++;
			len--;
		}
		// error
		sendErrorResponse(cd, 500);
	}
}


// process http headers and received strings
static void http_client_process(ConnDesc *cd, char *buf, int len)
{
	int status = 0;

	if(cd->phase) {
		if(cd->fn) {
			cd->fn(cd,buf,len);
			cd->woffs += len;
			if(cd->woffs >= cd->contlen) http_close(cd);
		} else http_close(cd);
		return;
	}

	// skip http/1.1
	while(*buf != ' ') { buf++; len--; }
	// get response code
	cd->fd = atoi(buf);

	while(len) {
		if(*buf == '\r' && (!status || status==2)) status++;
		else if(*buf == '\n') {
			if(status == 3) {
				// end of header
				buf++; len--;
				cd->phase = 1;
				cd->woffs = 0;
				// now it is response content buf, len
				if(cd->fn) {
					cd->fn(cd, buf, len);
					cd->woffs = len;
					return;
				}
				break;
			} else if(status == 1) {
				// new header
				++status;
				if(!strncmp(buf+1, "Content-Length:", 15)) cd->contlen = atoi(buf+16);
				// Transfer-Encoding
				// Location
				// Content-Transfer-Encoding
			} else status = 0;
		}
		else status = 0;
		buf++;
		len--;
	}
	// end of message
	http_close(cd);
}


// process connections, called in main lopp to send/receive/accept connections
void http_process()
{
	int i;

	if(listen_sock >= 0) {
		// accept
		SlSockAddrIn_t saddr;
		SlSocklen_t saddrlen = sizeof(saddr);

		_i16 sock = sl_Accept(listen_sock, (struct SlSockAddr_t *)&saddr, (SlSocklen_t*)&saddrlen);
		if(sock != SL_EAGAIN && sock < 0)
			UART_PRINT("accept failed\n\r");
		else if(sock >= 0)
			addConnection(sock, 1);
	}

	for(i=0; i<MAX_CONN; i++)
		if(g_conn[i].active) {
			// recv
			char buf[513];
			int res = sl_Recv(g_conn[i].sock, buf, sizeof(buf)-1, 0);
			if(res == SL_EAGAIN) continue;
			UART_PRINT("recv not again i=%d, sock=%d res=%d\r\n",i, g_conn[i].sock,res);
			if(res < 0) {
				UART_PRINT("sock recv err\r\n");
				http_close(&g_conn[i]);
				continue;
			}
			if(!res) {
				UART_PRINT("sock closed by remote\r\n");
				http_close(&g_conn[i]);
				continue;
			}
			buf[res] = 0;
			if(g_conn[i].incomming) http_incomming_process(&g_conn[i], buf, res);
			else http_client_process(&g_conn[i], buf, res);
		}
}


void http_close(ConnDesc *cd)
{
	if(!cd->active) return;
	sl_Close(cd->sock);
	if(cd->fd >= 0) {
		sl_FsClose(cd->fd, 0, 0, 0);
		cd->fd = -1;
	}
	cd->active = 0;
}


// connect to host with address using http method requesting object and sending optional data
int http_connect(_u32 addr, const char *method, const char *host, const char *object, char *optbuf, int buflen, processfn_t fn)
{
	SlSockAddrIn_t saddr;
	SlSockNonblocking_t nb;
	_i16 sock;
	_i16 ret;

	sock = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
	if(sock<0) {
		UART_PRINT("client sock err\r\n");
		return -1;
	}

	saddr.sin_family = SL_AF_INET;
	saddr.sin_port = sl_Htons(80);
	saddr.sin_addr.s_addr = addr;

	ret = sl_Connect(sock, (struct sockaddr *)&saddr, sizeof(saddr));
	if(ret == SL_EALREADY) {
		UART_PRINT("got EALREADY on connect\r\n");
	}
	else if(ret < 0) {
		UART_PRINT("connect error %d\r\n",ret);
		sl_Close(sock);
		return -2;
	}
	UART_PRINT("connect got sock %d from sock %d\r\n",ret,sock);

	// setting socket option to make the socket as non blocking
	nb.NonblockingEnabled = 1;
	if(sl_SetSockOpt(sock, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &nb, sizeof(nb)) < 0)
	{
		sl_Close(sock);
		UART_PRINT("set sock opt non block fail\r\n");
		return -4;
	}

	{
		ConnDesc *cd = addConnection(sock, 0);
		if(!cd) return -5; // already closed by addconnection
		cd->fn = fn;

		char buf[512];
		int sz = sprintf(buf,"%s %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: SmartToybox/1.0\r\n", method, object, host);
		if(buflen) {
			sz += sprintf(buf+sz, "Content-Length: %d\r\nContent-Type: application/x-octet-stream\r\n\r\n", buflen);
			memcpy(buf+sz, optbuf, buflen);
			sz += buflen;
		} else {
			strcpy(buf+sz,"\r\n");
			sz += 2;
		}
		ret = sl_Send(sock, buf, sz, 0);
		if(ret < 0) {
			UART_PRINT("client send failed err=%d\r\n",ret);
			http_close(cd);
			return -6;
		}
	}
	// now wait for response
	return 0;
}


// do dns lookup and return ipv4 address
_u32 http_resolve_host(const char *host)
{
	_u32 addr;
	int ret = sl_NetAppDnsGetHostByName((_i8*)host, strlen(host), &addr, SL_AF_INET);
	if (ret < 0) {
		UART_PRINT("host resolution failed err=%d\r\n",ret);
		return 0;
	}
	return sl_Htonl(addr);
}
