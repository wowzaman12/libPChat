/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#else
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "xchat.h"
#include "ignore.h"
#include "cfgfiles.h"
#include "fe.h"
#include "text.h"
#include "util.h"
#include "xchatc.h"


int ignored_ctcp = 0;			// keep a count of all we ignore
int ignored_priv = 0;
int ignored_chan = 0;
int ignored_noti = 0;
int ignored_invi = 0;
static int ignored_total = 0;

/* ignore_exists():
 * returns: struct ig, if this mask is in the ignore list already
 *          nullptr, otherwise
 */
ignore* ignore_exists(char *mask)
{
	ignore *ig = 0;
	GSList *list;

	list = ignore_list;
	while (list)
	{
		ig = (ignore*)list->data;
		if (!rfc_casecmp(ig->mask, mask))
			return ig;
		list = list->next;
	}
	return nullptr;

}

/* ignore_add(...)
 *
 * returns:
 *            0 fail
 *            1 success
 *            2 success (old ignore has been changed)
 */

int ignore_add(char *mask, int type)
{
	ignore *ig = 0;
	int change_only = FALSE;

	// first check if it's already ignored
	ig = ignore_exists(mask);
	if (ig)
		change_only = TRUE;

	if (!change_only)
		ig = (ignore*)malloc(sizeof(ignore));

	if (!ig)
		return 0;

	ig->mask = strdup(mask);
	ig->type = type;

	if (!change_only)
		ignore_list = g_slist_prepend(ignore_list, ig);
	fe_ignore_update(1);

	if (change_only)
		return 2;

	return 1;
}

void ignore_showlist(session *sess)
{
	ignore *ig;
	GSList *list = ignore_list;
	char tbuf[256];
	int i = 0;

	EMIT_SIGNAL(XP_TE_IGNOREHEADER, sess, 0, 0, 0, 0, 0);

	while (list)
	{
		ig = (ignore*)list->data;
		i++;

		snprintf(tbuf, sizeof(tbuf), " %-25s ", ig->mask);
		if (ig->type & IG_PRIV)
			strcat(tbuf, _("YES  "));
		else
			strcat(tbuf, _("NO   "));
		if (ig->type & IG_NOTI)
			strcat(tbuf, _("YES  "));
		else
			strcat(tbuf, _("NO   "));
		if (ig->type & IG_CHAN)
			strcat(tbuf, _("YES  "));
		else
			strcat(tbuf, _("NO   "));
		if (ig->type & IG_CTCP)
			strcat(tbuf, _("YES  "));
		else
			strcat(tbuf, _("NO   "));
		if (ig->type & IG_DCC)
			strcat(tbuf, _("YES  "));
		else
			strcat(tbuf, _("NO   "));
		if (ig->type & IG_INVI)
			strcat(tbuf, _("YES  "));
		else
			strcat(tbuf, _("NO   "));
		if (ig->type & IG_UNIG)
			strcat(tbuf, _("YES  "));
		else
			strcat(tbuf, _("NO   "));
		strcat(tbuf, "\n");
		PrintText(sess, tbuf);
		//EMIT_SIGNAL(XP_TE_IGNORELIST, sess, ig->mask, 0, 0, 0, 0);
		// use this later, when TE's support 7 args
		list = list->next;
	}

	if (!i)
		EMIT_SIGNAL(XP_TE_IGNOREEMPTY, sess, 0, 0, 0, 0, 0);

	EMIT_SIGNAL(XP_TE_IGNOREFOOTER, sess, 0, 0, 0, 0, 0);
}

/* ignore_del()
 *
 * one of the args must be nullptr, use mask OR *ig, not both
 *
 */

int ignore_del(char *mask, ignore *ig)
{
	if (!ig)
	{
		GSList *list = ignore_list;

		while (list)
		{
			ig = (ignore*)list->data;
			if (!rfc_casecmp(ig->mask, mask))
				break;
			list = list->next;
			ig = 0;
		}
	}
	if (ig)
	{
		ignore_list = g_slist_remove(ignore_list, ig);
		free(ig->mask);
		free(ig);
		fe_ignore_update(1);
		return TRUE;
	}
	return FALSE;
}

// check if a msg should be ignored by browsing our ignore list

int ignore_check(char *host, int type)
{
	ignore *ig;
	GSList *list = ignore_list;

	// check if there's an UNIGNORE first, they take precendance.
	while (list)
	{
		ig = (ignore*)list->data;
		if (ig->type & IG_UNIG)
		{
			if (ig->type & type)
			{
				if (match(ig->mask, host))
					return FALSE;
			}
		}
		list = list->next;
	}

	list = ignore_list;
	while (list)
	{
		ig = (ignore*)list->data;

		if (ig->type & type)
		{
			if (match(ig->mask, host))
			{
				ignored_total++;
				if (type & IG_PRIV)
					ignored_priv++;
				if (type & IG_NOTI)
					ignored_noti++;
				if (type & IG_CHAN)
					ignored_chan++;
				if (type & IG_CTCP)
					ignored_ctcp++;
				if (type & IG_INVI)
					ignored_invi++;
				fe_ignore_update(2);
				return TRUE;
			}
		}
		list = list->next;
	}

	return FALSE;
}

static char* ignore_read_next_entry(char *my_cfg, ignore *ignore)
{
	char tbuf[1024];

	// Casting to char* done below just to satisfy compiler

	if (my_cfg)
	{
		my_cfg = cfg_get_str(my_cfg, "mask", tbuf, sizeof(tbuf));
		if (!my_cfg)
			return nullptr;
		ignore->mask = strdup(tbuf);

		my_cfg = cfg_get_str(my_cfg, "type", tbuf, sizeof(tbuf));
		ignore->type = atoi(tbuf);
	}
	return my_cfg;
}

void ignore_load()
{
	ignore *ignore;
	struct stat st;
	char *cfg, *my_cfg;
	int fh, i;

	fh = xchat_open_file("ignore.conf", O_RDONLY, 0, 0);
	if (fh != -1)
	{
		fstat(fh, &st);
		if (st.st_size)
		{
			cfg = (char*)malloc(st.st_size + 1);
			cfg[0] = '\0';
			i = read(fh, cfg, st.st_size);
			if (i >= 0)
				cfg[i] = '\0';
			my_cfg = cfg;
			while (my_cfg)
			{
				ignore = (struct ignore*)malloc(sizeof(struct ignore));
				memset(ignore, 0, sizeof(struct ignore));
				if ((my_cfg = ignore_read_next_entry(my_cfg, ignore)))
					ignore_list = g_slist_prepend(ignore_list, ignore);
				else
					free(ignore);
			}
			free(cfg);
		}
		close(fh);
	}
}

void ignore_save()
{
	char buf[1024];
	int fh;
	GSList *temp = ignore_list;
	ignore *ig;

	fh = xchat_open_file("ignore.conf", O_TRUNC | O_WRONLY | O_CREAT, 0600, XOF_DOMODE);
	if (fh != -1)
	{
		while (temp)
		{
			ig = (ignore*)temp->data;
			if (!(ig->type & IG_NOSAVE))
			{
				snprintf(buf, sizeof(buf), "mask = %s\ntype = %d\n\n",
							ig->mask, ig->type);
				write(fh, buf, strlen (buf));
			}
			temp = temp->next;
		}
		close(fh);
	}

}

static bool flood_autodialog_timeout(void* data)
{
	prefs.autodialog = 1;
	return FALSE;
}

int flood_check(char *nick, char *ip, server *serv, session *sess, int what)	//0=ctcp  1=priv
{
	/* serv
		int ctcp_counter;
		time_t ctcp_last_time;
	   prefs
		unsigned int ctcp_number_limit;
		unsigned int ctcp_time_limit;*/
	char buf[512];
	char real_ip[132];
	int i;
	time_t current_time;
	current_time = time(nullptr);

	if (what == 0)
	{
		if (serv->ctcp_last_time == 0)	// first ctcp in this server
		{
			serv->ctcp_last_time = time(nullptr);
			serv->ctcp_counter++;
		}
		else
		{
			if (difftime(current_time, serv->ctcp_last_time) < prefs.ctcp_time_limit)	// if we got the ctcp in the seconds limit
			{
				serv->ctcp_counter++;
				if (serv->ctcp_counter == prefs.ctcp_number_limit)	// if we reached the maximun numbers of ctcp in the seconds limits
				{
					serv->ctcp_last_time = current_time;	// we got the flood, restore all the vars for next one
					serv->ctcp_counter = 0;
					for (i = 0; i < 128; i++)
						if (ip[i] == '@')
							break;
					snprintf(real_ip, sizeof(real_ip), "*!*%s", &ip[i]);
					/*ignore_add(char *mask, int priv, int noti, int chan,
						int ctcp, int invi, int unignore, int no_save)*/

					snprintf(buf, sizeof(buf),
						_("You are being CTCP flooded from %s, ignoring %s\n"),
							nick, real_ip);
					PrintText(sess, buf);

					// FIXME: only ignore ctcp or all?, its ignoring ctcps for now
					ignore_add(real_ip, IG_CTCP);
					return 0;
				}
			}
		}
	}
	else
	{
		if (serv->msg_last_time == 0)
		{
			serv->msg_last_time = time(nullptr);
			serv->ctcp_counter++;
		}
		else
		{
			if (difftime(current_time, serv->msg_last_time) <
				prefs.msg_time_limit)
			{
				serv->msg_counter++;
				if (serv->msg_counter == prefs.msg_number_limit)	// if we reached the maximun numbers of ctcp in the seconds limits
				{
					snprintf(buf, sizeof(buf),
						_("You are being MSG flooded from %s, setting gui_auto_open_dialog OFF.\n"), ip);
					PrintText(sess, buf);
					serv->msg_last_time = current_time;	// we got the flood, restore all the vars for next one
					serv->msg_counter = 0;
					/*ignore_add(char *mask, int priv, int noti, int chan,
						int ctcp, int invi, int unignore, int no_save)*/

					if (prefs.autodialog)
					{
						// FIXME: only ignore ctcp or all?, its ignoring ctcps for now
						prefs.autodialog = 0;
						// turn it back on in 30 secs
						fe_timeout_add(30000, (void*)flood_autodialog_timeout, nullptr);
					}
					return 0;
				}
			}
		}
	}
	return 1;
}
