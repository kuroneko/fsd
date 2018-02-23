/*
 * Authentication routines for validating network connections.
 */

#ifdef __GNUC__
# ident "$Id: authenticate.c,v 2.01 1998/11/19 10:18:12 marty Exp $"
#endif

#ifdef WIN32
	#include <stdlib.h>
	#include <string.h>
	#include <winsock2.h>
	#include "authenticate.h"
#else
	#include <unistd.h>
	#include <stdlib.h>
	#include <string.h>
	#include <netdb.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include "authenticate.h"
#endif
/*
 * Number of times to retry hostname lookup after temporary DNS failures.
 */
#define NUM_RETRY 2

/*
 * Domain suffix in which we look up IP addresses.
 */
#define RBL_DOMAIN ".mcdu.com"

/* FUNCTION: auth_validate_ip
 *
 * Validate an IP address against the banned addresses list.
 *
 * Returns 1 if the address is allowed to connect, or 0 if
 * the address is banned.
 */
int
auth_validate_ip(struct sockaddr_in *sa)
{
  char *ips;			/* IPV4 address in dotted quad notation */
  char *fqdn;			/* FQDN to look up */
  struct hostent *he;		/* Result from gethostbyname */
  int count;			/* Retry counter for hostname lookups */

  ips = inet_ntoa(sa->sin_addr);
  if (ips == NULL) {
    /* XXX log something */
    return 1;
  }
  fqdn = malloc(strlen(ips) + sizeof(RBL_DOMAIN));
  if (fqdn == NULL) {
    /* XXX log something */
    return 1;
  }
  strcpy(fqdn, ips);
  strcat(fqdn, RBL_DOMAIN);

  for (count = 0; count < NUM_RETRY; count++) {

    he = gethostbyname(fqdn);
      if (he == NULL) {

#ifdef WIN32
	switch (WSAGetLastError()) 
	{
	case WSAHOST_NOT_FOUND:	/* No entry - host is okay */
	  free(fqdn);
	  return 1;
	  break;

	case WSATRY_AGAIN:		/* Temporary DNS error. Sleep and retry. */
	  Sleep(2);
	  continue;
	  break;

      /*  
         * These errors indicate DNS problems. Default to letting
	 * the client in.
	 */
	case WSANO_DATA:		/* Exists in DNS, but no A record? */
	case WSANO_RECOVERY:	/* Permanent DNS server error. */
	default:		/* Other unknown error condition */
	  free(fqdn);
	  return 1;
	  break;

	}
#else
	switch (h_errno) {
	  
	case HOST_NOT_FOUND:	/* No entry - host is okay */
	  free(fqdn);
	  return 1;
	  break;

	case TRY_AGAIN:		/* Temporary DNS error. Sleep and retry. */
	  sleep(2);
	  continue;
	  break;

        /*  
         * These errors indicate DNS problems. Default to letting
	 * the client in.
	 */
	case NO_DATA:		/* Exists in DNS, but no A record? */
	case NO_RECOVERY:	/* Permanent DNS server error. */
	default:		/* Other unknown error condition */
	  free(fqdn);
	  return 1;
	  break;

	}
#endif
      }
  }

  free(fqdn);
  
  if (he == NULL) {
    /* Couldn't find a host entry */
    return 1;
  } else {
    /* Found a valid A record. Punt the client. */
    return 0;
  }
  /*NOTREACHED*/
}
	  

  
  
