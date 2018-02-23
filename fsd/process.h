#ifndef WIN32
  #include <sys/time.h>
  #include <sys/types.h>
  #include <unistd.h>
#else
  #include <winsock2.h>
#endif

#ifndef PROCESSHH
#define PROCESSHH

class pman;
class process
{
   process *next, *prev;
   protected:
   fd_set *rmask, *wmask;
   virtual int calcmasks(fd_set *, fd_set *);
   virtual int run();
   public:
   process();
   friend class pman;
};

class pman
{
   int busy;
   process *rootprocess;
   public:
   pman();
   void registerprocess(process *);
   void run();
};
#endif
