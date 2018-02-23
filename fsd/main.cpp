#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#ifndef WIN32
	#include <unistd.h>
	#include <syslog.h>
	#include <sys/types.h>
#endif

#include "support.h"
#include "fsd.h"
#include "fsdpaths.h"


void run(char *configfile)
{
   fsd fsdserver(configfile);
   while (1) fsdserver.run();
}
void dosignals()
{
   starttimer();
#ifndef WIN32
   signal(SIGPIPE, SIG_IGN);
   signal(SIGHUP, SIG_IGN);
#endif
}

#ifdef WIN32
	int fsdmain()
	{
		char currentopath[MAX_PATH];
		GetModuleFileName(NULL,currentopath,sizeof(currentopath));
		char currentpath[MAX_PATH];
		char* filename;
		GetFullPathName(currentopath,sizeof(currentpath),currentpath,&filename);
		*filename=0;
		SetCurrentDirectory(currentpath);
		char *configfile;
		configfile = PATH_FSD_CONF;
		WSADATA lpwsaData;
		unsigned short Version = MAKEWORD(2, 2);
		int rc = WSAStartup(Version, &lpwsaData);
   
		dosignals();
		run(configfile);
		return 0;
	}


	const char* ServerName = "FSD";
	const char* ServerDesc = "FSD Server";

	SERVICE_STATUS ServiceStatus;
	SERVICE_STATUS_HANDLE ServiceStatusHandle;
	DWORD IdThread;
	HANDLE hThread;

	VOID WINAPI ServiceCtrlHandler (DWORD Opcode) 
	{ 
		switch(Opcode) 
		{ 
		case SERVICE_CONTROL_PAUSE: 
			ServiceStatus.dwCurrentState = SERVICE_PAUSED; 
			break; 
	   case SERVICE_CONTROL_CONTINUE: 
			ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
			break; 
		case SERVICE_CONTROL_STOP: 
			ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
			TerminateThread(hThread,0);
			break;
		case SERVICE_CONTROL_INTERROGATE: 
			break; 
		} 

		SetServiceStatus(ServiceStatusHandle,&ServiceStatus);
		return; 
	} 
	BOOL ServiceInit()
	{
		hThread = CreateThread(NULL,4000,(LPTHREAD_START_ROUTINE)fsdmain,NULL,0,&IdThread);
		return hThread!=NULL;
	}
	void WINAPI ServiceMain (DWORD argc, LPTSTR *argv) 
	{ 
 
		ServiceStatus.dwServiceType        = SERVICE_WIN32; 
		ServiceStatus.dwCurrentState       = SERVICE_START_PENDING; 
		ServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP; 
		ServiceStatus.dwWin32ExitCode      = 0; 
		ServiceStatus.dwServiceSpecificExitCode = 0; 
		ServiceStatus.dwCheckPoint         = 0; 
		ServiceStatus.dwWaitHint           = 0; 
 
		ServiceStatusHandle = RegisterServiceCtrlHandler(ServerName,ServiceCtrlHandler); 
 
		if (ServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) 
			return; 
 
		if (!ServiceInit()) 
		{ 
			ServiceStatus.dwCurrentState       = SERVICE_STOPPED; 
			ServiceStatus.dwCheckPoint         = 0; 
			ServiceStatus.dwWaitHint           = 0; 
			ServiceStatus.dwWin32ExitCode      = -1; 
			ServiceStatus.dwServiceSpecificExitCode = 0; 
 
			SetServiceStatus (ServiceStatusHandle, &ServiceStatus); 
			return; 
		} 
 
		ServiceStatus.dwCurrentState       = SERVICE_RUNNING; 
		ServiceStatus.dwCheckPoint         = 0; 
		ServiceStatus.dwWaitHint           = 0; 
 
		SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
		return; 
	} 

	int main(int argc, char* argv[])
	{
		//SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
		if (argc==2)
		{
			if (stricmp(argv[1],"/INSTALL")==0)
			{
				SC_HANDLE scm = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
				if (scm==NULL)
				{
					printf("Error : Couldnt open Service Manager, make sure you are administrator\n");
					return -1;
				}
				char str[4096];
				char currentopath[4096];
				GetModuleFileName(NULL,currentopath,sizeof(currentopath));
				sprintf(str,"%s /SERVICE",currentopath);
				SC_HANDLE service = CreateService(scm,
					ServerName,
					ServerDesc,
					SERVICE_ALL_ACCESS,
					SERVICE_WIN32_OWN_PROCESS,
					SERVICE_AUTO_START,
					SERVICE_ERROR_SEVERE,
					str,
					NULL,
					NULL,
					NULL,
					NULL,
					NULL);
				if (service==NULL)
				{
					printf("Error : Couldnt create Service, make sure the service doesnt already exist\n");
					CloseServiceHandle(scm);
					return -1;
				}
				if (!StartService(service,NULL,NULL))
				{
					printf("Error : Couldnt start Service\n");
					CloseServiceHandle(service);
					CloseServiceHandle(scm);
					return -1;
				}
				CloseServiceHandle(service);
				CloseServiceHandle(scm);
				printf("Info : Service installed\n");
				return 0;
			}
			else if (stricmp(argv[1],"/UNINSTALL")==0)
			{
				SC_HANDLE scm = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
				if (scm==NULL)
				{
					printf("Error : Couldnt open Service Manager, make sure you are administrator\n");
					return -1;
				}
				
				SC_HANDLE service = OpenService(scm,ServerName,SERVICE_ALL_ACCESS);
				if (service==NULL)
				{
					printf("Error : Couldnt find Service, make sure the service exists\n");
					CloseServiceHandle(scm);
					return -1;
				}
				SERVICE_STATUS ss;
				ControlService(service,SERVICE_CONTROL_STOP,&ss);

				if (!DeleteService(service))
				{
					printf("Error : Couldnt start Service\n");
					CloseServiceHandle(service);
					CloseServiceHandle(scm);
					return -1;
				}
				CloseServiceHandle(service);
				CloseServiceHandle(scm);
				printf("Info : Service uninstalled\n");
				return 0;
			}
			else if (stricmp(argv[1],"/RUN")==0)
			{
				fsdmain();
				return 0;
			}
			else if (stricmp(argv[1],"/SERVICE")==0)
			{
				SERVICE_TABLE_ENTRY   DispatchTable[] = 
				{ 
					{ (char*)ServerName, ServiceMain}, 
					{ NULL,              NULL                 } 
				}; 
				if (!StartServiceCtrlDispatcher( DispatchTable)) 
				{ 
					return -1;
				}
				return 0;
			}
		}

			printf("USAGE : FSD\n");
			printf("\t/INSTALL : Install as Autostart service\n");
			printf("\t/UNINSTALL : Uninstall service, please reboot after\n");
			printf("\t/RUN : Run Interactive\n");

		return 0;
	}
#else
	int main(int argc, char **argv)
	{
	   char *configfile;
	   int c;
	   int debug;

	   configfile = PATH_FSD_CONF;
	   debug = 0;
   
	   while ((c = getopt(argc, argv, "df:")) != -1) {
		 switch (c) {
		 case 'd':
		   debug++;
		   break;
		 case 'f':
		   configfile = optarg;
		   break;
		 default:
		   fprintf(stdout, "usage: fsd [-d] [-f configfile]\n");
		   exit(1);
		 }
	   }

	   /* Fork/exec into the background */
	   if (!debug) {
		 /*daemon(0, 0);*/
	   }
   
	   dosignals();
	   run(configfile);
	}
#endif
