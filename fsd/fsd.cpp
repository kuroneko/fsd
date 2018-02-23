#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#ifndef WIN32
	#include <unistd.h>
#endif
#include <sys/stat.h>

#include "fsd.h"
#include "manage.h"
#include "support.h"
#include "global.h"
#include "server.h"
#include "mm.h"
#include "config.h"
#include "protocol.h"

clinterface *clientinterface=NULL;
servinterface *serverinterface=NULL;
sysinterface *systeminterface=NULL;
configmanager *configman=NULL;


fsd::fsd(char *configfile)
{
   certfile=NULL;
   whazzupfile=NULL;
   dolog(L_INFO,"Booting server");
   pmanager=new pman;

   /* Start the information manager */
   manager=new manage();
   
   configman=new configmanager(configfile);
   pmanager->registerprocess(configman);

   /* Create the METAR manager */
   metarmanager=new mm;
   pmanager->registerprocess(metarmanager);

   /* Read the system configuration */
   configure();

   /* Create the management variables */
   createmanagevars();

   /* Create the server and the client interfaces */
   createinterfaces();

   /* Connect to the other server */
   makeconnections();

   dolog(L_INFO,"We are up");
   prevnotify=prevlagcheck=timer=mtime();

   prevwhazzup=mtime();
   fileopen=0;
}
fsd::~fsd()
{
   delete clientinterface;
   delete serverinterface;
   delete systeminterface;
   delete metarmanager;
   delete manager;
}

/* Here we do timeout checks. This function is triggered every second to
   reduce the load on the server */
void fsd::dochecks()
{
   time_t now=mtime();
   if ((now-prevnotify)>NOTIFYCHECK)
   {
      configgroup *sgroup=configman->getgroup("system");
      if (sgroup&&sgroup->changed)
         configmyserver();
      serverinterface->sendservernotify("*", myserver, NULL);
      prevnotify=now;
   }
   if ((now-prevlagcheck)>LAGCHECK)
   { 
      char data[80];
      sprintf(data,"-1 %lu", mtime());
      serverinterface->sendping("*", data);
      prevlagcheck=now;
   }
   if ((now-prevcertcheck)>CERTFILECHECK)
   {
      configentry *entry;
      configgroup *sysgroup=configman->getgroup("system");
      if (sysgroup) if ((entry=sysgroup->getentry("certificates"))!=NULL)
      {
         if (certfile) free(certfile);
         certfile=strdup(entry->getdata());
         struct stat buf;
         prevcertcheck=now;
         if (!stat(certfile, &buf)) if (buf.st_mtime!=certfilestat)
         {
            certfilestat=buf.st_mtime;
            readcert();
         }
      }
   }
// WhazzUp Start
   if ((now-prevwhazzup)>=WHAZZUPCHECK)
   {
      configentry *entry;
      configgroup *sysgroup=configman->getgroup("system");
      if (sysgroup) if ((entry=sysgroup->getentry("whazzup"))!=NULL)
      {
         if (whazzupfile) free(whazzupfile);
         whazzupfile=strdup(entry->getdata());
         char whazzuptemp[100];
         sprintf(whazzuptemp,"%s%s", whazzupfile, ".tmp");
         prevwhazzup=now;
         if (fileopen==0)
         {
            FILE *wzfile=fopen(whazzuptemp, "w");
            if (wzfile)
            {
               //Ready to write data
               fileopen = 1;
               char s[32];
			   fprintf(wzfile,"%s%s\n","![DateStamp]",sprintgmtdate(now,s));
               fprintf(wzfile,"%s\n","!GENERAL");
               fprintf(wzfile,"%s = %d\n", "VERSION", 1);
               fprintf(wzfile,"%s = %d\n", "RELOAD", 1);
               fprintf(wzfile,"%s = %s\n", "UPDATE", sprintgmt(now, s));
               client *tempclient;
               flightplan *tempflightplan;
               server *tempserver;
               int clients=0;
               for (tempclient=rootclient;tempclient;tempclient=tempclient->next)
                  clients++;
               fprintf(wzfile,"%s = %d\n", "CONNECTED CLIENTS", clients);
               int servers=0;
               for (tempserver=rootserver;tempserver;tempserver=tempserver->next)
                  servers++;
               fprintf(wzfile,"%s = %d\n", "CONNECTED SERVERS", servers);
               fprintf(wzfile,"%s\n","!CLIENTS");
               char dataseg1[150]; char dataseg2[150]; char dataseg3[150]; char dataseg4[150]; char dataseg5[150]; char dataseg6[2000]; char dataseg7[50];
               for (tempclient=rootclient;tempclient;tempclient=tempclient->next)
               {
                  sprintf(dataseg1,"%s:%s:%s:%s", tempclient->callsign, tempclient->cid, tempclient->realname, tempclient->type==CLIENT_ATC?"ATC":"PILOT");
                  if (tempclient->frequency!=0 && tempclient->frequency<100000 && tempclient)
                     sprintf(dataseg2,"1%02d.%03d", tempclient->frequency/1000, tempclient->frequency%1000);
                  else
                     sprintf(dataseg2,"%s","");
                  tempflightplan=tempclient->plan;
                  if (tempclient->lat!=0 && tempclient->altitude < 100000 && tempclient->lon != 0)
                     sprintf(dataseg3,"%f:%f:%d:%d", tempclient->lat, tempclient->lon, tempclient->altitude, tempclient->groundspeed);
                  else
                     sprintf(dataseg3,"%s",":::");
                  if (tempflightplan)
                     sprintf(dataseg4,"%s:%d:%s:%s:%s", tempflightplan->aircraft, tempflightplan->tascruise, tempflightplan->depairport, tempflightplan->alt, tempflightplan->destairport);
                  else
                     sprintf(dataseg4,"%s","::::");
                  sprintf(dataseg5,"%s:%s:%d:%d:%d:%d", tempclient->location->ident, tempclient->protocol, tempclient->rating, tempclient->transponder, tempclient->facilitytype, tempclient->visualrange);
                  if (tempflightplan)
                     sprintf(dataseg6,"%d:%c:%d:%d:%d:%d:%d:%d:%s:%s:%s", tempflightplan->revision, tempflightplan->type, tempflightplan->deptime, tempflightplan->actdeptime, tempflightplan->hrsenroute, tempflightplan->minenroute, tempflightplan->hrsfuel, tempflightplan->minfuel, tempflightplan->altairport, tempflightplan->remarks, tempflightplan->route);
                  else
                     sprintf(dataseg6,"%s","::::::::::");
                  sprintf(dataseg7,"::::::%s", sprintgmt(tempclient->starttime,s));
                  fprintf(wzfile,"%s:%s:%s:%s:%s:%s:%s\n", dataseg1, dataseg2, dataseg3, dataseg4, dataseg5, dataseg6, dataseg7);
               }
               char dataline[150]; 
               fprintf(wzfile,"%s\n","!SERVERS");
               for (tempserver=rootserver;tempserver;tempserver=tempserver->next)
                  if (strcmp(tempserver->hostname,"n/a") != 0)
                  {
                     sprintf(dataline,"%s:%s:%s:%s:%d", tempserver->ident, tempserver->hostname, tempserver->location, tempserver->name, tempserver->flags&SERVER_SILENT?0:1);
                     fprintf(wzfile,"%s\n",dataline);
                  }; 
               fclose(wzfile);
			   remove(whazzupfile);
               rename(whazzuptemp, whazzupfile);
               fileopen=0;
            }
            else
               fileopen=0;
         }
      }
   }
// WhazzUp End
   server *tempserver=rootserver;
   while (tempserver)
   {
      server *next=tempserver->next;
      if ((now-tempserver->alive)>SERVERTIMEOUT&&(tempserver!=myserver))
         delete tempserver;
      tempserver=next;
   }
   client *tempclient=rootclient;

   /* Check for client timeouts. We should not drop clients if we are in
      silent mode; If we are in silent mode, we won't receive updates, so
      every client would timeout. When we are a silent server, the limit
      will be SILENTCLIENTTIMEOUT, which is about 10 hours  */
   int limit=(myserver->flags&SERVER_SILENT)?SILENTCLIENTTIMEOUT:CLIENTTIMEOUT;
   while (tempclient)
   {
      client *next=tempclient->next;
      if (tempclient->location!=myserver)
      if ((now-tempclient->alive)>limit)
         delete tempclient;
      tempclient=next;
   }
}
void fsd::run()
{
   pmanager->run();
   if (timer!=mtime())
   {
      timer=mtime();
      dochecks();
   }
}
void fsd::configmyserver()
{
   int mode=0;
   char *serverident=NULL, *servername=NULL;
   char *servermail=NULL, *serverhostname=NULL;
   char *serverlocation=NULL;
   configentry *entry;
   configgroup *sysgroup=configman->getgroup("system");
   if (sysgroup)
   {
      sysgroup->changed=0;
      if ((entry=sysgroup->getentry("ident"))!=NULL)
         serverident=entry->getdata();
      if ((entry=sysgroup->getentry("name"))!=NULL)
         servername=entry->getdata();
      if ((entry=sysgroup->getentry("email"))!=NULL)
         servermail=entry->getdata();
      if ((entry=sysgroup->getentry("hostname"))!=NULL)
         serverhostname=entry->getdata();
      if ((entry=sysgroup->getentry("location"))!=NULL)
         serverlocation=entry->getdata();
      if ((entry=sysgroup->getentry("mode"))!=NULL)
         if (!STRCASECMP(entry->getdata(),"silent")) mode=SERVER_SILENT;
   }
   char identbuf[100], mailbuf[1]="", hnamebuf[100], locationbuf[1]="";
   if (!serverident)
   {
      serverident=identbuf;
#ifdef WIN32
	      sprintf(serverident, "%d", GetCurrentProcessId());
#else
	      sprintf(serverident, "%d", getpid());
#endif
      dolog(L_ERR, "No serverident specified");
   }
   if (!servermail)
   {
      servermail=mailbuf;
      dolog(L_ERR,"No server mail address specified");
   }
   if (!serverhostname)
   {
      gethostname(hnamebuf, 100);
      serverhostname=hnamebuf;
      dolog(L_ERR,"No server host name specified");
   }
   if (!servername)
   {
      dolog(L_ERR,"No servername specified");
      servername=serverhostname;
   }
   if (!serverlocation)
   {
      dolog(L_ERR,"No serverlocation specified");
      serverlocation=locationbuf;
   }
   int flags=mode;
   if (metarmanager->source!=SOURCE_NETWORK) flags|=SERVER_METAR;
   if (myserver) myserver->configure(servername, servermail, serverhostname,
       VERSION, serverlocation); else
   myserver=new server(serverident, servername, servermail, serverhostname,
      VERSION, flags, serverlocation);
}
void fsd::configure()
{
   clientport=6809, serverport=3011, systemport=3012;
   configentry *entry;
   configgroup *sysgroup=configman->getgroup("system");
   /* Configure */
   if (sysgroup)
   {
      if ((entry=sysgroup->getentry("clientport"))!=NULL)
         clientport=entry->getint();
      if ((entry=sysgroup->getentry("serverport"))!=NULL)
         serverport=entry->getint();
      if ((entry=sysgroup->getentry("systemport"))!=NULL)
         systemport=entry->getint();
      if ((entry=sysgroup->getentry("certificates"))!=NULL)
         certfile=strdup(entry->getdata());
      if ((entry=sysgroup->getentry("whazzup"))!=NULL)
		 whazzupfile=strdup(entry->getdata());
   }
   configmyserver();
   readcert();
}
void fsd::handlecidline(char *line)
{
   certificate *tempcert;
   int mode, level;
   char *array[4], *cid, *pwd;
   if (line[0]==';'||line[0]=='#') return;
   if (breakargs(line, array, 4)<3) return;
   cid=array[0], level=atoi(array[2]), pwd=array[1];
   tempcert=getcert(cid);
   if (!tempcert)
   {
      tempcert=new certificate(cid, pwd, level, mgmtime(), myserver->ident);
      mode=CERT_ADD;
   } else
   {
      tempcert->livecheck=1;
      if (!STRCASECMP(tempcert->password, pwd)&&level==tempcert->level)
         return;
      tempcert->configure(pwd, level, mgmtime(), myserver->ident);
      mode=CERT_MODIFY;
   }
   if (serverinterface) serverinterface->sendcert("*", mode, tempcert, NULL);
}
void fsd::readcert()
{
   if (!certfile) return;
   FILE *io=fopen(certfile, "r");
   if (!io) dolog(L_ERR, "Could not open certificate file '%s'",
      certfile); else
   {
      certificate *temp;
      for (temp=rootcert;temp;temp=temp->next)
         temp->livecheck=0;
      char line[1000];
      dolog(L_INFO, "Reading certificates from '%s'", certfile);
      while (fgets(line, 1000, io))
         handlecidline(line);
      fclose(io);
      temp=rootcert;
      while (temp)
      {
         certificate *next=temp->next;
         if (!temp->livecheck)
         {
            serverinterface->sendcert("*", CERT_DELETE, temp, NULL);
            delete temp;
         }
         temp=next;
      }
   }
}
void fsd::createmanagevars()
{
   int varnum;
   varnum=manager->addvar("system.boottime",ATT_DATE);
   manager->setvar(varnum, time(NULL));
   varnum=manager->addvar("version.system",ATT_VARCHAR);
   manager->setvar(varnum, VERSION);
}
void fsd::createinterfaces()
{
   char prompt[100];
   sprintf(prompt,"%s> ",myserver->ident);
   clientinterface=new clinterface(clientport, "client", "client interface");
   serverinterface=new servinterface(serverport, "server", "server interface");
   systeminterface=new sysinterface(systemport, "system","system management interface");
   systeminterface->setprompt(prompt);
   
   serverinterface->setfeedstrategy(FEED_BOTH);

   /* Clients may send a maximum of 100000 bytes/second */
   clientinterface->setflood(100000);
   clientinterface->setfeedstrategy(FEED_IN);

   /* Clients may have a buffer of 100000 bytes */
   clientinterface->setoutbuflimit(100000);

   pmanager->registerprocess(clientinterface);
   pmanager->registerprocess(serverinterface);
   pmanager->registerprocess(systeminterface);
}
void fsd::makeconnections()
{
   configgroup *cgroup=configman->getgroup("connections");
   if (!cgroup) return;
   configentry *centry=cgroup->getentry("connectto");
   if (centry)
   {
      /* Connect to the configured servers */
      int x, nparts=centry->getnparts();
      for (x=0;x<nparts;x++)
      {
		 int portnum=3011;
		 char *data=centry->getpart(x), *sportnum;
		 sportnum=strchr(data,':');
		 if (sportnum)
		 {
			int num;
			if (sscanf(sportnum+1,"%d",&num)==1) portnum=num;
			*sportnum='\0';
		 }
		 if (serverinterface->adduser(data,portnum,NULL)!=1)
			dolog(L_ERR,"Connection to %s port %d failed!",data,portnum);
	    else dolog(L_INFO,"Connected to %s port %d",data,portnum);
      }
   }


   centry=cgroup->getentry("allowfrom");
   if (centry)
   {
      /* Allow the configured servers */
      int nparts=centry->getnparts(), x;
      for (x=0;x<nparts;x++)
         serverinterface->allow(centry->getpart(x));
   } else dolog(L_WARNING,
      "No 'allowfrom' found, allowing everybody on the server port");

   serverinterface->sendreset();
}



