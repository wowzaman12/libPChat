// per-channel/dialog settings :: /CHANOPT

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <unistd.h>
#else
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "xchat.h"
#include "xchatc.h"
#include "chanopt.h"
#include "cfgfiles.h"
#include "server.h"
#include "text.h"
#include "util.h"


static GSList *chanopt_list = nullptr;
static bool chanopt_open = false;
static bool chanopt_changed = false;


typedef struct
{
	char *name;
	char *alias;	// old names from 2.8.4
	int offset;
} channel_options;

#define S_F(xx) STRUCT_OFFSET_STR(session,xx)

static const channel_options chanopt[] =
{
	{"alert_beep", "BEEP", S_F(alert_beep)},
	{"alert_taskbar", nullptr, S_F(alert_taskbar)},
	{"alert_tray", "TRAY", S_F(alert_tray)},

	{"text_hidejoinpart", "CONFMODE", S_F(text_hidejoinpart)},
	{"text_logging", nullptr, S_F(text_logging)},
	{"text_scrollback", nullptr, S_F(text_scrollback)},
};

#undef S_F

static char* chanopt_value(unsigned char val)
{
	switch (val)
	{
	case SET_OFF:
		return "OFF";
	case SET_ON:
		return "ON";
	default:
		return "{unset}";
	}
}

// handle the /CHANOPT command

int chanopt_command(session *sess, char *tbuf, char *word[], char *word_eol[])
{
	int dots, i = 0, j, p = 0;
	unsigned char val;
	int offset = 2;
	char *find;
	bool quiet = false;
	int newval = -1;

	if (!strcmp(word[2], "-quiet"))
	{
		quiet = true;
		offset++;
	}

	find = word[offset++];

	if (word[offset][0])
	{
		if (!strcasecmp(word[offset], "ON"))
			newval = 1;
		else if (!strcasecmp(word[offset], "OFF"))
			newval = 0;
		else if (word[offset][0] == 'u')
			newval = SET_DEFAULT;
		else
			newval = atoi(word[offset]);
	}

	if (!quiet)
		PrintTextf(sess, "\002Network\002: %s \002Channel\002: %s\n",
					sess->server->network ? server_get_network(sess->server, TRUE) : _("<none>"),
					sess->channel[0] ? sess->channel : _("<none>"));

	while (i < sizeof(chanopt) / sizeof(channel_options))
	{
		if (find[0] == 0 || match(find, chanopt[i].name) || (chanopt[i].alias && match(find, chanopt[i].alias)))
		{
			if (newval != -1)	// set new value
			{
				*(int*)(sess + chanopt[i].offset) = newval;
			}

			if (!quiet)	// print value
			{
				strcpy(tbuf, chanopt[i].name);
				p = strlen(tbuf);

				tbuf[p++] = 3;
				tbuf[p++] = '2';

				dots = 20 - strlen(chanopt[i].name);

				for (j = 0; j < dots; j++)
					tbuf[p++] = '.';
				tbuf[p++] = 0;

				val = *(char*)(sess + chanopt[i].offset);
				PrintTextf(sess, "%s\0033:\017 %s", tbuf, chanopt_value(val));
			}
		}
		i++;
	}

	return TRUE;
}

// is a per-channel setting set? Or is it UNSET and the global version is set?

bool chanopt_is_set(unsigned int global, unsigned char per_chan_setting)
{
	if (per_chan_setting == SET_DEFAULT)
		return global;

	return per_chan_setting;
}

// additive version

bool chanopt_is_set_a(unsigned int global, unsigned char per_chan_setting)
{
	if (per_chan_setting == SET_DEFAULT)
		return global;

	return per_chan_setting || global;
}

// === below is LOADING/SAVING stuff only === //

typedef struct
{
	// Per-Channel Alerts
	// use a byte, because we need a pointer to each element
	unsigned char alert_beep;
	unsigned char alert_taskbar;
	unsigned char alert_tray;

	// Per-Channel Settings
	unsigned char text_hidejoinpart;
	unsigned char text_logging;
	unsigned char text_scrollback;

	char *network;
	char *channel;

} chanopt_in_memory;


static chanopt_in_memory* chanopt_find(char *network, char *channel, bool add_new)
{
	GSList *list;
	chanopt_in_memory *co;
	int i;

	for (list = chanopt_list; list; list = list->next)
	{
		co = (chanopt_in_memory*)list->data;
		if (!strcasecmp(co->channel, channel) &&
			!strcasecmp(co->network, network))
			return co;
	}

	if (!add_new)
		return nullptr;

	// allocate a new one
	co = (chanopt_in_memory*)malloc(sizeof(chanopt_in_memory));
	memset(co, 0, sizeof(chanopt_in_memory));
	co->channel = strdup(channel);
	co->network = strdup(network);

	// set all values to SET_DEFAULT
	i = 0;
	while (i < sizeof(chanopt) / sizeof(channel_options))
	{
		*(char*)(co + chanopt[i].offset) = SET_DEFAULT;
		i++;
	}

	chanopt_list = g_slist_prepend(chanopt_list, co);
	chanopt_changed = true;

	return co;
}

static void chanopt_add_opt(chanopt_in_memory *co, char *var, int new_value)
{
	int i;

	i = 0;
	while (i < sizeof(chanopt) / sizeof(channel_options))
	{
		if (!strcmp(var, chanopt[i].name))
		{
			*(char*)(co + chanopt[i].offset) = new_value;

		}
		i++;
	}
}

// load chanopt.conf from disk into our chanopt_list GSList

static void chanopt_load_all()
{
	int fh;
	char buf[256];
	char *eq;
	char *network = nullptr;
	chanopt_in_memory *current = nullptr;

	// 1. load the old file into our GSList
	fh = xchat_open_file("chanopt.conf", O_RDONLY, 0, 0);
	if (fh != -1)
	{
		while (waitline(fh, buf, sizeof buf, FALSE) != -1)
		{
			eq = strchr(buf, '=');
			if (!eq)
				continue;
			eq[0] = 0;

			if (eq != buf && eq[-1] == ' ')
				eq[-1] = 0;

			if (!strcmp(buf, "network"))
			{
				free(network);
				network = strdup(eq + 2);
			}
			else if (!strcmp(buf, "channel"))
			{
				current = chanopt_find(network, eq + 2, TRUE);
				chanopt_changed = false;
			}
			else
			{
				if (current)
					chanopt_add_opt(current, buf, atoi(eq + 2));
			}

		}
		close(fh);
		free(network);
	}
}

void chanopt_load(session *sess)
{
	int i;
	char val;
	chanopt_in_memory *co;
	char *network;

	if (sess->channel[0] == 0)
		return;

	network = server_get_network(sess->server, FALSE);
	if (!network)
		return;

	if (!chanopt_open)
	{
		chanopt_open = true;
		chanopt_load_all();
	}

	co = chanopt_find(network, sess->channel, FALSE);
	if (!co)
		return;

	// fill in all the sess->xxxxx fields
	i = 0;
	while (i < sizeof(chanopt) / sizeof(channel_options))
	{
		val = *(char*)(co + chanopt[i].offset);
		*(char*)(sess + chanopt[i].offset) = val;
		i++;
	}
}

void chanopt_save(session *sess)
{
	int i;
	char vals;
	char valm;
	chanopt_in_memory *co;
	char *network;

	if (sess->channel[0] == 0)
		return;

	network = server_get_network(sess->server, FALSE);
	if (!network)
		return;

	// 2. reconcile sess with what we loaded from disk

	co = chanopt_find(network, sess->channel, TRUE);

	i = 0;
	while (i < sizeof(chanopt) / sizeof(channel_options))
	{
		vals = *(char*)(sess + chanopt[i].offset);
		valm = *(char*)(co + chanopt[i].offset);

		if (vals != valm)
		{
			*(char*)(co + chanopt[i].offset) = vals;
			chanopt_changed = true;
		}

		i++;
	}
}

static void chanopt_save_one_channel(chanopt_in_memory *co, int fh)
{
	int i;
	char buf[256];
	char val;

	snprintf(buf, sizeof(buf), "%s = %s\n", "network", co->network);
	write(fh, buf, strlen(buf));

	snprintf(buf, sizeof(buf), "%s = %s\n", "channel", co->channel);
	write(fh, buf, strlen(buf));

	i = 0;
	while (i < sizeof(chanopt) / sizeof(channel_options))
	{
		val = *(char*)(co + chanopt[i].offset);
		if (val != SET_DEFAULT)
		{
			snprintf(buf, sizeof(buf), "%s = %d\n", chanopt[i].name, val);
			write(fh, buf, strlen(buf));
		}
		i++;
	}
}

void chanopt_save_all()
{
	int i;
	int num_saved;
	int fh;
	GSList *list;
	chanopt_in_memory *co;
	char val;

	if (!chanopt_list || !chanopt_changed)
	{
		return;
	}

	fh = xchat_open_file("chanopt.conf", O_TRUNC | O_WRONLY | O_CREAT, 0600, XOF_DOMODE);
	if (fh == -1)
	{
		return;
	}

	for (num_saved = 0, list = chanopt_list; list; list = list->next)
	{
		co = (chanopt_in_memory*)list->data;

		i = 0;
		while (i < sizeof(chanopt) / sizeof(channel_options))
		{
			val = *(char*)(co + chanopt[i].offset);
			// not using global/default setting, must save
			if (val != SET_DEFAULT)
			{
				if (num_saved != 0)
					write(fh, "\n", 1);

				chanopt_save_one_channel(co, fh);
				num_saved++;
				goto cont;
			}
			i++;
		}

cont:
		free(co->network);
		free(co->channel);
		free(co);
	}

	close(fh);

	// we're quiting, no need to free

	/*g_slist_free(chanopt_list);
	chanopt_list = nullptr;

	chanopt_open = false;
	chanopt_changed = false;*/
}
