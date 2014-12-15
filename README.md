FTP Server and Client
=====================
By- Harshil Lodhi - 11010121
	Shobhit Chaurasia - 11010179

Video Presentation: https://www.youtube.com/watch?v=lLrd38ExXZc (Please see it once to get the feel of the total project - See it in HD)

###Salient Features:
- Closely Follows the RFC 959.
- Authentication Feature is implemented.
- Supports File Transfer of any size (no buffer limitation).
- Handles many errors for the client (Details given below).
- Also works with the linux standard vsftpd server on localhost port 21. (You have to provide proper username password of the computer running vsftpd for this)
- Can view the raw ftp commands- request messages (for debugging) on client side. Give -d as 3rd arguement to client for this.
		eg. ./client 127.0.0.1 9000 -d


###Instructions:

To compile and run the server, goto server directory and.
	$ make
	$ ./server <control port> <data port>
	Eg - $ ./server 9000 9001


To compile client and connect to the server, goto client directory and do the following. 

	$ make
	$ ./client <ip> <control port> [-d]
	-d is optional for debuggin
	Eg - $ ./client 127.0.0.1 9000
	or $ ./client 127.0.0.1 9000 -d

####Client:
	Supported Commands:
	1. ls
		Lists the current server directory listing
	2. !ls
		Lists the current client directory listing
	3. pwd
		Gives the current server directory
	4. !pwd
		Gives the current client directory
	5. cd <path>
		Changes directory on server
	6. !cd <path>
		Changes directory on client
	7. put filename
		Uploads the given filename to server.
	6. get filename
		Downloads the given filename from server.
