#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#ifndef SUPPORTH
#define SUPPORTH

/* Levels for logging, standard SYSLOG numbers are used here */

#define MAXLOG       50
#define L_EMERG      0
#define L_ALERT      1
#define L_CRIT       2
#define L_ERR        3
#define L_WARNING    4
#define L_INFO       5
#define L_DEBUG      6
#define L_MAX        7

struct loghis
{
   char *msg;
   int level;
};

extern struct loghis loghistory[MAXLOG];
extern int logp;

void setconfigfile(char *name);
int findsection(char *section);
int finditem(char *what, char *buf);
char *configgets(char *s, int size);
void dolog(int level, const char *, ...);
void addfile(char *, char *, ...);
int breakpacket(char *, char **, int);
int breakargs(char *, char **, int);
char *catargs(char **, int, char *);
char *catcommand(char **, int, char *);
void snappacket(char *, char *, int);
void findhostname(unsigned long, char *);
char *strupr(char *);
double dist(double, double, double, double);
char *printloc(double, double, char *);
char *sprinttime(time_t, char *);
char *sprintdate(time_t, char *);
char *sprintgmt(time_t now, char *buf);
char *sprintgmtdate(time_t now, char *buf);
int mconnect(int, struct sockaddr *, int, int);
void dblog(char *, int);
void starttimer();
time_t mtime();
void msrand(int);
int mrand();
int mround(double);
time_t mgmtime();
#endif
