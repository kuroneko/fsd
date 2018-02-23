#include "interface.h"
#include "clinterface.h"
#include "user.h"
#include "client.h"

#ifndef CLUSERHH
#define CLUSERHH

class cluser : public absuser
{
   clinterface *parent;
   void doparse(char *s);
   int getcomm(char *);
   int callsignok(char *name);
   int showerror(int, char *);
   int checksource(char *);
   int checklogin(char *, char *, int);
   void execd(char **, int);
   void execaa(char **, int);
   void execap(char **, int);
   void execmulticast(char **, int, int, int, int);
   void execpilotpos(char **, int);
   void execatcpos(char **, int);
   void execfp(char **, int);
   void execweather(char **, int);
   void execacars(char **, int);
   void execcq(char **, int);
   void execkill(char **, int);
   void readmotd();
   public:

   client *thisclient;
   cluser(int, clinterface *, char *, int, int);
   virtual void parse (char *);
   cluser(user *);
   virtual ~cluser();
   friend class clinterface;
};
#endif
