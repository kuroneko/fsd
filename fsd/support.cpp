#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <ctime>
#include <cctype>
#ifdef WIN32
	#include <winsock2.h>
#else
	#include <netdb.h>
	#include <unistd.h>
	#include <sys/time.h>
	#include <sys/types.h>
	#include <netinet/in.h>
#endif
#include <climits>
#include <cmath>
#include <csignal>

#include "support.h"
#include "global.h"
#include "server.h"

int logp=0;
time_t timer=100000;
loghis loghistory[MAXLOG];
int mrandseed=0;

void addlog(int level, char *s)
{
   if (loghistory[logp].msg) free(loghistory[logp].msg);
   if (level>=L_MAX) level=L_MAX-1;
   loghistory[logp].msg=strdup(s), loghistory[logp].level=level;
   logp=(logp+1)%MAXLOG;
}
void addfile(char *name, char *string,...)
{
   char buf[1000];
   FILE *logfile=fopen(name,"a");
   va_list ap;

   if (!logfile) return;
   va_start(ap,string);
   vsprintf(buf,string,ap);
   va_end(ap);
   fprintf(logfile, buf);
   fclose(logfile);
}

void dolog(int level, const char *string, ...)
{
   char buf[1000], buf2[1200];
   long secs=time(NULL);
   struct tm *loctime;
   static int firsttime=1;
   FILE *logfile=fopen(LOGFILE,"a");
   char *sident=(char *)(myserver?myserver->ident:"");
   va_list ap;

   if (!logfile) return;
   loctime = localtime(&secs);
   va_start(ap,string);
   vsprintf(buf,string,ap);
   va_end(ap);

   sprintf(buf2,"%02d-%02d-%02d %02d:%02d:%02d %s: %s%s",
      loctime->tm_mday, loctime->tm_mon+1, loctime->tm_year,
      loctime->tm_hour,loctime->tm_min,loctime->tm_sec, sident,
      (level<L_WARNING)?"** ":"", buf);
   fprintf(logfile,"%s\n", buf2);
   fclose(logfile);
   addlog(level, buf2);
}
/* Breaks a packet into pieces, s is the packet, **a is the array
   of pointers to characters and max is the amount of entries in the
   array */
int breakpacket(char *s, char **a, int max)
{
   int num=0;
   char *p;
   while (1)
   {
      p=strchr(s,':');
      a[num++]=s;
      if (!p) return num;
      *(p++)='\0';
      s=p;
      if (num==max) return num;
   }
}
char *catcommand(char **array, int n, char *buf)
{
   int x;
   buf[0]='\0';
   for (x=0;x<n;x++)
   {
      if (x) strcat(buf,":");
      strcat(buf,array[x]);
   }
   return buf;
}

int breakargs(char *s, char **a, int max)
{
   int num=0;
   char *p;
   while (1)
   {
      while (isspace(*s)) s++;
      if (!*s) return num;
      p=s;
      while (*s&&!isspace(*s)) s++;
      if (*s) *(s++)='\0';
      a[num++]=p;
      if (num==max) return num;
   }
}
char *catargs(char **array, int count, char *buf)
{
   int c;
   buf[0]='\0';
   for (c=0;c<count;c++)
   { 
      if (buf[0]) strcat(buf," ");
      strcat(buf,array[c]);
   }
   return buf;
}
/* Takes <bytes> bytes from the packet s and puts it into buf */
void snappacket(char *s, char *buf, int bytes)
{
   strncpy(buf,s,bytes);
   buf[bytes]='\0';
}
void findhostname(unsigned long ip, char *buf)
{
   hostent *hent=gethostbyaddr((char*)&ip, sizeof(ip), AF_INET);
   ip=ntohl(ip);
   if (hent) strcpy(buf,hent->h_name); else
      sprintf(buf,"%ld.%ld.%ld.%ld", ip>>24, 0xff&(ip>>16), 0xff&(ip>>8), 0xff&ip);
}
char *strupr(char *line)
{
   char *p;
   for (p=line;*p;p++) *p=toupper(*p);
   return line;
}
double dist(double lat1, double lon1, double lat2, double lon2)
{
   double dist, dlon=lon2-lon1;
   lat1*=M_PI/180.0, lat2*=M_PI/180.0, dlon*=M_PI/180.0;
   dist=(sin(lat1)*sin(lat2))+(cos(lat1)*cos(lat2)*cos(dlon));
   if (dist>1.0) dist=1.0;
   dist=acos(dist)*60*180/M_PI;
   return dist;
}
char *printpart(double part, char p1, char p2, char *buf)
{
   char c=part<0.0?p2:p1;
   part=fabs(part);
   double degrees=floor(part), min, sec;
   part-=degrees; part*=60;
   min=floor(part); part-=min; part*=60;
   sec=part;
   sprintf(buf,"%c %02d %02d' %02d\"",c,(int)degrees,(int)min,(int)sec);
   return buf;
}
char *printloc(double lat, double lon, char *buf)
{
   char north[100], east[100];
   printpart(lat, 'N', 'S', north);
   printpart(lon, 'E', 'W', east);
   sprintf(buf,"%s %s",north,east);
   return buf;
}
void handlealarm(...)
{
   timer++;
}
time_t mtime()
{
   /*return timer;*/
   return time(NULL);
}
void starttimer()
{
/* We try without all signal/timer stuff this release */
/*

   struct itimerval t;
   struct sigaction act;
   sigset_t set;
   act.sa_handler=handlealarm;

   sigemptyset(&set);
   act.sa_mask=set;

   act.sa_flags=SA_RESTART; 
   sigaction(SIGALRM, &act, NULL);
   t.it_value.tv_sec=1, t.it_value.tv_usec=0;
   t.it_interval.tv_sec=1, t.it_interval.tv_usec=0;
   setitimer(ITIMER_REAL, &t, NULL);
*/
}
char *sprinttime(time_t now, char *buf)
{
   int s=now%60, m=(now%3600)/60, h=now/3600;
   sprintf(buf,"%02d:%02d:%02d",h,m,s);
   return buf;
}
char *sprintdate(time_t now, char *buf)
{
   struct tm *m=localtime(&now);
   if (!now) return strcpy(buf, "<none>");
   sprintf(buf,"%02d-%02d-%04d %02d:%02d:%02d", m->tm_mday, m->tm_mon+1,
      m->tm_year+1900, m->tm_hour, m->tm_min, m->tm_sec);
   return buf;
}
char *sprintgmtdate(time_t now, char *buf)
{
   struct tm *m=gmtime(&now);
   sprintf(buf, "%02d/%02d/%04d %02d:%02d", m->tm_mday, m->tm_mon+1, m->tm_year+1900,
       m->tm_hour, m->tm_min);
   return buf;
}
char *sprintgmt(time_t now, char *buf)
{
   struct tm *m=gmtime(&now);
   sprintf(buf, "%4d%2d%2d%2d%2d%2d", m->tm_year+1900, m->tm_mon+1,
      m->tm_mday, m->tm_hour, m->tm_min, m->tm_sec);
   char *p;
   for (p=buf;*p;++p)
      if (*p ==' ')
          *p='0';
   return buf;
}
void msrand(int seed)
{
   mrandseed=seed;
}
int mrand()
{
   mrandseed^=0x22591d8c;
   int part1=(mrandseed<<1)&0xffffffff, part2=mrandseed>>31;
   mrandseed^=(part1|part2);
   if (sizeof(int)>4) mrandseed&=0xffffffff;
   return mrandseed;
}
int mround(double val)
{
   double frac;
  /* quick patch for the floating point exceptions */
   if (val>INT_MAX) return INT_MAX;

   frac=val-((double)((int)val));
   if (frac>0.5) return ((int)val)+1; else
   if (frac<-0.5) return ((int)val)-1; else
      return (int)val;
}
time_t mgmtime()
{
   time_t now=time(NULL);
   return mktime(gmtime(&now));
}
void donothing(int)
{
}
int mconnect(int sockfd, struct sockaddr *serv_addr, int addrlen, int timeout)
{
   int result;
#ifdef WIN32
   result=connect(sockfd, serv_addr, addrlen);
#else
   struct sigaction sanew, saold;
   sanew.sa_handler=donothing;
   sigemptyset(&sanew.sa_mask);
   sanew.sa_flags=0;
   sigaction(SIGALRM, &sanew, &saold);
   alarm(timeout);
   result=connect(sockfd, serv_addr, addrlen);
   alarm(0);
   sigaction(SIGALRM, &saold, NULL);
#endif
   return result;
}
void dblog(char *msg, int num)
{
   if (num==0) return;
   dolog(L_ERR, "%d,%s", num,msg);
}
