#include <cstdio>
#include <cstdlib>
#ifndef WIN32
	#include <unistd.h>
#endif
#include <cstring>
#include <cmath>

#include "client.h"
#include "cluser.h"
#include "fsd.h"
#include "support.h"
#include "global.h"

client *rootclient=NULL;

client::client(char *i, server *where, char *cs, int t, int reqrating,
   char *rev, char *real, int st)
{
   next=rootclient, prev=NULL;
   if (next) next->prev=this;
   rootclient=this;
   location=where, cid=strdup(i), type=t, callsign=strdup(cs);
   protocol=strdup(rev), sector=NULL, identflag=NULL, facilitytype=0;
   rating=reqrating, visualrange=40;
   plan=NULL, positionok=0, altitude=0, simtype=st;
   realname=strdup(real), starttime=alive=mtime();
   frequency=0; transponder=0; groundspeed=0; lat=0; lon=0;
}
client::~client()
{
   serverinterface->clientdropped(this);
   if (next) next->prev=prev;
   if (prev) prev->next=next; else rootclient=next;
   free(cid);
   free(callsign);
   if (plan) delete plan;
   if (realname) free(realname);
   if (protocol) free(protocol);
   if (sector) free(sector);
   if (identflag) free(identflag);
}
void client::handlefp(char **array)
{
   int revision=plan?plan->revision+1:0;
   if (plan) delete plan;
   plan=new flightplan(callsign, *array[0], array[1],
      atoi(array[2]), array[3], atoi(array[4]),
      atoi(array[5]), array[6], array[7], atoi(array[8]),
      atoi(array[9]), atoi(array[10]), atoi(array[11]), array[12],
      array[13], array[14]);
   plan->revision=revision;
}
/* Update the client fields, given a packet array */
void client::updatepilot(char **array)
{
   unsigned a,b,c;
   unsigned x,y,z;
   transponder=atoi(array[2]);
   if (identflag) free(identflag);
   identflag=strdup(array[0]);
   sscanf(array[4],"%lf",&lat);
   sscanf(array[5],"%lf",&lon);
if (lat>90.0||lat<-90.0||lon>180.0||lon<-180.0) dolog(L_DEBUG, "POSERR: s=(%s,%s) got(%f,%f)", array[4], array[5], lat, lon);

   altitude=atoi(array[6]);
//if (altitude > 100000 || altitude < 0)
//	altitude=0;
   groundspeed=atoi(array[7]);
   pbh=(unsigned int)strtoul(array[8],(char **)NULL,10);

//x=(pbh&4290772992)>>22;
//y=(pbh&4190208)>>12;
//z=(pbh&4092)>>2;
//dolog(L_INFO,"PBH value in text is %s", array[8]);
//dolog(L_INFO,"PBH unsigned value is %u",pbh);
//dolog(L_INFO,"P=%u B=%u H=%u",x,y,z);

   flags=atoi(array[9]);
   setalive();
   positionok=1;
}
void client::updateatc(char **array)
{
   int newfreq=atoi(array[0]);
   frequency=newfreq;
   facilitytype=atoi(array[1]);
   visualrange=atoi(array[2]);
   sscanf(array[4],"%lf",&lat);
   sscanf(array[5],"%lf",&lon);
   altitude=atoi(array[6]);
   setalive();
   positionok=1;
}
client *getclient(char *ident)
{
   client *temp;
   for (temp=rootclient;temp;temp=temp->next)
      if (!STRCASECMP(ident,temp->callsign))
      return temp;
   return NULL;
}
void client::setalive()
{
   alive=mtime();
}
double client::distance(client *other)
{
   if (!other) return -1;
   if (!positionok||!other->positionok) return -1;
   return dist(lat,lon,other->lat,other->lon);
   return 1;
}
int client::getrange()
{
   if (type==CLIENT_PILOT)
   { 
      if (altitude<0) altitude=0;
      return (int) (10+1.414*sqrt((double)altitude));
   }
   switch (facilitytype)
   {
      case 0: return 40;       /* Unknown */
      case 1: return 1500;      /* FSS */
      case 2: return 5;        /* CLR_DEL */
      case 3: return 5;        /* GROUND */
      case 4: return 30;       /* TOWER  */
      case 5: return 100;      /* APP/DEP */
      case 6: return 400;      /* CENTER */
      case 7: return 1500;     /* MONITOR */
      default: return 40;
   }
}

flightplan::flightplan(char *cs, char itype, char *iaircraft, int
   itascruise, char *idepairport, int ideptime, int iactdeptime, char *ialt,
   char *idestairport, int ihrsenroute, int iminenroute, int ihrsfuel, int
   iminfuel, char *ialtairport, char *iremarks, char *iroute)
{
   callsign=strdup(cs);
   type=itype;
   aircraft=strdup(iaircraft);
   tascruise=itascruise;
   depairport=strdup(idepairport);
   deptime=ideptime;
   actdeptime=iactdeptime;
   alt=strdup(ialt);
   destairport=strdup(idestairport);
   hrsenroute=ihrsenroute;
   minenroute=iminenroute;
   hrsfuel=ihrsfuel;
   minfuel=iminfuel;
   altairport=strdup(ialtairport);
   remarks=strdup(iremarks);
   route=strdup(iroute);
}

flightplan::~flightplan()
{
   if (callsign) free(callsign);
   if (aircraft) free(aircraft);
   if (depairport) free(depairport);
   if (destairport) free(destairport);
   if (alt) free(alt);
   if (altairport) free(altairport);
   if (remarks) free(remarks);
   if (route) free(route);
}
