// $Id: core.c,v 1.2 2004/01/09 03:00:18 rovert Exp $
// original : core.c 2003/02/26 18:03:12 Rev 1.7

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "core.h"
#include "socket.h"
#include "timer.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static void (*term_func)(void)=NULL;

/*======================================
 *	CORE : Set function
 *--------------------------------------
 */
void set_termfunc(void (*termfunc)(void))
{
	term_func = termfunc;
}

/*======================================
 *	CORE : Signal Sub Function
 *--------------------------------------
 */

static void sig_proc(int sn)
{
	int i;
	switch(sn){
	case SIGINT:
	case SIGTERM:
		if(term_func)
			term_func();
		for(i=0;i<fd_max;i++){
			if(!session[i])
				continue;
			close(i);
		}
		exit(0);
		break;
	}
}



/*======================================
 *	CORE : MAINROUTINE
 *--------------------------------------
 */

int main(int argc,char **argv)
{
	int next;

	printf("\033[2J"); // Clear the screen.
	printf("______  __           __ \n");
	printf("/\\  _  \\/\\ \\         /\\ \\ \n");
	printf("\\ \\ \\ \\ \\ \\ \\   _____\\ \\ \\___      __ \n");
	printf(" \\ \\  __ \\ \\ \\ /\\  _ `\\ \\  _ `\\  /'__`\\ \n");
	printf("  \\ \\ \\/\\ \\ \\ \\_ \\ \\ \\ \\ \\ \\ \\ \\/\\ \\ \\.\\_ \n");
	printf("   \\ \\_\\ \\_\\ \\__\\ \\  __/\\ \\_\\ \\_\\ \\__/.\\_\\ \n");
	printf("    \\/_/\\/_/\\/__/\\ \\  _/ \\/_/\\/_/\\/__/\\/_/ \n");
	printf("                  \\ \\_\\ \n");
	printf("                   \\/_/ \n\n");
	printf("  Alpha rev159v1.2.0 (< ..................... >)\n\n");

	do_socket();
	signal(SIGPIPE,SIG_IGN);
	signal(SIGTERM,sig_proc);
	signal(SIGINT,sig_proc);
	do_init(argc,argv);
	while(1){
		next=do_timer(gettick_nocache());
		do_sendrecv(next);
		do_parsepacket();
	}
	return 0;
}
