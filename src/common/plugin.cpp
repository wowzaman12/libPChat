/* X-Chat
 * Copyright (C) 2002 Peter Zelezny.
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
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "xchat.h"
#include "xchatc.h"
#define PLUGIN_C
typedef struct session xchat_context;
#include "../../plugins/xchat-plugin.h"
#include "plugin.h"
#include "cfgfiles.h"
#include "dcc.h"
#include "fe.h"
#include "ignore.h"
#include "modes.h"
#include "notify.h"
#include "outbound.h"
#include "server.h"
#include "servlist.h"
#include "text.h"
#include "userlist.h"
#include "util.h"

// the USE_PLUGIN define only removes libdl stuff

#ifdef USE_PLUGIN
#ifdef USE_GMODULE
#include <gmodule.h>
#else
#include <dlfcn.h>
#endif
#endif

#define DEBUG(x) {x;}

// crafted to be an even 32 bytes
struct _xchat_hook
{
	xchat_plugin *pl;	// the plugin to which it belongs
	char *name;			// "xdcc"
	void *callback;		// pointer to xdcc_callback
	char *help_text;	// help_text for commands only
	void *userdata;		// passed to the callback
	int tag;			// for timers & FDs only
	int type;			// HOOK_*
	int pri;	// fd	// priority / fd for HOOK_FD only
};

struct _xchat_list
{
	int type;			// LIST_*
	GSList *pos;		// current pos
	GSList *next;		// next pos
	GSList *head;		// for LIST_USERS only
	notify_per_server *notifyps;	// notify_per_server*
};

typedef int (xchat_cmd_cb)(char *word[], char *word_eol[], void *user_data);
typedef int (xchat_serv_cb)(char *word[], char *word_eol[], void *user_data);
typedef int (xchat_print_cb)(char *word[], void *user_data);
typedef int (xchat_fd_cb)(int fd, int flags, void *user_data);
typedef int (xchat_timer_cb)(void *user_data);
typedef int (xchat_init_func)(xchat_plugin*, char**, char**, char**, char*);
typedef int (xchat_deinit_func)(xchat_plugin*);

enum
{
	LIST_CHANNELS,
	LIST_DCC,
	LIST_IGNORE,
	LIST_NOTIFY,
	LIST_USERS
};

enum
{
	HOOK_COMMAND,	// /command
	HOOK_SERVER,	// PRIVMSG, NOTICE, numerics
	HOOK_PRINT,		// All print events
	HOOK_TIMER,		// timeouts
	HOOK_FD,		// sockets & fds
	HOOK_DELETED	// marked for deletion
};

GSList *plugin_list = nullptr;	// export for plugingui.c
static GSList *hook_list = nullptr;

//extern const struct prefs vars[];	// cfgfiles.c


// unload a plugin and remove it from our linked list

static int plugin_free(xchat_plugin *pl, int do_deinit, int allow_refuse)
{
	GSList *list, *next;
	xchat_hook *hook;
	xchat_deinit_func *deinit_func;

	// fake plugin added by xchat_plugingui_add()
	if (pl->fake)
		goto xit;

	// run the plugin's deinit routine, if any
	if (do_deinit && pl->deinit_callback != nullptr)
	{
		deinit_func = (xchat_deinit_func*)pl->deinit_callback;
		if (!deinit_func(pl) && allow_refuse)
			return FALSE;
	}

	// remove all of this plugin's hooks
	list = hook_list;
	while (list)
	{
		hook = (xchat_hook*)list->data;
		next = list->next;
		if (hook->pl == pl)
			xchat_unhook(nullptr, hook);
		list = next;
	}

#ifdef USE_PLUGIN
	if (pl->handle)
#ifdef USE_GMODULE
		g_module_close((GModule*)pl->handle);
#else
		dlclose(pl->handle);
#endif
#endif

xit:
	if (pl->free_strings)
	{
		if (pl->name)
			free(pl->name);
		if (pl->desc)
			free(pl->desc);
		if (pl->version)
			free(pl->version);
	}
	if (pl->filename)
		free(pl->filename);
	free(pl);

	plugin_list = g_slist_remove(plugin_list, pl);

#ifdef USE_PLUGIN
	fe_pluginlist_update();
#endif

	return TRUE;
}

static xchat_plugin* plugin_list_add(xchat_context *ctx, char *filename, const char *name,
					const char *desc, const char *version, void *handle,
					void *deinit_func, int fake, int free_strings)
{
	xchat_plugin *pl;

	pl = (xchat_plugin*)malloc(sizeof(xchat_plugin));
	pl->handle = handle;
	pl->filename = filename;
	pl->context = ctx;
	pl->name = (char*)name;
	pl->desc = (char*)desc;
	pl->version = (char*)version;
	pl->deinit_callback = deinit_func;
	pl->fake = fake;
	pl->free_strings = free_strings;	// free() name,desc,version?

	plugin_list = g_slist_prepend(plugin_list, pl);

	return pl;
}

static void* xchat_dummy(xchat_plugin *ph)
{
	return nullptr;
}

#ifdef WIN32
static int xchat_read_fd(xchat_plugin *ph, GIOChannel *source, char *buf, int *len)
{
	return g_io_channel_read(source, buf, *len, (gsize*)len);
}
#endif

// Load a static plugin

typedef void* (*read_fd)(xchat_plugin*);
void plugin_add(session *sess, char *filename, void *handle, void *init_func,
				void *deinit_func, char *arg, int fake)
{
	xchat_plugin *pl;
	char *file;

	file = nullptr;
	if (filename)
		file = strdup(filename);

	pl = plugin_list_add(sess, file, file, nullptr, nullptr, handle, deinit_func, fake, FALSE);

	if (!fake)
	{
		// win32 uses these because it doesn't have --export-dynamic!
		pl->xchat_hook_command = xchat_hook_command;
		pl->xchat_hook_server = xchat_hook_server;
		pl->xchat_hook_print = xchat_hook_print;
		pl->xchat_hook_timer = xchat_hook_timer;
		pl->xchat_hook_fd = xchat_hook_fd;
		pl->xchat_unhook = xchat_unhook;
		pl->xchat_print = xchat_print;
		pl->xchat_printf = xchat_printf;
		pl->xchat_command = xchat_command;
		pl->xchat_commandf = xchat_commandf;
		pl->xchat_nickcmp = xchat_nickcmp;
		pl->xchat_set_context = xchat_set_context;
		pl->xchat_find_context = xchat_find_context;
		pl->xchat_get_context = xchat_get_context;
		pl->xchat_get_info = xchat_get_info;
		pl->xchat_get_prefs = xchat_get_prefs;
		pl->xchat_list_get = xchat_list_get;
		pl->xchat_list_free = xchat_list_free;
		pl->xchat_list_fields = xchat_list_fields;
		pl->xchat_list_str = xchat_list_str;
		pl->xchat_list_next = xchat_list_next;
		pl->xchat_list_int = xchat_list_int;
		pl->xchat_plugingui_add = xchat_plugingui_add;
		pl->xchat_plugingui_remove = xchat_plugingui_remove;
		pl->xchat_emit_print = xchat_emit_print;
#ifdef WIN32
		pl->xchat_read_fd = (read_fd)xchat_read_fd;
#else
		pl->xchat_read_fd = xchat_dummy;
#endif
		pl->xchat_list_time = xchat_list_time;
		pl->xchat_gettext = xchat_gettext;
		pl->xchat_send_modes = xchat_send_modes;
		pl->xchat_strip = xchat_strip;
		pl->xchat_free = xchat_free;

		// in case new plugins are loaded on older xchat
		pl->xchat_dummy4 = xchat_dummy;
		pl->xchat_dummy3 = xchat_dummy;
		pl->xchat_dummy2 = xchat_dummy;
		pl->xchat_dummy1 = xchat_dummy;

		// run xchat_plugin_init, if it returns 0, close the plugin
		if (((xchat_init_func*)init_func)(pl, &pl->name, &pl->desc, &pl->version, arg) == 0)
		{
			plugin_free(pl, FALSE, FALSE);
			return;
		}
	}

#ifdef USE_PLUGIN
	fe_pluginlist_update();
#endif
}

// kill any plugin by the given (file) name (used by /unload)

int plugin_kill(char *name, int by_filename)
{
	GSList *list;
	xchat_plugin *pl;

	list = plugin_list;
	while (list)
	{
		pl = (xchat_plugin*)list->data;
		// static-plugins (plugin-timer.c) have a nullptr filename
		if ((by_filename && pl->filename && strcasecmp(name, pl->filename) == 0) ||
			(by_filename && pl->filename && strcasecmp(name, file_part(pl->filename)) == 0) ||
			(!by_filename && strcasecmp(name, pl->name) == 0))
		{
			// statically linked plugins have a nullptr filename
			if (pl->filename != nullptr && !pl->fake)
			{
				if (plugin_free(pl, TRUE, TRUE))
					return 1;
				return 2;
			}
		}
		list = list->next;
	}

	return 0;
}

// kill all running plugins (at shutdown)

void plugin_kill_all()
{
	GSList *list, *next;
	xchat_plugin *pl;

	list = plugin_list;
	while (list)
	{
		pl = (xchat_plugin*)list->data;
		next = list->next;
		if (!pl->fake)
			plugin_free((xchat_plugin*)list->data, TRUE, FALSE);
		list = next;
	}
}

#ifdef USE_PLUGIN

// load a plugin from a filename. Returns: nullptr-success or an error string
typedef int (*xif)(xchat_plugin*, char**, char**, char**, char*);
typedef int (*xdf)(xchat_plugin*);
char* plugin_load(session *sess, char *filename, char *arg)
{
	void *handle;
	xchat_init_func *init_func;
	xchat_deinit_func *deinit_func;

#ifdef USE_GMODULE
	// load the plugin
	handle = g_module_open(filename, (GModuleFlags)0);
	if (handle == nullptr)
		return (char*)g_module_error();

	// find the init routine xchat_plugin_init
	if (!g_module_symbol((GModule*)handle, "xchat_plugin_init", (gpointer*)&init_func))
	{
		g_module_close((GModule*)handle);
		return _("No xchat_plugin_init symbol; is this really an xchat plugin?");
	}

	// find the plugin's deinit routine, if any
	if (!g_module_symbol((GModule*)handle, "xchat_plugin_deinit", (gpointer*)&deinit_func))
		deinit_func = nullptr;

#else
	char *error;
	char *filepart;

// OpenBSD lacks this!
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif

	// get the filename without path
	filepart = file_part(filename);

	// load the plugin
	if (filepart &&
		// xsys draws in libgtk-1.2, causing crashes, so force RTLD_LOCAL
		(strstr(filepart, "local") || strncmp (filepart, "libxsys-1", 9) == 0))
		handle = dlopen(filename, RTLD_NOW);
	else
		handle = dlopen(filename, RTLD_GLOBAL | RTLD_NOW);
	if (handle == nullptr)
		return (char*)dlerror();
	dlerror();		// Clear any existing error

	// find the init routine xchat_plugin_init
	init_func = (xif)dlsym(handle, "xchat_plugin_init");
	error = (char*)dlerror();
	if (error != nullptr)
	{
		dlclose(handle);
		return _("No xchat_plugin_init symbol; is this really an xchat plugin?");
	}

	// find the plugin's deinit routine, if any
	deinit_func = (xdf)dlsym(handle, "xchat_plugin_deinit");
	error = (char*)dlerror();
#endif

	// add it to our linked list
	plugin_add(sess, filename, handle, (void*)init_func, (void*)deinit_func, arg, FALSE);

	return nullptr;
}

static session *ps;

static void plugin_auto_load_cb(char *filename)
{
	char *pMsg;

#ifndef WIN32	/* black listed */
	if (!strcmp(file_part(filename), "dbus.so"))
		return;
#endif

	pMsg = plugin_load(ps, filename, nullptr);
	if (pMsg)
	{
		PrintTextf(ps, "AutoLoad failed for: %s\n", filename);
		PrintText(ps, pMsg);
	}
}

void plugin_auto_load(session *sess)
{
	ps = sess;
#ifdef WIN32
	for_files("./plugins", "*.dll", plugin_auto_load_cb);
	for_files(get_xdir_fs(), "*.dll", plugin_auto_load_cb);
#else
#if defined(__hpux)
	for_files(XCHATLIBDIR"/plugins", "*.sl", plugin_auto_load_cb);
	for_files(get_xdir_fs(), "*.sl", plugin_auto_load_cb);
#else
	for_files(XCHATLIBDIR"/plugins", "*.so", plugin_auto_load_cb);
	for_files(get_xdir_fs(), "*.so", plugin_auto_load_cb);
#endif
#endif
}

#endif

static GSList* plugin_hook_find(GSList *list, int type, char *name)
{
	xchat_hook *hook;

	while (list)
	{
		hook = (xchat_hook*)list->data;
		if (hook->type == type)
		{
			if (strcasecmp(hook->name, name) == 0)
				return list;

			if (type == HOOK_SERVER)
			{
				if (strcasecmp(hook->name, "RAW LINE") == 0)
					return list;
			}
		}
		list = list->next;
	}

	return nullptr;
}

// check for plugin hooks and run them

static int plugin_hook_run(session *sess, char *name, char *word[], char *word_eol[], int type)
{
	GSList *list, *next;
	xchat_hook *hook;
	int ret, eat = 0;

	list = hook_list;
	while (1)
	{
		list = plugin_hook_find(list, type, name);
		if (!list)
			goto xit;

		hook = (xchat_hook*)list->data;
		next = list->next;
		hook->pl->context = sess;

		// run the plugin's callback function
		switch (type)
		{
		case HOOK_COMMAND:
			ret = ((xchat_cmd_cb*)hook->callback)(word, word_eol, hook->userdata);
			break;
		case HOOK_SERVER:
			ret = ((xchat_serv_cb*)hook->callback)(word, word_eol, hook->userdata);
			break;
		default: //case HOOK_PRINT:
			ret = ((xchat_print_cb*)hook->callback)(word, hook->userdata);
			break;
		}

		if ((ret & XCHAT_EAT_XCHAT) && (ret & XCHAT_EAT_PLUGIN))
		{
			eat = 1;
			goto xit;
		}
		if (ret & XCHAT_EAT_PLUGIN)
			goto xit;	// stop running plugins
		if (ret & XCHAT_EAT_XCHAT)
			eat = 1;	// eventually we'll return 1, but continue running plugins

		list = next;
	}

xit:
	// really remove deleted hooks now
	list = hook_list;
	while (list)
	{
		hook = (xchat_hook*)list->data;
		next = list->next;
		if (hook->type == HOOK_DELETED)
		{
			hook_list = g_slist_remove(hook_list, hook);
			free(hook);
		}
		list = next;
	}

	return eat;
}

// execute a plugged in command. Called from outbound.c

int plugin_emit_command(session *sess, char *name, char *word[], char *word_eol[])
{
	return plugin_hook_run(sess, name, word, word_eol, HOOK_COMMAND);
}

// got a server PRIVMSG, NOTICE, numeric etc...

int plugin_emit_server(session *sess, char *name, char *word[], char *word_eol[])
{
	return plugin_hook_run(sess, name, word, word_eol, HOOK_SERVER);
}

// see if any plugins are interested in this print event

int plugin_emit_print(session *sess, char *word[])
{
	return plugin_hook_run(sess, word[0], word, nullptr, HOOK_PRINT);
}

int plugin_emit_dummy_print(session *sess, char *name)
{
	char *word[32];
	int i;

	word[0] = name;
	for (i = 1; i < 32; i++)
		word[i] = "\000";

	return plugin_hook_run(sess, name, word, nullptr, HOOK_PRINT);
}

int plugin_emit_keypress(session *sess, unsigned int state, unsigned int keyval,
							int len, char *string)
{
	char *word[PDIWORDS];
	char keyval_str[16];
	char state_str[16];
	char len_str[16];
	int i;

	if (!hook_list)
		return 0;

	sprintf(keyval_str, "%u", keyval);
	sprintf(state_str, "%u", state);
	sprintf(len_str, "%d", len);

	word[0] = "Key Press";
	word[1] = keyval_str;
	word[2] = state_str;
	word[3] = string;
	word[4] = len_str;
	for (i = 5; i < PDIWORDS; i++)
		word[i] = "\000";

	return plugin_hook_run(sess, word[0], word, nullptr, HOOK_PRINT);
}

static int plugin_timeout_cb(xchat_hook *hook)
{
	int ret;

	// timer_cb's context starts as front-most-tab
	hook->pl->context = current_sess;

	// call the plugin's timeout function
	ret = ((xchat_timer_cb*)hook->callback)(hook->userdata);

	// the callback might have already unhooked it!
	if (!g_slist_find(hook_list, hook) || hook->type == HOOK_DELETED)
		return 0;

	if (ret == 0)
	{
		hook->tag = 0;	// avoid fe_timeout_remove, returning 0 is enough!
		xchat_unhook(hook->pl, hook);
	}

	return ret;
}

// insert a hook into hook_list according to its priority

static void plugin_insert_hook(xchat_hook *new_hook)
{
	GSList *list;
	xchat_hook *hook;

	list = hook_list;
	while (list)
	{
		hook = (xchat_hook*)list->data;
		if (hook->type == new_hook->type && hook->pri <= new_hook->pri)
		{
			hook_list = g_slist_insert_before(hook_list, list, new_hook);
			return;
		}
		list = list->next;
	}

	hook_list = g_slist_append(hook_list, new_hook);
}

static bool plugin_fd_cb(GIOChannel *source, GIOCondition condition, xchat_hook *hook)
{
	int flags = 0, ret;
	typedef int (xchat_fd_cb2)(int fd, int flags, void *user_data, GIOChannel*);

	if (condition & G_IO_IN)
		flags |= XCHAT_FD_READ;
	if (condition & G_IO_OUT)
		flags |= XCHAT_FD_WRITE;
	if (condition & G_IO_PRI)
		flags |= XCHAT_FD_EXCEPTION;

	ret = ((xchat_fd_cb2*)hook->callback)(hook->pri, flags, hook->userdata, source);

	// the callback might have already unhooked it!
	if (!g_slist_find(hook_list, hook) || hook->type == HOOK_DELETED)
		return 0;

	if (ret == 0)
	{
		hook->tag = 0; // avoid fe_input_remove, returning 0 is enough!
		xchat_unhook(hook->pl, hook);
	}

	return ret;
}

// allocate and add a hook to our list. Used for all 4 types

static xchat_hook* plugin_add_hook(xchat_plugin *pl, int type, int pri, const char *name,
					const char *help_text, void *callb, int timeout, void *userdata)
{
	xchat_hook *hook;

	hook = (xchat_hook*)malloc(sizeof(xchat_hook));
	memset(hook, 0, sizeof(xchat_hook));

	hook->type = type;
	hook->pri = pri;
	if (name)
		hook->name = strdup(name);
	if (help_text)
		hook->help_text = strdup(help_text);
	hook->callback = callb;
	hook->pl = pl;
	hook->userdata = userdata;

	// insert it into the linked list
	plugin_insert_hook(hook);

	if (type == HOOK_TIMER)
		hook->tag = fe_timeout_add(timeout, (void*)plugin_timeout_cb, hook);

	return hook;
}

GList* plugin_command_list(GList *tmp_list)
{
	xchat_hook *hook;
	GSList *list = hook_list;

	while (list)
	{
		hook = (xchat_hook*)list->data;
		if (hook->type == HOOK_COMMAND)
			tmp_list = g_list_prepend(tmp_list, hook->name);
		list = list->next;
	}
	return tmp_list;
}

void plugin_command_foreach(session *sess, void *userdata,
			void (*cb)(session *sess, void *userdata, char *name, char *help))
{
	GSList *list;
	xchat_hook *hook;

	list = hook_list;
	while (list)
	{
		hook = (xchat_hook*)list->data;
		if (hook->type == HOOK_COMMAND && hook->name[0])
		{
			cb(sess, userdata, hook->name, hook->help_text);
		}
		list = list->next;
	}
}

int plugin_show_help(session *sess, char *cmd)
{
	GSList *list;
	xchat_hook *hook;

	list = plugin_hook_find(hook_list, HOOK_COMMAND, cmd);
	if (list)
	{
		hook = (xchat_hook*)list->data;
		if (hook->help_text)
		{
			PrintText(sess, hook->help_text);
			return 1;
		}
	}

	return 0;
}

/* ========================================================= */
/* ===== these are the functions plugins actually call ===== */
/* ========================================================= */

void* xchat_unhook(xchat_plugin *ph, xchat_hook *hook)
{
	// perl.c trips this
	if (!g_slist_find(hook_list, hook) || hook->type == HOOK_DELETED)
		return nullptr;

	if (hook->type == HOOK_TIMER && hook->tag != 0)
		fe_timeout_remove(hook->tag);

	if (hook->type == HOOK_FD && hook->tag != 0)
		fe_input_remove(hook->tag);

	hook->type = HOOK_DELETED;	// expunge later

	if (hook->name)
		free(hook->name);	// nullptr for timers & fds
	if (hook->help_text)
		free(hook->help_text);	// nullptr for non-commands

	return hook->userdata;
}

xchat_hook* xchat_hook_command(xchat_plugin *ph, const char *name, int pri,
						xchat_cmd_cb *callb, const char *help_text, void *userdata)
{
	return plugin_add_hook(ph, HOOK_COMMAND, pri, name, help_text, (void*)callb, 0,
									userdata);
}

xchat_hook* xchat_hook_server(xchat_plugin *ph, const char *name, int pri,
						xchat_serv_cb *callb, void *userdata)
{
	return plugin_add_hook(ph, HOOK_SERVER, pri, name, 0, (void*)callb, 0, userdata);
}

xchat_hook* xchat_hook_print(xchat_plugin *ph, const char *name, int pri,
						xchat_print_cb *callb, void *userdata)
{
	return plugin_add_hook(ph, HOOK_PRINT, pri, name, 0, (void*)callb, 0, userdata);
}

xchat_hook* xchat_hook_timer(xchat_plugin *ph, int timeout, xchat_timer_cb *callb, void *userdata)
{
	return plugin_add_hook(ph, HOOK_TIMER, 0, 0, 0, (void*)callb, timeout, userdata);
}

xchat_hook* xchat_hook_fd(xchat_plugin *ph, int fd, int flags,
					xchat_fd_cb *callb, void *userdata)
{
	xchat_hook *hook;

	hook = plugin_add_hook(ph, HOOK_FD, 0, 0, 0, (void*)callb, 0, userdata);
	hook->pri = fd;
	// plugin hook_fd flags correspond exactly to FIA_* flags (fe.h)
	hook->tag = fe_input_add(fd, flags, (void*)plugin_fd_cb, hook);

	return hook;
}

void xchat_print(xchat_plugin *ph, const char *text)
{
	if (!is_session(ph->context))
	{
		DEBUG(PrintTextf(0, "%s\txchat_print called without a valid context.\n", ph->name));
		return;
	}

	PrintText(ph->context, (char*)text);
}

void xchat_printf(xchat_plugin *ph, const char *format, ...)
{
	va_list args;
	char *buf;

	va_start(args, format);
	buf = g_strdup_vprintf(format, args);
	va_end(args);

	xchat_print(ph, buf);
	g_free(buf);
}

void xchat_command(xchat_plugin *ph, const char *command)
{
	char *conv;
	int len = -1;

	if (!is_session(ph->context))
	{
		DEBUG(PrintTextf(0, "%s\txchat_command called without a valid context.\n", ph->name));
		return;
	}

	// scripts/plugins continue to send non-UTF8... *sigh*
	conv = text_validate((char**)&command, &len);
	handle_command(ph->context, (char*)command, FALSE);
	g_free (conv);
}

void xchat_commandf(xchat_plugin *ph, const char *format, ...)
{
	va_list args;
	char *buf;

	va_start(args, format);
	buf = g_strdup_vprintf(format, args);
	va_end(args);

	xchat_command(ph, buf);
	g_free(buf);
}

int xchat_nickcmp(xchat_plugin *ph, const char *s1, const char *s2)
{
	return ((session*)ph->context)->server->p_cmp(s1, s2);
}

xchat_context* xchat_get_context(xchat_plugin *ph)
{
	return ph->context;
}

int xchat_set_context(xchat_plugin *ph, xchat_context *context)
{
	if (is_session(context))
	{
		ph->context = context;
		return 1;
	}
	return 0;
}

xchat_context* xchat_find_context(xchat_plugin *ph, const char *servname, const char *channel)
{
	GSList *slist, *clist, *sessions = nullptr;
	server *serv;
	session *sess;
	char *netname;

	if (servname == nullptr && channel == nullptr)
		return current_sess;

	slist = serv_list;
	while (slist)
	{
		serv = (server*)slist->data;
		netname = server_get_network(serv, TRUE);

		if (servname == nullptr ||
			 rfc_casecmp(servname, serv->servername) == 0 ||
			 strcasecmp(servname, serv->hostname) == 0 ||
			 strcasecmp(servname, netname) == 0)
		{
			if (channel == nullptr)
				return serv->front_session;

			clist = sess_list;
			while (clist)
			{
				sess = (session*)clist->data;
				if (sess->server == serv)
				{
					if (rfc_casecmp(channel, sess->channel) == 0)
					{
						if (sess->server == ph->context->server)
						{
							g_slist_free(sessions);
							return sess;
						}
						else
						{
							sessions = g_slist_prepend(sessions, sess);
						}
					}
				}
				clist = clist->next;
			}
		}
		slist = slist->next;
	}

	if (sessions)
	{
		sessions = g_slist_reverse(sessions);
		sess = (session*)sessions->data;
		g_slist_free(sessions);
		return sess;
	}

	return nullptr;
}

const char* xchat_get_info(xchat_plugin *ph, const char *id)
{
	session *sess;
	unsigned int hash;

	// 1234567890
	if (!strncmp(id, "event_text", 10))
	{
		char *e = (char*)id + 10;
		if (*e == ' ') e++;	// 2.8.0 only worked without a space
		return text_find_format_string(e);
	}

	hash = str_hash(id);
	// do the session independant ones first
	switch (hash)
	{
	case 0x325acab5:	// libdirfs
		return XCHATLIBDIR;

	case 0x14f51cd8: // version
		return PACKAGE_VERSION;

	case 0xdd9b1abd:	// xchatdir
		return get_xdir_utf8();

	case 0xe33f6c4a:	// xchatdirfs
		return get_xdir_fs();
	}

	sess = ph->context;
	if (!is_session(sess))
	{
		DEBUG(PrintTextf(0, "%s\txchat_get_info called without a valid context.\n", ph->name));
		return nullptr;
	}

	switch (hash)
	{
	case 0x2de2ee: // away
		if (sess->server->is_away)
			return sess->server->last_away_reason;
		return nullptr;

	case 0x2c0b7d03: // channel
		return sess->channel;

	case 0x2c0d614c: // charset
		{
			const char *locale;

			if (sess->server->encoding)
				return sess->server->encoding;

			locale = nullptr;
			g_get_charset(&locale);
			return locale;
		}

	case 0x30f5a8: // host
		return sess->server->hostname;

	case 0x1c0e99c1: // inputbox
		return fe_get_inputbox_contents(sess);

	case 0x633fb30:	// modes
		return sess->current_modes;

	case 0x6de15a2e:	// network
		return server_get_network(sess->server, FALSE);

	case 0x339763: // nick
		return sess->server->nick;

	case 0x438fdf9: // nickserv
		if (sess->server->network)
			return ((ircnet*)sess->server->network)->nickserv;
		return nullptr;

	case 0xca022f43: // server
		if (!sess->server->connected)
			return nullptr;
		return sess->server->servername;

	case 0x696cd2f: // topic
		return sess->topic;

	case 0x3419f12d: // gtkwin_ptr
		return (const char*)fe_gui_info_ptr(sess, 1);

	case 0x506d600b: // native win_ptr
		return (const char*)fe_gui_info_ptr(sess, 0);

	case 0x6d3431b5: // win_status
		switch (fe_gui_info(sess, 0))	// check window status
		{
		case 0: return "normal";
		case 1: return "active";
		case 2: return "hidden";
		}
		return nullptr;
	}

	return nullptr;
}

int xchat_get_prefs(xchat_plugin *ph, const char *name, const char **string, int *integer)
{
	int i = 0;

	// some special run-time info (not really prefs, but may aswell throw it in here)
	switch (str_hash(name))
	{
		case 0xf82136c4: // state_cursor
			*integer = fe_get_inputbox_cursor(ph->context);
			return 2;

		case 0xd1b: // id
			*integer = ph->context->server->id;
			return 2;
	}

	do
	{
		if (!strcasecmp(name, vars[i].name))
		{
			switch (vars[i].type)
			{
			case TYPE_STR:
				*string = ((char*)&prefs + vars[i].offset);
				return 1;

			case TYPE_INT:
				*integer = *((int*)&prefs + vars[i].offset);
				return 2;

			default:
			//case TYPE_BOOL:
				if (*((int*)&prefs + vars[i].offset))
					*integer = 1;
				else
					*integer = 0;
				return 3;
			}
		}
		i++;
	}
	while (vars[i].name);

	return 0;
}

xchat_list* xchat_list_get(xchat_plugin *ph, const char *name)
{
	xchat_list *list;

	list = (xchat_list*)malloc(sizeof(xchat_list));
	list->pos = nullptr;

	switch (str_hash(name))
	{
	case 0x556423d0: // channels
		list->type = LIST_CHANNELS;
		list->next = sess_list;
		break;

	case 0x183c4:	// dcc
		list->type = LIST_DCC;
		list->next = dcc_list;
		break;

	case 0xb90bfdd2:	// ignore
		list->type = LIST_IGNORE;
		list->next = ignore_list;
		break;

	case 0xc2079749:	// notify
		list->type = LIST_NOTIFY;
		list->next = notify_list;
		list->head = (GSList*)ph->context;	// reuse this pointer
		break;

	case 0x6a68e08: // users
		if (is_session(ph->context))
		{
			list->type = LIST_USERS;
			list->head = list->next = userlist_flat_list(ph->context);
			fe_userlist_set_selected(ph->context);
			break;
		}	// fall through

	default:
		free(list);
		return nullptr;
	}

	return list;
}

void xchat_list_free(xchat_plugin *ph, xchat_list *xlist)
{
	if (xlist->type == LIST_USERS)
		g_slist_free(xlist->head);
	free(xlist);
}

int xchat_list_next(xchat_plugin *ph, xchat_list *xlist)
{
	if (xlist->next == nullptr)
		return 0;

	xlist->pos = xlist->next;
	xlist->next = xlist->pos->next;

	/* NOTIFY LIST: Find the entry which matches the context
		of the plugin when list_get was originally called.*/
	if (xlist->type == LIST_NOTIFY)
	{
		xlist->notifyps = notify_find_server_entry((notify*)xlist->pos->data,
													((session*)xlist->head)->server);
		if (!xlist->notifyps)
			return 0;
	}

	return 1;
}

const char* const* xchat_list_fields(xchat_plugin *ph, const char *name)
{
	static const char* const dcc_fields[] =
	{
		"iaddress32","icps",		"sdestfile","sfile",		"snick",	"iport",
		"ipos", "iposhigh", "iresume", "iresumehigh", "isize", "isizehigh", "istatus", "itype", nullptr
	};
	static const char* const channels_fields[] =
	{
		"schannel",	"schantypes", "pcontext", "iflags", "iid", "ilag", "imaxmodes",
		"snetwork", "snickmodes", "snickprefixes", "iqueue", "sserver", "itype", "iusers",
		nullptr
	};
	static const char* const ignore_fields[] =
	{
		"iflags", "smask", nullptr
	};
	static const char* const notify_fields[] =
	{
		"iflags", "snetworks", "snick", "toff", "ton", "tseen", nullptr
	};
	static const char* const users_fields[] =
	{
		"iaway", "shost", "tlasttalk", "snick", "sprefix", "srealname", "iselected", nullptr
	};
	static const char* const list_of_lists[] =
	{
		"channels",	"dcc", "ignore", "notify", "users", nullptr
	};

	switch (str_hash(name))
	{
	case 0x556423d0:	// channels
		return channels_fields;
	case 0x183c4:		// dcc
		return dcc_fields;
	case 0xb90bfdd2:	// ignore
		return ignore_fields;
	case 0xc2079749:	// notify
		return notify_fields;
	case 0x6a68e08:		// users
		return users_fields;
	case 0x6236395:		// lists
		return list_of_lists;
	}

	return nullptr;
}

time_t xchat_list_time(xchat_plugin *ph, xchat_list *xlist, const char *name)
{
	unsigned int hash = str_hash(name);
	void* data;

	switch (xlist->type)
	{
	case LIST_NOTIFY:
		if (!xlist->notifyps)
			return (time_t)-1;
		switch (hash)
		{
		case 0x1ad6f:	// off
			return xlist->notifyps->lastoff;
		case 0xddf:	// on
			return xlist->notifyps->laston;
		case 0x35ce7b:	// seen
			return xlist->notifyps->lastseen;
		}
		break;

	case LIST_USERS:
		data = xlist->pos->data;
		switch (hash)
		{
		case 0xa9118c42:	// lasttalk
			return ((User*)data)->lasttalk;
		}
	}

	return (time_t)-1;
}

const char* xchat_list_str(xchat_plugin *ph, xchat_list *xlist, const char *name)
{
	unsigned int hash = str_hash(name);
	void* data = ph->context;
	int type = LIST_CHANNELS;

	// a nullptr xlist is a shortcut to current "channels" context
	if (xlist)
	{
		data = xlist->pos->data;
		type = xlist->type;
	}

	switch (type)
	{
	case LIST_CHANNELS:
		switch (hash)
		{
		case 0x2c0b7d03: // channel
			return ((session*)data)->channel;
		case 0x577e0867: // chantypes
			return ((session*)data)->server->chantypes;
		case 0x38b735af: // context
			return (const char*)data;	// this is a session*
		case 0x6de15a2e: // network
			return server_get_network (((session*)data)->server, FALSE);
		case 0x8455e723: // nickprefixes
			return ((session*)data)->server->nick_prefixes;
		case 0x829689ad: // nickmodes
			return ((session*)data)->server->nick_modes;
		case 0xca022f43: // server
			return ((session*)data)->server->servername;
		}
		break;

	case LIST_DCC:
		switch (hash)
		{
		case 0x3d9ad31e:	// destfile
			return ((DCC*)data)->destfile;
		case 0x2ff57c:		// file
			return ((DCC*)data)->file;
		case 0x339763:		// nick
			return ((DCC*)data)->nick;
		}
		break;

	case LIST_IGNORE:
		switch (hash)
		{
		case 0x3306ec:	// mask
			return ((ignore*)data)->mask;
		}
		break;

	case LIST_NOTIFY:
		switch (hash)
		{
		case 0x4e49ec05:	// networks
			return ((notify*)data)->networks;
		case 0x339763:		// nick
			return ((notify*)data)->name;
		}
		break;

	case LIST_USERS:
		switch (hash)
		{
		case 0x339763: // nick
			return ((User*)data)->nick;
		case 0x30f5a8: // host
			return ((User*)data)->hostname;
		case 0xc594b292: // prefix
			return ((User*)data)->prefix;
		case 0xccc6d529: // realname
			return ((User*)data)->realname;
		}
		break;
	}

	return nullptr;
}

int xchat_list_int(xchat_plugin *ph, xchat_list *xlist, const char *name)
{
	unsigned int hash = str_hash(name);
	void* data = ph->context;
	int tmp, type = LIST_CHANNELS;

	// a nullptr xlist is a shortcut to current "channels" context
	if (xlist)
	{
		data = xlist->pos->data;
		type = xlist->type;
	}

	switch (type)
	{
	case LIST_DCC:
		switch (hash)
		{
		case 0x34207553: // address32
			return ((DCC*)data)->addr;
		case 0x181a6: // cps
			return ((DCC*)data)->cps;
		case 0x349881: // port
			return ((DCC*)data)->port;
		case 0x1b254: // pos
			return ((DCC*)data)->pos & 0xffffffff;
		case 0xe8a945f6: // poshigh
			return (((DCC*)data)->pos >> 32) & 0xffffffff;
		case 0xc84dc82d: // resume
			return ((DCC*)data)->resumable & 0xffffffff;
		case 0xded4c74f: /* resumehigh */
			return (((DCC*)data)->resumable >> 32) & 0xffffffff;
		case 0x35e001: // size
			return ((DCC*)data)->size & 0xffffffff;
		case 0x3284d523: // sizehigh
			return (((DCC*)data)->size >> 32) & 0xffffffff;
		case 0xcacdcff2: // status
			return ((DCC*)data)->dccstat;
		case 0x368f3a: // type
			return ((DCC*)data)->type;
		}
		break;

	case LIST_IGNORE:
		switch (hash)
		{
		case 0x5cfee87:	// flags
			return ((ignore*)data)->type;
		}
		break;

	case LIST_CHANNELS:
		switch (hash)
		{
		case 0xd1b:	// id
			return ((session*)data)->server->id;
		case 0x5cfee87:	// flags
			tmp = ((session*)data)->alert_taskbar;		// bit 10
			tmp <<= 1;
			tmp |= ((session*)data)->alert_tray;		// 9
			tmp <<= 1;
			tmp |= ((session*)data)->alert_beep;		// 8
			tmp <<= 1;
			//tmp |= ((session*)data)->color_paste;		// 7
			tmp <<= 1;
			tmp |= ((session*)data)->text_hidejoinpart;	// 6
			tmp <<= 1;
			tmp |= ((session*)data)->server->have_idmsg; // 5
			tmp <<= 1;
			tmp |= ((session*)data)->server->have_whox;	// 4
			tmp <<= 1;
			tmp |= ((session*)data)->server->end_of_motd; // 3
			tmp <<= 1;
			tmp |= ((session*)data)->server->is_away;	// 2
			tmp <<= 1;
			tmp |= ((session*)data)->server->connecting; // 1
			tmp <<= 1;
			tmp |= ((session*)data)->server->connected;	// 0
			return tmp;
		case 0x1a192: // lag
			return ((session*)data)->server->lag;
		case 0x1916144c: // maxmodes
			return ((session*)data)->server->modes_per_line;
		case 0x66f1911: // queue
			return ((session*)data)->server->sendq_len;
		case 0x368f3a:	// type
			return ((session*)data)->type;
		case 0x6a68e08: // users
			return ((session*)data)->total;
		}
		break;

	case LIST_NOTIFY:
		if (!xlist->notifyps)
			return -1;
		switch (hash)
		{
		case 0x5cfee87: // flags
			return xlist->notifyps->ison;
		}

	case LIST_USERS:
		switch (hash)
		{
		case 0x2de2ee:	// away
			return ((User*)data)->away;
		case 0x4705f29b: // selected
			return ((User*)data)->selected;
		}
		break;

	}

	return -1;
}

void* xchat_plugingui_add(xchat_plugin *ph, const char *filename,
							const char *name, const char *desc,
							const char *version, char *reserved)
{
#ifdef USE_PLUGIN
	ph = plugin_list_add(nullptr, strdup(filename), strdup(name), strdup(desc),
								strdup(version), nullptr, nullptr, TRUE, TRUE);
	fe_pluginlist_update();
#endif

	return ph;
}

void xchat_plugingui_remove(xchat_plugin *ph, void *handle)
{
#ifdef USE_PLUGIN
	plugin_free((xchat_plugin*)handle, FALSE, FALSE);
#endif
}

int xchat_emit_print(xchat_plugin *ph, const char *event_name, ...)
{
	va_list args;
	/* currently only 4 because no events use more than 4.
		This can be easily expanded without breaking the API.*/
	char *argv[4] = {nullptr, nullptr, nullptr, nullptr};
	int i = 0;

	va_start(args, event_name);
	while (1)
	{
		argv[i] = va_arg(args, char*);
		if (!argv[i])
			break;
		i++;
		if (i >= 4)
			break;
	}

	i = text_emit_by_name((char*)event_name, ph->context, argv[0], argv[1],
								argv[2], argv[3]);
	va_end (args);

	return i;
}

char* xchat_gettext(xchat_plugin *ph, const char *msgid)
{
	// so that plugins can use xchat's internal gettext strings.
	// e.g. The EXEC plugin uses this on Windows.
	return (char*)_(msgid);
}

void xchat_send_modes(xchat_plugin *ph, const char **targets, int ntargets, int modes_per_line, char sign, char mode)
{
	char tbuf[514];	// modes.c needs 512 + nullptr

	send_channel_modes(ph->context, tbuf, (char**)targets, 0, ntargets, sign, mode, modes_per_line);
}

char* xchat_strip(xchat_plugin *ph, const char *str, int len, int flags)
{
	return strip_color((char*)str, len, flags);
}

void xchat_free(xchat_plugin *ph, void *ptr)
{
	g_free(ptr);
}
