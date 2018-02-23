#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

#include "wprofile.h"
#include "support.h"
#include "global.h"
#include "mm.h"
#include "client.h"

wprofile *rootprofile=NULL;

wprofile::wprofile(char *n, time_t c, char *o)
{
   creation=c, rawcode=NULL, active=0;
   origin=o?strdup(o):(char*)NULL;
   winds=(windlayer*)calloc(4, sizeof(windlayer));
   clouds=(cloudlayer*)calloc(2, sizeof(cloudlayer));
   tstorm=(cloudlayer*)calloc(1, sizeof(cloudlayer));
   temps=(templayer*)calloc(4, sizeof(templayer));

   int x;
   winds[0].floor=winds[0].ceiling=-1;
   winds[1].floor=2500, winds[1].ceiling=10400;
   winds[2].floor=10400, winds[2].ceiling=22600;
   winds[3].floor=22700, winds[3].ceiling=90000;
   temps[0].ceiling=100;
   temps[1].ceiling=10000;
   temps[2].ceiling=18000;
   temps[3].ceiling=35000;
   dewpoint=0;
   for (x=0;x<2;x++) clouds[x].floor=clouds[x].ceiling=-1;
   tstorm->floor=tstorm->ceiling=-1;

   visibility=15.00, barometer=2950;
   next=rootprofile, prev=NULL;
   if (next) next->prev=this;
   rootprofile=this;
   name=strdup(n);
}
wprofile::~wprofile()
{
   free(name);
   if (rawcode) free(rawcode);
   free(winds); free(clouds); free(temps); free(tstorm);
   if (origin) free(origin);
   if (next) next->prev=prev;
   if (prev) prev->next=next; else rootprofile=next;
}
void wprofile::activate()
{
   active=1;
}
void wprofile::parsemetar(char **array, int count)
{
   int index=0;
   char *station;

   /* First field could be 'METAR' */
   if (!array[index]) return;
   if (!STRCASECMP(array[index],"metar")) index++;
   
   /* The station field */
   if (!array[index]) return;
   station=array[index++];

   /* date and time */
   if (!array[index]) return;
   if (array[index][strlen(array[index])-1]=='Z') index++;

   /* Here there could be 'AUTO' or 'COR' */
   if (!array[index]) return;
   if (!STRCASECMP(array[index],"auto")) index++; else
   if (!STRCASECMP(array[index],"cor")) index++;   

   /* Wind speed and direction */
   index+=parsewind(array+index, count-index);

   /* Visibility */
   index+=parsevis(array+index, count-index);

   /* Runway visual range */
   index+=parservr(array+index, count-index);

   /* Weather phenomena */
   index+=parsewx(array+index, count-index);

   /* Sky conditions */
   index+=parsesky(array+index, count-index);

   /* Temperature */
   index+=parsetemp(array+index, count-index);

   /* Barometer */
   index+=parsealt(array+index, count-index);

   /* Use dewpoint to fix visibility */
   fixvisibility();
}
void wprofile::loadarray(char **array, int count)
{
   int x;
   barometer=atoi(array[0]); array++;
   visibility=atof(array[0]); array++;
   /* load cloud data */
   for (x=0;x<2;x++)
   {
      cloudlayer *l=&clouds[x];
      l->ceiling=atoi(array[0]); array++;
      l->floor=atoi(array[0]); array++;
      l->coverage=atoi(array[0]); array++;
      l->icing=atoi(array[0]); array++;
      l->turbulence=atoi(array[0]); array++;
   }
   /* load thunderstorm data */
   tstorm->ceiling=atoi(array[0]); array++;
   tstorm->floor=atoi(array[0]); array++;
   tstorm->coverage=atoi(array[0]); array++;
   tstorm->icing=atoi(array[0]); array++;
   tstorm->turbulence=atoi(array[0]); array++;
   /* load wind data */
   for (x=0;x<4;x++)
   {
      windlayer *l=&winds[x];
      l->ceiling=atoi(array[0]); array++;
      l->floor=atoi(array[0]); array++;
      l->direction=atoi(array[0]); array++;
      l->speed=atoi(array[0]); array++;
      l->gusting=atoi(array[0]); array++;
      l->turbulence=atoi(array[0]); array++;
   }
   /* load temp data */
   for (x=0;x<4;x++)
   {
      templayer *l=&temps[x];
      l->ceiling=atoi(array[0]); array++;
      l->temp=atoi(array[0]); array++;
   }
}
void wprofile::print(char *data)
{
   int x;
   char piece[100];
   sprintf(data,"%d:%.2f:", barometer, visibility);
   /* Add cloud data */
   for (x=0;x<2;x++)
   {
      cloudlayer *c=&clouds[x];
      sprintf(piece,"%d:%d:%d:%d:%d:", c->ceiling, c->floor, c->coverage,
         c->icing, c->turbulence);
      strcat(data, piece);
   }
   /* Add thunderstorm data data */
    cloudlayer *c=tstorm;
    sprintf(piece,"%d:%d:%d:%d:%d:", c->ceiling, c->floor, c->coverage,
       c->icing, c->turbulence);
    strcat(data, piece);
   /* Add wind data */
   for (x=0;x<4;x++)
   {
      windlayer *l=&winds[x];
      sprintf(piece,"%d:%d:%d:%d:%d:%d:", l->ceiling, l->floor, l->direction,
         l->speed, l->gusting, l->turbulence);
      strcat(data, piece);
   }
   /* Add temp data */
   for (x=0;x<4;x++)
   {
      templayer *l=&temps[x];
      sprintf(piece,"%d:%d:", l->ceiling, l->temp);
      strcat(data, piece);
   }
}
int wprofile::parsewind(char **array, int count)
{
   if (!count) return 0;
   int len=strlen(array[0]);
   if (len<3) return 0;
   char *p=array[0]+len-3;
   if (STRCASECMP(p+1,"kt")&&STRCASECMP(p,"mps")) return 0;
   p[1]='\0';
   p=strchr(array[0],'G');
   if (p) *p='\0', winds[0].gusting=1;
   winds[0].speed=atoi(array[0]+3);
   winds[0].ceiling=2500;
   winds[0].floor=0;
   array[0][3]='\0';
   winds[0].direction=atoi(array[0]);
   
  int a,b;
  if (count==1) return 1;
  if (sscanf(array[1],"%dV%d",&a, &b)==2) return 2;
  return 1;
}
int wprofile::parsevis(char **array, int count)
{
   if (!count) return 0;
   visibility=10.00;
   if (!STRCASECMP(array[0],"M1/4SM"))
   {
      visibility=0.15;
      return 1;
   }
   if (!STRCASECMP(array[0],"1/4SM"))
   {
      visibility=0.25;
      return 1;
   }
   if (!STRCASECMP(array[0],"1/2SM"))
   {
      visibility=0.50;
      return 1;
   }
   if (!STRCASECMP(array[0],"CAVOK") || !strcmp(array[0],"////") ||
      !strcmp(array[0], "CLR"))
   {
      visibility=15.00;
      clouds[1].ceiling=26000, clouds[1].floor=24000;
      clouds[1].icing=clouds[1].turbulence=0;
      clouds[1].coverage=1;
      return 1;
   }
   sscanf(array[0],"%f",&visibility);
   if (strstr(array[0], "SM")) return 1;
   if (count>1&&strstr(array[1], "SM")) return 2;
   if (strstr(array[0], "KM"))
   {
      visibility=visibility/1.609;
      return 1;
   }
   if (count>1&&strstr(array[1], "KM"))
   {
      visibility=visibility/1.609;
      return 2;
   }
   if (visibility==9999) visibility=15.00; else
      visibility=visibility/1609.0;
   return 1;
}
int wprofile::parsewx(char **array, int count)
{
   int amount=0, x;
   char *patterns[]={"+", "-", "VC", "MI", "BL", "PR", "SH", "BC", "TS", "DR",
      "FZ", "DZ", "OC", "UP", "RA", "PE", "SN", "GR", "SG", "GS", "BR", "DU",
      "FG", "SA", "FU", "HZ", "VA", "PY", "PO", "DS", "SQ", "FC", "SS", "PLUS",
       NULL};
   do
   {
      if (amount==count) break;
      for (x=0;patterns[x];x++)
         if (!strncmp(array[amount], patterns[x], strlen(patterns[x]))) break;
      if (patterns[x]) amount++;
   } while (patterns[x]);
   return amount;
}
int wprofile::parsesky(char **array, int count)
{
   int amount=0, x;
   char *patterns[]={"SKC", "CLR",  "VV",  "FEW",  "SCT",  "BKN",  "OVC", NULL};
   int coverage[]=  {    0,     0,     8,      1,      3,      5,      8, 0   };
   do
   {
      if (amount==count) break;
      for (x=0;patterns[x];x++)
         if (!strncmp(array[amount], patterns[x], strlen(patterns[x]))) break;
      if (patterns[x])
      {
         if (amount<2)
         {
            int base;
            char *basestring=array[amount]+strlen(patterns[x]);
            if (sscanf(basestring, "%d", &base)==0) base=10;
            base*=100;
            clouds[amount].coverage=coverage[x];
            clouds[amount].floor=base;
         }
         amount++;
      }
   } while (patterns[x]);
   if (amount==1)
   {
      clouds[0].ceiling=clouds[0].floor+3000;
      clouds[0].turbulence=17;
   } else if (amount>1)
   {
      if (clouds[1].floor>clouds[0].floor)
         clouds[0].ceiling=clouds[0].floor+(clouds[1].floor-clouds[0].floor)/2,
         clouds[1].ceiling=clouds[1].floor+3000; else
      clouds[1].ceiling=clouds[1].floor+(clouds[0].floor-clouds[1].floor)/2,
         clouds[0].ceiling=clouds[0].floor+3000; 
      clouds[0].turbulence=(clouds[0].ceiling-clouds[0].floor)/175;
      clouds[1].turbulence=(clouds[1].ceiling-clouds[1].floor)/175;
   }
   return amount;
}
int wprofile::parsetemp(char **array, int count)
{
   if (!count) return 0;
   char *p=strchr(array[0],'/'), *s=array[0];
   if (!p) return 0;
   *(p++)='\0';
   if (*s=='M') temps[0].temp=-atoi(s+1); else temps[0].temp=atoi(s);
   temps[0].ceiling=100;
   if (*p=='M') dewpoint=-atoi(p+1); else dewpoint=atoi(p);
   if (temps[0].temp>-10&&temps[0].temp<10)
   {
      if (clouds[0].ceiling<12000) clouds[0].icing=1;
      if (clouds[1].ceiling<12000) clouds[1].icing=1;
   }
   return 1;
}
int wprofile::parservr(char **array, int count)
{
   int amount=0;
   while (amount<count&&strchr(array[amount],'/'))
      amount++;
   return amount;
}
int wprofile::parsealt(char **array, int count)
{
   if (!count) return 0;
   barometer=2992;
   switch (*array[0])
   {
      case 'A' : barometer=atoi(array[0]+1); break;
      case 'Q' : barometer=atoi(array[0]+1);
                 barometer=mround((barometer*100.0*100.0)/(1333.0*2.54));
                 break;
      default : return 0;
   }
   return 1;
}
void wprofile::fixvisibility()
{
return;
   if (visibility>=10.00)
   {
      visibility+=temps[0].temp-dewpoint-10;
      if (visibility<=30.00)
      {
         if ((int)visibility%5>2) visibility+=5;
         visibility-=(int)visibility%5;
      } else
      {
         if ((int)visibility%10>5) visibility+=10;
         visibility-=(int)visibility%10;
      }
   }
}
wprofile *getwprofile(char *name)
{
   wprofile *temp;
   for (temp=rootprofile;temp;temp=temp->next) if (!STRCASECMP(temp->name,name))
      return temp;
   return NULL;
}
#define VAR_UPDIRECTION  0
#define VAR_MIDCOR       1
#define VAR_LOWCOR       2
#define VAR_MIDDIRECTION 3
#define VAR_MIDSPEED     4
#define VAR_LOWDIRECTION 5
#define VAR_LOWSPEED     6
#define VAR_UPTEMP       7
#define VAR_MIDTEMP      8
#define VAR_LOWTEMP      9
int wprofile::getseason(double lat)
{
   struct tm *lt;
   time_t now=time(NULL);
   lt=localtime(&now);
   int season;
   switch(lt->tm_mon)
   {
      case 11: case 0: case 1: season=0; break;
      case 5: case 6: case 7: season=2; break;
      case 2: case 3: case 4: season=1; break;
      case 8: case 9: case 10: season=1; break;
   }
   if (lat<0)
   {
      if (season==0) season=2; else
         if (season==2) season=0;
   }
   return season;
}
void wprofile::fix(double lat, double lon)
{
   double a1=lat;
   double a2=fabs(lon/18.0);
   int season=getseason(a1), coriolisvar;
   int latvar=metarmanager->getvariation(VAR_UPDIRECTION,-25,25);
   if (a1>0) winds[3].direction=mround(6*a1+latvar+a2);
      else winds[3].direction=mround(-6*a1+latvar+a2);
   winds[3].direction=(winds[3].direction+360)%360;

   int maxvelocity;
   switch (getseason(a1))
   {
      case 0: maxvelocity=120; break;
      case 1: maxvelocity=80; break;
      case 2: maxvelocity=50; break;
   }
   
   winds[3].speed=mround(fabs(sin(a1*M_PI/180.0))*maxvelocity);
/**************************************/

   latvar=metarmanager->getvariation(VAR_MIDDIRECTION, 10, 45);
   coriolisvar=metarmanager->getvariation(VAR_MIDCOR, 10, 30);
   if (a1>0) winds[2].direction=mround(6*a1+latvar+a2-coriolisvar);
      else winds[2].direction=mround(-6*a1+latvar+a2-coriolisvar);
   winds[2].direction=(winds[2].direction+360)%360;

   winds[2].speed=(int)(winds[3].speed*(metarmanager->getvariation(
      VAR_MIDSPEED, 500, 800)/1000.0));

/**************************************/
   int coriolisvarlow=coriolisvar+metarmanager->getvariation(VAR_LOWCOR,
      10, 30);
   latvar=metarmanager->getvariation(VAR_LOWDIRECTION, 10, 45);
   if (a1>0) winds[1].direction=mround(6*a1+latvar+a2-coriolisvarlow);
      else winds[1].direction=mround(-6*a1+latvar+a2-coriolisvarlow);
   winds[1].direction=(winds[1].direction+360)%360;

   winds[1].speed=(int)((winds[0].speed+winds[1].speed)/2);


/**************************************/
   temps[3].temp=-57+metarmanager->getvariation(VAR_UPTEMP, -4, 4);
   temps[2].temp=-21+metarmanager->getvariation(VAR_MIDTEMP, -7, 7);
   temps[1].temp=-5+metarmanager->getvariation(VAR_LOWTEMP, -12, 12);
}

void wprofile::genrawcode()
{
   char data[1000], piece[100];
   int x;
   time_t now=time(NULL);
   struct tm *gm=gmtime(&now);

   /* Station */
   sprintf(data, "%s ", name);

   /* Zulu time */
   sprintf(piece, "%02d%02d%02dZ ", gm->tm_mday, gm->tm_hour, gm->tm_min);
   strcat(data, piece);

   /* Winds */
   sprintf(piece, "%03d%02dKT ", winds[0].direction, winds[0].speed);
   strcat(data, piece);

   /* Visibility */
   sprintf(piece, "%02dSM ", ((int)visibility));
   strcat(data, piece);

   /* Clouds */
   for (x=0;x<2;x++) if (clouds[x].ceiling!=-1)
   {
      int c=clouds[0].coverage;
      sprintf(piece, "%s%03d ", c==0?"CLR":(c==1||c==2)?"FEW":(c==3||c==4)?
         "SCT":(c==5||c==6)?"BKN":"OVC", clouds[0].floor/100);
      strcat(data, piece);
   }
   
   /* Temp */
   int temperature=temps[0].temp;
   int dew=visibility<5?temperature-1:temperature+1;
   sprintf(piece, "%s%02d/%s%02d ", temperature<0?"M":"", abs(temperature), 
      dew<0?"M":"", abs(dew));
   strcat(data, piece);

   /* QNH */
   sprintf(piece, "A%04d", barometer);
   strcat(data, piece);
   if (rawcode) free(rawcode);
   rawcode=strdup(data);
}
