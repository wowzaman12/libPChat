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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __GNUC__
#include <unistd.h>
#else
#include <io.h>
#endif

#include <glib/ghash.h>

#include "xchat.h"
#include "xchatc.h"
#include "servlist.h"
#include "cfgfiles.h"
#include "fe.h"
#include "server.h"
#include "text.h"
#include "util.h" // token_foreach

#undef fprintf

struct defaultserver
{
	char *network;
	char *host;
	char *channel;
	char *charset;
};

static const struct defaultserver def[] =
{
	{"2600net",	0},
	{0,			"irc.2600.net"},


	{"AccessIRC",	0},
	{0,			"irc.accessirc.net"},
	{0,			"eu.accessirc.net"},

	{"AfterNET",	0},
	{0,			"irc.afternet.org"},
	{0,			"us.afternet.org"},
	{0,			"eu.afternet.org"},

	{"Aitvaras",	0},
#ifdef USE_IPV6
#ifdef USE_OPENSSL
	{0,			"irc6.ktu.lt/+7668"},
#endif
	{0,			"irc6.ktu.lt/7666"},
#endif
#ifdef USE_OPENSSL
	{0,			"irc.data.lt/+6668"},
	{0,			"irc-ssl.omnitel.net/+6668"},
	{0,			"irc-ssl.le.lt/+9999"},
#endif
	{0,			"irc.data.lt"},
	{0,			"irc.omnitel.net"},
	{0,			"irc.ktu.lt"},
	{0,			"irc.le.lt"},
	{0,			"irc.takas.lt"},
	{0,			"irc.5ci.net"},
	{0,			"irc.kis.lt"},

	{"AmigaNet",	0},
	{0,			"irc.amiganet.org"},
	{0,			"us.amiganet.org"},
	{0,			"uk.amiganet.org"},
	/*{0,			"no.amiganet.org"},
	{0,			"au.amiganet.org"},*/

	{"ARCNet",	0},
	{0,			"se1.arcnet.vapor.com"},
	{0,			"us1.arcnet.vapor.com"},
	{0,			"us2.arcnet.vapor.com"},
	{0,			"us3.arcnet.vapor.com"},
	{0,			"ca1.arcnet.vapor.com"},
	{0,			"de1.arcnet.vapor.com"},
	{0,			"de3.arcnet.vapor.com"},
	{0,			"ch1.arcnet.vapor.com"},
	{0,			"be1.arcnet.vapor.com"},
	{0,			"nl3.arcnet.vapor.com"},
	{0,			"uk1.arcnet.vapor.com"},
	{0,			"uk2.arcnet.vapor.com"},
	//{0,			"uk3.arcnet.vapor.com"},
	{0,			"fr1.arcnet.vapor.com"},

	{"AstroLink",	0},
	{0,			"irc.astrolink.org"},

	{"AustNet",	0},
	{0,			"au.austnet.org"},
	{0,			"us.austnet.org"},
	{0,			"ca.austnet.org"},

	/*{"AxeNet",	0},
	{0,			"irc.axenet.org"},
	{0,			"angel.axenet.org"},
	{0,			"energy.axenet.org"},
	{0,			"python.axenet.org"},*/

	{"AzzurraNet",	0},
	{0,			"irc.azzurra.org"},
	{0,			"crypto.azzurra.org"},

	{"Beirut", 0},
	{0,			"irc.beirut.com"},

	{"ChatJunkies",	0},
	{0,			"irc.chatjunkies.org"},
	{0,			"nl.chatjunkies.org"},

	{"ChatNet",	0},
	{0,			"US.ChatNet.Org"},
	{0,			"EU.ChatNet.Org"},

	{"ChatSociety", 0},
	{0,			"us.chatsociety.net"},
	{0,			"eu.chatsociety.net"},

	{"ChatSpike", 0},
	{0,			"irc.chatspike.net"},

	{"CoolChat",	0},
	{0,			"irc.coolchat.net"},
	/*	{0,			"unix.coolchat.net"},
		{0,			"toronto.coolchat.net"},*/

	{"Criten", 0},
	{0,			"irc.criten.net"},
	{0,			"irc.eu.criten.net"},

	{"DALnet",	0},
	{0,			"irc.dal.net"},
	{0,			"irc.eu.dal.net"},

	{"Dark-Tou-Net",	0},
	{0,			"irc.d-t-net.de"},
	{0,			"bw.d-t-net.de"},
	{0,			"nc.d-t-net.de"},
	{0,			"wakka.d-t-net.de"},

	{"DarkMyst", 0},
	{0,			"irc.darkmyst.org"},

	{"DeepIRC", 0},
	{0,			"irc.deepirc.net"},

	{"DeltaAnime", 0},
	{0,			"irc.deltaanime.net"},

	{"EFnet",	0},
	{0,			"irc.blackened.com"},
	{0,			"irc.Prison.NET"},
	{0,			"irc.Qeast.net"},
	{0,			"irc.efnet.pl"},
	{0,			"efnet.demon.co.uk"},
	//{0,			"irc.lagged.org"},
	{0,			"irc.lightning.net"},
	{0,			"irc.mindspring.com"},
	{0,			"irc.easynews.com"},
	{0,			"irc.servercentral.net"},

	{"EnterTheGame",	0},
	{0,			"IRC.EnterTheGame.Com"},

	{"EUIrc",	0},
	{0,			"irc.euirc.net"},
	{0,			"irc.ham.de.euirc.net"},
	{0,			"irc.ber.de.euirc.net"},
	{0,			"irc.ffm.de.euirc.net"},
	{0,			"irc.bre.de.euirc.net"},
	{0,			"irc.hes.de.euirc.net"},
	{0,			"irc.vie.at.euirc.net"},
	{0,			"irc.inn.at.euirc.net"},
	{0,			"irc.bas.ch.euirc.net"},

	{"EuropNet", 0},
	{0,			"irc.europnet.org"},

	{"EU-IRC",	0},
	{0,			"irc.eu-irc.net"},

	{"Ext3", 	0},
	{0,			"irc.ext3.net"},

	{"FDFNet",	0},
	{0,			"irc.fdfnet.net"},
	{0,			"irc.eu.fdfnet.net"},

	{"FEFNet",	0},
	{0,			"irc.fef.net"},
	{0,			"irc.ggn.net"},
	{0,			"irc.vendetta.com"},

#ifdef PORTABLE_MODE
	{"FreeNode",	0, "#portableapps"},
#else
	{"FreeNode",	0},
#endif
	{0,				"irc.freenode.net"},

	/*{"Freeworld",	0},
	{0,			"kabel.freeworld.nu"},
	{0,			"irc.freeworld.nu"},*/

	{"FyreChat", 0},
	{0,			"irc.fyrechat.net"},

	{"GalaxyNet",	0},
	{0,			"irc.galaxynet.org"},
	/*{0,			"sprynet.us.galaxynet.org"},
	{0,			"atlanta.ga.us.galaxynet.org"},*/

	{"GamesNET",	0},
	{0,				"irc.gamesnet.net"},
	/*{0,				"irc.us.gamesnet.net"},
	{0,				"east.us.gamesnet.net"},
	{0,				"west.us.gamesnet.net"},*/
	{0,				"irc.ca.gamesnet.net"},
	{0,				"irc.eu.gamesnet.net"},

	{"German-Elite",	0},
	{0,			"dominion.german-elite.net"},
	{0,			"komatu.german-elite.net"},
	//{0,			"liberty.german-elite.net"},

	{"GimpNet",		0},
	{0,			"irc.gimp.org"},
	//{0,			"irc.au.gimp.org"},
	{0,			"irc.us.gimp.org"},

	{"HabberNet",	0},
	{0,			"irc.habber.net"},

	{"Hashmark",	0},
	{0,			"irc.hashmark.net"},

	{"IdleMonkeys", 0},
	{0,			"irc.idlemonkeys.net"},

	/*{"Infinity-IRC",	0},
	{0,			"Atlanta.GA.US.Infinity-IRC.Org"},
	{0,			"Babylon.NY.US.Infinity-IRC.Org"},
	{0,			"Sunshine.Ca.US.Infinity-IRC.Org"},
	{0,			"IRC.Infinity-IRC.Org"},*/

	{"iZ-smart.net",	0},
	{0,			"irc.iZ-smart.net/6666"},
	{0,			"irc.iZ-smart.net/6667"},
	{0,			"irc.iZ-smart.net/6668"},

	{"IrcLink",	0},
	{0,			"irc.irclink.net"},
	{0,			"Alesund.no.eu.irclink.net"},
	{0,			"Oslo.no.eu.irclink.net"},
	{0,			"frogn.no.eu.irclink.net"},
	{0,			"tonsberg.no.eu.irclink.net"},

	{"IRCNet",		0},
	{0,				"irc.ircnet.com"},
	{0,				"irc.stealth.net/6668"},
	{0,				"ircnet.demon.co.uk"},
	//{0,				"ircnet.hinet.hr"},
	{0,				"irc.datacomm.ch"},
	/*{0,				"ircnet.kaptech.fr"},
	{0,				"ircnet.easynet.co.uk"},*/
	{0,				"random.ircd.de"},
	{0,				"ircnet.netvision.net.il"},
	//{0,				"irc.seed.net.tw"},
	{0,				"irc.cs.hut.fi"},

	{"Irctoo.net",	0},
	{0,			"irc.irctoo.net"},

	{"Krstarica", 0},
	{0,			"irc.krstarica.com"},

	{"Librenet",	0},
	{0,			"irc.librenet.net"},
	{0,			"ielf.fr.librenet.net"},

	{"LinkNet",	0},
	{0,			"irc.link-net.org"},
	{0,			"irc.no.link-net.org"},
	//{0,			"irc.gamesden.net.au"},
	{0,			"irc.bahnhof.se"},
	/*{0,			"irc.kinexuseurope.co.uk"},
	{0,			"irc.gamiix.com"},*/

	{"Live2K IRC", 0},
	{0,			"fergie.live2k.com"},
	{0,			"schumacher.live2k.com"},
	{0,			"webber.live2k.com"},
	{0,			"fremont.live2k.com"},
	{0,			"wallen.live2k.com"},
	{0,			"vodacom.live2k.com"},

	{"MagicStar",	0},
	{0,			"irc.magicstar.net"},

	{"Majistic",	0},
	{0,			"irc.majistic.net"},

	{"MindForge",	0},
	{0,			"irc.mindforge.org"},

	{"MintIRC",	0},
	{0,			"irc.mintirc.net"},

	{"MIXXnet",		0},
	{0,			"irc.mixxnet.net"},

	{"NeverNET",	0},
	{0,			"irc.nevernet.net"},
	{0,			"imagine.nevernet.net"},
	{0,			"dimension.nevernet.net"},
	{0,			"universe.nevernet.net"},
	{0,			"wayland.nevernet.net"},
	{0,			"forte.nevernet.net"},

	{"NixHelpNet",	0},
	{0,			"irc.nixhelp.org"},
	{0,			"us.nixhelp.org"},
	{0,			"uk.nixhelp.org"},
	{0,			"uk2.nixhelp.org"},
	{0,			"uk3.nixhelp.org"},
	{0,			"nl.nixhelp.org"},
	{0,			"ca.ld.nixhelp.org"},
	{0,			"us.co.nixhelp.org"},
	{0,			"us.ca.nixhelp.org"},
	{0,			"us.pa.nixhelp.org"},

	{"nullptrusNet",	0},
	{0,			"irc.nullptrus.net"},

	{"Oceanius", 0},
	{0,			"irc.oceanius.com"},

	{"OFTC",	0},
	{0,			"irc.oftc.net"},

	{"OtherNet",	0},
	{0,			"irc.othernet.org"},

	{"OzNet",	0},
	{0,			"irc.oz.org"},

	{"PIRC.PL",	0},
	{0,			"irc.pirc.pl"},

	{"PTlink",	0},
	{0,			"irc.PTlink.net"},
	{0,			"aaia.PTlink.net"},

	{"PTNet.org",	0},
	{0,			"irc.PTNet.org"},
	{0,			"world.PTnet.org"},
	{0,			"netvisao.PTnet.org"},
	{0,			"uevora.PTnet.org"},
	{0,			"vianetworks.PTnet.org"},
	{0,			"uc.PTnet.org"},
	{0,			"nfsi.ptnet.org"},

	{"QuakeNet",	0},
	{0,			"irc.quakenet.org"},
	{0,			"irc.se.quakenet.org"},
	{0,			"irc.dk.quakenet.org"},
	{0,			"irc.no.quakenet.org"},
	{0,			"irc.fi.quakenet.org"},
	{0,			"irc.be.quakenet.org"},
	{0,			"irc.uk.quakenet.org"},
	{0,			"irc.de.quakenet.org"},
	{0,			"irc.it.quakenet.org"},

	{"RizeNET", 0},
	{0,			"irc.rizenet.org"},
	{0,			"omega.rizenet.org"},
	{0,			"evelance.rizenet.org"},
	{0,			"lisa.rizenet.org"},
	{0,			"scott.rizenet.org"},

	{"Rizon", 0},
	{0,			"irc.rizon.net"},

	{"RusNet", 0, 0, "KOI8-R (Cyrillic)"},
	{0,			"irc.tomsk.net"},
	{0,			"irc.rinet.ru"},
	{0,			"irc.run.net"},
	{0,			"irc.ru"},
	{0,			"irc.lucky.net"},

	{"SceneNet",	0},
	{0,			"irc.scene.org"},
	{0,			"irc.eu.scene.org"},
	{0,			"irc.us.scene.org"},

	{"SeilEn.de",	0},
	{0,			"irc.seilen.de"},

	{"SlashNET",	0},
	{0,			"irc.slashnet.org"},
	{0,			"area51.slashnet.org"},
	{0,			"moo.slashnet.org"},
	{0,			"radon.slashnet.org"},

	{"Sohbet.Net", 0},
	{0,			"irc.sohbet.net"},

	{"SolidIRC", 0},
	{0,			"irc.solidirc.com"},

	{"SorceryNet",	0},
	{0,			"irc.sorcery.net/9000"},
	{0,			"irc.us.sorcery.net/9000"},
	{0,			"irc.eu.sorcery.net/9000"},

	{"Spidernet",	0},
	{0,			"us.spidernet.org"},
	{0,			"eu.spidernet.org"},
	{0,			"irc.spidernet.org"},

	{"StarChat", 0},
	{0,			"irc.starchat.net"},
	{0,			"gainesville.starchat.net"},
	{0,			"freebsd.starchat.net"},
	{0,			"sunset.starchat.net"},
	{0,			"revenge.starchat.net"},
	{0,			"tahoma.starchat.net"},
	{0,			"neo.starchat.net"},

	{"Talon/Terra Odyssey Chat Network", 0},
	{0,			"irc.tochat.org"},
	{0,			"thetalon.tochat.org"},
	{0,			"enterprise.tochat.org"},
	{0,			"voyager.tochat.org"},
	{0,			"vodacom.tochat.org"},

	{"TNI3",			0},
	{0,			"irc.tni3.com"},

	{"UnderNet",	0},
	{0,			"us.undernet.org"},
	{0,			"eu.undernet.org"},

	{"UniBG",		0},
	{0,			"irc.lirex.com"},
	{0,			"irc.naturella.com"},
	{0,			"irc.spnet.net"},
	{0,			"irc.techno-link.com"},
	{0,			"irc.telecoms.bg"},
	{0,			"irc.tu-varna.edu"},

	{"Whiffle",	0},
	{0,			"irc.whiffle.org"},

	{"Worldnet",		0},
	{0,			"irc.worldnet.net"},
	{0,			"irc.fr.worldnet.net"},

	{"Xentonix.net",	0},
	{0,			"irc.xentonix.net"},

	{"XWorld",	0},
	{0,			"Buffalo.NY.US.XWorld.org"},
	{0,			"Minneapolis.MN.US.Xworld.Org"},
	{0,			"Rochester.NY.US.XWorld.org"},
	{0,			"Bayern.DE.EU.XWorld.Org"},
	{0,			"Chicago.IL.US.XWorld.Org"},

	{"YourIRC", 0, "#pchat"},
	{0,			"irc.yourirc.net"},
	{0,			"irc.yourirc.net/8067"},
	{0,			"irc.e10host.com"},
	{0,			"irc.e10host.com/8067"},

	{0,0}
};

GSList *network_list = 0;

void servlist_connect(session *sess, ircnet *net, bool join)
{
	ircserver *ircserv;
	GSList *list;
	char *port;
	server *serv;

	if (!sess)
		sess = new_ircwindow(nullptr, nullptr, SESS_SERVER, TRUE);

	serv = sess->server;

	// connect to the currently selected Server-row
	list = g_slist_nth(net->servlist, net->selected);
	if (!list)
		list = net->servlist;
	if (!list)
		return;
	ircserv = (ircserver*)list->data;

	// incase a protocol switch is added to the servlist gui
	server_fill_her_up(sess->server);

	if (join)
	{
		sess->willjoinchannel[0] = 0;

		if (net->autojoin)
		{
			if (serv->autojoin)
				free(serv->autojoin);
			serv->autojoin = strdup(net->autojoin);
		}
	}

	serv->password[0] = 0;
	if (net->pass)
		safe_strcpy(serv->password, net->pass, sizeof(serv->password));

	if (net->flags & FLAG_USE_GLOBAL)
	{
		strcpy(serv->nick, prefs.nick1);
	}
	else
	{
		if (net->nick[0])
			strcpy(serv->nick, net->nick[0]);
	}

	serv->dont_use_proxy = (net->flags & FLAG_USE_PROXY) ? FALSE : TRUE;

#ifdef USE_OPENSSL
	serv->use_ssl = (net->flags & FLAG_USE_SSL) ? TRUE : FALSE;
	serv->accept_invalid_cert =
		(net->flags & FLAG_ALLOW_INVALID) ? TRUE : FALSE;
#endif

	serv->network = net;

	port = strrchr(ircserv->hostname, '/');
	if (port)
	{
		*port = 0;

		// support "+port" to indicate SSL (like mIRC does)
		if (port[1] == '+')
		{
#ifdef USE_OPENSSL
			serv->use_ssl = TRUE;
#endif
			serv->connect(serv, ircserv->hostname, atoi(port + 2), FALSE);
		}
		else
		{
			serv->connect(serv, ircserv->hostname, atoi(port + 1), FALSE);
		}

		*port = '/';
	}
	else
		serv->connect(serv, ircserv->hostname, -1, FALSE);

	server_set_encoding(serv, net->encoding);
}

int servlist_connect_by_netname(session *sess, char *network, bool join)
{
	ircnet *net;
	GSList *list = network_list;

	while (list)
	{
		net = (ircnet*)list->data;

		if (strcasecmp(net->name, network) == 0)
		{
			servlist_connect(sess, net, join);
			return 1;
		}

		list = list->next;
	}

	return 0;
}

int servlist_have_auto()
{
	GSList *list = network_list;
	ircnet *net;

	while (list)
	{
		net = (ircnet*)list->data;

		if (net->flags & FLAG_AUTO_CONNECT)
			return 1;

		list = list->next;
	}

	return 0;
}

int servlist_auto_connect(session *sess)
{
	GSList *list = network_list;
	ircnet *net;
	int ret = 0;

	while (list)
	{
		net = (ircnet*)list->data;

		if (net->flags & FLAG_AUTO_CONNECT)
		{
			servlist_connect(sess, net, TRUE);
			ret = 1;
		}

		list = list->next;
	}

	return ret;
}

static int servlist_cycle_cb(server *serv)
{
	if (serv->network)
	{
		PrintTextf(serv->server_session,
					_("Cycling to next server in %s...\n"), ((ircnet*)serv->network)->name);
		servlist_connect(serv->server_session, (ircnet*)serv->network, TRUE);
	}

	return 0;
}

int servlist_cycle(server *serv)
{
	ircnet *net;
	int max, del;

	net = (ircnet*)serv->network;
	if (net)
	{
		max = g_slist_length(net->servlist);
		if (max > 0)
		{
			// try the next server, if that option is on
			if (net->flags & FLAG_CYCLE)
			{
				net->selected++;
				if (net->selected >= max)
					net->selected = 0;
			}

			del = prefs.recon_delay * 1000;
			if (del < 1000)
				del = 500;				// so it doesn't block the gui

			if (del)
				serv->recondelay_tag = fe_timeout_add(del, (void*)servlist_cycle_cb, serv);
			else
				servlist_connect(serv->server_session, net, TRUE);

			return TRUE;
		}
	}

	return FALSE;
}

ircserver* servlist_server_find(ircnet *net, char *name, int *pos)
{
	GSList *list = net->servlist;
	ircserver *serv;
	int i = 0;

	while (list)
	{
		serv = (ircserver*)list->data;
		if (strcmp(serv->hostname, name) == 0)
		{
			if (pos)
				*pos = i;
			return serv;
		}
		i++;
		list = list->next;
	}

	return nullptr;
}

/* find a network (e.g. (ircnet *) to "FreeNode") from a hostname
 * (e.g. "irc.eu.freenode.net") */

ircnet* servlist_net_find_from_server(char *server_name)
{
	GSList *list = network_list;
	GSList *slist;
	ircnet *net;
	ircserver *serv;

	while (list)
	{
		net = (ircnet*)list->data;

		slist = net->servlist;
		while (slist)
		{
			serv = (ircserver*)slist->data;
			if (strcasecmp(serv->hostname, server_name) == 0)
				return net;
			slist = slist->next;
		}

		list = list->next;
	}

	return nullptr;
}

ircnet* servlist_net_find(char *name, int *pos, int (*cmpfunc)(const char*, const char*))
{
	GSList *list = network_list;
	ircnet *net;
	int i = 0;

	while (list)
	{
		net = (ircnet*)list->data;
		if (cmpfunc(net->name, name) == 0)
		{
			if (pos)
				*pos = i;
			return net;
		}
		i++;
		list = list->next;
	}

	return nullptr;
}

ircserver* servlist_server_add(ircnet *net, char *name)
{
	ircserver *serv;

	serv = (ircserver*)malloc(sizeof(ircserver));
	memset(serv, 0, sizeof(ircserver));
	serv->hostname = strdup(name);

	net->servlist = g_slist_append(net->servlist, serv);

	return serv;
}

void servlist_server_remove(ircnet *net, ircserver *serv)
{
	free(serv->hostname);
	free(serv);
	net->servlist = g_slist_remove(net->servlist, serv);
}

static void servlist_server_remove_all(ircnet *net)
{
	ircserver *serv;

	while (net->servlist)
	{
		serv = (ircserver*)net->servlist->data;
		servlist_server_remove(net, serv);
	}
}

static void free_and_clear(char *str)
{
	if (str)
	{
		char *orig = str;
		while (*str)
			*str++ = 0;
		free(orig);
	}
}

// executed on exit: Clear any password strings

void servlist_cleanup()
{
	GSList *list;
	ircnet *net;

	for (list = network_list; list; list = list->next)
	{
		net = (ircnet*)list->data;
		free_and_clear(net->pass);
		free_and_clear(net->nickserv);
	}
}

void servlist_net_remove(ircnet *net)
{
	GSList *list;
	server *serv;

	servlist_server_remove_all(net);
	network_list = g_slist_remove(network_list, net);

	for (int i = 0; i < (sizeof(net->nick)/sizeof(net->nick[0])); i++)
		if (net->nick[i])
			free(net->nick[i]);
	if (net->user)
		free(net->user);
	if (net->real)
		free(net->real);
	free_and_clear(net->pass);
	if (net->autojoin)
		free(net->autojoin);
	if (net->command)
		free(net->command);
	free_and_clear(net->nickserv);
	if (net->comment)
		free(net->comment);
	if (net->encoding)
		free(net->encoding);
	free(net->name);
	free(net);

	// for safety
	list = serv_list;
	while (list)
	{
		serv = (server*)list->data;
		if (serv->network == net)
			serv->network = nullptr;
		list = list->next;
	}
}

ircnet* servlist_net_add(char *name, char *comment, int prepend)
{
	ircnet *net;

	net = (ircnet*)malloc(sizeof(ircnet));
	memset(net, 0, sizeof(ircnet));
	net->name = strdup(name);
	//net->comment = strdup(comment);
	net->flags = FLAG_CYCLE | FLAG_USE_GLOBAL | FLAG_USE_PROXY;

	if (prepend)
		network_list = g_slist_prepend(network_list, net);
	else
		network_list = g_slist_append(network_list, net);

	return net;
}

static void servlist_load_defaults()
{
	int i = 0, j = 0;
	ircnet *net = nullptr;

	while (1)
	{
		if (def[i].network)
		{
			net = servlist_net_add(def[i].network, def[i].host, FALSE);
			net->encoding = strdup("IRC (Latin/Unicode Hybrid)");
			if (def[i].channel)
				net->autojoin = strdup(def[i].channel);
			if (def[i].charset)
			{
				free(net->encoding);
				net->encoding = strdup(def[i].charset);
			}
			if (g_str_hash(def[i].network) == 0x8e1b96f7)
				prefs.slist_select = j;
			j++;
		}
		else
		{
			servlist_server_add(net, def[i].host);
			if (!def[i+1].host && !def[i+1].network)
				break;
		}
		i++;
	}
}

static int servlist_load()
{
	FILE *fp;
	char buf[2048];
	int len, ni=0;
	char *tmp;
	ircnet *net = nullptr;

	fp = xchat_fopen_file("servlist_.conf", "r", 0);
	if (!fp)
		return FALSE;

	while (fgets(buf, sizeof(buf) - 2, fp))
	{
		len = strlen(buf);
		buf[len] = 0;
		buf[len-1] = 0;	// remove the trailing \n
		if (net)
		{
			switch (buf[0])
			{
			case 'I':
				net->nick[ni++] = strdup(buf + 2);
				break;
			case 'U':
				net->user = strdup(buf + 2);
				break;
			case 'R':
				net->real = strdup(buf + 2);
				break;
			case 'P':
				net->pass = strdup(buf + 2);
				break;
			case 'J':
				net->autojoin = strdup(buf + 2);
				break;
			case 'C':
				if (net->command)
				{
					// concat extra commands with a \n separator
					tmp = net->command;
					net->command = (char*)malloc(strlen(tmp) + strlen(buf + 2) + 2);
					strcpy(net->command, tmp);
					strcat(net->command, "\n");
					strcat(net->command, buf + 2);
					free(tmp);
				}
				else
					net->command = strdup(buf + 2);
				break;
			case 'F':
				net->flags = atoi(buf + 2);
				break;
			case 'D':
				net->selected = atoi(buf + 2);
				break;
			case 'E':
				net->encoding = strdup(buf + 2);
				break;
			case 'S':	// new server/hostname for this network
				servlist_server_add(net, buf + 2);
				break;
			case 'B':
				net->nickserv = strdup(buf + 2);
				break;
			}
		}
		if (buf[0] == 'N') {
			net = servlist_net_add(buf + 2, /* comment */ nullptr, FALSE);
			ni = 0;
		}
	}
	fclose(fp);

	return TRUE;
}

void servlist_init()
{
	if (!network_list)
		if (!servlist_load())
			servlist_load_defaults();
}

// check if a charset is known by Iconv
int servlist_check_encoding(char *charset)
{
	GIConv gic;
	char *c;

	c = strchr(charset, ' ');
	if (c)
		c[0] = 0;

	if (!strcasecmp(charset, "IRC")) // special case
	{
		if (c)
			c[0] = ' ';
		return TRUE;
	}

	gic = g_iconv_open(charset, "UTF-8");

	if (c)
		c[0] = ' ';

	if (gic != (GIConv)-1)
	{
		g_iconv_close(gic);
		return TRUE;
	}

	return FALSE;
}

static int servlist_write_ccmd(char *str, void *fp)
{
	return fprintf((FILE*)fp, "C=%s\n", (str[0] == '/') ? str + 1 : str);
}

int servlist_save()
{
	FILE *fp;
	char buf[256];
	ircnet *net;
	ircserver *serv;
	GSList *list;
	GSList *hlist;
#ifndef WIN32
	int first = FALSE;

	snprintf(buf, sizeof(buf), "%s/servlist_.conf", get_xdir_fs());
	if (access(buf, F_OK) != 0)
		first = TRUE;
#endif

	fp = xchat_fopen_file("servlist_.conf", "w", 0);
	if (!fp)
		return FALSE;

#ifndef WIN32
	if (first)
		chmod(buf, 0600);
#endif
	fprintf(fp, "v="PACKAGE_VERSION"\n\n");

	list = network_list;
	while (list)
	{
		net = (ircnet*)list->data;

		fprintf(fp, "N=%s\n", net->name);
		for (int i = 0; i < (sizeof(net->nick)/sizeof(net->nick[0])); i++)
			if (net->nick[i])
				fprintf(fp, "I=%s\n", net->nick[i]);
		if (net->user)
			fprintf(fp, "U=%s\n", net->user);
		if (net->real)
			fprintf(fp, "R=%s\n", net->real);
		if (net->pass)
			fprintf(fp, "P=%s\n", net->pass);
		if (net->autojoin)
			fprintf(fp, "J=%s\n", net->autojoin);
		if (net->nickserv)
			fprintf(fp, "B=%s\n", net->nickserv);
		if (net->encoding && strcasecmp(net->encoding, "System") &&
				strcasecmp(net->encoding, "System default"))
		{
			fprintf(fp, "E=%s\n", net->encoding);
			if (!servlist_check_encoding(net->encoding))
			{
				snprintf(buf, sizeof(buf), _("Warning: \"%s\" character set is unknown. No conversion will be applied for network %s."),
						 net->encoding, net->name);
				fe_message(buf, FE_MSG_WARN);
			}
		}

		if (net->command)
			token_foreach(net->command, '\n', servlist_write_ccmd, fp);

		fprintf(fp, "F=%d\nD=%d\n", net->flags, net->selected);

		hlist = net->servlist;
		while (hlist)
		{
			serv = (ircserver*)hlist->data;
			fprintf(fp, "S=%s\n", serv->hostname);
			hlist = hlist->next;
		}

		if (fprintf(fp, "\n") < 1)
		{
			fclose(fp);
			return FALSE;
		}

		list = list->next;
	}

	fclose(fp);
	return TRUE;
}

static void joinlist_free1(GSList *list)
{
	GSList *head = list;

	for (; list; list = list->next)
		g_free(list->data);
	g_slist_free(head);
}

void joinlist_free(GSList *channels, GSList *keys)
{
	joinlist_free1(channels);
	joinlist_free1(keys);
}

bool joinlist_is_in_list(server *serv, char *channel)
{
	GSList *channels, *keys;
	GSList *list;

	if (!serv->network || !((ircnet*)serv->network)->autojoin)
		return FALSE;

	joinlist_split(((ircnet*)serv->network)->autojoin, &channels, &keys);

	for (list = channels; list; list = list->next)
	{
		if (serv->p_cmp((const char*)list->data, channel) == 0)
			return TRUE;
	}

	joinlist_free(channels, keys);

	return FALSE;
}

char* joinlist_merge(GSList *channels, GSList *keys)
{
	GString *out = g_string_new(nullptr);
	GSList *list;
	int i, j;

	for (; channels; channels = channels->next)
	{
		g_string_append(out, (gchar*)channels->data);

		if (channels->next)
			g_string_append_c(out, ',');
	}

	// count number of REAL keys
	for (i = 0, list = keys; list; list = list->next)
		if (list->data)
			i++;

	if (i > 0)
	{
		g_string_append_c(out, ' ');

		for (j = 0; keys; keys = keys->next)
		{
			if (keys->data)
			{
				g_string_append(out, (gchar*)keys->data);
				j++;
				if (j == i)
					break;
			}

			if (keys->next)
				g_string_append_c(out, ',');
		}
	}

	return g_string_free(out, FALSE);
}

void joinlist_split(char *autojoin, GSList **channels, GSList **keys)
{
	char *parta, *partb;
	char *chan, *key;
	int len;

	*channels = nullptr;
	*keys = nullptr;

	// after the first space, the keys begin
	parta = autojoin;
	partb = strchr(autojoin, ' ');
	if (partb)
		partb++;

	while (1)
	{
		chan = parta;
		key = partb;

		if (1)
		{
			while (parta[0] != 0 && parta[0] != ',' && parta[0] != ' ')
			{
				parta++;
			}
		}

		if (partb)
		{
			while (partb[0] != 0 && partb[0] != ',' && partb[0] != ' ')
			{
				partb++;
			}
		}

		len = parta - chan;
		if (len < 1)
			break;
		*channels = g_slist_append(*channels, g_strndup(chan, len));

		len = partb - key;
		*keys = g_slist_append(*keys, len ? g_strndup(key, len) : nullptr);

		if (parta[0] == ' ' || parta[0] == 0)
			break;
		parta++;

		if (partb)
		{
			if (partb[0] == 0 || partb[0] == ' ')
				partb = nullptr;	// no more keys, but maybe more channels?
			else
				partb++;
		}
	}

#if 0
	GSList *lista, *listb;
	int i;

	printf("-----\n");
	i = 0;
	lista = *channels;
	listb = *keys;
	while (lista)
	{
		printf("%d. |%s| |%s|\n", i, lista->data, listb->data);
		i++;
		lista = lista->next;
		listb = listb->next;
	}
	printf("-----\n\n");
#endif
}
