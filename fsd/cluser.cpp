#include <cstdio>
#include <cstdlib>
#ifndef WIN32
	#include <unistd.h>
#endif
#include <cctype>
#include <cstring>

#include "global.h"
#include "cluser.h"
#include "support.h"
#include "server.h"
#include "fsd.h"
#include "protocol.h"
#include "mm.h"
#include "fsdpaths.h"

/* The client communication command names */
const char *clcmdnames[]=
{
   "#AA",
   "#DA",
   "#AP",
   "#DP",
   "$HO",
   "#TM",
   "#RW",
   "@",
   "%",
   "$PI",
   "$PO",
   "$HA",
   "$FP",
   "#SB",
   "#PC",
   "#WX",
   "#CD",
   "#WD",
   "#TD",
   "$C?",
   "$CI",
   "$AX",
   "$AR",
   "$ER",
   "$CQ",
   "$CR",
   "$!!",
   "#DL",
   NULL
};

const char *errstr[]=
{
   "No error",
   "Callsign in use",
   "Invalid callsign",
   "Already registerd",
   "Syntax error",
   "Invalid source callsign",
   "Invalid CID/password",
   "No such callsign",
   "No flightplan",
   "No such weather profile",
   "Invalid protocol revision",
   "Requested level too high",
   "Too many clients connected",
   "CID/PID was suspended"
};

/* The client user */
cluser::cluser(int fd, clinterface *p, char *pn, int portnum, int gg):
   absuser(fd,p,pn,portnum, gg)
{
   parent=p;
   thisclient=NULL;
   configgroup *gu=configman->getgroup("system");
   configentry *e=gu?gu->getentry("maxclients"):(configentry*)NULL;
   int total=manager->getvar(p->varcurrent)->value.number;
   if (e&&atoi(e->getdata())<=total)
   {
      showerror(ERR_SERVFULL, "");
      kill(KILL_COMMAND);
   }
}
cluser::~cluser()
{
   if (thisclient) 
   {
      int type=thisclient->type;
      serverinterface->sendrmclient(NULL,"*",thisclient, this);
      delete thisclient;
   }
}
void cluser::readmotd()
{
   FILE *io=fopen(PATH_FSD_MOTD,"r");
   char line[1000];
 	sprintf(line, "%s", PRODUCT);
	clientinterface->sendgeneric(thisclient->callsign, thisclient, NULL,
	  NULL, "server", line, CL_MESSAGE);
  if (!io) return;
  while (fgets(line,1000,io))
   {
      line[strlen(line)-1]='\0';
      clientinterface->sendgeneric(thisclient->callsign, thisclient, NULL,
         NULL, "server", line, CL_MESSAGE);
   }
   fclose(io);
}
void cluser::parse(char *s)
{
   setactive();
   doparse(s);
}
/* Checks if the given callsign is OK. returns 0 on success or errorcode
   on failure */
int cluser::callsignok(char *name)
{
   client *temp;
   if (strlen(name)<2||strlen(name)>CALLSIGNBYTES) return ERR_CSINVALID;
   if (strpbrk(name, "!@#$%*:& \t")) return ERR_CSINVALID;
   for (temp=rootclient;temp;temp=temp->next)
      if (!STRCASECMP(temp->callsign,name)) return ERR_CSINUSE;
   return ERR_OK;
}
int cluser::checksource(char *from)
{
   if (STRCASECMP(from,thisclient->callsign))
   {
      showerror(ERR_SRCINVALID, from);
      return 0;
   }
   return 1;
}
int cluser::getcomm(char *cmd)
{
   int index;
   for (index=0;clcmdnames[index];index++)
      if (!strncmp(cmd,clcmdnames[index],strlen(clcmdnames[index])))
      return index;
   return -1;
}
int cluser::showerror(int num, char *env)
{
   uprintf("$ERserver:%s:%03d:%s:%s\r\n",thisclient?thisclient->callsign:
      "unknown",num,env,errstr[num]);
   return num;
}
int cluser::checklogin(char *id, char *pwd, int req)
{
   if (id[0]=='\0') return -2;
   int max, ok=maxlevel(id, pwd, &max);
   if (!ok)
   {
      showerror(ERR_CIDINVALID, id);
      return -1;
   }
   return req>max?max:req;
}
void cluser::execaa(char **s, int count)
{
   if (thisclient)
   {
      showerror(ERR_REGISTERED, "");
      return;
   }
   if (count<7)
   {
      showerror(ERR_SYNTAX, "");
      return;
   }
   int err=callsignok(s[0]);
   if (err)
   {
      showerror(err, "");
      kill(KILL_COMMAND);
      return;
   }
   if (atoi(s[6])!=NEEDREVISION)
   {
      showerror(ERR_REVISION, "");
      kill(KILL_PROTOCOL);
      return;
   }
   int req=atoi(s[5]);
   if (req<0) req=0;
   int level=checklogin(s[3], s[4], req);
   if (level==0)
   {
      showerror(ERR_CSSUSPEND, "");
      kill(KILL_COMMAND);
      return;
   }
   else if (level==-1)
   {
      kill(KILL_COMMAND);
      return;
   }
   else if (level==-2) level=1;
   if (level<req)
   {
      showerror(ERR_LEVEL, s[5]);
      kill(KILL_COMMAND);
      return;
   }
   thisclient=new client(s[3], myserver, s[0], CLIENT_ATC, level, s[6], s[2],
      -1);
   serverinterface->sendaddclient("*",thisclient, NULL, this, 0);
   readmotd();
}
void cluser::execap(char **s, int count)
{
   if (thisclient)
   {
      showerror(ERR_REGISTERED, "");
      return;
   }
   if (count<8)
   {
      showerror(ERR_SYNTAX, "");
      return;
   }
   int err=callsignok(s[0]);
   if (err)
   {
      showerror(err, "");
      kill(KILL_COMMAND);
      return;
   }
   if (atoi(s[5])!=NEEDREVISION)
   {
      showerror(ERR_REVISION, "");
      kill(KILL_PROTOCOL);
      return;
   }
   int req=atoi(s[4]);
   if (req<0) req=0;
   int level=checklogin(s[2], s[3], req);
   if (level<0)
   {
      kill(KILL_COMMAND);
      return;
   }
   else if (level==0)
   {
      showerror(ERR_CSSUSPEND, "");
      kill(KILL_COMMAND);
      return;
   }
   if (level<req)
   {
      showerror(ERR_LEVEL, s[4]);
      kill(KILL_COMMAND);
      return;
   }
   thisclient=new client(s[2], myserver, s[0], CLIENT_PILOT, level, s[4], s[7],
      atoi(s[6]));
   serverinterface->sendaddclient("*",thisclient, NULL, this, 0);
   readmotd();
}
void cluser::execmulticast(char **s, int count, int cmd, int nargs, int multiok)
{
   nargs+=2;
   if (count<nargs)
   {
      showerror(ERR_SYNTAX, "");
      return;
   }
   char *from, *to, data[1000]="";
   catcommand(s+2, count-2, data);
   from=s[0], to=s[1];
   if (!checksource(from)) return;
   serverinterface->sendmulticast(thisclient, to, data, cmd, multiok, this);
}
void cluser::execd(char **s, int count)
{
   if (count==0)
   {
      showerror(ERR_SYNTAX, "");
      return;
   }
   if (!checksource(s[0])) return;
   kill(KILL_COMMAND);
}
void cluser::execpilotpos(char **array, int count)
{
   if (count<10)
   { 
      showerror(ERR_SYNTAX, "");
      return;
   }
   if (!checksource(array[1])) return;
   thisclient->updatepilot(array);
   serverinterface->sendpilotdata(thisclient, this);
}
void cluser::execatcpos(char **array, int count)
{
   if (count<8)
   { 
      showerror(ERR_SYNTAX, "");
      return;
   }
   if (!checksource(array[0])) return;
   thisclient->updateatc(array+1);
   serverinterface->sendatcdata(thisclient, this);
}
void cluser::execfp(char **array, int count)
{
   if (count<17)
   {
      showerror(ERR_SYNTAX, "");
      return;
   }
   if (!checksource(array[0])) return;
   thisclient->handlefp(array+2);
   serverinterface->sendplan("*", thisclient, NULL);
}
void cluser::execweather(char **array, int count)
{
   if (count<3)
   {
      showerror(ERR_SYNTAX, "");
      return;
   }
   if (!checksource(array[0])) return;
   char source[CALLSIGNBYTES+2];
   sprintf(source, "%%%s", thisclient->callsign);
   metarmanager->requestmetar(source, array[2], 1, -1);
}
void cluser::execacars(char **array, int count)
{
   if (count<3)
   {
      showerror(ERR_SYNTAX, "");
      return;
   }
   if (!checksource(array[0])) return;
   if (!STRCASECMP(array[2],"METAR")&&count>3)
   {
      char source[CALLSIGNBYTES+2];
      sprintf(source, "%%%s", thisclient->callsign);
      metarmanager->requestmetar(source, array[3], 0, -1);
   }
}
void cluser::execcq(char **array, int count)
{
   if (count < 3 )
   {
      showerror(ERR_SYNTAX, "");
      return;
   }
   if (STRCASECMP(array[1], "server"))
   { 
      execmulticast(array, count, CL_CQ, 1, 1);
      return;
   }
   if (!STRCASECMP(strupr(array[2]), "RN"))
   {
	client *cl=getclient(array[1]);
	if (cl)
	{
		char data[1000];
		sprintf(data, "%s:%s:RN:%s:USER:%d", cl->callsign, thisclient->callsign, cl->realname,cl->rating);
		clientinterface->sendpacket(thisclient,cl,NULL,CLIENT_ALL,-1,CL_CR,data);
		return;
	}
   }
   if (!STRCASECMP(array[2], "fp"))
   { 
      client *cl=getclient(array[3]);
      if (!cl)
      {
         showerror(ERR_NOSUCHCS, array[3]);
         return;
      }
      if (!cl->plan)
      {
         showerror(ERR_NOFP, "");
         return;
      }
      clientinterface->sendplan(thisclient, cl, -1);
      return;
   }
}

void cluser::execkill(char ** array, int count)
{
   char junk[64];
   if (count < 3 )
   {
     showerror(ERR_SYNTAX, "");
     return;
   }
   client *cl=getclient(array[1]);
   if (!cl)
   {
     showerror(ERR_NOSUCHCS, array[1]);
     return;
   }

   if (thisclient->rating<11)
   {
	sprintf(junk, "You are not allowed to kill users!");
	clientinterface->sendgeneric(thisclient->callsign, thisclient, NULL,
		NULL, "server", junk, CL_MESSAGE);
	sprintf(junk,"%s attempted to remove %s, but was not allowed to",thisclient->callsign,array[1]);
	dolog(L_ERR,junk);
   }
   else
   {
	sprintf(junk, "Attempting to kill %s", array[1]);
	clientinterface->sendgeneric(thisclient->callsign, thisclient, NULL,
		NULL, "server", junk, CL_MESSAGE);
        sprintf(junk,"%s Killed %s",thisclient->callsign,array[1]);
	dolog(L_INFO,junk);
	serverinterface->sendkill(cl,array[2]);
   }
   return;
}
void cluser::doparse(char *s)
{
   char cmd[4], *array[100];
   snappacket(s, cmd, 3);
   int index=getcomm(cmd), count;
   if (index==-1)
   {
      showerror(ERR_SYNTAX, "");
      return;
   }
   if (!thisclient&&index!=CL_ADDATC&&index!=CL_ADDPILOT) return;

   /* Just a hack to put the pointer on the first arg here */
   s+=strlen(clcmdnames[index]);
   count=breakpacket(s,array,100);
   switch (index)
   {
      case CL_ADDATC     : execaa(array,count);  break;
      case CL_ADDPILOT   : execap(array,count);  break;
      case CL_PLAN       : execfp(array,count); break;
      case CL_RMATC      : /* Handled like RMPILOT */
      case CL_RMPILOT    : execd(array,count); break;
      case CL_PILOTPOS   : execpilotpos(array,count); break;
      case CL_ATCPOS     : execatcpos(array,count); break;
      case CL_PONG       :
      case CL_PING       : execmulticast(array,count,index,0,1); break;
      case CL_MESSAGE    : execmulticast(array,count,index,1,1); break; 
      case CL_REQHANDOFF :
      case CL_ACHANDOFF  : execmulticast(array,count,index,1,0); break;
      case CL_SB         :
      case CL_PC         : execmulticast(array,count,index,0,0); break;
      case CL_WEATHER    : execweather(array, count); break;
      case CL_REQCOM     : execmulticast(array,count,index,0,0); break;
      case CL_REPCOM     : execmulticast(array,count,index,1,0); break;
      case CL_REQACARS   : execacars(array, count); break;
      case CL_CR         : execmulticast(array, count, index, 2, 0); break;
      case CL_CQ         : execcq(array, count); break;
      case CL_KILL       : execkill(array, count); break;
      default            : showerror(ERR_SYNTAX, ""); break;
   }
}
