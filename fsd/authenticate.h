#ifdef __GNUC__
# ident "$Id: authenticate.h,v 2.01 1998/11/19 10:18:12 marty Exp $"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int auth_validate_ip (struct sockaddr_in *sa);

#ifdef __cplusplus
}
#endif /* __cplusplus */
