#include <cstdio>
#include <cstdlib>
#ifdef WIN32
	#include <winsock2.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/timeb.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <netdb.h>
#endif
#include <ctime>
#include <cctype>
#include <fcntl.h>
#include <cstdarg>
#include <cstring>
#include <cerrno>

#include "interface.h"
#include "global.h"
#include "support.h"
#include "user.h"

const char *killreasons[]=
{
   "",
   "closed on command",
   "flooding",
   "initial timeout",
   "socket stalled",
   "connection closed",
   "write error",
   "killed on command",
   "protocol revision error"
};
tcpinterface::tcpinterface(int port, char *code, char *descr)
{
   int on=1;
   struct sockaddr_in sa;
   description=strdup(descr);
   rootallow=NULL, floodlimit=-1, outbuflimit=-1;
   makevars(code);
   memset(&sa,0,sizeof(sa));
   sa.sin_family=AF_INET;
   sa.sin_port=htons(port);
   sa.sin_addr.s_addr=htonl(INADDR_ANY);
   sock=socket(AF_INET,SOCK_STREAM,0);
   if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&on,sizeof(on))<0)
   {
      dolog(L_ERR,"setsockopt error on port %d",port);
      perror("setsockopt");
      exit(1);
   }
   if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on,sizeof(on))<0)
   {
      dolog(L_ERR,"Could not set TCP_NODELAY on port %d",port);
   }
   if (bind(sock,(struct sockaddr *)&sa, sizeof(sa))<0)
   {
      dolog(L_CRIT,"Bind error on port %d",port);
      perror("bind");
      exit(1);
   }
   if (listen(sock,5)<0)
   {
      perror("listen");
      exit(1);
   }
   rootuser=NULL, prompt=NULL, rootgs=NULL;
   feedstrategy=0, prevchecks=0;
   dolog(L_INFO,"Booting port %d (%s)",port,description);
}
tcpinterface::~tcpinterface()
{
   if (prompt) free(prompt);
   if (description) free(description);
   CLOSESOCKET(sock);
}
void tcpinterface::makevars(char *code)
{
   char varname[100];
   sprintf(varname,"interface.%s.current", code);
   varcurrent=manager->addvar(varname, ATT_INT);
   sprintf(varname,"interface.%s.total", code);
   vartotal=manager->addvar(varname, ATT_INT);
   sprintf(varname,"interface.%s.peak", code);
   varpeak=manager->addvar(varname, ATT_INT);


   sprintf(varname,"interface.%s.command", code);
   varclosed[1]=manager->addvar(varname, ATT_INT);
   sprintf(varname,"interface.%s.flood", code);
   varclosed[2]=manager->addvar(varname, ATT_INT);
   sprintf(varname,"interface.%s.initialtimeout", code);
   varclosed[3]=manager->addvar(varname, ATT_INT);
   sprintf(varname,"interface.%s.stalled", code);
   varclosed[4]=manager->addvar(varname, ATT_INT);
   sprintf(varname,"interface.%s.closed", code);
   varclosed[5]=manager->addvar(varname, ATT_INT);
   sprintf(varname,"interface.%s.writeerr", code);
   varclosed[6]=manager->addvar(varname, ATT_INT);
   sprintf(varname,"interface.%s.killed", code);
   varclosed[7]=manager->addvar(varname, ATT_INT);
   sprintf(varname,"interface.%s.protocol", code);
   varclosed[8]=manager->addvar(varname, ATT_INT);
}
void tcpinterface::addguard(absuser *who)
{
   guardstruct *temp=(guardstruct *)malloc(sizeof(guardstruct));
   temp->next=rootgs, temp->prev=NULL;
   if (temp->next) temp->next->prev=temp;
   temp->host=strdup(who->peer), temp->port=who->port;
   rootgs=temp, temp->prevtry=mtime();
}
void tcpinterface::setflood(int limit)
{
   floodlimit=limit;
}
void tcpinterface::setfeedstrategy(int strat)
{
   feedstrategy=strat;
}
void tcpinterface::setoutbuflimit(int limit)
{
   outbuflimit=limit;
}
void tcpinterface::allow(char *name)
{
   struct hostent *hent=gethostbyname(name);
   if (!hent)
   {
      dolog(L_ERR,"Server %s unknown in allowfrom", name);
      return;
   }
   allowstruct *a=new allowstruct();
   a->next=rootallow;
   rootallow=a;
   memcpy(a->ip, (char *)hent->h_addr, 4);
}
int tcpinterface::calcmasks(fd_set *rmask, fd_set *wmask)
{
   absuser *temp;
   int max=sock;
   FD_SET(sock,rmask);
   for (temp=rootuser;temp;temp=temp->next) if (!temp->killflag)
   {
      if (temp->fd>max) max=temp->fd;
      temp->setmasks(rmask, wmask);
   }
   return max;
}
void tcpinterface::newuser(void)
{
   struct sockaddr_in saddr;
   allowstruct *temp;
   char buf[1000];
#ifdef WIN32
   int len=sizeof(saddr);
#else
   socklen_t len=sizeof(saddr);
#endif
   int ok=0;
   int fd=accept(sock,(struct sockaddr *) &saddr, &len);
   if (fd<0) return;
   for (temp=rootallow;temp;temp=temp->next)
      if ((*(int*)temp->ip)==saddr.sin_addr.s_addr)
   {
      ok=1;
      break;
   }
#ifdef WIN32
   strcpy(buf, inet_ntoa(saddr.sin_addr));
#else
   findhostname(saddr.sin_addr.s_addr, buf);
#endif
   if (ok==0&&rootallow)
   {
      char *message="#You are not allowed on this port.\r\n";
	  WRITESOCK(fd,message,strlen(message));
      dolog(L_INFO,"Connection rejected from %s",buf);
      CLOSESOCKET(fd);
      return;
   }
   dolog(L_INFO,"Connection accepted from %s on %s", buf, description);
   int off=1;
   if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&off,
      sizeof(off))<0)
   {
      dolog(L_ERR,"Could not set off TCP_NODELAY");
   }
#ifdef WIN32
   unsigned long io=1;
   ioctlsocket(sock,FIONBIO,&io);
#endif
   newuser(fd, buf, ntohs(saddr.sin_port), 0);
}
void tcpinterface::newuser(int fd, char *peername, int portnum, int g)
{
   /* insertuser(new user(fd, peername,portnum)); */
}
void tcpinterface::insertuser(absuser *u)
{
   u->next=rootuser;
   if (rootuser) rootuser->prev=u;
   rootuser=u;
   if (prompt)
   {
      u->setprompt(prompt);
      u->printprompt();
   }
   manager->incvar(vartotal);
   manager->incvar(varcurrent);
   int peak=manager->getvar(varpeak)->value.number;
   int now=manager->getvar(varcurrent)->value.number;
   if (now>peak) manager->setvar(varpeak, now);
   
}
void tcpinterface::removeuser(absuser *u)
{
   if (u->next) u->next->prev=u->prev;
   if (u->prev) u->prev->next=u->next; else rootuser=u->next;
   if (varcurrent!=-1)
      manager->decvar(varcurrent);
   delete u;
}
int tcpinterface::run()
{
   int busy=0;
   time_t now=mtime();
   if (FD_ISSET(sock,rmask)) newuser();
   absuser *temp=rootuser;
   while (temp)
   {
      absuser *next=temp->next;
      if (temp->killflag==0)
      {
         if (FD_ISSET(temp->fd,wmask)) temp->output();
         if (FD_ISSET(temp->fd,rmask)) temp->input();
         if (temp->run()) busy=1;
      } else removeuser(temp);
      temp=next;
   }
   if (prevchecks!=mtime()) dochecks();
   return busy;
}
void tcpinterface::dochecks()
{
   absuser *temp;
   time_t now=mtime();
   prevchecks=now;
   for (temp=rootuser;temp;temp=temp->next) if (!temp->killflag)
   { 
      if (temp->timeout)
         temp->kill(KILL_DATATIMEOUT);
      if (feedstrategy&(now-temp->prevfeedcheck>=USERFEEDCHECK))
         temp->calcfeed();
      if (now-temp->lastping>=USERPINGTIMEOUT)
      {
         temp->sendping();
         temp->lastping=now;
      } else if (now-temp->lastactive>=USERTIMEOUT)
      {
         temp->uprintf("# Timeout\r\n");
         temp->timeout=1;
      }
   }
   guardstruct *tgs=rootgs;
   while (tgs)
   {
      guardstruct *next=tgs->next;
      if ((mtime()-tgs->prevtry)>GUARDRETRY)
      {
         tgs->prevtry=mtime();
         if (adduser(tgs->host, tgs->port, NULL)!=0)
         {
            free(tgs->host);
            if (tgs->next) tgs->next->prev=tgs->prev;
            if (tgs->prev) tgs->prev->next=tgs->next; else rootgs=tgs->next;
            free(tgs);
         }
      }
      tgs=next;
   }
}
void tcpinterface::setprompt(char *s)
{
   absuser *temp;
   if (prompt) free(prompt);
   prompt=strdup(s);
   for (temp=rootuser;temp;temp=temp->next) temp->setprompt(s);
}
int tcpinterface::adduser(char *name, int port, absuser *terminal)
{
   int sock;
   struct sockaddr_in sin;
   struct hostent *hent;
   absuser *temp;
   for (temp=rootuser;temp;temp=temp->next)
      if (temp->port==port&&!STRCASECMP(temp->peer, name))
   {
      if (terminal)
         terminal->uprintf("Already connected to %s\r\n",name);
      return -1;
   }
   hent=gethostbyname(name);
   if (!hent)
   {
      if (terminal)
         terminal->uprintf("Unknown hostname: %s\r\n",name);
      return 0;
   }
   sock=socket(AF_INET,SOCK_STREAM,0);
   if (sock<0) return 0;
   memcpy((char*)&sin.sin_addr, (char*)hent->h_addr, hent->h_length);
   sin.sin_port=htons(port), sin.sin_family=AF_INET;
   if (terminal)
      terminal->uprintf("Connecting to %s port %d.\r\n",name,port);
   int error, count=0;
#ifdef WIN32
   while ((error=mconnect(sock,(struct sockaddr*)&sin,sizeof(sin),3))==-1&&WSAGetLastError()==WSAEINTR) 
#else
   while ((error=mconnect(sock,(struct sockaddr*)&sin,sizeof(sin),3))==-1&&errno==EINTR) 
#endif
	   if (++count==3) break;
   if (error==-1)
   {
#ifdef WIN32
      error=WSAGetLastError();
      if (error==WSAEINTR) error=WSAETIMEDOUT;
      if (terminal)
         terminal->uprintf("Could not connect to server: %s\r\n",
         strerror(error));
#else
      error=errno;
      if (error==EINTR) error=ETIMEDOUT;
      if (terminal)
         terminal->uprintf("Could not connect to server: %s\r\n",
         strerror(error));
#endif
	  CLOSESOCKET(sock);
      return 0;
   }
   if (terminal) 
      terminal->uprintf("Connection established.\r\n");
   int off=1;
   if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&off,
      sizeof(off))<0)
   {
      dolog(L_ERR,"Could not set off TCP_NODELAY");
   }
#ifdef WIN32
   unsigned long io=1;
   ioctlsocket(sock,FIONBIO,&io);
#endif
   newuser(sock, name, port, 1);
   return 1;
}
void tcpinterface::delguard()
{
   guardstruct *gs=rootgs;
   while (gs) 
   {
      guardstruct *next=gs->next;
      free(gs->host);
      free(gs);
      gs=next;
   }
   rootgs=NULL;
}
