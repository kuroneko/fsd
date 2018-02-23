#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "certificate.h"
#include "global.h"

const char *certlevels[]=
{
   "SUSPENDED", "OBSPILOT", "STUDENT1", "STUDENT2", "STUDENT3",
   "CONTROLLER1", "CONTROLLER2", "CONTROLLER3", "INSTRUCTOR1",
   "INSTRUCTOR2", "INSTRUCTOR3", "SUPERVISOR", "ADMINISTRATOR"
};
certificate *rootcert=NULL;
certificate::certificate(char *c, char *p, int l, time_t crea, char *o)
{
   next=rootcert, prev=NULL;
   if (next) next->prev=this;
   rootcert=this, livecheck=1;
   cid=strdup(c), password=strdup(p), level=l, origin=strdup(o);
   if (level>LEV_MAX) level=LEV_MAX;
   prevvisit=(time_t) 0, creation=crea;
}
certificate::~certificate()
{
   if (next) next->prev=prev;
   if (prev) prev->next=next; else rootcert=next;
   free(cid); free(password); free(origin);
}
void certificate::configure(char *pwd, int l, time_t c, char *o)
{
   level=l;
   if (pwd)
   {
      if (password) free(password);
      password=strdup(pwd);
   }
   if (o)
   {
      free(origin);
      origin=strdup(o);
   }
   creation=c;
}
int maxlevel(char *id, char *p, int *max)
{
   certificate *temp=getcert(id);
   if (!temp)
   {
      *max=LEV_OBSPILOT;
      return 0;
   }
   if (!STRCASECMP(temp->password,p))
   {
      *max=temp->level;
      temp->prevvisit=time(NULL);
      return 1;
   }
   *max=LEV_OBSPILOT;
   return 0;
}
certificate *getcert(char *cid)
{
   certificate *temp;
   for (temp=rootcert;temp;temp=temp->next)
      if (!STRCASECMP(temp->cid,cid)) return temp;
   return NULL;
}
