#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "global.h"
#include "manage.h"

manage *manager=NULL;

manage::manage()
{
   nvars=0, variables=NULL;
}
manage::~manage()
{
   int x;
   for (x=0;x<nvars;x++) if (variables[x].name)
   {
      free(variables[x].name);
      if (variables[x].type==ATT_VARCHAR) free(variables[x].value.string);
   }
   free(variables);
}

int manage::addvar(char *name, int type)
{
   int x, num=-1;
   for (x=0;x<nvars;x++) if (variables[x].name==NULL)
   {
      num=x;
      break;
   }
   if (num==-1)
   {
      num=nvars;
      if (variables) variables=(managevar*)realloc(variables,
        sizeof(managevar)*++nvars); else
        variables=(managevar*)malloc(sizeof(managevar)*++nvars);
   }
   memset(&variables[num], 0, sizeof(managevar));
   variables[num].name=strdup(name);
   variables[num].type=type;
   return num;
}
void manage::delvar(int num)
{
   if (variables[num].name) free(variables[num].name);
   variables[num].name=NULL;
}
void manage::incvar(int num)
{
   variables[num].value.number++;
}
void manage::decvar(int num)
{
   variables[num].value.number--;
}
void manage::setvar(int num, int val)
{
   variables[num].value.number=val;
}
void manage::setvar(int num, time_t tval)
{
   variables[num].value.timeval=tval;
}
void manage::setvar(int num, char *string)
{
   if (variables[num].value.string) free(variables[num].value.string);
   variables[num].value.string=string?strdup(string):(char*)NULL;
}
managevar *manage::getvar(int num)
{
   if (num>=nvars) return NULL;
   return variables+num;
}
int manage::getnvars()
{
   return nvars;
}
char *manage::sprintvalue(int num, char *buf)
{
   if (num>=nvars||variables[num].name==NULL) return NULL;
   switch (variables[num].type)
   {
      case ATT_VARCHAR: sprintf(buf,"\"%s\"",variables[num].value.string);
                        break;
      case ATT_INT    : sprintf(buf,"%d",variables[num].value.number); break;
      case ATT_DATE   : 
                        sprintf(buf,"%s",ctime(&variables[num].value.timeval)); 
                        buf[strlen(buf)-1]='\0';
                        break;
   }
   return buf;
}
int manage::getvarnum(char *name)
{
   int x;
   for (x=0;x<nvars;x++) if (variables[x].name)
      if (!strcmp(variables[x].name,name)) return x;
   return -1;
}
