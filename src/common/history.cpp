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
#include <string.h>
#include "history.h"

void history_add(history *his, char *text)
{
	if (his->lines[his->realpos])
		free(his->lines[his->realpos]);
	his->lines[his->realpos] = strdup(text);
	his->realpos++;
	if (his->realpos == HISTORY_SIZE)
		his->realpos = 0;
	his->pos = his->realpos;
}

void history_free(history *his)
{
	int i;
	for (i = 0; i < HISTORY_SIZE; i++)
	{
		if (his->lines[i])
		{
			free(his->lines[i]);
			his->lines[i] = 0;
		}
	}
}

char* history_down(history *his)
{
	int next;

	if (his->pos == his->realpos)	// allow down only after up
		return 0;
	if (his->realpos == 0)
	{
		if (his->pos == HISTORY_SIZE - 1)
		{
			his->pos = 0;
			return "";
		}
	} else
	{
		if (his->pos == his->realpos - 1)
		{
			his->pos++;
			return "";
		}
	}

	next = 0;
	if (his->pos < HISTORY_SIZE - 1)
		next = his->pos + 1;

	if (his->lines[next])
	{
		his->pos = next;
		return his->lines[his->pos];
	}

	return 0;
}

char* history_up(history *his, char *current_text)
{
	int next;

	if (his->realpos == HISTORY_SIZE - 1)
	{
		if (his->pos == 0)
			return 0;
	}
	else
	{
		if (his->pos == his->realpos + 1)
			return 0;
	}

	next = HISTORY_SIZE - 1;
	if (his->pos != 0)
		next = his->pos - 1;

	if (his->lines[next])
	{
		if
		(
			current_text[0] && strcmp(current_text, his->lines[next]) &&
			(!his->lines[his->pos] || strcmp(current_text, his->lines[his->pos])) &&
			(!his->lines[his->realpos] || strcmp(current_text, his->lines[his->pos]))
		)
		{
			history_add(his, current_text);
		}
		
		his->pos = next;
		return his->lines[his->pos];
	}

	return 0;
}
