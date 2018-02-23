#ifndef PROTOCOLH
#define PROTOCOLH

#define CMD_NOTIFY    0
#define CMD_REQMETAR  1
#define CMD_PING      2
#define CMD_PONG      3
#define CMD_SYNC      4
#define CMD_LINKDOWN  5
#define CMD_NOWX      6
#define CMD_ADDCLIENT 7
#define CMD_RMCLIENT  8
#define CMD_PLAN      9
#define CMD_PD        10
#define CMD_AD        11
#define CMD_CERT      12
#define CMD_MULTIC    13
#define CMD_WEATHER   14
#define CMD_METAR     15
#define CMD_ADDWPROF  16
#define CMD_DELWPROF  17
#define CMD_KILL      18
#define CMD_RESET     19

#define CL_ADDATC     0
#define CL_RMATC      1
#define CL_ADDPILOT   2
#define CL_RMPILOT    3
#define CL_REQHANDOFF 4
#define CL_MESSAGE    5
#define CL_REQWEATHER 6
#define CL_PILOTPOS   7
#define CL_ATCPOS     8
#define CL_PING       9
#define CL_PONG       10
#define CL_ACHANDOFF  11
#define CL_PLAN       12
#define CL_SB         13
#define CL_PC         14
#define CL_WEATHER    15
#define CL_CLOUDDATA  16
#define CL_WINDDATA   17
#define CL_TEMPDATA   18
#define CL_REQCOM     19
#define CL_REPCOM     20
#define CL_REQACARS   21
#define CL_REPACARS   22
#define CL_ERROR      23
#define CL_CQ         24
#define CL_CR         25
#define CL_KILL       26
#define CL_WDELTA     27

#define CL_MAX        27

#define CERT_ADD       0
#define CERT_DELETE    1
#define CERT_MODIFY    2

#define ERR_OK         0         /* No error */
#define ERR_CSINUSE    1         /* Callsign in use */
#define ERR_CSINVALID  2         /* Callsign invalid */
#define ERR_REGISTERED 3         /* Already registered */
#define ERR_SYNTAX     4         /* Syntax error */
#define ERR_SRCINVALID 5         /* Invalid source in packet */
#define ERR_CIDINVALID 6         /* Invalid CID/password */
#define ERR_NOSUCHCS   7         /* No such callsign */
#define ERR_NOFP       8         /* No flightplan */
#define ERR_NOWEATHER  9         /* No such weather profile*/
#define ERR_REVISION   10        /* Invalid protocol revision */
#define ERR_LEVEL      11        /* Requested level too high */
#define ERR_SERVFULL   12        /* No more clients */
#define ERR_CSSUSPEND  13        /* CID/PID suspended */

extern const char *cmdnames[];
extern const char *clcmdnames[];
extern const char *errstr[];

#endif
