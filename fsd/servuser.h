#include "interface.h"
#include "servinterface.h"

#ifndef SERVUSERHH
#define SERVUSERHH

extern char *errors[100];
extern int nerrors;
extern int silentok[];

class servuser : public absuser
{
   servinterface *parent;
   void doparse(char *s);
   void feed();
   void list(char *);
   int runcmd(int, char **, int);
   void execnotify(char **, int);
   void execping(char **, int);
   void execpong(char **, int);
   void execlinkdown(char **, int);
   int execaddclient(char **, int);
   void execrmclient(char **, int);
   void execad(char **, int);
   void execpd(char **, int);
   int execcert(char **, int);
   void execmulticast(char **, int);
   int execplan(char **, int);
   void execreqmetar(char **, int);
   void execweather(char **, int);
   void execmetar(char **, int);
   void execnowx(char **, int);
   void execaddwp(char **, int);
   void execdelwp(char **, int);
   void execkill(char **, int);
   void execreset(char **, int);
   int needlocaldelivery(char *);
   public:

   server *thisserver;
   time_t startuptime;
   int clientok;
   virtual void parse (char *);
   servuser(int, servinterface *, char *, int, int);
   virtual ~servuser();
};
#endif
