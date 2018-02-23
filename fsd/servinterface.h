#include <ctime>
#include "interface.h"
#include "user.h"
#include "client.h"
#include "certificate.h"
#include "wprofile.h"

#ifndef SERVINTERFACEHH
#define SERVINTERFACEHH

class servinterface:public tcpinterface
{
   int packetcount, varmcdrops, varinterr;
   int varmchandled, varuchandled, varucoverrun, varfailed, varshape, varbounce;
   char *serverident;
   time_t lastsync;
   public:
   servinterface(int, char *, char *);
   virtual int run();
   virtual void newuser(int, char *, int, int);
   void incpacketcount();
   void sendsync();
   void sendservernotify(char *, server *, absuser *);
   void sendreqmetar(char *, char *, int, int, server *);
   void sendlinkdown(char *);
   void sendpong(char *, char *);
   void sendping(char *, char *);
   void sendaddclient(char *, client *, absuser *, absuser *, int);
   void sendrmclient(absuser *, char *, client *, absuser *);
   void sendpilotdata(client *, absuser *);
   void sendatcdata(client *, absuser *);
   void sendcert(char *, int, certificate *, absuser *);
   void sendweather(char *, int, wprofile *);
   void sendmetar(char *, int, char *, char *);
   void sendnowx(char *, int, char *);
   void sendaddwp(absuser *, wprofile *);
   void senddelwp(wprofile *);
   void sendkill(client *, char *);
   void sendreset();
   int sendmulticast(client *, char *, char *s, int, int, absuser *);
   void sendplan(char *, client *, absuser *);
   void assemble(char *, int, char *, char *, int ,int ,int, char *);

   void sendpacket(absuser *, absuser *, int , char *, char *, int, int,
      int, char *);
   void clientdropped(client *);
   friend class servuser;
   friend class server;
};
#endif
