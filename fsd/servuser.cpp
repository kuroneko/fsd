#include <cstdio>
#include <cstdlib>
#ifndef WIN32
	#include <unistd.h>
#endif
#include <ctime>
#include <cstring>
#include <cctype>

#include "interface.h"
#include "global.h"
#include "support.h"
#include "protocol.h"
#include "client.h"
#include "server.h"
#include "servuser.h"
#include "fsd.h"
#include "certificate.h"
#include "mm.h"

/* The server user class */

const char *cmdnames[]=
{
   "NOTIFY",
   "REQMETAR",
   "PING",
   "PONG",
   "SYNC",
   "LINKDOWN",
   "NOWX",
   "ADDCLIENT",
   "RMCLIENT",
   "PLAN",
   "PD",    /* Pilot data */
   "AD",    /* ATC data */
   "ADDCERT",
   "MC",
   "WX",
   "METAR",
   "AWPROF",
   "DWPROF",
   "KILL",
   "RESET",
   NULL
};
int silentok[]=
{
   1,  /* notify */
   1,  /* reqmetar */
   1,  /* ping */
   1,  /* pong */
   1,  /* sync */
   1,  /* linkdown */
   1,  /* nowx */
   1,  /* addclient */
   1,  /* rmclient */
   1,  /* plan */
   0,  /* pd */
   0,  /* ad */
   1,  /* addcert */
   0,  /* mc */
   1,  /* wx */
   1,  /* metar */
   1,  /* add w profile */
   1,  /* del w profile */
   1,  /* kill client */
   1,  /* reset */
   0
};
/*******************************************************************/
/* The USER class. There's not much interesting here, just handling
   the low level communication with the clients */
servuser::servuser(int d, servinterface *p, char *pn, int portnum, int g):
   absuser(d,p,pn,portnum,g)
{
   parent=p, startuptime=mtime(), clientok=0, thisserver=NULL;
   feed();
}
servuser::~servuser()
{
   /* This server link is killed, send out a LINKDOWN message to the
      others */

   char buf[1000]="";
   server *temp;
   for (temp=rootserver;temp;temp=temp->next) if (temp->path==this)
   {
      temp->setpath(NULL, -1);
      if (buf[0]) strcat(buf,":");
      strcat(buf, temp->ident);
   }
   if (buf[0])
   {
      parent->sendlinkdown(buf);
      dolog(L_ALERT, "link down");
   }
   serverinterface->sendsync();
}
void servuser::parse(char *s)
{
   if (s[0]=='#') return;
   setactive();
   if (s[0]!='\\') doparse(s); else switch(s[1])
   {
      case 'q': kill(KILL_COMMAND); break;
      case 'l': list(s); break;
      default : uprintf("Syntax error, type \\h for help\r\n");
   }
}
void servuser::list(char *s)
{
   char buf[100];
   s+=2; while (isspace(*s)) s++;
   if (!*s) s=NULL;
   int nvars=manager->getnvars();
   for (int x=0;x<nvars;x++) if (manager->sprintvalue(x,buf))
   {
      char *varname=manager->getvar(x)->name;
      if (!s||strstr(varname,s)) uprintf("%s=%s\r\n",varname, buf);
   }
}

/* Feed all data from this server to a direct neighbor */
void servuser::feed()
{
   server *tempserv;
   client *tempcl;
   certificate *tempcert;
   wprofile *tempwp;
   for (tempserv=rootserver;tempserv;tempserv=tempserv->next) 
      serverinterface->sendservernotify("*", tempserv, this);
   for (tempcl=rootclient;tempcl;tempcl=tempcl->next) 
   {
      serverinterface->sendaddclient("*", tempcl, this, NULL, 1);
      if (tempcl->plan) serverinterface->sendplan("*", tempcl, this);
   }
   for (tempcert=rootcert;tempcert;tempcert=tempcert->next) 
      serverinterface->sendcert("*", CERT_ADD, tempcert, this);
   for (tempwp=rootprofile;tempwp;tempwp=tempwp->next) 
      serverinterface->sendaddwp(this, tempwp);
}

/*
 * When given the destination of a packet, needlocaldelivery() checks if this
 * packet needs local delivery.
 */
int servuser::needlocaldelivery(char *dest)
{
   client *tc;

   /* wildcards are always delivered locally. Note : '*P' and '*A' are
      also broadcast addresses */
   if (dest[0]=='*') return 1;

   /* We don't know yet, who is on what frequency, so deliver frequency
      addressed packets */
   if (dest[0]=='@') return 1;

   switch (dest[0])
   {
      case '%':
         /* This is a pilot packet, check if the client is connected to
           this server */
         for (tc=rootclient;tc;tc=tc->next)
            if (!STRCASECMP(tc->callsign,dest+1)&&tc->location==myserver)
               return 1;
         break;

      default: 
         /* We have a server name here, check if the name equals our server
            name */

         if (!STRCASECMP(dest,myserver->ident)) return 1;
         break;
   }
   return 0;
}

/*
 * Takes care of a packet, that comes in from a server connection.
 */

void servuser::doparse(char *s)
{
   int index=0, count, packetcount, hops, bi;
   char *array[100], *from, *to;
   server *origin;

   if (s[0]=='#') return;

/*if (strlen(s)>80) dolog(L_ERR,"POSERR2:%s", s);*/
   /* Take all the fields from the packet */
   count=breakpacket(s,array,100);


   if (count<6)
   {
      /* This packet doesn't have the basic fields, show an error and
         drop it  */
      manager->incvar(parent->varshape);
      uprintf("# Missing basic packet fields, syntax error\r\n");
      return;
   }

   /* Some statistics */
   int packeterr=0;
   if (sscanf(array[3]+1, "%d", &packetcount)==0) packeterr=1;
   if (sscanf(array[4], "%d", &hops)==0) packeterr=1;
   from=array[2], to=array[1], bi=(*(array[3])=='B'); 
   if (packeterr)
   { 
      manager->incvar(parent->varinterr);
      return; 
   }

   if (hops > MAXHOPS)
   {
      /* This packet seems to bounce, drop it */
      manager->incvar(parent->varbounce);
      return;
   }

   /* Find the origin of the packet */
   origin=getserver(from);
   if (origin)
   {
      /* This server seems to be alive */
      origin->setalive();

      /* Discard if it originated from our server */
      if (origin==myserver) return;

      /* Check if this packet travelled a shorter route */
      if (hops<origin->hops||origin->hops==-1)
         origin->setpath(this,hops);

      /* Discard if the packet was broadcast and the packetcount doesn't
         match the packetcount (we already handled this packet earlier)
         The broadcast check (bi) is nessecary to prevent newer broadcast
         packets from passing stalled unicast packets with a quicker route.
      */
      #define MAXSPACE 1000000

      if (packetcount<=origin->pcount||packetcount>(origin->pcount+MAXSPACE))
      {
         if ((packetcount==origin->pcount)||(bi&&origin->packetdrops<100)||
            (bi&&packetcount>(origin->pcount+MAXSPACE)))
         {
            manager->incvar(parent->varmcdrops);
            origin->packetdrops++;
            return;
         }
         manager->incvar(parent->varucoverrun);
      }
      if (origin->packetdrops>=100)
         origin->pcount=packetcount;
      if (packetcount>origin->pcount)
      {
         /* Update the packetcount for this server */
         origin->pcount=packetcount;
      }
      if (bi)
      {
         /* Set the path to the server */
         origin->setpath(this,hops);
         origin->packetdrops=0;
      }
   } 

   /* Now find the command number for this packet */
   while (cmdnames[index])
   {
      if (!STRCASECMP(cmdnames[index],array[0]))
      {
         char buf[1000];
         /* This user is correct */
         clientok=1;

         /* If we don't know the server, we only allow the NOTIFY packet from
            it */
         if (!origin&&index!=CMD_NOTIFY) return;
         
         /* If the packet doesn't need local delivery, forward and return */
         if (!needlocaldelivery(to))
         {
            /* call sendpacket() to relay the packet to the other servers */
            if (STRCASECMP(to,myserver->ident))
               parent->sendpacket(this,NULL,index,to,from,packetcount,
                  hops, bi, catcommand(array+5,count-5,buf));
            return;
         }

         /* Execute the command */
         int needforward=runcmd(index,array+1,count-1);
         /* if the packet needs forwarding, call sendpacket() to relay the
            packet to the other servers */
         if (STRCASECMP(to,myserver->ident)&&needforward)
            parent->sendpacket(this,NULL,index,to,from,packetcount,
               hops, bi, catcommand(array+5,count-5,buf));

         manager->incvar(bi?parent->varmchandled:parent->varuchandled);
         return;
      } else index++;
   }
   uprintf("# Unknown command, Syntax error\r\n");
   manager->incvar(parent->varfailed);
}

/* Handle a packet, returns 1 if packet needs to be forwarded to other
   servers
*/
int servuser::runcmd(int num, char **data, int count)
{
   switch (num)
   {
      case CMD_NOTIFY   : execnotify(data,count); break;
      case CMD_PING     : execping(data,count); break;
      case CMD_LINKDOWN : execlinkdown(data,count); break;
      case CMD_PONG     : execpong(data,count); break;
      case CMD_SYNC     : break; /* SYNC packets are for routing only */
      case CMD_ADDCLIENT: return execaddclient(data,count);
      case CMD_RMCLIENT : execrmclient(data,count); break;
      case CMD_PD       : execpd(data, count); break;
      case CMD_AD       : execad(data, count); break;
      case CMD_CERT     : return execcert(data,count);  
      case CMD_MULTIC   : execmulticast(data,count); break;
      case CMD_PLAN     : return execplan(data,count);
      case CMD_REQMETAR : execreqmetar(data,count); break;
      case CMD_WEATHER  : execweather(data, count); break;
      case CMD_METAR    : execmetar(data, count); break;
      case CMD_NOWX     : execnowx(data, count); break;
      case CMD_ADDWPROF : execaddwp(data, count); break;
      case CMD_DELWPROF : execdelwp(data, count); break;
      case CMD_KILL     : execkill(data, count); break;
      case CMD_RESET    : execreset(data, count); break;
   }
   return 1;
}
/* Handle a NOTIFY packet */
void servuser::execnotify(char **array, int count)
{
   server *s;
   int packetcount, hops, feedflag;

   if (count<11) return;
   packetcount=atoi(array[2]), hops=atoi(array[3]), feedflag=atoi(array[4]);
   s=getserver(array[5]);
   if (s==myserver) return;
   if (!s)
   {
      s=new server(array[5],array[6],array[7],array[8],array[9],
         atoi(array[10]), (char *)(count==12?array[11]:""));
   } else s->configure(array[6],array[7],array[8],array[9],
        (char *)(count==12?array[11]:""));

   /* If this is a notify from our peer, set 'thisserver' */
   if (!feedflag&&hops==1) thisserver=s;

   /* If this is a server feed, or we already have routing info, we don't
      need to set the routing info */
   if (feedflag||(s->hops>-1&&hops>=s->hops)) return;
   s->setpath(this, hops);
}
/* Handle a PING packet */
void servuser::execping(char **array, int count)
{
   char buf[1000];
   parent->sendpong(array[1],catcommand(array+4,count-4,buf));
}
/* Handle a PONG packet */
void servuser::execpong(char **array, int count)
{
   int fd;
   if (count<5) return;
   sscanf(array[4],"%d",&fd);
   server *source=getserver(array[1]);
   if (!source) return;
   source->receivepong(array[4]);
   if (fd!=-1) systeminterface->receivepong(array[1], array[4], array[2],
      array[3]);
}
/* Handle a LINKDOWN packet */
void servuser::execlinkdown(char **array, int count)
{
   int x;
   for (x=4;x<count;x++)
   {
      /* Get the next server ident from the packet and look it up */
      server *temp=getserver(array[x]);
      if (!temp) continue;

      /* If the shortest path to the server is pointing at the direction
         which the packet came from, we can no longer route packets
         to the server in that direction: reset the path to NULL; packets for
         the server will then be broadcast, until a new shortest path is
         found */
      if (temp->path==this)
         temp->setpath(NULL, -1);
   }
   serverinterface->sendsync();
}
/* Handle an ADD CLIENT packet */
int servuser::execaddclient(char **array, int count)
{
   int type;
   client *c;
   if (count<12) return 1;
   server *location=getserver(array[5]);
   /* If we can't find the server, or the indicated server is our server,
      we don't need the new client. */
   if (!location||location==myserver) return 1;
   type=atoi(array[7]);
   c=getclient(array[6]);
   if (c) return 0;
   c=new client(array[4],location,array[6],type,atoi(array[8]), array[9],
      array[10], atoi(array[11]));
   if (type==CLIENT_ATC) clientinterface->sendaa(c, NULL); else
   if (type==CLIENT_PILOT) clientinterface->sendap(c, NULL);
   return 1;
}
/* Handle an RM CLIENT packet */
void servuser::execrmclient(char **array, int count)
{
   if (count<5) return;
   client *c=getclient(array[4]);
   if (c)
   {
      int type=c->type;
      if (c->location==myserver) return;
      if (type==CLIENT_ATC) clientinterface->sendda(c, NULL); else
      if (type==CLIENT_PILOT) clientinterface->senddp(c, NULL);
      delete c;
   }
}
void servuser::execad(char **array, int count)
{
   if (count<12) return;
   client *who=getclient(array[4]);
   if (!who) return;
   who->updateatc(array+5);
   clientinterface->sendatcpos(who, NULL);
}
void servuser::execpd(char **array, int count)
{
   if (count<14) return;
   client *who=getclient(array[5]);
   if (!who) return;
   who->updatepilot(array+4);
   clientinterface->sendpilotpos(who, NULL);
}
int servuser::execcert(char **array, int count)
{
   certificate *c;
   if (count<10) return 0;
   configgroup *group=configman->getgroup("hosts");
   configentry *entry=group?group->getentry("certificates"):(configentry*)NULL;
   int ok=entry?entry->inlist(array[9]):1;
   int level=atoi(array[7]);
   c=getcert(array[5]);
   time_t now=atoi(array[8]);
   switch (atoi(array[4]))
   {
      case CERT_ADD:
         if (!c)
         {
            if (ok)
               c=new certificate(array[5],array[6], level, now, array[9]); 
         } else if (now>c->creation)
         {
            if (ok)
               c->configure(NULL, atoi(array[7]), now, array[9]);
         } else return 0;
         break;
      case CERT_DELETE:
         if (c)
         {
            if (ok) delete c;
         } else return 0;
         break;
      case CERT_MODIFY:
         if (c&&now>c->creation)
         {
            if (ok) c->configure(array[6],atoi(array[7]), now, array[9]);
         } else return 0;
         break;
   }
   return 1;
}
void servuser::execmulticast(char **array, int count)
{
   char *dest;
   if (count<7) return;
   client *c=NULL, *source=getclient(array[5]);
   int cmd=atoi(array[4]);
   if (cmd > CL_MAX) return;
   switch (*array[0])
   {
      case '*' : case '@':
         dest=array[0];
         break;
      default:
         c=getclient(array[0]+1);
         if (!c) return;
         dest=array[0]+1;
         break;
   }
   char buf[1000];
   clientinterface->sendgeneric(dest, c, NULL, source, array[5],
      catcommand(array+6, count-6, buf), cmd);
}
int servuser::execplan(char **array, int count)
{
   if (count<21) return 0;
   int revision=atoi(array[5]);
   client *who=getclient(array[4]);
   if (!who) return 1;
   if (who->plan&&who->plan->revision>=revision) return 0;
   who->handlefp(array+6);
   who->plan->revision=revision;
   clientinterface->sendplan(NULL, who, 400);
   return 1;
}
void servuser::execreqmetar(char **array, int count)
{
   if (count<8) return;
   metarmanager->requestmetar(array[4], array[5], atoi(array[6]),
      atoi(array[7]));   
}
void servuser::execweather(char **array, int count)
{
   if (count<56) return;
   wprofile prof(array[4], 0, NULL);
   int fd=atoi(array[5]);
   prof.loadarray(array+6, count-6);
   if (fd==-1)
   {
      client *c=getclient(array[0]+1);
      if (!c) return;
      clientinterface->sendweather(c, &prof);
   } else systeminterface->receiveweather(fd, &prof);
}
void servuser::execmetar(char **array, int count)
{
   if (count<7) return;
   int fd=atoi(array[5]);
   if (fd!=-1) systeminterface->receivemetar(fd, array[4], array[6]); else
   {
      client *c=getclient(array[0]+1);
      if (!c) return;
      clientinterface->sendmetar(c, array[6]);
   } 
}
void servuser::execnowx(char **array, int count)
{
   if (count<6) return;
   int fd=atoi(array[5]);
   if (fd==-1)
   {
      client *c=getclient(array[0]+1);
      if (!c) return;
      clientinterface->sendnowx(c, array[4]);
   } else systeminterface->receivenowx(fd, array[4]);
}
void servuser::execaddwp(char **array, int count)
{
   /* Non METAR hosts, should not store weather profiles */
   if (count<57||metarmanager->source==SOURCE_NETWORK) return;
   configgroup *group=configman->getgroup("hosts");
   configentry *entry=group?group->getentry("weather"):(configentry*)NULL;
   int ok=entry?entry->inlist(array[6]):1;
   if (!ok) return;
   time_t version;
   sscanf(array[5], "%lu", &version);
   wprofile *prof;
   prof=getwprofile(array[4]);
   if (prof&&prof->version>=version) return;
   if (prof) delete prof;
   prof=new wprofile(array[4], version, array[6]);
   prof->loadarray(array+7, count-7);
   prof->genrawcode();
}
void servuser::execdelwp(char **array, int count)
{
   if (count<5||metarmanager->source==SOURCE_NETWORK) return;
   configgroup *group=configman->getgroup("hosts");
   configentry *entry=group?group->getentry("weather"):(configentry*)NULL;
   int ok=entry?entry->inlist(array[1]):1;
   if (!ok) return;
   wprofile *prof=getwprofile(array[4]);
   if (!prof||!prof->active) return;
   delete prof;  
}
void servuser::execkill(char **array, int count)
{
   if (count<6) return;
   client *c=getclient(array[4]);
   if (!c) return;
   clientinterface->handlekill(c, array[5]);
}
void servuser::execreset(char **array, int count)
{
   server *s=getserver(array[1]);
   if (!s) return;
   s->pcount=0;
}
