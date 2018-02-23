#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "server.h"
#include "support.h"
#include "client.h"
#include "servuser.h"
#include "fsd.h"
#include "global.h"

server *rootserver=NULL;
server *myserver=NULL;
server::server(char *i, char *n, char *e, char *h, char *v, int fl, char *l)
{
   next=rootserver, prev=NULL;
   if (next) next->prev=this;
   rootserver=this;
   ident=strdup(i), name=strdup(n), email=strdup(e), hostname=strdup(h);
   version=strdup(v), flags=fl, packetdrops=0, location=strdup(l);
   path=NULL, hops=-1, pcount=-1, lag=-1;
   alive=mtime();
}
void server::configure(char *n, char *e, char *h, char *v, char *l)
{
   if (name) free(name);
   if (email) free(email);
   if (hostname) free(hostname);
   if (version) free(version);
   if (location) free(location);
   name=strdup(n), email=strdup(e), hostname=strdup(h), version=strdup(v);
   location=strdup(l);
}
server::~server()
{
   client *tempclient=rootclient;
   dolog(L_ERR,"Dropping server %s(%s)", ident, name);
   free(ident); free(name); free(email); free(hostname); free(version);
   if (next) next->prev=prev;
   if (prev) prev->next=next; else rootserver=next;
   while (tempclient)
   {
      client *next=tempclient->next;
      if (tempclient->location==this)
         delete tempclient;
      tempclient=next;
   }
   absuser *t;
   for (t=serverinterface->rootuser;t;t=t->next)
      if (((servuser*)t)->thisserver==this)
         ((servuser*)t)->thisserver=NULL;
}
server *getserver(char *ident)
{
   server *temp;
   for (temp=rootserver;temp;temp=temp->next) 
      if (!STRCASECMP(temp->ident,ident)) return temp;
   return NULL;
}
void server::setpath(absuser *who, int h)
{
   path=who, hops=h;
   if (path==NULL) pcount=-1;
}
void server::setalive()
{
   alive=mtime();
}
void server::receivepong(char *data)
{
   int fd;
   time_t t;
   if (sscanf(data,"%d %ld", &fd, &t)!=2) return;
   lag=mtime()-t;
}
void clearserverchecks()
{
   server *temp;
   for (temp=rootserver;temp;temp=temp->next) temp->check=0;
}
