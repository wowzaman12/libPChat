#ifndef XCHAT_SERVLIST_H
#define XCHAT_SERVLIST_H

#include <list>

typedef struct ircserver
{
	char *hostname;
} ircserver;

typedef struct ircnet
{
	char *name;
	char *nick[3];
	char *user;
	char *real;
	char *pass;
	char *autojoin;
	char *command;
	char *nickserv;
	char *comment;
	char *encoding;
	GSList *servlist;
	int selected;
	unsigned int flags;
} ircnet;

extern GSList *network_list;
extern std::list<ircserver*> srvlist;

#define FLAG_CYCLE				1
#define FLAG_USE_GLOBAL			2
#define FLAG_USE_SSL			4
#define FLAG_AUTO_CONNECT		8
#define FLAG_USE_PROXY			16
#define FLAG_ALLOW_INVALID		32
#define FLAG_FAVORITE			64	// only used by XChat-Aqua for now
#define FLAG_COUNT				7

void servlist_init();
int servlist_save();
int servlist_cycle(server *serv);
void servlist_connect(session *sess, ircnet *net, bool join);
int servlist_connect_by_netname(session *sess, char *network, bool join);
int servlist_auto_connect(session *sess);
int servlist_have_auto();
int servlist_check_encoding(char *charset);
void servlist_cleanup();

ircnet *servlist_net_add(char *name, char *comment, int prepend);
void servlist_net_remove(ircnet *net);
ircnet *servlist_net_find(char *name, int *pos, int (*cmpfunc)(const char*, const char*));
ircnet *servlist_net_find_from_server(char *server_name);

ircserver *servlist_server_add(ircnet *net, char *name);
void servlist_server_remove(ircnet *net, ircserver *serv);
ircserver *servlist_server_find(ircnet *net, char *name, int *pos);

void joinlist_split(char *autojoin, GSList **channels, GSList **keys);
bool joinlist_is_in_list(server *serv, char *channel);
void joinlist_free(GSList *channels, GSList *keys);
char *joinlist_merge(GSList *channels, GSList *keys);

#endif
