#include <ctime>
#include "process.h"

#ifndef CONFIGHH
#define CONFIGHH

#define CONFIGINTERVAL 10

class configmanager;
class configentry
{
   char *var, *data, **parts, *partbuf;
   int changed, nparts;
   void fillparts();
   void setdata(char *);
   public:
   configentry(char *, char *);
   ~configentry();
   char *getdata();
   int getint();
   int inlist(char *);
   int getnparts();
   char *getpart(int);
   friend class configgroup;
};

class configgroup
{
   char *name;
   void handleentry(char *, char *);
   configentry **entries;
   int nentries;

   public:
   int changed;
   configgroup(char *);
   ~configgroup();
   configentry *getentry(char *);
   configentry *createentry(char *, char *);
   friend class configmanager;
};

class configmanager : public process
{
   char *filename;
   configgroup **groups;
   int ngroups;
   int varaccess;
   int changed;
   time_t prevcheck, lastmodify;
   configgroup *creategroup(char *);
   void parsefile();

   public:
   configmanager(char *);
   ~configmanager();
   virtual int run();
   configgroup *getgroup(char *);
};


#endif
