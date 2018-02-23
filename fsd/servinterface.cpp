#include <cstdio>
#include <cstdlib>
#ifndef WIN32
	#include <unistd.h>
	#include <sys/time.h>
#endif
#include <ctime>
#include <cstring>

#include "interface.h"
#include "servinterface.h"
#include "global.h"
#include "support.h"
#include "server.h"
#include "protocol.h"
#include "client.h"
#include "servuser.h"
#include "fsd.h"

/* The serverinterface class */

servinterface::servinterface(int port, char *code, char *d):
    tcpinterface(port, code, d)
{
   packetcount=0;
   varmchandled=manager->addvar("protocol.multicast.handled", ATT_INT);
   varmcdrops=manager->addvar("protocol.multicast.dropped", ATT_INT);
   varuchandled=manager->addvar("protocol.unicast.handled", ATT_INT);
   varucoverrun=manager->addvar("protocol.unicast.overruns", ATT_INT);
   varfailed=manager->addvar("protocol.errors.invalidcommand", ATT_INT);
   varbounce=manager->addvar("protocol.errors.bounce", ATT_INT);
   varshape=manager->addvar("protocol.errors.shape", ATT_INT);
   varinterr=manager->addvar("protocol.errors.integer", ATT_INT);
   serverident=myserver->ident;
}
int servinterface::run()
{
   absuser *temp;
   time_t now=mtime();
   int busy=tcpinterface::run();
   if ((now-lastsync)>SYNCTIMEOUT) sendsync();
   for (temp=rootuser;temp;temp=temp->next)
   {
      servuser *stemp=(servuser *)temp;
      if (!stemp->clientok&&(now-stemp->startuptime)>SERVERMAXTOOK)
         stemp->kill(KILL_INITTIMEOUT);
   }
   return busy;
}
void servinterface::newuser(int fd, char *peer, int portnum, int g)
{
   insertuser(new servuser(fd,this,peer,portnum,g));
}

void servinterface::incpacketcount()
{
   packetcount++;
   /* Now check if the integer looped */
   if (packetcount<0||packetcount>2000000000)
      sendreset();
}
/* Send a SYNC packet */
void servinterface::sendsync()
{
   sendpacket(NULL, NULL, CMD_SYNC, "*", serverident, packetcount, 0, 0, "");
   lastsync=mtime();
   incpacketcount();
}
/* Send a NOTIFY packet to a destination */
void servinterface::sendservernotify(char *dest, server *subject,
   absuser *towho)
{
   char buf[1000];
   sprintf(buf,"%d:%s:%s:%s:%s:%s:%d:%s", subject==myserver?0:1, subject->ident,
      subject->name, subject->email, subject->hostname, subject->version,
      subject->flags, subject->location);
   sendpacket(NULL, towho, CMD_NOTIFY, dest, serverident, packetcount, 0,
      1, buf);
   incpacketcount();
}
/* Send a REQMETAR packet */
void servinterface::sendreqmetar(char *client, char *metar, int fd,
   int parsed, server *dest)
{
   char buf[1000];
   sprintf(buf,"%s:%s:%d:%d",client,metar,parsed,fd);
   sendpacket(NULL, NULL, CMD_REQMETAR, dest->ident, serverident,
      packetcount, 0, 0, buf);
   incpacketcount();
}
/* Send a LINKDOWN packet to a destination */
void servinterface::sendlinkdown(char *data)
{
   sendpacket(NULL, NULL, CMD_LINKDOWN, "*", serverident, packetcount, 
      0, 1, data);
   incpacketcount();
}
/* Send a PONG packet to a destination */
void servinterface::sendpong(char *dest, char *data)
{
   sendpacket(NULL, NULL, CMD_PONG, dest, serverident, packetcount, 0, 0, data);
   incpacketcount();
}
/* Broadcast a PING packet to a destination*/
void servinterface::sendping(char *dest, char *data)
{
   sendpacket(NULL, NULL, CMD_PING, dest, serverident, packetcount, 0, 0, data);
   incpacketcount();
}
/* Send an ADDCLIENT packet to a destination */
void servinterface::sendaddclient(char *dest, client *who, absuser *direction,
   absuser *source, int feed)
{
   char buf[1000];
   sprintf(buf,"%s:%s:%s:%d:%d:%s:%s:%d",who->cid,who->location->ident,
      who->callsign,who->type, who->rating, who->protocol, who->realname,
      who->simtype);
   sendpacket(NULL, direction, CMD_ADDCLIENT, dest, serverident, 
      packetcount, 0, 0, buf);
   incpacketcount();
   /* This command is important for clients as well, so if it's not a feed,
      clients should get it as well */
   if (!feed)
   {
      if (who->type==CLIENT_ATC) clientinterface->sendaa(who, source); else
      if (who->type==CLIENT_PILOT) clientinterface->sendap(who, source);
   }
}
/* Send an RMCLIENT packet to a destination */
void servinterface::sendrmclient(absuser *direction, char *dest, client *who,
   absuser *ex)
{
   char buf[1000];
   sprintf(buf,"%s",who->callsign);
   sendpacket(NULL, direction, CMD_RMCLIENT, dest, serverident, packetcount,
      0, 0, buf);
   incpacketcount();
   /* This command is important for clients as well, so if it's send to the
      broadcast address, clients should get it as well */
   if (!strcmp(dest,"*"))
   {
      if (who->type==CLIENT_ATC) clientinterface->sendda(who, ex); else
         clientinterface->senddp(who, ex);
   }
}
void servinterface::sendpilotdata(client *who, absuser *ex)
{
   char data[1000];
   sprintf(data,"%s:%s:%d:%d:%.5f:%.5f:%d:%d:%u:%d", who->identflag,
      who->callsign, who->transponder, who->rating, who->lat, who->lon,
      who->altitude, who->groundspeed, who->pbh, who->flags);
//dolog(L_INFO,"PBH unsigned value is %u",who->pbh); 
//dolog(L_INFO,"SendPilotData is sending: %s",data); 
   sendpacket(NULL, NULL, CMD_PD, "*", serverident, packetcount, 0, 0, data);
   clientinterface->sendpilotpos(who, ex);
   incpacketcount();
}
void servinterface::sendatcdata(client *who, absuser *ex)
{
   char data[1000];
   sprintf(data,"%s:%d:%d:%d:%d:%.5f:%.5f:%d",who->callsign,
      who->frequency, who->facilitytype, who->visualrange, who->rating,
      who->lat, who->lon, who->altitude);
   sendpacket(NULL, NULL, CMD_AD, "*", serverident, packetcount, 0, 0, data);
   clientinterface->sendatcpos(who, ex);
   incpacketcount();
}

void servinterface::sendcert(char *dest, int cmd, certificate *who,
   absuser *direction)
{
   char data[1000];
   sprintf(data,"%d:%s:%s:%d:%lu:%s", cmd, who->cid, who->password, who->level,
      who->creation,who->origin);
   sendpacket(NULL, direction, CMD_CERT, dest, serverident, packetcount,
      0, 0, data);
   incpacketcount();
}

void servinterface::sendreset()
{
   sendpacket(NULL, NULL, CMD_RESET, "*", serverident, packetcount, 0, 0, "");
   packetcount=0;
}

int servinterface::sendmulticast(client *source, char *dest, char *s,
   int cmd, int multiok, absuser *ex)
{
   client *destination=NULL;
   char data[1000], servdest[100];
   char *sourcestr=(char *)(source?source->callsign:"server");
   if (source&&!STRCASECMP(dest, "server"))
   {
      switch(cmd)
      {
          case CL_PING : 
             clientinterface->sendgeneric(source->callsign, source, NULL,
                NULL, "server", s, CL_PONG);
             break;
      }
      return 1;
   }
   switch (dest[0])
   {
      case '@': case '*':
         if (!multiok) return 0;
         strcpy(servdest, dest);
         break;
      default:
         sprintf(servdest,"%%%s",dest);
         destination=getclient(dest);
         if (!destination) return 0;
         break;
   }
   sprintf(data,"%d:%s:%s", cmd, sourcestr, s);
   sendpacket(NULL, NULL, CMD_MULTIC, servdest, serverident, packetcount, 0,
      0, data);
   incpacketcount();
   clientinterface->sendgeneric(dest, destination, ex, source, sourcestr, s,
      cmd); 
   return 1;
}
void servinterface::sendplan(char *dest, client *who, absuser *direction)
{
   char buf[1000];
   flightplan *plan=who->plan;
   if (!plan) return;
   sprintf(buf,"%s:%d:%c:%s:%d:%s:%d:%d:%s:%s:%d:%d:%d:%d:%s:%s:%s",
      who->callsign, plan->revision, plan->type, plan->aircraft,
      plan->tascruise, plan->depairport, plan->deptime,
      plan->actdeptime, plan->alt, plan->destairport, plan->hrsenroute,
      plan->minenroute, plan->hrsfuel, plan->minfuel, plan->altairport,
      plan->remarks, plan->route);
   sendpacket(NULL, direction, CMD_PLAN, dest, serverident, packetcount, 0,
      0, buf);
   incpacketcount();
   clientinterface->sendplan(NULL, who, 400);
}
void servinterface::sendweather(char *dest, int fd, wprofile *w)
{
   if (!STRCASECMP(dest, serverident))
   {
      systeminterface->receiveweather(fd, w);
      return;
   }
   if (dest[0]=='%')
   {
      client *c=getclient(dest+1);
      if (!c) return;
      if (c->location==myserver)
      {
         clientinterface->sendweather(c, w);
         return;
      }
   }
   int x;
   char data[1000]="", piece[100];
   char weather[1000];
   w->print(weather);
   sprintf(data,"%s:%d:%s", w->name, fd, weather);
   sendpacket(NULL, NULL, CMD_WEATHER, dest, serverident, packetcount,
      0, 0, data);
   incpacketcount();
}
void servinterface::sendmetar(char *dest, int fd, char *station, char *data)
{
   if (!STRCASECMP(dest, serverident))
   {
      systeminterface->receivemetar(fd, station, data);
      return;
   }
   if (dest[0]=='%')
   {
      client *c=getclient(dest+1);
      if (!c) return;
      if (c->location==myserver)
      {
         clientinterface->sendmetar(c, data);
         return;
      }
   }
   char buf[1000];
   sprintf(buf, "%s:%d:%s", station, fd, data);
   sendpacket(NULL, NULL, CMD_METAR, dest, serverident, packetcount,
      0, 0, buf);
   incpacketcount();
}
void servinterface::sendnowx(char *dest, int fd, char *station)
{
   if (!STRCASECMP(dest, serverident))
   {
      systeminterface->receivenowx(fd, station);
      return;
   }
   if (dest[0]=='%')
   {
      client *c=getclient(dest+1);
      if (!c) return;
      if (c->location==myserver)
      {
         clientinterface->sendnowx(c, station);
         return;
      }
   }
   char buf[1000];
   sprintf(buf, "%s:%d", station, fd);
   sendpacket(NULL, NULL, CMD_NOWX, dest, serverident, packetcount,
      0, 0, buf);
   incpacketcount();
}
void servinterface::sendaddwp(absuser *direction, wprofile *wp)
{
   char buf[1000], weather[1000];
   wp->print(weather);
   sprintf(buf, "%s:%u:%s:%s", wp->name, wp->version, wp->origin, weather);
   sendpacket(NULL, direction, CMD_ADDWPROF, "*", serverident, packetcount,
      0, 0, buf);
   incpacketcount();
}
void servinterface::senddelwp(wprofile *wp)
{
   sendpacket(NULL, NULL, CMD_DELWPROF, "*", serverident, packetcount,
      0, 0, wp->name);
   incpacketcount();
}
void servinterface::sendkill(client *who, char *reason)
{
   char data[100];
   if (who->location==myserver)
   {
      clientinterface->handlekill(who, reason); 
      return;
   }
   sprintf(data, "%s:%s", who->callsign, reason);
   sendpacket(NULL, NULL, CMD_KILL, who->location->ident, serverident,
      packetcount, 0, 1, data);
   incpacketcount();
}

void servinterface::assemble(char *buf, int cmdnum,
    char *to, char *from, int bi, int pc, int hc, char *data)
{
   sprintf(buf,"%s:%s:%s:%c%d:%d:%s",cmdnames[cmdnum], to, from,
     bi?'B':'U', pc, hc, data);
}
/* Send the packet on its way. This function will also do the routing */
void servinterface::sendpacket(absuser *exclude, absuser *direction, int cmdnum,
    char *to, char *from, int pc, int hc, int bi, char *data)
{
   char buf[1000];
   absuser *tempuser;
   server *tempserver;
   client *tempclient;

   /* variable to determine wheter or not to do the softlimit check on the
      server connection output buffer. currently only do the check on
      client position updates */
   int slcheck=cmdnum==CMD_PD||cmdnum==CMD_AD;

   /* Increase the hopcount */
   hc++;

   /* Assemble the packet */
   assemble(buf, cmdnum, to, from, bi, pc, hc, data);

   if (direction)
   {
      direction->uslprintf("%s\n", slcheck, buf);
      return;
   }

   /* Now look at the destionation field, to determine the route for this
      packet */
   switch (to[0])
   {

      case '@' : /* Fallthrough to broadcast */

      /* This is a broadcast packet.
         Note: '*P' and '*A' are broadcast destinations too! */
      case '*' :
         /* Reassemble the packet to indicate a broadcast */
         if (!bi) assemble(buf, cmdnum, to, from, 1, pc, hc, data);
#define SERVOK(x) ((x!=exclude)&&(silentok[cmdnum]||((x)->thisserver==NULL||\
   (((x)->thisserver->flags&SERVER_SILENT)==0))))
         for (tempuser=rootuser;tempuser;tempuser=tempuser->next)
            if (SERVOK((servuser*)tempuser))
               tempuser->uslprintf("%s\r\n", slcheck, buf);
         break;

      /* This is a packet for a pilot. Lookup the pilot, and send in the
         direction of the appropriate server */
      case '%' :
         for (tempclient=rootclient;tempclient;tempclient=tempclient->next)
            if (!STRCASECMP(tempclient->callsign,to+1))
         {
            /* We got the pilot, and his connected server, now if we know the
               correct path, send it; otherwise we'll have to broadcast the
               packet */
            tempserver=tempclient->location;
            /* It the pilot is connected to this server, don't send out
               the packet to other servers */
            if (tempserver==myserver) break;
               if (tempserver->path)
               tempserver->path->uslprintf("%s\r\n", slcheck, buf); else
            {
               /* Reassemble the packet to indicate a broadcast */
               if (!bi) assemble(buf, cmdnum, to, from, 1, pc, hc, data);
               for (tempuser=rootuser;tempuser;tempuser=tempuser->next)
                  if (SERVOK((servuser*)tempuser))
                     tempuser->uslprintf("%s\r\n", slcheck, buf);
            }
            break;
         }
         break;

         /* This packet is on its way to a single server */
      default :
         for (tempserver=rootserver;tempserver;tempserver=tempserver->next)
            if (!STRCASECMP(tempserver->ident,to))
         {
            /* We got the server, now if we know the correct path, send it;
               otherwise we'll have to broadcast the packet */
            if (tempserver->path)
               tempserver->path->uslprintf("%s\r\n", slcheck, buf); else
            {
               /* Reassemble the packet to indicate a broadcast */
               if (!bi) assemble(buf, cmdnum, to, from, 1, pc, hc, data);
               for (tempuser=rootuser;tempuser;tempuser=tempuser->next)
                  if (SERVOK((servuser*)tempuser))
                     tempuser->uslprintf("%s\r\n", slcheck, buf);
            }
            break;
         }
         break;
   }
}

/* This routine is called whenever a client is dropped. We have to check
   here if there's a silent server connected to us. In that case, we'll
   have to send him a RMCLIENT */

void servinterface::clientdropped(client *who)
{
   absuser *temp;
   for (temp=rootuser;temp;temp=temp->next)
   {
      servuser *s=(servuser *)temp;
      if (s->thisserver&&(s->thisserver->flags&SERVER_SILENT))
	 sendrmclient(s, s->thisserver->name, who, NULL);
   }
}
