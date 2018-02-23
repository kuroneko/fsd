#include <cstdio>
#include <cstdlib>
#ifdef WIN32
	#include <winsock2.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netdb.h>
#endif
#include <ctime>
#include <cctype>
#include <fcntl.h>
#include <cstdarg>
#include <cstring>
#include <cerrno>
#include "user.h"
#include "interface.h"
#include "manage.h"
#include "support.h"
#include "global.h"

/*******************************************************************/
/* The USER class. There's not much interesting here, just handling
   the low level communication with the clients */

absuser::absuser(int d, tcpinterface *p, char *peername, int portnum, int g)
{
   next=prev=NULL, killflag=0, fd=d;
   inbuf=(char *)malloc(1);
   outbuf=(char *)malloc(1);
   insize=1;
   outsize=1;
   *outbuf=0;
   *inbuf=0;
   blocked=0, prompt=NULL, feedcount=0, feed=-1;
   baseparent=p, timeout=0, peer=strdup(peername), port=portnum;
   prevfeedcheck=0, guardflag=g, outbufsoftlimit=-1;
   setactive();
}
absuser::~absuser()
{
   dolog(L_INFO,"client from %s removed from %s:%s", peer,
      baseparent->description, killreasons[killflag]);
   manager->incvar(baseparent->varclosed[killflag]);
   if (killflag!=KILL_CLOSED&&killflag!=KILL_WRITEERR)
      output();
   if (killflag!=KILL_COMMAND&&guardflag)
      baseparent->addguard(this);
   CLOSESOCKET(fd); 
   free(inbuf); free(outbuf);
   if (prompt) free(prompt);
   if (peer) free(peer);
}
void absuser::setactive()
{
   lastactive=lastping=mtime();
}
void absuser::setmasks(fd_set *rmask, fd_set *wmask)
{
   FD_SET(fd,rmask);
   if (outbuf[0]) FD_SET(fd,wmask);
}
void absuser::input()
{
   char buf[1025];
   int bytes=READSOCK(fd,buf,1024);
#ifdef WIN32
   if (bytes<0)
	   if (WSAGetLastError()==WSAEWOULDBLOCK)
	   {
		   return;
	   }
   if (bytes==0)
   {
      kill(KILL_CLOSED);
      return;
   }
   if (bytes<0)
	   if (WSAGetLastError()!=WSAEWOULDBLOCK)
	   {
	      kill(KILL_CLOSED);
		  return;
	   }
#else
	if ((bytes<0&&errno!=EINTR)||bytes==0)
	{
	  kill(KILL_CLOSED);
	  return;
	}
#endif
   if (baseparent->feedstrategy&FEED_IN) feedcount+=bytes;
   buf[bytes]='\0';
   int inbufbytes = strlen(inbuf);
   
  if (insize>4096&&inbufbytes==0)
  {
	  free(inbuf);
	  insize=2048;
	  inbuf=(char*) malloc(insize);
	  *inbuf=0;
  }
	if (bytes+inbufbytes+1>insize)
	{
		insize= bytes+inbufbytes+1000;
		char* newinbuf = (char*) malloc(insize);
		if (inbufbytes) memcpy(newinbuf,inbuf,inbufbytes);
		memcpy(&newinbuf[inbufbytes],buf,bytes);
		newinbuf[bytes+inbufbytes]=0;
		free(inbuf);
		inbuf = newinbuf;
	}
	else
	{
		memcpy(&inbuf[inbufbytes],buf,bytes);
		inbuf[bytes+inbufbytes]=0;
	}
}
int absuser::nextline(char *source, char *dest)
{
   int len=strcspn(source, "\r\n");
   if (source[len]=='\0') return -1;
   while (source[len]=='\n'||source[len]=='\r')
      source[len++]='\0';
   char *where=source+len;
   if (len<MAXLINELENGTH) strcpy(dest,source);
   memcpy(source,where,strlen(where)+1);
   if (!dest[0]||len>=MAXLINELENGTH) return -1;
   return where-source;
}
void absuser::output()
{
   int len=strlen(outbuf);
   int bytes=len>2048?2048:len;
#ifdef WIN32
   if (((bytes=::WRITESOCK(fd,outbuf,bytes))<0)&&WSAGetLastError()!=WSAEWOULDBLOCK)
#else
   if (((bytes=::WRITESOCK(fd,outbuf,bytes))<0)&&errno!=EINTR)
#endif
   {
      kill(KILL_WRITEERR);
      return;
   }
   if (baseparent->feedstrategy&FEED_OUT) feedcount+=bytes;
   memmove(outbuf,&outbuf[bytes],len-bytes);
   outbuf[len-bytes] =0;
   /*if (len==bytes) outbuf[0]='\0';
      else memmove(outbuf, outbuf+bytes, len-bytes+1);*/
}
void absuser::calcfeed()
{
   time_t now=mtime();
   int elapsed=now-prevfeedcheck;
   double fact1=(double)elapsed/300, fact2=1.0-fact1;
   int newfeed=feedcount/elapsed;
   if (feed==-1) feed=newfeed;
      else feed=(int)((double)newfeed*fact1+(double)feed*fact2);
   feedcount=0, prevfeedcheck=now;
   int bandwith=feed;
   if (baseparent->feedstrategy==FEED_BOTH) bandwith/=2;
   if (bandwith>50) outbufsoftlimit=bandwith*30;
}
int absuser::send(char *buf)
{
   int outbufbytes=strlen(outbuf);
   int bytes=strlen(buf);
   /*int len=strlen(outbuf);
   int totalnew=size+len+10;
   if ((baseparent->outbuflimit!=-1)&&(totalnew>baseparent->outbuflimit))
      return 0;
   if (totalnew<(outsize-1000)) outbuf=(char*)realloc(outbuf,outsize=totalnew);
   while (totalnew>=outsize) outbuf=(char*)realloc(outbuf,outsize+=1000);
   strcat(outbuf,buf);*/
   if (outbufbytes)
   {
		if (outbufbytes+bytes+1>outsize)
		{
			outsize= outbufbytes+bytes+1000;
			char* ptr = (char*) malloc(outsize);
			memcpy(ptr,outbuf,outbufbytes);
			memcpy(&ptr[outbufbytes],buf,bytes);
			ptr[bytes+outbufbytes]=0;
			free(outbuf);
			outbuf = ptr;
		}
		else
		{
			memcpy(&outbuf[outbufbytes],buf,bytes);
			outbuf[bytes+outbufbytes]=0;
		}
   }
   else
   {
	   int sendbytes=bytes>2048?2048:bytes;
#ifdef WIN32
	   if (((sendbytes=::WRITESOCK(fd,buf,sendbytes))<0)&&WSAGetLastError()!=WSAEWOULDBLOCK)
#else
	   if (((sendbytes=::WRITESOCK(fd,buf,sendbytes))<0)&&errno!=EINTR)
#endif
	   {
		  kill(KILL_WRITEERR);
		  return 0;
	   }
	   if (baseparent->feedstrategy&FEED_OUT) feedcount+=sendbytes;
	   if (bytes-sendbytes+1>outsize)
	   {
			outsize= bytes-sendbytes+1000;
			char* ptr = (char*) malloc(outsize);
			memcpy(ptr,&buf[sendbytes],bytes-sendbytes);
			ptr[bytes-sendbytes]=0;
			free(outbuf);
			outbuf = ptr;
	   }
	   else
		{
			memcpy(&outbuf[outbufbytes],&buf[sendbytes],bytes-sendbytes);
			outbuf[bytes-sendbytes+outbufbytes]=0;
		}
   }
   return bytes;
}
void absuser::uprintf(char *s, ...)
{
   char b[10000];
   if (killflag) return;
   va_list ap;
   va_start(ap,s);
   vsprintf(b,s,ap);
   va_end(ap);
   send(b);
}
void absuser::uslprintf(char *s, int check, ...)
{
   char b[10000];
   if (killflag) return;
   va_list ap;
   va_start(ap,check);
   vsprintf(b,s,ap);
   va_end(ap);
   if (check&&outbufsoftlimit!=-1&&(strlen(b)+strlen(outbuf))>outbufsoftlimit)
      return;
   send(b);
}
int absuser::run()
{
   char buf[1000];
   int count=0, stat, ok=0;
   if (blocked) return 0;
   while ((stat=nextline(inbuf,buf))!=-1&&(++count<60))
   {
      if (baseparent->floodlimit!=-1&&(feed>baseparent->floodlimit))
         kill(KILL_FLOOD);
      parse(buf);
      ok=1;
      if (blocked) break;
   }
   if (ok&&stat==-1) printprompt();
   return (stat==-1)?0:1;
}
void absuser::parse(char *s)  { } 
void absuser::block()
{
   blocked=1;
}
void absuser::unblock()
{
   blocked=0;
   printprompt();
}
void absuser::setprompt(char *s)
{
   if (prompt) free(prompt);
   prompt=strdup(s);
}
void absuser::printprompt()
{
   if (!prompt||killflag||blocked) return;
   uprintf("%s",prompt);
}
void absuser::kill(int reason)
{
   killflag=reason;
}
void absuser::sendping()
{
}
