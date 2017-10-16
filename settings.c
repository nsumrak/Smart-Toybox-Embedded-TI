// Settings INI file reader
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

#include <stdio.h>
#include <ctype.h>
#include "platform.h"
#include "common.h"
#include "settings.h"

BOOL settings_get(const char *iname, char *value, int valsize)
{
	fs_file fh;
	int offset, readed, inname;
	int state=0, navodnik=0;
	char buffek[256], name[32];
//	char section[32];

	if((fh=fs_open(SETTINGS_FILE, FS_OPEN_MODE_READ)) == -1) return FALSE;
//	section[0]=0;
	valsize--; // trailing zero

	for(offset = 0; state!=6 && (readed = fs_read(fh,offset,buffek,sizeof(buffek)-1)) > 0; offset += readed) {
		char *t = buffek, *e = buffek+readed;
		while(state!=6 && t<e) {
			switch(state) {

			case 0: // new line beginning
				if(isspace(*t)) break;
				state = 1; // drop through

			case 1: // beginning of string
//				if(*t=='[') { // section
//					state = ;
//					break;
//				} else
				if(*t==';') { // comment - skip till EOL
					navodnik = 0;
					state = 5;
					break;
				}
				inname = 0;
				state = 2; // drop through

			case 2: // in name of name/value
				if(inname == sizeof(name)-1) break;
				if(isalnum(*t)) {
					name[inname++] = *t;
					break;
				} else {
					name[inname] = 0;
					state = 3; // drop through
				}

			case 3: // between name and value
				if(*t == '\n' || *t == '\r') {
					if(!strncmp(name, iname, sizeof(name)-1)) {
						*(value++) = '1';  // boolean
						valsize--;
						state = 6;
						continue;
					} else {
						state = 0;
						break;
					}
				}
				if(*t != ' ' && *t != '\t' && *t != '=' && *t != ':') {
					state = (!strncmp(name, iname, sizeof(name)-1)) ? 4 : 5;
					if((navodnik = (*t=='"'))) break;
					continue;
				}
				break;

			case 4: // in value of name/value - copying
				if(navodnik) {
					if(*t == '"') {
						state = 6;
						continue;
					}
				} else if(*t == '\n' || *t == '\r') {
					state = 6;
					continue;
				}
				if(!valsize) break;
				*(value++) = *t;
				valsize--;
				break;

			case 5: // in value of name/value - skipping
				if(navodnik) {
					if(*t == '"') {
						navodnik = 0; // after closed quote skip till EOL
						break;
					}
				} else if(*t == '\n' || *t == '\r') {
					state = 0;
					continue;
				}
				break;

			case 6: // found name/value
				continue;
			}
			t++;
		}
	}
	fs_close(fh);
	if(state==4 || state==6) {
		*value=0;
		return TRUE;
	}
	return FALSE;
}
