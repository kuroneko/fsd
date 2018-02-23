#include <cstdio>
#include <cstdlib>
#ifdef WIN32
	#include <winsock2.h>
#else
	#include <unistd.h>
	#include <netdb.h>
	#include <sys/types.h>
	#include <sys/time.h>
	#include <netinet/in.h>
#endif
#include <cstring>
#include <ctime>
#include <cerrno>
#include <sys/stat.h>
#include "mm.h"
#include "fsd.h"
#include "wprofile.h"
#include "manage.h"
#include "support.h"
#include "config.h"
#include "global.h"

mm *metarmanager=NULL;

mm::mm()
{
   prevdownload=0L, ioin=ioout=NULL, newfileready=0;
   sock=datasock=datareadsock=-1;
   passivemode=1;	sockrecvbufferlen=0;

   source=SOURCE_NETWORK, downloading=0, rootq=NULL, ftpemail=NULL;
   prevhour=-1, nstations=0, stationlist=NULL, metarhost=metardir=NULL;
   configgroup *wgroup=configman->getgroup("weather");
   configentry *wentry=wgroup?wgroup->getentry("source"):(configentry*)NULL;
   if (wentry)
   {
      char *buf=wentry->getdata();
      if (!STRCASECMP(buf,"network")) source=SOURCE_NETWORK; else
      if (!STRCASECMP(buf,"file")) source=SOURCE_FILE; else
      if (!STRCASECMP(buf,"download")) source=SOURCE_DOWNLOAD; else
         dolog(L_ERR,"Unknown METAR source '%s' in config file", buf);
   }
   wentry=wgroup?wgroup->getentry("server"):(configentry*)NULL;
   if (wentry) metarhost=strdup(wentry->getdata());
   wentry=wgroup?wgroup->getentry("dir"):(configentry*)NULL;
   if (wentry) metardir=strdup(wentry->getdata());
   wentry=wgroup?wgroup->getentry("ftpmode"):(configentry*)NULL;
   if (wentry)
   {
      char *buf=wentry->getdata();
      if (!STRCASECMP(buf,"passive")) passivemode=1; else
      if (!STRCASECMP(buf,"ative")) passivemode=0;
   }
	
   wgroup=configman->getgroup("system");
   wentry=wgroup?wgroup->getentry("email"):(configentry*)NULL;
   if (wentry) ftpemail=strdup(wentry->getdata());

   if (source==SOURCE_DOWNLOAD)
   {
      if (!metarhost) metarhost=strdup("weather.noaa.gov");
      if (!metardir) metardir=strdup("data/observations/metar/cycles/");
   }

   int var=manager->addvar("metar.method", ATT_VARCHAR);
   manager->setvar(var, (char *)(source==SOURCE_NETWORK?"network":
      source==SOURCE_FILE?"file":"download"));
   varprev=manager->addvar("metar.current", ATT_DATE);
   vartotal=manager->addvar("metar.requests", ATT_INT);
   varstations=manager->addvar("metar.stations", ATT_INT);
   manager->setvar(varprev, time(NULL));
   if (source==SOURCE_FILE) buildlist();
}
mm::~mm()
{
   if (ioin) fclose(ioin);
   if (ioout) fclose(ioout);
   if (stationlist) free(stationlist);
   if (metarhost) free(metarhost);
   if (metardir) free(metardir);
}
int mm::calcmasks(fd_set *rmask, fd_set *wmask)
{
   if (downloading!=1) return 0;
   int max=sock;
   if (datasock>max) max=datasock;
   if (datareadsock>max) max=datareadsock;
   FD_SET(sock, rmask);
   if (datareadsock!=-1) FD_SET(datareadsock, rmask); else
      FD_SET(datasock, rmask);
   return max;
}
void mm::check()
{
   time_t now=time(NULL);
   struct tm *t=gmtime(&now);
   if (prevhour==-1||(prevhour!=t->tm_hour&&(t->tm_hour==12||t->tm_hour==0)))
   {
      int x;
      prevhour=t->tm_hour;
      msrand((+t->tm_hour)*t->tm_year*(1+t->tm_mon));
      for (x=0;x<VAR_AMOUNT;x++) variation[x]=mrand();
   }
   if (source==SOURCE_DOWNLOAD)
   {
      time_t mnow=mtime();
      struct tm *p=gmtime(&now);
      int elapsed=mnow-prevdownload;
      if ((p->tm_min==10&&elapsed>60)||(elapsed>3600)||prevdownload==0)
         startdownload();
   }
}
int comparestation(const void *p1, const void *p2)
{
   station *s1=(station *)p1, *s2=(station *)p2;
   return strcmp(s1->name, s2->name);
}
void mm::buildlist()
{
   FILE *io=fopen(METARFILE,"r");
   struct stat statbuf;
   off_t offset;
   int max=2000;
   char line[100], *array[3];
   if (!io) return;
   fstat(fileno(io), &statbuf);
   metarfiletime=statbuf.st_mtime;
   nstations=0;
   if (stationlist) free(stationlist);
   stationlist=(station*)malloc(sizeof(station)*max);
   for (offset=(off_t) 0;fgets(line, 100, io);offset=(off_t)ftell(io))
   {
      if (strlen(line)<30) continue;
      int count=breakargs(line, array, 3);
      if (count<3) continue;
      if (strncmp(line,"     ",5)==0) continue;
      char *statname=(strlen(array[0])==4)?array[0]:strlen(array[1])==4?
         array[1]:(char*)NULL; 
      if (!statname) continue;
      if (nstations==max)
         stationlist=(station*) realloc(stationlist, sizeof(station)*
         (max+=1000));
      stationlist[nstations].location=offset;
      strupr(statname);
      strcpy(stationlist[nstations++].name,statname);
   }
   stationlist=(station*)realloc(stationlist, sizeof(station)*nstations);
   qsort(stationlist, nstations, sizeof(station), comparestation);
   manager->setvar(varstations, nstations);
   fclose(io);
}
void mm::setupnewfile()
{
   newfileready=0;
   if (metarsize<100000)
   {
      unlink(METARFILENEW);
      dolog(L_WARNING, "METAR: Size of new METAR file (%d) is too small, dropping.",
         metarsize);
      return;
   }
   unlink(METARFILE);
   if (rename(METARFILENEW, METARFILE))
      dolog(L_ERR, "METAR: Can't move %s to %s: %s",METARFILENEW, METARFILE, 
      strerror(errno));
   else dolog(L_INFO,"METAR: Installed new METAR data.");
   buildlist();
   manager->setvar(varprev, time(NULL));
}
int mm::run()
{
   if (source==SOURCE_NETWORK) return 0;
   if (downloading) dodownload();
   while (rootq) if (doparse(rootq)==0) break;
   if (!downloading&&newfileready) setupnewfile();
   check();
   return 0;
}
void mm::stopdownload()
{
	if (sockrecvbufferlen)
		free(sockrecvbuffer);
	sockrecvbufferlen=0;
   if (ioin) fclose(ioin);
   if (sock!=-1) CLOSESOCKET(sock);
   if (datasock!=-1) CLOSESOCKET(datasock);
   if (datareadsock!=-1) CLOSESOCKET(datareadsock);
   ioin=NULL, downloading=0, sock=datasock=datareadsock=-1;
}
void mm::startdownload()
{
   struct sockaddr_in sin, dataadr;
   struct hostent *hent;
   char *s, cv[100];
   if (downloading)
   {
      dolog(L_ERR, "METAR: server seems to be still loading");
      return;
   }
   prevdownload=mtime();
   ioin=fopen(METARFILENEW,"w");
   if (!ioin)
   {
      dolog(L_ERR, "METAR: Can't open %s.", METARFILENEW);
      stopdownload();
      return;
   }
   hent=gethostbyname(metarhost);
   if (!hent)
   {
      dolog(L_ERR, "METAR: Could not lookup METAR host name %s.", metarhost);
      stopdownload();
      return;
   }
   sock=socket(AF_INET,SOCK_STREAM,0);
   if (sock<0) return;
   memcpy((char*)&sin.sin_addr, (char*)hent->h_addr, hent->h_length);
   sin.sin_port=htons(21), sin.sin_family=AF_INET;
   if (connect(sock,(struct sockaddr*)&sin,sizeof(sin))==-1)
   {
      dolog(L_ERR, "METAR: Could not connect to ftp port on METAR host");
      stopdownload();
      return;
   }
   dataadr.sin_family=AF_INET;
   dataadr.sin_port=htons(0);
   dataadr.sin_addr.s_addr=htonl(INADDR_ANY);
   datasock=socket(AF_INET,SOCK_STREAM,0);
   bind(datasock, (struct sockaddr *)&dataadr, sizeof(dataadr));
   listen(datasock, 1);

   char data[1000], port[100];
   socklen_t name=sizeof(dataadr);
   getsockname(datasock, (struct sockaddr *)&dataadr, &name);
   int portnum=ntohs(dataadr.sin_port);
   name=sizeof(dataadr);
   getsockname(sock, (struct sockaddr *)&dataadr, &name);
   unsigned long adr=ntohl((unsigned long)dataadr.sin_addr.s_addr);

   sprintf(port, "%d,%d,%d,%d,%d,%d", (adr>>24)&0xff, (adr>>16)&0xff,
      (adr>>8)&0xff, adr&0xff, (portnum>>8)&0xff, portnum&0xff);

   time_t now=time(NULL);
   struct tm *tp=gmtime(&now);
	if (passivemode)
	{
	   sprintf(data,"USER anonymous\nPASS %s\nCWD %s\nPASV\n",
		  ftpemail, metardir);
	   WRITESOCK(sock, data, strlen(data));
	   downloading=1, metarsize=0, datareadsock=-1;
	}
	else
	{
	   sprintf(data,"USER anonymous\nPASS %s\nCWD %s\nPORT %s\nRETR %02dZ.TXT\n",
		  ftpemail, metardir, port, (tp->tm_hour+21)%24);
	   WRITESOCK(sock, data, strlen(data));
	   dolog(L_INFO,"METAR: Starting download of METAR data");
	   downloading=1, metarsize=0, datareadsock=-1;
	}
}
void mm::dodownload()
{
   if ((mtime()-prevdownload)>MAXMETARDOWNLOADTIME)
   {
      dolog(L_WARNING, "METAR download interrupted due to timeout");
      stopdownload();
      startdownload();
      return;
   }
   if (FD_ISSET(sock, rmask))
   {
	   char buf[4096];
	   int bytes=READSOCK(sock, buf, 4096);
   /*char* sockrecvbuffer;
   int sockrecvbufferlen;*/
       if (bytes<=0)
	   {
		  stopdownload();
		  newfileready=1;
	   }
	   else if (passivemode)//&&(datareadsock==-1))
	   {
		   char* newbuffer = (char*) malloc(bytes+sockrecvbufferlen);
		   memcpy(newbuffer,sockrecvbuffer,sockrecvbufferlen);
		   	if (sockrecvbufferlen)
				free(sockrecvbuffer);
		   memcpy(&newbuffer[sockrecvbufferlen],buf,bytes);
		   sockrecvbufferlen+=bytes;
		   sockrecvbuffer=newbuffer;
		   char* sentence = newbuffer;
		   int p = 0;
		   while (p<sockrecvbufferlen)
		   {
			   if (sockrecvbuffer[p]==0xd)
			   {
					sockrecvbuffer[p]=0;
					if (atoi(sentence)==227)
					{
						char* sep = sentence;
						while (*sep !='(') sep++;
						char* port = sep+1;
						sep = port;
						while (*sep !=')') sep++;
						*sep=0;
						unsigned int adr24,adr16,adr8,adr0,port8,port0;
						sscanf(port, "%d,%d,%d,%d,%d,%d", &adr24,&adr16,&adr8,&adr0,&port8,&port0);
						struct sockaddr_in dataadr;
						dataadr.sin_addr.s_addr = (adr24)+(adr16<<8)+(adr8<<16)+(adr0<<24);
						dataadr.sin_port = (port8) + (port0<<8);
						dataadr.sin_family=AF_INET;
					    datareadsock=socket(AF_INET,SOCK_STREAM,0);
					    if (connect(datareadsock,(struct sockaddr*)&dataadr,sizeof(dataadr))==-1)
						{
						  dolog(L_ERR, "METAR: Could not connect to ftp port on METAR host");
						  stopdownload();
						  return;
						}
						{
						   char data[1000];
    					   time_t now=time(NULL);
	    				   struct tm *tp=gmtime(&now);
						   sprintf(data,"RETR %02dZ.TXT\n",(tp->tm_hour+21)%24);
						   WRITESOCK(sock, data, strlen(data));
						   dolog(L_INFO,"METAR: Starting download of METAR data");
						}
					}
			   }
			   if (sockrecvbuffer[p]==0xa)
			   {
				   sentence = &sockrecvbuffer[p+1];
			   }
			   p++;
		   }
		   if ((sentence<sockrecvbuffer+sockrecvbufferlen)&&(sentence!=sockrecvbuffer))
		   {
			   newbuffer = (char*) malloc(sockrecvbuffer+sockrecvbufferlen-sentence);
			   memcpy(newbuffer,sentence,sockrecvbuffer+sockrecvbufferlen-sentence);
			   sockrecvbufferlen = sockrecvbuffer+sockrecvbufferlen-sentence;
			   free(sockrecvbuffer);
			   sockrecvbuffer= newbuffer;
		   }
		   else if (sentence>=sockrecvbuffer+sockrecvbufferlen)
		   {
			   if (sockrecvbufferlen)
				   free(sockrecvbuffer);
			   sockrecvbufferlen=0;
		   }
	   }
   }
   if (datareadsock==-1)
   {
      if (!FD_ISSET(datasock, rmask)) return;
      datareadsock=accept(datasock, NULL, NULL);
      return;
   }
   if (!FD_ISSET(datareadsock, rmask)) return;
   char buf[1024];
   int bytes=READSOCK(datareadsock, buf, 1024);
   if (bytes<=0)
   {
      stopdownload();
      newfileready=1;
   } else
   {
      fwrite(buf, 1, bytes, ioin);
      metarsize+=bytes;
   }
}
void mm::checkmetarfile()
{
   struct stat sb;
   if (stat(METARFILE, &sb)==-1) return;
   if (sb.st_mtime!=metarfiletime) buildlist();
}
int mm::doparse(mmq *q)
{
   station key;
   checkmetarfile();
   strncpy(key.name, q->metarid, 4);
   key.name[4]='\0';
   strupr(key.name);
   station *location=(station *) bsearch(&key, stationlist, nstations,
      sizeof(station), comparestation);
   if (!location)
   {
      serverinterface->sendnowx(q->destination, q->fd, q->metarid);
      delq(q);
      return 1;
   }
   ioout=fopen(METARFILE, "r");
   if (!ioout) return 0;
   fseek(ioout, location->location, SEEK_SET);
   char line[500]="", piece[100];
   int first=1;
   do
   {
      if (!getline(piece)) break;
      int addition=strncmp(piece, "     ", 5)==0;
      if (addition&&first) break;
      if (!addition&&!first) break;
      first=0;
      strcat(line, piece+(addition?4:0));
   } while (1);
   fclose(ioout);
   ioout=NULL;
   if (q->parsed)
   {
      char *array[100];
      int count=breakargs(line, array, 100);
      wprofile pr(location->name, 0, NULL);
      pr.parsemetar(array,count);
      serverinterface->sendweather(q->destination, q->fd, &pr);
   } else
   {
      serverinterface->sendmetar(q->destination, q->fd,
         location->name, line);
   }
   manager->incvar(vartotal);
   delq(q);
   return 1;
}


int mm::getline(char *line)
{
   if (!fgets(line, 100, ioout))
      return 0;
   prepareline(line);
   return 1;
}
void mm::prepareline(char *line)
{
   int pos=strlen(line)-1;
   while (line[pos]=='\n'||line[pos]=='\r'||line[pos]=='=')
      line[pos--]='\0';
}
server *mm::requestmetar(char *client, char *metar, int parsed, int fd)
{
   if (source==SOURCE_NETWORK)
   {
      server *temp, *best=NULL;
      int hops=-1;
      for (temp=rootserver;temp;temp=temp->next)
         if (temp!=myserver&&(temp->flags&SERVER_METAR))
            if (hops==-1||(temp->hops<hops&&temp->hops!=-1))
               best=temp, hops=temp->hops;
      if (!best) return NULL;
      serverinterface->sendreqmetar(client, metar, fd, parsed, best);
      return best;
   } else addq(client, metar, parsed, fd);
   return myserver;
}
void mm::addq(char *dest, char *metar, int parsed, int fd)
{
   wprofile *prof=getwprofile(metar);
   if (prof&&prof->active)
   {
      if (parsed) serverinterface->sendweather(dest, fd, prof); else
         serverinterface->sendmetar(dest, fd, metar, prof->rawcode);
      return;
   }
   mmq *temp=new mmq;
   temp->destination=strdup(dest), temp->metarid=strdup(metar);
   temp->fd=fd, temp->next=rootq, temp->prev=NULL;
   temp->parsed=parsed;
   if (temp->next) temp->next->prev=temp;
   rootq=temp;
}
void mm::delq(mmq *p)
{
   free(p->destination);
   free(p->metarid);
   if (p->next) p->next->prev=p->prev;
   if (p->prev) p->prev->next=p->next; else rootq=p->next;
   delete p;
}
int mm::getvariation(int num, int min, int max)
{
   int val=variation[num], range=max-min+1;
   val=(abs(val)%range)+min;
   return val;
}
