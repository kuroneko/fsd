#ifndef CERTIFICATEHH
#define CERTIFICATEHH

#include <time.h>
#include "interface.h"

#define CERTPILOT   1
#define CERTATC     2

class certificate
{
   public:
   char *cid, *password, *origin;
   int level, livecheck;
   time_t prevvisit, creation;
   certificate *next, *prev;
   certificate(char *, char *, int, time_t, char *);
   void configure(char *, int, time_t, char *);
   ~certificate();
};
extern char *certlevels[];
int maxlevel(char *, char *, int *);
certificate *getcert(char *name);
extern certificate *rootcert;
#endif
