#include "interface.h"
#include "sysinterface.h"
#include "user.h"
#include "wprofile.h"

#ifndef SYSUSERHH
#define SYSUSERHH

#define SYS_SERVERS   0
#define SYS_INFORMATION  1
#define SYS_PING      2
#define SYS_CONNECT   3
#define SYS_TIME      4
#define SYS_ROUTE     5
#define SYS_WEATHER   6
#define SYS_DISCONNECT  7
#define SYS_HELP      8
#define SYS_QUIT      9
#define SYS_STAT      10
#define SYS_SAY       11
#define SYS_CLIENTS   12
#define SYS_CERT      13
#define SYS_PWD       14
#define SYS_DISTANCE  15
#define SYS_RANGE     16
#define SYS_LOG       17
#define SYS_WALL      18
#define SYS_DELGUARD  19
#define SYS_METAR     20
#define SYS_WP        21
#define SYS_KILL      22
#define SYS_POS       23
#define SYS_DUMP      24
#define SYS_SERVERS2  25
#define SYS_REFMETAR  26

class sysuser : public absuser
{
   int authorized;
   sysinterface *parent;
   void doparse(char *s);
   void exechelp(char **, int);
   void list(char *);
   void usage(int, char *);
   void execservers(char **, int, int);
   void execping(char **, int);
   void execconnect(char **, int);
   void execdisconnect(char **, int);
   void execroute(char **, int);
   void execsay(char **, int);
   void execclients(char **, int);
   void execcert(char **, int);
   void execdistance(char **, int);
   void exectime(char **, int);
   void execweather(char **, int);
   void execrange(char **, int);
   void execlog(char **, int);
   void execpwd(char **, int);
   void execwall(char **, int);
   void execdelguard(char **, int);
   void execmetar(char **, int);
   void execwp(char **, int);
   void execkill(char **, int);
   void execpos(char **, int);
   void execdump(char **, int);
   void execrefmetar(char **, int);
   void information();
   void runcmd(int, char **, int);
   void doset(wprofile *, char *, int);
   public:

   double lat, lon;
   void printweather(wprofile *);
   void printmetar(char *, char *);
   sysuser(int, sysinterface *, char *, int, int);
   virtual void parse (char *);
   sysuser(user *);
   virtual ~sysuser();
};
#endif
