#ifndef FSDHH
#define FSDHH
#include "servinterface.h"
#include "clinterface.h"
#include "sysinterface.h"
#include "interface.h"
#include "process.h"
#include "config.h"
#include <ctime>

class fsd
{
   int clientport, serverport, systemport;
   pman *pmanager;
   char *certfile;
   char *whazzupfile;
   time_t timer, prevnotify, prevlagcheck, certfilestat, prevcertcheck;
   void configmyserver();
   void configure();
   void createmanagevars();
   void createinterfaces();
   void readcert();
   void handlecidline(char *);
   void makeconnections();
   void dochecks();
   void initdb();
   time_t prevwhazzup;
   int fileopen;
   public:
   fsd(char *);
   ~fsd();
   void run();
};
extern clinterface *clientinterface;
extern servinterface *serverinterface;
extern sysinterface *systeminterface;
extern configmanager *configman;

#endif
