#ifdef WIN32
#include <winsock2.h>
#endif
#include <sys/types.h>

#ifndef USERHH
#define USERHH

class tcpinterface;
class absuser
{
   protected:
   int fd, killflag, insize, outsize,feed, feedcount, guardflag;
   int outbufsoftlimit;
   time_t lastactive, lastping, prevfeedcheck;
   int timeout;
   int blocked;
   char *peer;
   int port;
   absuser *next, *prev;
   char *inbuf, *outbuf;
   char *prompt;
   void setmasks(fd_set *, fd_set *);
   void input();
   void output();
   int run();
   void setprompt(char *);
   void calcfeed();
   virtual void parse(char *);
   tcpinterface *baseparent;

   public:
   absuser(int, tcpinterface *, char *, int, int);
   virtual ~absuser();
   int nextline(char *, char *);
   void kill(int);
   int send(char *);
   void uprintf(char *, ...);
   void uslprintf(char *, int, ...);
   void block();
   void unblock();
   void printprompt();
   void setactive();
   virtual void sendping();
   friend class tcpinterface;
   friend class servinterface;
   friend class clinterface;
   friend class sysinterface;
   friend class sysuser;
   friend class server;
};
#endif
