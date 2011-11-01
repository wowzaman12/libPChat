#ifndef XCHAT_HISTORY_H
#define XCHAT_HISTORY_H

#define HISTORY_SIZE 100

struct history
{
	char *lines[HISTORY_SIZE];
	int pos;
	int realpos;
};

void history_add(history *his, char *text);
void history_free(history *his);
char *history_up(history *his, char *current_text);
char *history_down(history *his);

#endif
