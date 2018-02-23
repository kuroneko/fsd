#include <ctime>

#include "interface.h"

#ifndef SERVERHH
#define SERVERHH

#define SERVER_METAR  1
#define SERVER_SILENT 2

class server
{
   public:
   int pcount, hops, check, lag, flags;
   int packetdrops;
   time_t alive;
   char *name;
   char *email;
   char *ident;
   char *hostname;
   char *version;
   char *location;
   absuser *path;
   server *next, *prev;
   server(char *, char *, char *, char *, char *, int, char *);
   void configure(char *, char *, char *, char *, char *);
/*   void sendping();*/
   ~server();
   void setpath(absuser *, int);
   void setalive();
   void receivepong(char *);
};
server *getserver(char *);
void clearserverchecks();
extern server *rootserver;
extern server *myserver;
#endif
