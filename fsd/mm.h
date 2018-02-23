#include <cstdio>
#include <ctime>
#ifndef WIN32
  #include <sys/time.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

#include "server.h"
#include "process.h"

#ifndef mmhh
#define mmhh

#define SOURCE_NETWORK  0
#define SOURCE_FILE     1
#define SOURCE_DOWNLOAD 2

#define VAR_AMOUNT 10

struct station
{
   char name[5];
   off_t location;
};
struct mmq
{
   mmq *next, *prev;
   char *destination, *metarid;
   int fd, parsed;
};

class mm : public process
{
   FILE *ioin, *ioout;
   mmq *rootq;
   time_t prevdownload, metarfiletime;
   int nstations;
   station *stationlist;
   int prevhour;
   int sock, datasock, datareadsock, newfileready, metarsize;
   char* sockrecvbuffer;
   int sockrecvbufferlen;
   int varprev, vartotal, varstations;
   int variation[VAR_AMOUNT];
   char *metarhost, *metardir, *ftpemail;
   int passivemode;
   void setupnewfile();
   void buildlist();
   int doparse(mmq *);
   void dodownload();
   void prepareline(char *);
   int getline(char *);
   void check();
   void checkmetarfile();
   void addq(char *, char *, int, int);
   void delq(mmq *);
   virtual int calcmasks(fd_set *, fd_set *);

   public:
   void startdownload();
   void stopdownload();
   int downloading, source;
   mm();
   ~mm();
   virtual int run();
   server *requestmetar(char *, char *, int, int);
   int getvariation(int, int, int);
};

extern mm *metarmanager;
#endif
