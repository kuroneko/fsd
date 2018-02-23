#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <cstdlib>
#ifndef WIN32
	#include <unistd.h>
#else
	#include <io.h>
#endif
#include <cctype>
#include <cstring>

#include "global.h"
#include "sysuser.h"
#include "wprofile.h"
#include "support.h"
#include "server.h"
#include "client.h"
#include "fsd.h"
#include "protocol.h"
#include "certificate.h"
#include "mm.h"
#include "cluser.h"
#include "servuser.h"
#include "config.h"
#include "fsdpaths.h"

const char *syscmds[]=
{
   "servers",
   "info",
   "ping",
   "connect",
   "time",
   "route",
   "weather",
   "disconnect",
   "help",
   "quit",
   "stat",
   "say",
   "clients",
   "cert",
   "pwd",
   "distance",
   "range",
   "log",
   "wall",
   "delguard",
   "metar",
   "wp",
   "kill",
   "pos",
   "dump",
   "servers2",
    "refreshmetar",
    NULL
};

int needauthorization[]=
{
  0,          /* servers */
  1,          /* info    */
  1,          /* ping    */
  1,          /*  connect */
  0,          /* time    */
  0,          /* route   */
  0,          /* weather  */
  1,          /* disconnect */
  0,          /* help    */
  0,          /* quit  */
  1,          /* stat */
  1,          /* say */
  1,          /*  clients */
  1,          /* cert */
  0,          /* pwd  */
  1,          /*  distance */
  1,          /*  range */
  1,          /* log */
  1,          /* wall */
  1,          /* delguard */
  0,          /* metar */
  1,          /* wp */
  1,          /* kill */
  0,          /* pos */
  1,          /* dump */
  0,          /* servers2 */
  1,          /* refresh metar */
  0
};
/* The client user */

sysuser::sysuser(int fd, sysinterface *p, char *pn, int portnum, int g):
   absuser(fd,p,pn,portnum,g)
{
   int logs[L_MAX], x;
   lat=lon=0.0;
   memset(logs,0,sizeof(int)*L_MAX);
   for (x=0;x<MAXLOG;x++) if (loghistory[x].msg)
      logs[loghistory[x].level]++;
   uprintf("# " PRODUCT " system interface ready.\r\n");
   uprintf("# Logs: Emerg: %d, Alert: %d, Crit: %d, Err: %d, Warn: %d, Info:"\
      " %d, Debug: %d\r\n", logs[0], logs[1], logs[2], logs[3], logs[4],
      logs[5], logs[6]);
   uprintf("# Type 'help' for help.\r\n");
   parent=p;

   configgroup *sgroup=configman->getgroup("system");
   configentry *sentry=sgroup?sgroup->getentry("password"):(configentry*)NULL;
   char *buf=sentry?sentry->getdata():(char*)NULL;
   authorized=(buf==NULL||buf[0]=='\0');
}
sysuser::~sysuser()
{
}
void sysuser::parse(char *s)
{
   setactive();
   doparse(s);
}
void sysuser::information()
{
   int servers=0, clients=0, certs=0;
   server *tempserver;
   client *tempclient;
   certificate *tempcert;
   for (tempserver=rootserver;tempserver;tempserver=tempserver->next) 
      servers++;
   for (tempclient=rootclient;tempclient;tempclient=tempclient->next) 
      clients++;
   for (tempcert=rootcert;tempcert;tempcert=tempcert->next) 
      certs++;
   uprintf("Number of servers in the network: %d\r\n",servers);
   uprintf("Number of clients in the network: %d\r\n",clients);
   uprintf("Number of certificates in the network: %d\r\n",certs);
}
void sysuser::usage(int cmd, char *string)
{
   uprintf("Usage: %s %s\r\n", syscmds[cmd], string);
}
void sysuser::exechelp(char **array, int count)
{
   int copymode=0, topicmode=0, globalmode=0;
   char topic[100],line[100];
   char *s=(count>0)?array[0]:(char *)NULL;
   if (s) strcpy(topic,s); else
   {
      strcpy(topic,"global");
      globalmode=1;
   }
   FILE *io=fopen(PATH_FSD_HELP,"r"); 
   if (!io)
   {
      uprintf("Could not open help file 'help.txt'\r\n");
      return;
   }
   if (!strcmp(topic,"topics")) topicmode=1;
   while (fgets(line,100,io))
   {
      if (strncmp(line,"%topic",6)==0)
      {
         char top[100], *p;
         if (copymode) break;
         if (sscanf(line,"%%topic %s",top)!=1) continue;
         p=strchr(top,':');
         if (p) *(p++)='\0';
         p=strchr(line,':');
         if (p) *(p++)='\0';
         if (topicmode)
         {
            uprintf("%s: %s",top,p);
            continue;
         }
         if (STRCASECMP(top,topic)==0)
         {
            int x;
            char line[100];
            char *header="FSD ONLINE DOCUMENTATION";
            if (p==NULL) p="";
            uprintf("%s %55s",header,p);
            for (x=0;x<79;x++) line[x]='=';
            line[79]='\0';
            uprintf("%s\r\n",line);
            copymode=1;
         }
      } else if (copymode) uprintf("%s",line);
   }
   fclose(io);
   if (globalmode)
   {
      char line[100]="";
      int count=0;
      for (count=0;syscmds[count];count++)
      {
         if (line[0]) strcat(line," ");
         strcat(line,syscmds[count]);
         if (strlen(line)>65)
         {
            uprintf("%s\r\n",strupr(line));
            line[0]='\0';
         }
      }
      if (line[0]) uprintf("%s\r\n",strupr(line));
   }
}
void sysuser::list(char *s)
{
   char buf[100];
   while (isspace(*s)) s++;
   if (!*s) s=NULL;
   int nvars=manager->getnvars();
   for (int x=0;x<nvars;x++) if (manager->sprintvalue(x,buf))
   {
      char *varname=manager->getvar(x)->name;
      if (!s||strstr(varname,s)) uprintf("%s=%s\r\n",varname, buf);
   }
}
void sysuser::doparse(char *s)
{
   char *array[100];
   int index;
   int count=breakargs(s,array,100);
   if (count==0) return;
   for (index=0;syscmds[index];index++)
   {
      if (!STRCASECMP(array[0],syscmds[index]))
      {
         if (needauthorization[index]&&!authorized)
            uprintf("Not authorized, use 'pwd'.\r\n");
         else runcmd(index,array+1,count-1);
         return;
      }
   }
   uprintf("Syntax error.\r\n");
}
void sysuser::runcmd(int num, char **array, int count)
{
   switch (num)
   {
      case SYS_SERVERS: execservers(array,count, 1); break;
      case SYS_SERVERS2: execservers(array,count, 2); break;
      case SYS_INFORMATION: information(); break;
      case SYS_PING   : execping(array,count); break;
      case SYS_CONNECT: execconnect(array,count); break;
      case SYS_ROUTE  : execroute(array,count); break;
      case SYS_DISCONNECT: execdisconnect(array,count); break;
      case SYS_HELP   : exechelp(array,count); break;
      case SYS_QUIT   : kill(KILL_COMMAND); break;
      case SYS_STAT   : list((char*)(count==0?"":array[0])); break;
      case SYS_SAY    : execsay(array,count); break;
      case SYS_CLIENTS: execclients(array,count); break;
      case SYS_CERT   : execcert(array,count); break;
      case SYS_DISTANCE: execdistance(array,count); break;
      case SYS_TIME   : exectime(array,count); break;
      case SYS_WEATHER: execweather(array,count); break;
      case SYS_RANGE  : execrange(array,count); break;
      case SYS_LOG    : execlog(array,count); break;
      case SYS_PWD    : execpwd(array,count); break;
      case SYS_WALL   : execwall(array,count); break;
      case SYS_DELGUARD: execdelguard(array,count); break;
      case SYS_METAR  : execmetar(array,count); break;
      case SYS_WP     : execwp(array, count); break;
      case SYS_KILL   : execkill(array, count); break;
      case SYS_DUMP   : execdump(array, count); break;
      case SYS_POS    : execpos(array, count); break;
      case SYS_REFMETAR: execrefmetar(array, count); break;
   }
}
void sysuser::execconnect(char **array, int count)
{
   if (count<1)
   {
      if (serverinterface->rootuser==NULL)
         uprintf("No active server connections.\r\n");
      else
      {
         uprintf("fd  Limit  Out-Q  In-Q  Feed Peer\r\n");
         uprintf("---------------------------------\r\n");
         absuser *temp;
         for (temp=serverinterface->rootuser;temp;temp=temp->next)
         {
            servuser *su=(servuser*)temp;
            uprintf("%02d %5d   %5d %5d %5d %s:%d (%s)\r\n",temp->fd,
               temp->outbufsoftlimit, strlen(temp->outbuf), strlen(temp->inbuf),
               temp->feed, temp->peer,temp->port, su->thisserver?
               su->thisserver->ident:"<Unknown>");
         }
      }
      if (serverinterface->rootgs==NULL)
         uprintf("No pending connections.\r\n");
      else
      {
         guardstruct *temp;
         uprintf("Pending connections:\r\n");
         for (temp=serverinterface->rootgs;temp;temp=temp->next)
            uprintf("  %s:%d\r\n", temp->host, temp->port);
      }
      return;
   }
   int port=3011;
   if (count>1) port=atoi(array[1]);
   if (serverinterface->adduser(array[0],port,this)==1)
      serverinterface->sendservernotify("*", myserver, NULL);
}
void sysuser::execdisconnect(char **array, int count)
{
   if (count==0)
   {
      usage(SYS_DISCONNECT,"<fd>");
      return;
   }
   int fd=atoi(array[0]);
   absuser *temp;
   for (temp=serverinterface->rootuser;temp;temp=temp->next) if (temp->fd==fd)
   {
      temp->kill(KILL_COMMAND);
      uprintf("Disconnected\r\n");
      return;
   }
   uprintf("No such connection was found.\r\n");
}
void sysuser::execping(char **array, int count)
{
   if (count==0)
   {
      usage(SYS_PING,"<ident>");
      return;
   }
   time_t now=mtime();
   char data[100];
   sprintf(data,"%d %lu",fd,now);
   serverinterface->sendping(array[0],data);
}
void sysuser::execservers(char **array, int count, int type)
{
   server *temp, *match=NULL;
   if (count==1)
   {
      for (temp=rootserver;temp;temp=temp->next)
         if (strstr(temp->name,array[0])||strstr(temp->ident,array[0]))
      {
         if (match) break;
         match=temp;
      }
      if (!temp&&match)
      {
         char flags[100]="";
         uprintf("ID      : %s\r\n", match->ident); 
         uprintf("Name    : %s\r\n", match->name); 
         uprintf("Version : %s\r\n", match->version); 
         uprintf("Mail    : %s\r\n", match->email); 
         uprintf("Host    : %s\r\n", match->hostname); 
         uprintf("Location: %s\r\n", match->location); 
         uprintf("Hops    : %d\r\n", match->hops); 
         uprintf("Lag     : %d seconds\r\n", match->lag); 
         if (match->flags&SERVER_SILENT) strcpy(flags, "SILENT");
         if (match->flags&SERVER_METAR)
         {
            if (flags[0]) strcat(flags,", ");
            strcat(flags, "METAR");
         }
         uprintf("Flags  : %s\r\n", flags); 
         return;
      }
   }
   if (type==1)
   {
      uprintf("ID         Host                      Fl Hops Lag  Name\r\n");
      uprintf("-------------------------------------------------------------\r\n");
      for (temp=rootserver;temp;temp=temp->next)
         if (count==0||strstr(temp->name,array[0])||
         strstr(temp->ident,array[0]))
         uprintf("%-10s %-25s %c%c   %02d  %02d  %s\r\n",temp->ident,
         temp->hostname, temp->flags&SERVER_METAR?'M':'-',
         temp->flags&SERVER_SILENT?'S':'-',
         temp->hops, temp->lag, temp->name);
         uprintf("\r\n");
   } else
   {
      uprintf("ID         Version     Mail                 Location\r\n");
      uprintf("-------------------------------------------------------------\r\n");
      for (temp=rootserver;temp;temp=temp->next)
         if (count==0||strstr(temp->name,array[0])||
         strstr(temp->ident,array[0]))
         uprintf("%-10s %-11s %-20s %s\r\n",temp->ident,
         temp->version, temp->email, temp->location);
         uprintf("\r\n");
   }
}
void sysuser::execroute(char **array, int count)
{
   server *temp;
   char dest[1000];
   uprintf("ID         Next hop\r\n");
   uprintf("-------------------\r\n");
   for (temp=rootserver;temp;temp=temp->next)
      if (count==0||strstr(temp->name,array[0])||strstr(temp->ident,array[0]))
   {
      if (temp==myserver) strcpy(dest,"*local*"); else
      if (!temp->path) strcpy(dest,"*broadcast*"); else
         sprintf(dest,"%s:%d",temp->path->peer,temp->path->port);
      uprintf("%-10s %s\r\n",temp->ident,dest);
   }
   uprintf("\r\n");
}
void sysuser::execsay(char **array, int count)
{
   if (count<2)
   {
      usage(SYS_SAY,"<user> <text>");
      return;
   }
   char buf[1024];
   catargs(array+1,count-1,buf);
   if (!serverinterface->sendmulticast(NULL,array[0],buf,CL_MESSAGE,1, NULL))
   {
      uprintf("client '%s' unknown.\r\n",array[0]);
      return;
   }
}
void sysuser::execclients(char **array, int count)
{
   client *temp=NULL;
   if (count>0)
   {
      client *match=NULL;
      for (temp=rootclient;temp;temp=temp->next)
         if (strstr(temp->callsign,array[0])||strstr(temp->cid,array[0]))
      {
         if (match) break;
         match=temp;
      }
      if (!temp)
      {
         if (!match)
         {
            uprintf("No match.\r\n");
            return;
         }
         char buf[100];
         uprintf("Callsign    : %s\r\n" ,match->callsign);
         if (match->realname[0])
            uprintf("Real name   : %s\r\n" ,match->realname);
         uprintf("Type        : %s\r\n", match->type==CLIENT_ATC?
            "atc":"pilot");
         uprintf("Online      : %s\r\n", sprinttime(mtime()-match->starttime,
            buf));
         uprintf("Idle        : %s\r\n", sprinttime(mtime()-match->alive,
            buf));
         if (match->frequency!=0 && match->frequency<100000)
            uprintf("Frequency   : 1%02d.%03d\r\n", match->frequency/1000, match->frequency%1000);
	 if (match->positionok)
	 {
	    uprintf("Location    : %s\r\n", printloc(match->lat, match->lon,
	       buf));
	    uprintf("Altitude    : %d\r\n", match->altitude);
	    if (match->type==CLIENT_PILOT)
	    {
	       uprintf("Groundspeed : %d\r\n", match->groundspeed);
	       uprintf("Transponder : %d\r\n", match->transponder);
	    } else
	    {
	       uprintf("Facility    : %d\r\n", match->facilitytype);
	    }
	 }
	 return;
      }
   }
   uprintf("Call-sign    CID     Online   Idle     Type  Server     Frequency\r\n");
   uprintf("-----------------------------------------------------------------\r\n");
   char time1[20], time2[20];
   for (temp=rootclient;temp;temp=temp->next)
      if (count==0||strstr(temp->callsign,array[0])||strstr(temp->cid,array[0]))
      {
         uprintf("%-10s   %-7s %s %s %-5s %-10s", temp->callsign, temp->cid,
            sprinttime(mtime()-temp->starttime,time1),
	    sprinttime(mtime()-temp->alive, time2),
	    temp->type==CLIENT_ATC?"ATC":"PILOT", temp->location->ident,
	    temp->frequency);
         if (temp->frequency!=0 && temp->frequency<100000)
            uprintf(" 1%02d.%03d", temp->frequency/1000, temp->frequency%1000);
         uprintf("%s\r\n", "");
      }
}
void sysuser::execcert(char **array, int count)
{
  if (count<2)
   {
      char buf1[100], buf2[100];
      certificate *c;
      uprintf("CID        Max level      Origin     Creation (UTC)       Last used (local)\r\n");
      uprintf("---------------------------------------------------------------------------\r\n");
      for (c=rootcert;c;c=c->next)
	 if (count==0||strstr(c->cid,array[0]))
	 uprintf("%-10s %-13s  %-10s %s  %s\r\n", c->cid, certlevels[c->level],
	    c->origin,sprintdate(c->creation, buf1), sprintdate(c->prevvisit,
	    buf2));
      return;
   }
   int mode;
   if (!STRCASECMP(array[0], "add")) mode=CERT_ADD; else
   if (!STRCASECMP(array[0], "delete")) mode=CERT_DELETE; else
   if (!STRCASECMP(array[0], "modify")) mode=CERT_MODIFY; else
   {
      uprintf("Invalid mode, should be 'add', 'delete' or 'modify'.\r\n");
      return;
   }
   int level, ok=0;
   certificate *c=getcert(array[1]);
   switch (mode)
   {
      case CERT_ADD:
         if (count<4)
         {
            usage(SYS_CERT, "add <cid> <pwd> <level>");
            return;
         }
         if (c)
         {
            uprintf("Certificate already exists.\r\n");
            return;
         }
         for (level=0;level<=LEV_MAX;level++)
            if (!STRCASECMP(array[3],certlevels[level]))
         {
            ok=1;
            break;
         }
         if (!ok)
         {
            uprintf("Unknown level, use 'help cert' for help.\r\n");
            return;
         }
         c=new certificate(array[1],array[2],level,mgmtime(),myserver->ident);
         serverinterface->sendcert("*", mode, c, NULL);
         break;
      case CERT_DELETE:
         if (!c)
         {
            uprintf("Certificate does not exist.\r\n");
            return;
         }
         serverinterface->sendcert("*", mode, c, NULL);
         delete c;
         break;
      case CERT_MODIFY:
         if (!c)
         {
            uprintf("Certificate does not exist.\r\n");
            return;
         }
         if (count<3)
         {
            usage(SYS_CERT, "modify <cid> <level>");
            return;
         }
         for (level=0;level<=LEV_MAX;level++)
            if (!STRCASECMP(array[2],certlevels[level]))
         {
            ok=1;
            break;
         }
         if (!ok)
         {
            uprintf("Unknown level, use 'help cert' for help.\r\n");
            return;
         }
         c->configure(NULL, level, mgmtime(), myserver->ident);
         serverinterface->sendcert("*", mode, c, NULL);
         break;
   }
   uprintf("Your change has been executed, but the certificate file\r\n");
   uprintf("has not been updated. Please update this file if your change\r\n");
   uprintf("should be permanent.\r\n");
}
void sysuser::execdistance(char **array, int count)
{
   if (count<2)
   {
      usage(SYS_DISTANCE,"<client1> <client2>");
      return;
   }
   client *c1=getclient(array[0]);
   if (!c1)
   {
      uprintf("client '%s' not found.\r\n",array[0]);
      return;
   }
   client *c2=getclient(array[1]);
   if (!c2)
   {
      uprintf("client '%s' not found.\r\n",array[1]);
      return;
   }
   if (!c1->positionok)
   {
      uprintf("client '%s' did not send a position report.\r\n",array[0]);
      return;
   }
   if (!c2->positionok)
   {
      uprintf("client '%s' did not send a position report.\r\n",array[1]);
      return;
   }
   uprintf("%f NM.\r\n",c1->distance(c2));
}
void sysuser::exectime(char **array, int count)
{
   time_t now=time(NULL);
   struct tm *p;
   p=localtime(&now);
   uprintf("%02d:%02d:%02d local\r\n", p->tm_hour, p->tm_min, p->tm_sec);
   p=gmtime(&now);
   uprintf("%02d:%02d:%02d UTC\r\n", p->tm_hour, p->tm_min, p->tm_sec);
}
void sysuser::execweather(char **array, int count)
{
   if (count<1)
   {
      usage(SYS_WEATHER,"<profile>");
      return;
   }
   server *s=metarmanager->requestmetar(myserver->ident, array[0], 1, fd);
   if (s) uprintf("Weather request sent to METAR host %s.\n\r",s->ident); else
      uprintf("No METAR host in network.\r\n");
}
void sysuser::printweather(wprofile *wp)
{
   uprintf("\r\nWeather profile %s:\r\n\r\n", wp->name);
   int x;
   char line[100], buf[100];

   sprintf(line,"%-20s%-10s%-10s","Cloud layer", "1", "2");
   uprintf("%s\r\n",line);
   sprintf(line,"%-20s","  Floor"); for (x=0;x<2;x++)
   {
      sprintf(buf, "%-10d", wp->clouds[x].floor);
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);
   sprintf(line,"%-20s","  Ceiling"); for (x=0;x<2;x++)
   {
      sprintf(buf, "%-10d", wp->clouds[x].ceiling);
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);
   sprintf(line,"%-20s","  Coverage"); for (x=0;x<2;x++)
   {
      sprintf(buf, "%-10d", wp->clouds[x].coverage);
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);
   sprintf(line,"%-20s","  Turbulence"); for (x=0;x<2;x++)
   {
      sprintf(buf, "%-10d", wp->clouds[x].turbulence);
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);
   sprintf(line,"%-20s","  Icing"); for (x=0;x<2;x++)
   {
      sprintf(buf, "%-10s", wp->clouds[x].icing?"Yes":"No");
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);


   sprintf(line,"%-20s%-10s%-10s%-10s%-10s","Wind layer", "1", "2", "3", "4");
   uprintf("\r\n%s\r\n",line);
   sprintf(line,"%-20s","  Floor"); for (x=0;x<4;x++)
   {
      sprintf(buf, "%-10d", wp->winds[x].floor);
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);
   sprintf(line,"%-20s","  Ceiling"); for (x=0;x<4;x++)
   {
      sprintf(buf, "%-10d", wp->winds[x].ceiling);
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);
   sprintf(line,"%-20s","  Direction"); for (x=0;x<4;x++)
   {
      sprintf(buf, "%-10d", wp->winds[x].direction);
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);
   sprintf(line,"%-20s","  Speed"); for (x=0;x<4;x++)
   {
      sprintf(buf, "%-10d", wp->winds[x].speed);
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);
   sprintf(line,"%-20s","  Gusting"); for (x=0;x<4;x++)
   {
      sprintf(buf, "%-10s", wp->winds[x].gusting?"Yes":"No");
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);

   sprintf(line,"%-20s%-10s%-10s%-10s%-10s","Temp layer", "1", "2", "3", "4");
   uprintf("\r\n%s\r\n",line);
   sprintf(line,"%-20s","  Ceiling"); for (x=0;x<4;x++)
   {
      sprintf(buf, "%-10d", wp->temps[x].ceiling);
      strcat(line, buf);
   }
   uprintf("%s\r\n", line);
   sprintf(line,"%-20s","  Temperature"); for (x=0;x<4;x++)
   {
      sprintf(buf, "%-10d", wp->temps[x].temp);
      strcat(line, buf);
   }
   uprintf("%s\r\n\r\n", line);

   uprintf("Visibility  : %.2f SM\r\n", wp->visibility);
   uprintf("Barometer   : %d Hg\r\n", wp->barometer);
   printprompt();
}
void sysuser::printmetar(char *wp, char *data)
{
   uprintf("\r\nWeather profile %s:\r\n\r\n", wp);
   uprintf("%s\r\n", data);
   printprompt();
}
void sysuser::execrange(char **array, int count)
{
   if (count<1)
   {
      usage(SYS_RANGE,"<callsign>");
      return;
   }
   client *temp=getclient(array[0]);
   if (!temp)
   {
      uprintf("No such callsign.\n\r");
      return;
   }
   uprintf("Range : %d NM.\r\n", temp->getrange());
}
void sysuser::execlog(char **array, int count)
{
   if (count<2)
   {
      usage(SYS_LOG,"(show|delete) <maxlevel> [amount]\r\n  Where level "
         "is 0=Emergency, 1=Alert, 2=Critical, 3=Error, 4=Warning, 5=Info\r\n"
         "  6=Debug");
      return;
   }
   int mode, maxlevel=atoi(array[1]), amount;
   if (count>2) amount=atoi(array[2]); else amount=MAXLOG;
   if (amount>MAXLOG) amount=MAXLOG;
   if (maxlevel>=L_MAX) maxlevel=L_MAX-1;
   if (!STRCASECMP(array[0],"show")) mode=1; else
   if (!STRCASECMP(array[0],"delete")) mode=2; else
   {
      uprintf("Invalid operation mode, use 'show' or 'delete'.\r\n");
      return;
   }
   int msgp=logp;
   while (amount)
   {
      if (--msgp<0) msgp+=MAXLOG;
      if (msgp==logp) break;
      if (loghistory[msgp].msg==NULL) break;
      if (loghistory[msgp].level>maxlevel) continue;
      if (mode==1) uprintf("%s\r\n", loghistory[msgp].msg); else
      {
         free(loghistory[msgp].msg);
         loghistory[msgp].msg=NULL;
      }
      amount--;
   } 
}
void sysuser::execpwd(char **array, int count)
{
   if (count<1)
   {
      usage(SYS_PWD,"<password>");
      return;
   }
   configgroup *sgroup=configman->getgroup("system");
   configentry *sentry=sgroup?sgroup->getentry("password"):(configentry*)NULL;
   char *buf=sentry?sentry->getdata():(char*)NULL;
   if (buf&&!strcmp(buf,array[0]))
      authorized=1;
   if (!authorized)
      uprintf("Password incorrect.\r\n");
   else uprintf("Password correct.\r\n");
}
void sysuser::execwall(char **array, int count)
{
   if (count<1)
   {
      usage(SYS_PWD,"<text>");
      return;
   }
   absuser *temp;
   char buf[1000];
   catargs(array, count, buf);
   for (temp=clientinterface->rootuser;temp;temp=temp->next)
   {
      cluser *ctemp=(cluser*)temp;
      client *c=ctemp->thisclient;
      if (!c) continue;
      clientinterface->sendgeneric(c->callsign, c, NULL, NULL, "server",
         buf, CL_MESSAGE);
   }
}
void sysuser::execdelguard(char **array, int count)
{
   serverinterface->delguard();
   uprintf("Guard connections deleted.\r\n");
}
void sysuser::execmetar(char **array, int count)
{
   if (count<1)
   {
      usage(SYS_METAR,"<profile>");
      return;
   }
   server *s=metarmanager->requestmetar(myserver->ident, array[0], 0, fd);
   if (s) uprintf("Metar request sent to METAR host %s.\n\r",s->ident); else
      uprintf("No METAR host in network.\r\n");
}
void sysuser::execwp(char **array, int count)
{
   char *mode=array[0];
   char *help="<show|create|delete|activate|set> <name> ...";
   wprofile *wp;
   if (count==0)
   {
      usage(SYS_WP, help);
      return;
   }
   if (!STRCASECMP(mode, "show"))
   {
      if (count==1)
      {
         char buf[100];
         uprintf("Active  Name  Origin     Creation\r\n");
         uprintf("---------------------------------\r\n");
         for (wp=rootprofile;wp;wp=wp->next)
            uprintf("%-6s  %-4s  %-10s %s\r\n", wp->active?"Yes":"No", wp->name,
            wp->origin?wp->origin:"<local>", sprintdate(wp->creation,buf));
      } else
      {
         wp=getwprofile(array[1]);
         if (!wp)
         {
            uprintf("Could not find weather profile '%s'\r\n", array[1]);
            return;
         } else printweather(wp);
      }
      return;
   }
   if (count<2)
   {
      usage(SYS_WP, help);
      return;
   }
   strupr(array[1]);
   wp=getwprofile(array[1]);
   if (!STRCASECMP(mode, "create"))
   {
      if (wp)
      {
         uprintf("Weather profile already exists!\r\n");
         return;
      }
      server *s=metarmanager->requestmetar(myserver->ident, array[1], 1, -2);
      if (s)
      {
         uprintf("A METAR request has been sent out to server %s to get the initial data.\r\n", s->ident);
         uprintf("The weather profile will created when the data arrives.\r\n");
      } else 
      {
         new wprofile(array[1], mgmtime(), myserver->ident);
         uprintf("No METAR host in network.\r\n");
        uprintf("The weather profile has been created without default data.\r\n");
      }
      return;
   }
   if (!wp)
   {
      uprintf("Could not find weather profile '%s'\r\n", array[1]);
      return;
   }
   if (!STRCASECMP(mode, "delete"))
   {
      if (wp->active)
         serverinterface->senddelwp(wp);
      delete wp;
      uprintf("Weather profile deleted.\r\n");
   } else if (!STRCASECMP(mode, "activate"))
   {
      wp->activate();
      wp->genrawcode();
      serverinterface->sendaddwp(NULL, wp);
      uprintf("Weather profile now in effect!\r\n");
   } else if (!STRCASECMP(mode, "set"))
   {
      char *p=array[2], *v=array[3];
      int val;
      if (sscanf(v, "%d", &val)==0)
      {
         uprintf("Invalid value '%s'\r\n", v);
      } else doset(wp, p, val);
   } else uprintf ("Invalid wp mode.\r\n");
}

void sysuser::doset(wprofile *wp, char *p, int val)
{
   int x, ok;
   char *cat1[]={"winds", "clouds", "temperature", "pressure", "visibility"};  
   char *where=strchr(p,'.'), *more;
   if (where) *(where++)='\0';
   ok=0;
   for (x=0;x<5;x++) if (!STRCASECMP(p, cat1[x]))
   {
      ok=1;
      break;
   }
   if (!ok)
   {
      uprintf("Invalid category '%s'.\r\n", p);
      return;
   }
   switch (x)
   {
      case 3:
         wp->barometer=val;
         return;
      case 4:
         wp->visibility=(float)val;
         return;
   }
   if (!where)
   {
      uprintf("Invalid parameter\r\n");
      return;
   }
   more=strchr(where,'.');
   if (more) *(more++)='\0';
   int index;
   if (sscanf(where, "%d", &index)==0)
   {
      uprintf("Invalid index '%s'\r\n", where);
      return;
   }
   index--;
   if (index<0||(x==1&&index>2)||index>4)
   {
      uprintf("Invalid index %d\r\n", index);
      return;
   }
   if (!more)
   {
      uprintf("Invalid parameter\r\n");
      return;
   }
   where=more;
   char *strings[10];
   switch (x)
   {
      case 0 :
         strings[0]="floor", strings[1]="ceiling", strings[2]="direction",
         strings[3]="speed", strings[4]="gusting", strings[5]=NULL; break;
      case 1 :
         strings[0]="floor", strings[1]="ceiling", strings[2]="coverage",
         strings[3]="turbulence", strings[4]="icing", strings[5]=NULL; break;
      case 2 :
         strings[0]="ceiling", strings[1]="temperature", strings[2]=NULL;
         break;
   }
   int y;
   ok=0;
   for (y=0;strings[y];y++) if (!STRCASECMP(where, strings[y]))
   {
      ok=1;
      break;
   }
   if (!ok)
   {
      uprintf("Invalid specifier '%s'\r\n", where);
      return;
   }
   switch (x)
   {
      case 0:
         switch (y)
         {
            case 0: wp->winds[index].floor=val; break;
            case 1: wp->winds[index].ceiling=val; break;
            case 2: wp->winds[index].direction=val; break;
            case 3: wp->winds[index].speed=val; break;
            case 4: wp->winds[index].gusting=val?1:0; break;
         }
         break;
      case 1:
         switch (y)
         {
            case 0: wp->clouds[index].floor=val; break;
            case 1: wp->clouds[index].ceiling=val; break;
            case 2: wp->clouds[index].coverage=val; break;
            case 3: wp->clouds[index].turbulence=val; break;
            case 4: wp->clouds[index].icing=val?1:0; break;
         }
         break;
      case 2:
         switch (y)
         {
            case 0: wp->temps[index].ceiling=val; break;
            case 1: wp->temps[index].temp=val; break;
         }
         break;
   }
}

void sysuser::execkill(char **array, int count)
{
   if (count<2)
   {
      usage(SYS_KILL, "<callsign> <reason>");
      return;
   }
   client *c=getclient(array[0]);
   if (!c)
   {
      uprintf("Could not find client '%s'\r\n", array[0]);
      return ;
   }
   char buf[1000];
   catargs(array+1, count-1, buf);
   serverinterface->sendkill(c, buf);
   uprintf("Killed\r\n");
}
void sysuser::execpos(char **array, int count)
{
   if (count<2)
   {
      usage(SYS_POS, "<lat> <lon");
      return;
   }
   sscanf(array[0],"%lf", &lat);
   sscanf(array[1],"%lf", &lon);
   uprintf("Position set\r\n");
}
void sysuser::execdump(char **array, int count)
{
   if (count<2)
   {
      usage(SYS_DUMP, "<fd> <file>");
      return;
   }
   int num=atoi(array[0]);
   absuser *temp;
   for (temp=serverinterface->rootuser;temp;temp=temp->next)
      if (temp->fd==num)
   {
      int fd=open(array[1],O_WRONLY|O_CREAT|O_TRUNC, 0666);
      if (fd==-1)
      {
         uprintf("%s: %s\r\n", array[1], strerror(errno));
         return;
      }
      write(fd, temp->outbuf, strlen(temp->outbuf));
      close(fd);
      uprintf("Done\r\n");
      return;
   }
   uprintf("Can't find fd\r\n");
}
void sysuser::execrefmetar(char **array, int count)
{
   if (metarmanager->source==SOURCE_NETWORK)
   {
      uprintf("Servers doesn't use a local METAR file.\r\n");
      return;
   }
   if (metarmanager->downloading)
   {
      uprintf("Already downloading latest METAR information.\r\n");
      return;
   }
   uprintf("Starting download of metar data.\r\n");
   metarmanager->startdownload();
}
