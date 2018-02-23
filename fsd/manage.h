#include <ctime>

#ifndef MANAGEHH
#define MANAGEHH

#define ATT_INT         1
#define ATT_FLOAT       2
#define ATT_MONEY       3
#define ATT_CHAR        4
#define ATT_VARCHAR     5
#define ATT_DATE        6
#define ATT_BLOB        7
#define ATT_NULL        10

struct managevar
{
   int type;
   char *name;
   union
   {
      int number; 
      char *string;
      time_t timeval;
   } value;
};
class manage
{
   int nvars;
   managevar *variables;
   public:
   manage();
   ~manage();
   int addvar(char *, int);
   int getvarnum(char *name);
   void incvar(int);
   void setvar(int, int);
   void setvar(int, char *);
   void setvar(int, time_t);
   void decvar(int);
   void delvar(int);
   managevar *getvar(int);
   int getnvars();
   char *sprintvalue(int, char *);
};

extern manage *manager;


#endif
