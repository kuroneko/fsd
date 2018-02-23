#include <ctime>
#include <sys/types.h>

#include "user.h"
#include "process.h"

#ifndef INTERFACEHH
#define INTERFACEHH

struct allowstruct
{
   allowstruct *next;
   char ip[4];
};
struct guardstruct
{
   guardstruct *next, *prev;
   time_t prevtry;
   char *host;
   int port;
};

extern const char *killreasons[];

#define KILL_NONE 0
#define KILL_COMMAND 1
#define KILL_FLOOD 2
#define KILL_INITTIMEOUT 3
#define KILL_DATATIMEOUT 4
#define KILL_CLOSED 5
#define KILL_WRITEERR 6
#define KILL_KILL 7
#define KILL_PROTOCOL 8

#define FEED_IN 1
#define FEED_OUT 2
#define FEED_BOTH 3

class user;
class tcpinterface : public process
{
   protected:
   char *description;
   int sock;
   int varcurrent, vartotal, varpeak;
   int varclosed[9];
   int feedstrategy;
   int floodlimit, outbuflimit;
   time_t prevchecks;
   allowstruct *rootallow;
   char *prompt;

   void makevars(char *);
   void addguard(absuser *);
   virtual int calcmasks(fd_set *, fd_set *);
   void newuser(void);
   virtual void newuser(int , char *, int, int);
   void removeuser(absuser *);
   void insertuser(absuser *);
   void dochecks();

   public:
   absuser *rootuser;
   guardstruct *rootgs;
   tcpinterface(int, char *, char *);
   ~tcpinterface();
   void setprompt(char *);
   void setflood(int);
   void setfeedstrategy(int);
   void setoutbuflimit(int);
   virtual int run();
   int adduser(char *, int, absuser *);
   void allow(char *);
   void delguard();
   friend class absuser;
};
#endif
