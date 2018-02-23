#include <cstdio>
#include <cstdlib>
#ifndef WIN32
	#include <unistd.h>
#endif
#include <ctime>
#include <cstring>

#include "interface.h"
#include "clinterface.h"
#include "global.h"
#include "support.h"
#include "server.h"
#include "protocol.h"
#include "client.h"
#include "cluser.h"
#include "user.h"

/* The client interface class */

clinterface::clinterface(int port, char *code, char *d):
   tcpinterface(port, code, d)
{
   prevwinddelta=mtime();
}
int clinterface::run()
{
   int busy=tcpinterface::run();
   if ((mtime()-prevwinddelta)>WINDDELTATIMEOUT)
   {
      prevwinddelta=mtime();
      sendwinddelta();
   }
   
   return busy;
}

void clinterface::newuser(int fd, char *peer, int portnum, int g)
{
   insertuser(new cluser(fd,this,peer,portnum,g));
}

void clinterface::sendaa(client *who, absuser *ex)
{
   char data[1000];
   sprintf(data,"%s:SERVER:%s:%s::%d", who->callsign, who->realname,
      who->cid,who->rating,who->protocol);
   sendpacket(NULL, NULL, ex, CLIENT_ALL, -1, CL_ADDATC, data);
}
void clinterface::sendap(client *who, absuser *ex)
{
   char data[1000];
   sprintf(data,"%s:SERVER:%s::%d:%s:%d",who->callsign,who->cid,who->rating,
      who->protocol,who->simtype);
   sendpacket(NULL, NULL, ex, CLIENT_ALL, -1, CL_ADDPILOT, data);
}
void clinterface::sendda(client *who, absuser *ex)
{
   char data[1000];
   sprintf(data,"%s:%s",who->callsign,who->cid);
   sendpacket(NULL, NULL, ex, CLIENT_ALL, -1, CL_RMATC, data);
}
void clinterface::senddp(client *who, absuser *ex)
{
   char data[1000];
   sprintf(data,"%s:%s",who->callsign,who->cid);
   sendpacket(NULL, NULL, ex, CLIENT_ALL, -1, CL_RMPILOT, data);
}
void clinterface::sendweather(client *who, wprofile *p)
{
   int x;
   char buf[1000], part[200];
   p->fix(who->lat, who->lon);
   sprintf(buf,"%s:%s", "server", who->callsign);
   for (x=0;x<4;x++)
   {
      templayer *l=&p->temps[x];
      sprintf(part, ":%d:%d", l->ceiling, l->temp);
      strcat(buf, part);
   }
   sprintf(part,":%d", p->barometer);
   strcat(buf, part);
   sendpacket(who, NULL, NULL, CLIENT_ALL, -1, CL_TEMPDATA, buf);

   sprintf(buf,"%s:%s", "server", who->callsign);
   for (x=0;x<4;x++)
   {
      windlayer *l=&p->winds[x];
      sprintf(part,":%d:%d:%d:%d:%d:%d", l->ceiling, l->floor, l->direction,
         l->speed, l->gusting, l->turbulence);
      strcat(buf, part);
   }
   sendpacket(who, NULL, NULL, CLIENT_ALL, -1, CL_WINDDATA, buf);

   sprintf(buf,"%s:%s", "server", who->callsign);
   for (x=0;x<3;x++)
   {
      cloudlayer *c=(x==2?p->tstorm:&p->clouds[x]);
      sprintf(part,":%d:%d:%d:%d:%d", c->ceiling, c->floor, c->coverage,
         c->icing, c->turbulence);
      strcat(buf, part);
   }
   sprintf(part,":%.2f", p->visibility);
   strcat(buf, part);
   sendpacket(who, NULL, NULL, CLIENT_ALL, -1, CL_CLOUDDATA, buf);
}
void clinterface::sendmetar(client *who, char *data)
{
   char buf[1000];
   sprintf(buf,"server:%s:METAR:%s", who->callsign, data);
   sendpacket(who, NULL, NULL, CLIENT_ALL, -1, CL_REPACARS, buf);
}
void clinterface::sendnowx(client *who, char *station)
{
   absuser *temp;
   for (temp=rootuser;temp;temp=temp->next)
   {
      cluser *ctemp=(cluser *)temp;
      if (ctemp->thisclient==who)
      {
         ctemp->showerror(ERR_NOWEATHER, station);
         break;
      }
   }
}
int clinterface::getbroad(char *s)
{
   int broad=CLIENT_ALL;
   if (!strcmp(s,"*P")) broad=CLIENT_PILOT; else
   if (!strcmp(s,"*A")) broad=CLIENT_ATC;
   return broad;
}
void clinterface::sendgeneric(char *to, client *dest, absuser *ex,
   client *source, char *from, char *s, int cmd)
{
   char buf[1000];
   int range=-1;
   sprintf(buf,"%s:%s:%s",from,to,s);
   if (to[0]=='@'&&source)
      range=source->getrange();     
   sendpacket(dest, source, ex, getbroad(to), range, cmd, buf);
}
void clinterface::sendpilotpos(client *who, absuser *ex)
{
   char data[1000];
   sprintf(data,"%s:%s:%d:%d:%.5f:%.5f:%d:%d:%u:%d", who->identflag,
      who->callsign, who->transponder, who->rating, who->lat, who->lon,
      who->altitude, who->groundspeed, who->pbh, who->flags);
//dolog(L_INFO,"PBH unsigned value is %u",who->pbh);
//dolog(L_INFO,"SendPilotPos is sending: %s",data);
   sendpacket(NULL, who, ex, CLIENT_ALL, -1, CL_PILOTPOS, data);
}
void clinterface::sendatcpos(client *who, absuser *ex)
{
   char data[1000];
   sprintf(data,"%s:%d:%d:%d:%d:%.5f:%.5f:%d",who->callsign,
      who->frequency, who->facilitytype, who->visualrange, who->rating,
      who->lat, who->lon, who->altitude);
   sendpacket(NULL, who, ex, CLIENT_ALL, -1, CL_ATCPOS, data);
}

void clinterface::sendplan(client *dest, client *who, int range)
{
   char buf[1000], *cs=(char *)(dest?dest->callsign:"*A");
   flightplan *plan=who->plan;
   sprintf(buf,"%s:%s:%c:%s:%d:%s:%d:%d:%s:%s:%d:%d:%d:%d:%s:%s:%s",
      who->callsign, cs, plan->type, plan->aircraft,
      plan->tascruise, plan->depairport, plan->deptime, plan->actdeptime,
      plan->alt, plan->destairport, plan->hrsenroute, plan->minenroute,
      plan->hrsfuel, plan->minfuel, plan->altairport, plan->remarks,
      plan->route);
   sendpacket(dest, NULL, NULL, CLIENT_ATC, range, CL_PLAN, buf);
}
void clinterface::handlekill(client *who, char *reason)
{
   if (who->location!=myserver) return;
   absuser *temp;
   for (temp=rootuser;temp;temp=temp->next)
   {
      cluser *ctemp=(cluser*)temp;
      if (ctemp->thisclient==who)
      {
         char buf[1000];
         sprintf(buf,"SERVER:%s:%s", who->callsign, reason);
         sendpacket(who, NULL, NULL, CLIENT_ALL, -1, CL_KILL, buf);
         temp->kill(KILL_KILL);
      }
   }
}
void clinterface::sendwinddelta()
{
   msrand(time(NULL));
   char buf[100];
   int speed=mrand()%11-5;
   int direction=mrand()%21-10;
   sprintf(buf,"SERVER:*:%d:%d", speed, direction);
   sendpacket(NULL, NULL, NULL, CLIENT_ALL, -1, CL_WDELTA, buf);
}

int clinterface::calcrange(client *from, client *to, int type, int range)
{
   int x, y;
   switch (type)
   {
      case CL_PILOTPOS:
      case CL_ATCPOS:
         if (to->type==CLIENT_ATC) return to->visualrange;
         x=to->getrange(), y=from->getrange();
         if (from->type==CLIENT_PILOT) return x+y;
         return x>y?x:y;
      case CL_MESSAGE:
         x=to->getrange(), y=from->getrange();
         if (from->type==CLIENT_PILOT&&to->type==CLIENT_PILOT) return x+y;
         return x>y?x:y;
      default : return range;
   }
}

/* Send a packet to a client.
   If <dest> is specified, only the client <dest> will receive the message.
   <broad> indicates if only pilot, only atc, or both will receive the
   message
*/
void clinterface::sendpacket(client *dest, client *source, absuser *exclude,
   int broad, int range, int cmd, char *data)
{
   absuser *temp;
   if (dest) if (dest->location!=myserver) return;
   for (temp=rootuser;temp;temp=temp->next) if (!temp->killflag)
   {
      client *cl=((cluser*)temp)->thisclient;
      if (!cl) continue;
      if (exclude==temp) continue;
      if (dest&&cl!=dest) continue;
      if (!(cl->type&broad)) continue;
      if (source&&(range!=-1||cmd==CL_PILOTPOS||cmd==CL_ATCPOS))
      {
         int checkrange=calcrange(source, cl, cmd, range);
         double distance=cl->distance(source);
         if (distance==-1||distance>checkrange) continue;
      }
      temp->uslprintf("%s%s\r\n", cmd==CL_ATCPOS||cmd==CL_PILOTPOS,
         clcmdnames[cmd], data);
   }
}
