#include <ctime>

#include "interface.h"
#include "server.h"
#include "client.h"

#ifndef WPROFILEHH
#define WPROFILEHH

struct cloudlayer
{
   int ceiling, floor;
   int coverage;
   int icing;
   int turbulence;
};
struct windlayer
{
   int ceiling, floor;
   int direction, speed;
   int gusting, turbulence;
};
struct templayer
{
   int ceiling, temp;
};
class wprofile
{
   public:
   wprofile *next, *prev;
   time_t creation, version;
   int active;
   char *rawcode;
   windlayer *winds;
   cloudlayer *clouds;
   templayer *temps;
   cloudlayer *tstorm;
   int barometer;
   float visibility;
   int dewpoint;
   char *name, *origin;

   wprofile(char *, time_t, char *);
   ~wprofile();
   int getseason(double);
   void loadarray(char **, int );
   void parsemetar(char **, int);
   void print(char *);
   void genrawcode();
   void activate();
   int parsewind(char **, int);
   int parsevis(char **, int);
   int parsewx(char **, int);
   int parsesky(char **, int);
   int parsetemp(char **, int);
   int parservr(char **, int);
   int parsealt(char **, int);
   void fixvisibility();
   void fix(double , double);
};
extern wprofile *rootprofile;
wprofile *getwprofile(char *);
#endif
