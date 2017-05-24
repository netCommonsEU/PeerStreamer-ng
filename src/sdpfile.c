/*******************************************************************
* PeerStreamer-ng is a P2P video streaming application exposing a ReST
* interface.
* Copyright (C) 2015 Luca Baldesi <luca.baldesi@unitn.it>
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

#include<stdint.h>
#include<stdio.h>
#include<sdpfile.h>
#include<debug.h>
#include<tokens.h>


#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

struct pssdpfile {
	const struct pstreamer * ps;
	char path[MAX_PATH_LENGTH];
};

void sdpfile_dump(const char * filename, const char * sdpdesc, const struct pstreamer * ps)
{
	FILE * fp;
	char ** lines;
	char ** records;
	uint32_t n_lines, n_records, l, r, i;
	uint16_t audio_port, video_port;

	audio_port = pstreamer_base_port(ps);
	video_port = audio_port + 2;

	debug("Saving sdpfile to %s\n", filename);
	lines = tokens_create((char *) sdpdesc, '\n', &n_lines);

	fp = fopen(filename, "w");
	for(l = 0; l < n_lines; l++)
	{
		records = tokens_create(lines[l], ' ', &n_records);
		if ((r = tokens_check(records, n_records, "m=audio")) + 1 > 0)
		{
			for(i = 0; i < r+1; i++)
				fprintf(fp, "%s ", records[i]);
			fprintf(fp, "%d ", audio_port);
			for(i = r+2; i < n_records-1; i++)
				fprintf(fp, "%s ", records[i]);
			fprintf(fp, "%s\n", records[i]);
		}
		else if ((r = tokens_check(records, n_records, "m=video")) + 1 > 0)
		{
			for(i = 0; i < r+1; i++)
				fprintf(fp, "%s ", records[i]);
			fprintf(fp, "%d ", video_port);
			for(i = r+2; i < n_records-1; i++)
				fprintf(fp, "%s ", records[i]);
			fprintf(fp, "%s\n", records[i]);
		}
		else
			if (lines[l])
				fprintf(fp, "%s\n", lines[l]);
		tokens_destroy(&records, n_records);
	}
	fclose(fp);

	tokens_destroy(&lines, n_lines);
}

void sdpfile_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct http_message *hm = (struct http_message *) ev_data;
	struct pssdpfile * psdp;
	char * sdpdesc;

	psdp = nc->user_data;

	switch (ev) {
		case MG_EV_CONNECT:
			if (*(int *) ev_data != 0)
				debug("SDPFILE retrieval failure\n");
			break;
		case MG_EV_HTTP_REPLY:
			switch (hm->resp_code) {
				case 200:
					sdpdesc = malloc(sizeof(char) * (hm->body.len + 1));
					strncpy(sdpdesc, hm->body.p, hm->body.len);
					sdpdesc[hm->body.len] = '\0';  // make sure string terminates
					sdpfile_dump(psdp->path, sdpdesc, psdp->ps);
					free(sdpdesc);
				default:
					debug("SDPFILE server answers: %d\n", hm->resp_code);
			}
			free(psdp);
			break;
		case MG_EV_CLOSE:
			debug("SDPFILE server closed connection\n");
			break;
		default:
			break;
	}
}


char * sdpfile_create(const struct context * c, const struct pschannel * ch, const struct pstreamer * ps)
{
	struct mg_connection * conn;
	struct pssdpfile * psdp;
	char sdpfile[MAX_SDPFILENAME_LENGTH];

	psdp = malloc(sizeof(struct pssdpfile));
	psdp->ps = ps;
	snprintf(psdp->path, MAX_PATH_LENGTH, "%s/%s.sdp", c->http_opts.document_root, pstreamer_id(ps));
	snprintf(sdpfile, MAX_SDPFILENAME_LENGTH, "%s.sdp", pstreamer_id(ps));

	conn = mg_connect_http(c->mongoose_srv, sdpfile_handler, ch->sdpfile, NULL, NULL);
	conn->user_data = (void *) psdp;

	return strdup(sdpfile);
}
