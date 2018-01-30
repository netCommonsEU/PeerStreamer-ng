/*******************************************************************
* PeerStreamer-ng is a P2P video streaming application exposing a ReST
* interface.
* Copyright (C) 2017 Luca Baldesi <luca.baldesi@unitn.it>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************/

#ifndef __PSCHANNEL_H__
#define __PSCHANNEL_H__

#include<stdint.h>
#include<name_lengths.h>
#include<pstreamer.h>

struct pschannel {
	char name[MAX_NAME_LENGTH];
	char ipaddr[MAX_IPADDR_LENGTH];
	char port[MAX_PORT_LENGTH];
	char quality[MAX_QUALITY_LENGTH];
	char sdpfile[MAX_SDPFILENAME_LENGTH];
};

struct pschannel_bucket;
struct pstreamer_manager;

struct pschannel_bucket * pschannel_bucket_new(const char * csvfilename, const struct pstreamer_manager * psm);

uint8_t pschannel_bucket_insert(struct pschannel_bucket * pb, const char * name, const char * ip, const char * port, const char * quality, const char * sdpfile);

const struct pschannel * pschannel_bucket_iter(const struct pschannel_bucket * pb, const struct pschannel * iter);

void pschannel_bucket_destroy(struct pschannel_bucket ** pb);

char * pschannel_bucket_to_json(const struct pschannel_bucket * pb);

const struct pschannel * pschannel_bucket_find(const struct pschannel_bucket * psb, const char * ipaddr, const char * port);

int8_t pschannel_bucket_loadfile(struct pschannel_bucket * psb);

int8_t pschannel_bucket_load_local_streams(struct pschannel_bucket * pb);

int8_t pschannel_bucket_reset(struct pschannel_bucket * psb);

int8_t pschannel_bucket_save2file(struct pschannel_bucket * psb);

#endif
