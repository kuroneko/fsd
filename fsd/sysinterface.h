#include "interface.h"
#include "wprofile.h"

#ifndef SYSINTERFACEHH
#define SYSINTERFACEHH

class sysinterface:public tcpinterface
{
   public:
   sysinterface(int, char *, char *);
   virtual int run();
   virtual void newuser(int, char *, int, int);
   void receivepong(char *, char *, char *, char *);
   void receiveweather(int fd, wprofile *);
   void receivemetar(int fd, char *, char *);
   void receivenowx(int fd, char *);
};
#endif
