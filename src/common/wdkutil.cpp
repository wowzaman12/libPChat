#ifdef WIN32
#include <io.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#endif
int
portable_mode ()
{
#ifdef WIN32
	if ((_access( "portable-mode", 0 )) != -1)
#else
	if ((access( "portable-mode", 0 )) != -1)
#endif
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
