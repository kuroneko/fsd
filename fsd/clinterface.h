#include <ctime>

#include "interface.h"
#include "user.h"
#include "client.h"
#include "wprofile.h"

#ifndef CLINTERFACEHH
#define CLINTERFACEHH

class clinterface:public tcpinterface
{
   time_t prevwinddelta;
   int getbroad(char *);
   public:
   void sendaa(client *, absuser *);
   void sendap(client *, absuser *);
   void sendda(client *, absuser *);
   void senddp(client *, absuser *);
   void sendgeneric(char *, client *, absuser *, client *, char *, char *, int);
   void sendpilotpos(client *, absuser *);
   void sendatcpos(client *, absuser *);
   void sendplan(client *, client *, int);
   void sendweather(client *, wprofile *);
   void sendmetar(client *, char *);
   void sendnowx(client *, char *);
   void sendpacket(client *, client *, absuser *, int, int, int, char *);
   void sendwinddelta();
   void handlekill(client *, char *);
   int calcrange(client *, client *, int, int);
   clinterface(int, char *, char *);
   virtual int run();
   virtual void newuser(int, char *, int, int);
   friend class cluser;
};
#endif
