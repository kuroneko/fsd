FSD for Unix and Windows
------------------------

This distrib is made to make a single source for FSD on Unix and Windows

to compile fsd for unix : 
	move to fsd subfolder : cd fsd
	make the project : make
	the fsd will be generated in unix directory
	clean the project : make clean
 
to compile fsd for windows :
	launch Visual Studio 6 (or above) with fsd/fsd.dsw
	the fsd Release will be generated in windows directory
	clean the project removing generated Debug or Release folder


For Windows : 
	There is scripts to run interactivly FSD, install and run FSD as a service, uninstall the service

For Unix :
	Look fsd.sh to install fsd as autostart at server boot
	Launch ./fsd_d.sh to launch fsd as daemon process
	Launch killfsd.sh to kill the daemon


