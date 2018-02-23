#! /bin/bash
#----- This file is intend to be used as automatic start and stop of FSD
#-----  at server boot and server  shutdown
#----- Copy this file to /etc/init.d as root : cp fsd /etc/init.d
#----- Link as startup the init.d file to /etc/rc3.d : ln -s /etc/init.d/fsd /etc/rc3.d/S85fsd
#----- Link as shutdown the init.d file to /etc/rc3.d : ln -s /etc/init.d/fsd /etc/rc3.d/K15fsd
# START OF CONFIG SECTION
# WARNING ! For security reasons we advise: DO NOT RUN THE SERVER AS ROOT
#----- USER is the unix user that will run fsd process, you can set user root
#-----  but it is not recommanded. Create a unix user to run fsd
USER=fsd
#----- DIR must point to fsd installation path
DIR=/home/fsd/fsd
# END OF CONFIG SECTION

# See how we were called.
case "$1" in
        start)
		WD=`pwd`
		cd $DIR
		su $USER -c "nohup ./fsd &" &
		cd $WD
        ;;
    	stop)
		killall -9 fsd
        ;;
        restart)
                $0 stop && $0 start || exit 1
        ;;
        *)
                echo "Usage: $0 {start|stop|restart}"
                exit 2
esac
exit 0
