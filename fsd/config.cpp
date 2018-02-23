#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <sys/stat.h>

#include "manage.h"
#include "config.h"
#include "support.h"
#include "global.h"

configentry::configentry(char *v, char *d)
{
   var=strdup(v), data=strdup(d), changed=1, parts=NULL, partbuf=NULL;
}
configentry::~configentry()
{
   free(var);
   free(data);
   if (parts) free(parts);
   if (partbuf) free(partbuf);
}
char *configentry::getdata()
{
   return data;
}
void configentry::setdata(char *d)
{
   free(data);
   data=strdup(d);
   changed=1;
   if (parts) free(parts);
   if (partbuf) free(partbuf);
   parts=NULL, partbuf=NULL;
}
int configentry::getint()
{
   return atoi(data);
}

void configentry::fillparts()
{
   nparts=0;
   if (parts) free(parts);
   if (partbuf) free(partbuf);
   parts=NULL;
   partbuf=strdup(data);
   char *p=partbuf;
   while (p)
   {
      char *end=strchr(p, ',');
      if (end)
      {
         char *seek=end-1;
         *(end++)='\0';
         while (isspace(*seek)) *(seek--)='\0';
      }
      while (isspace(p[0])) p++;
      if (parts) parts=(char **)realloc(parts,sizeof(char*)*++nparts);
         else parts=(char **)malloc(sizeof(char*)*(nparts=1));
      parts[nparts-1]=p;
      p=end;
   }
}
int configentry::inlist(char *entry)
{
   int x;
   if (!parts) fillparts();
   for (x=0;x<nparts;x++) if (!STRCASECMP(parts[x], entry)) return 1;
   return 0;
}
int configentry::getnparts()
{
   if (!parts) fillparts();
   return nparts;
}
char *configentry::getpart(int num)
{ 
   if (!parts) fillparts();
   if (num>=nparts) return NULL;
   return parts[num];
}
configgroup::configgroup(char *n)
{
   name=strdup(n);
   entries=NULL, nentries=0, changed=1;
}
configgroup::~configgroup()
{
   int x;
   for (x=0;x<nentries;x++) delete entries[x];
   if (entries) free(entries);
   free(name);
}
configentry *configgroup::getentry(char *name)
{
   int x;
   for (x=0;x<nentries;x++) if (!STRCASECMP(name, entries[x]->var))
      return entries[x];
   return NULL;
}
configentry *configgroup::createentry(char *var, char *data)
{
   if (entries)
      entries=(configentry**)realloc(entries, sizeof(configentry *)*++nentries);
   else entries=(configentry**)malloc(sizeof(configentry *)), nentries=1;
   return entries[nentries-1]=new configentry(var, data);
}
void configgroup::handleentry(char *var, char *data)
{
   configentry *e=getentry(var);
   if (!e)
   {
      createentry(var, data);
      changed=1;
      return;
   }
   if (!strcmp(e->data, data)) return;
   e->setdata(data);
   changed=1;
   return;
}



configmanager::configmanager(char *name)
{
   filename=strdup(name);
   groups=NULL, ngroups=0, changed=1;
   int fname=manager->addvar("config.filename", ATT_VARCHAR);
   manager->setvar(fname, name);
   varaccess=manager->addvar("config.lastread", ATT_DATE);
   parsefile();
}
configmanager::~configmanager()
{
   int x;
   for (x=0;x<ngroups;x++) delete groups[x];
   if (groups) free(groups);
   free(filename);
}
configgroup *configmanager::getgroup(char *name)
{
   int x;
   for (x=0;x<ngroups;x++) if (!STRCASECMP(name, groups[x]->name))
      return groups[x];
   return NULL;
}
configgroup *configmanager::creategroup(char *name)
{
   if (groups)
      groups=(configgroup**)realloc(groups, sizeof(configgroup *)*++ngroups);
   else groups=(configgroup**)malloc(sizeof(configgroup *)), ngroups=1;
   return groups[ngroups-1]=new configgroup(name);
}
int configmanager::run()
{
   if ((mtime()-prevcheck)<CONFIGINTERVAL) return 0;
   prevcheck=mtime();
   struct stat buf;
   if (stat(filename, &buf)==-1) return 0;
   if (buf.st_mtime==lastmodify) return 0;
   lastmodify=buf.st_mtime;
   parsefile();
   return 0;
}
void configmanager::parsefile()
{
   char line[300];
   FILE *io=fopen(filename, "r");
   if (!io) return;
   manager->setvar(varaccess, time(NULL));
   configgroup *current=NULL;
   while (fgets(line, 300, io))
   {
      if (line[0]=='#'||line[0]=='\n'||line[0]=='\r') continue;
      if (line[0]=='[')
      {
         char *p=strchr(line, ']');
         if (p) *p='\0';
         current=getgroup(line+1);
         if (!current) current=creategroup(line+1);
         continue;
      }     
      if (!current) continue;
      line[strlen(line)-1]='\0';
      char *sep=strchr(line,'=');
      if (!sep) continue;
      *(sep++)='\0';
      while (isspace(*sep)) sep++;
      if (sep) current->handleentry(line, sep);
      if (current->changed) changed=1;
   }
   fclose(io);
   prevcheck=mtime();
}
