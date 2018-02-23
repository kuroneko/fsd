#include <cstdio>
#include <cstdlib>
#ifndef WIN32
	#include <unistd.h>
	#include <sys/time.h>
#endif
#include <ctime>
#include <cstring>

#include "interface.h"
#include "sysinterface.h"
#include "global.h"
#include "support.h"
#include "server.h"
#include "protocol.h"
#include "client.h"
#include "sysuser.h"

/* The client interface class */

sysinterface::sysinterface(int port, char *code , char *d):
    tcpinterface(port,code,d)
{
}
int sysinterface::run()
{
   return tcpinterface::run();
}

void sysinterface::newuser(int fd, char *peer, int portnum, int g)
{
   insertuser(new sysuser(fd,this,peer,portnum,g));
}

void sysinterface::receivepong(char *from, char *data, char *pc, char *hops)
{
   int fd;
   absuser *temp;
   time_t now;
   if (sscanf(data,"%d %lu",&fd,&now)!=2) return;
   if (fd==-1) return;
   for (temp=rootuser;temp;temp=temp->next) if (temp->fd==fd)
   {
      temp->uprintf("\r\nPONG received from %s: %d seconds (%s,%s)\r\n",
         from, mtime()-now, pc, hops);
      temp->printprompt();
      return;
   }
}
void sysinterface::receiveweather(int fd, wprofile *w)
{
   absuser *temp;
   if (fd==-2)
   {
      char buffer[1000], *array[100];
      w->print(buffer);
      int count=breakpacket(buffer, array, 100);
      wprofile *wp=new wprofile(w->name, mgmtime(), myserver->ident);
      wp->loadarray(array, count);
      return;
   }
   for (temp=rootuser;temp;temp=temp->next) if (temp->fd==fd)
   {
      sysuser *st=(sysuser *)temp;
      w->fix(st->lat, st->lon);
      ((sysuser *)temp)->printweather(w);
      break;
   }
}
void sysinterface::receivemetar(int fd, char *wp, char *w)
{
   absuser *temp;
   for (temp=rootuser;temp;temp=temp->next) if (temp->fd==fd)
   {
      ((sysuser *)temp)->printmetar(wp,w);
      break;
   }
}
void sysinterface::receivenowx(int fd, char *st)
{
   absuser *temp;
   if (fd==-2)
   {
      wprofile *wp=new wprofile(st, mgmtime(), myserver->ident);
   }
   for (temp=rootuser;temp;temp=temp->next) if (temp->fd==fd)
   {
      temp->uprintf("\r\nNo weather available for %s.\r\n", st);
      temp->printprompt();
      break;
   }
}
