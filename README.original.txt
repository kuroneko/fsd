FSD v2.0 Release notes.
=======================

This is the 2.0 release of FSD, the multiserver system for Squawkbox/Pro
Controller clients. 

In this package, you'll find these directories:

fsd       : Here's the source of the FSD system.
libdbms   : This is the MDBMS library.
etc       : This is the directory that contains sample configuration and help
            files.
docs      : Here are some docs. Mmm, here will be some docs.

Here are the steps you need to take to get to a running system:

Step 1 : Compiling
==================
Configuration of the server code is handled by the GNU autoconf
system. Type './configure' to configure the servercode. The preferred
directories are these:

	Component		Directory
	----------------------------------------------------------
	fsd server binary	/usr/local/sbin/
	fsd help files 		/usr/local/etc/fsd/

The configure script will ask you where the binary and the help files
should be placed. If you are not root, make sure the path you specify
here is accessible.
After running the configure script, type 'make' and then 'make install'.


Sorry, no man pages yet.

Step 2 : Configuring the server
===============================
All the files you need to run the server are in the 'fsd help files' directory.
Here you'll find the following files:
fsd.conf.sample : The configuration file.
help.txt        : The online help file for the system interface.
motd.txt.sample : The message that will be shown to clients once they connect.

All configuration takes place in the fsd.conf file. You must edit this file
before you start the server. First copy the fsd.conf.sample file to fsd.conf,
and then edit it. There's not much that needs to be changed.
Here's what you need to do:

- Find the 'system' group, it looks like [system]
- Edit the 'ident' field. This is the identification of the server. It needs
  to be unique in the network. This ident will later be provided, but for the
  purpose of beta testing, just make up a unique ident. Make sure it doesn't
  contain spaces or 'strange' characters. You can use something like:
  ident=marty, or ident=serv1. Just make sure it's a unique name.
  NOTE: Please don't make this field too long, it has to be sent in every
  packet. Usually, 5 characters should be enough
- Edit the email field. This is your email address.
- Edit the hostname field. This is the hostname or the IP address of your
  server.
- Edit the name field. This is a textual description of your server.
  It may contains spaces. Examples: 'Test server 1' or 'METAR weather server'.
  It's not really important what you put here.
- Edit the password field. This field contains the password you will have
  to enter if you want to access privileged commands on the system port.
  Leave the field empty if you don't want to use the password protection.
- Edit the location field. This is the physical location of the server in
  the world. You could enter a state or a city here. It may contain spaces.
- Save the file.

You may want to edit the motd.txt file, to send a personal greeting to
your clients.

If you want to test with more than 1 server on the same host, make sure
that the clientport/serverport/systemport in the config files all have
unique values.  

Now you're ready for your first testrun.

Step 3 : Start the server
=========================
Now we'll start the server for the first time. Make sure you are in the
target directory and you have the 'fsd' binary.
Now type ./fsd& to start the server.
If you've done that, you should find a file 'log.txt' in the 'help files'
directory. Look at this file to see if there are any errors here.
You should see the line 'We are up' at the bottom.

Step 4 : Test the server
========================

Upon startup, the server opens port 3012 (or whatever you configured).
This port can be used to manage the server. You can simply use telnet
to connect to this port:
type 'telnet localhost 3012'

You should now see a prompt.

From here you can issue commands. There's online help available with the
'help' command.

If you configured a password, you must enter this password before you can use
any privileged commands.  You can do that by typing: 'pwd <password>'
Try that first. If you see 'password correct', you have access to
privileged commands.

The first thing you should check is the timezone. Type 'time' to see the
local and UTC time. If these are not correct you should update the clock
and/or the timezone of your machine.

Another useful tool is the 'log' command. This command can be used to
quickly extract entries from the logfile. For example: if you want to
quickly check if anything went wrong, you type 'log show 3' (Show me
all messages that are at least of level WARNING). You may want
to type 'log delete 3' to delete these messages. Note that the deletion of
messages does not effect the log file itself. To learn more about the
log command, type 'help log'.

If you have access to Squawkbox or Pro Controller software, you should
check if it can connect to your server.


Step 5 : Connecting to the network (optional)
=============================================
If your server will be on the internet 24 hours a day, you can connect
to the FSD network. Don't do this unless you have enough bandwith available,
and you want to accept clients to test. If you have enough bandwith, go ahead,
you can always decide to disconnect from the network later.

Connectin to the network is very simple. You'll only have to find
the host address of another server running FSD. There's a list of servers
available at http://www.hinttech.com/~marty/sbserver. Once you are connected
to the network, your address will appear here automatically. Don't bother
trying my server (at hinttech), it's behind a firewall and I only use it
for METAR and databases features. You won't be able to reach it.

Make sure that you choose a server with a fast link and a minimal amount of
hops to your server. When we start testing, there might not be many servers in
the network, so your choice is limited, but there could be more servers every
day, so it might be wise to check regularly to see if there's a better server
nearby. If you find more than one server with a good link, you can decide
to connect to both. You can test the connectivity to a server by using the
standard unix tools 'ping' and traceroute.  Once you've found a good server,
connect to the system port of your local servers and type 'connect <hostname>'.
If all goes well, you should see 'Connection established'.

After a while a complete server list should be available with the command
'servers'. If anything fails you can view the log file, or use the 'log'
command to find more information.

Congratulations, you are now part of the (experimental) fsd network.
All network configurations, like the certificates and the weather system
will be available automatically.

We will now check the weather system.
Type 'weather KJFK'. If for some reason you get the message 'No METAR host
in network' then there is something wrong in the network, and you won't
be able to resolve weather requests. If there is a METAR server in the
network, the server will respond with the message 'weather request sent
to METAR host <name>'. You should now receive an overview of the current
weather at KJFK (New York).

Step 6 : Learn more about the system
====================================

I put some small hints here to get you on your way. All functionality
of the server has to be documented in the final release (any documenters out
there ?)

You can use the online help to find more information about the available
commands. Here's some more background information:

You can always use the 'servers' command to get an overview of the servers.
The 'Lag' field should be monitored now and then. It indicates the amount
of seconds it takes for a packet to make a trip to this server. This Lag value
should not get too high. If all the values are high, this indicates that
your connection to the other server is bad and you might consider connecting
to a different server. You can use the 'disconnect' command to disconnect a
server connection. If you have clients online, make sure that at least one
server connection remains.

The 'ping' command can be used to check the connection to a single server.

The 'connect' command without parameters shows important information about
the current server connections. Especially the Feed value is interesting.
It indicates the average amount of bytes per seconds that went through
the link in the last 10 minutes. The In-Q and Out-Q should not get too
big, otherwise you're probably dealing with a slow link. If this happens
regularly, you should consider connecting to a different server.

Certificates can be viewed with the 'cert' command. You can also add, delete
and modify certificates with the this command. Be careful with this, your
changes will be available in the entire network!

You might wonder about the security of the server network. The current beta
servers do not have much security features. You can control who is allowed
to connect to your serverport by changing the 'allowfrom' line in the config
file. You'll have to restart the server to make this work.

The server has an autoconnect feature which makes sure that server connections
will be restored when one is disconnected. After a disconnect on the server
port, the server will attempt to reconnect after 2 minutes. This feature
is only enabled when you made the first connect, and not for any connection
to your server that might have been initiated by other servers. The server
will repeat the connection attempts every 2 minutes forever if it fails.
To stop this process, use the 'delguard' command.

The 'clients' command can be used to view the list of clients that are
in the network. If you specify a name as the argument to clients, you'll
get an overview of this client. A pattern can also be specified. For
example 'clients rt' will show all clients that have 'rt' in their callsigns,
like 'marty' or 'bert'
