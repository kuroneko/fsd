#include <cmath>

#include "manage.h"
#include "fsdpaths.h"

#ifndef GLOBALHH
#define GLOBALHH

#ifdef WIN32
	#define STRCASECMP(a,b)	_stricmp(a,b)
	#define WRITESOCK(a,b,c) send(a,b,c,0) 
	#define READSOCK(a,b,c) recv(a,b,c,0) 
	#define socklen_t	int
	#define CLOSESOCKET(a) closesocket(a) 
#else
	#define STRCASECMP(a,b)	strcasecmp(a,b)
	#define WRITESOCK(a,b,c) write(a,b,c) 
	#define READSOCK(a,b,c) read(a,b,c) 
	#define CLOSESOCKET(a) close(a) 
#endif

#define PRODUCT "FSFDT Windows FSD Beta from FSD V3.000 draft 9"
#define VERSION "V3.000 d9"
#define NEEDREVISION 9

/*
    WARNING!!!: The USERTIMEOUT (idle time of a SOCKET before it's dropped)
                should not be higher than the SERVERTIMEOUT (idle time of a
                server)
*/
#define USERTIMEOUT 500
#define SERVERTIMEOUT 800
#define CLIENTTIMEOUT 800
#define SILENTCLIENTTIMEOUT 36000
#define WINDDELTATIMEOUT 70

#define USERPINGTIMEOUT 200
#define USERFEEDCHECK 3
#define LAGCHECK 60
#define NOTIFYCHECK 300
#define SYNCTIMEOUT 120
#define SERVERMAXTOOK 240
#define MAXHOPS 10
#define GUARDRETRY 120
#define CALLSIGNBYTES 12
#define MAXLINELENGTH 512
#define MAXMETARDOWNLOADTIME 900
#define CERTFILECHECK 120

#define WHAZZUPCHECK 30
#define CONNECTDELAY 20

#define LEV_SUSPENDED           0
#define LEV_OBSPILOT            1
#define LEV_STUDENT1            2
#define LEV_STUDENT2            3
#define LEV_STUDENT3            4
#define LEV_CONTROLLER1         5
#define LEV_CONTROLLER2         6
#define LEV_CONTROLLER3         7
#define LEV_INSTRUCTOR1         8
#define LEV_INSTRUCTOR2         9
#define LEV_INSTRUCTOR3         10
#define LEV_SUPERVISOR          11
#define LEV_ADMINISTRATOR       12
#define LEV_MAX                 12

#endif
