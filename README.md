FTP Server and Client
=====================
By- Harshil Lodhi - 11010121
	Shobhit Chaurasia - 11010179

Salient Features:
	- Follows the RFC 959.
	- Authentication Feature is implemented.
	- Supports File Transfer of any size (no buffer limitation).
	- Handles many errors for the client (Details given below).
	- Also works with the linux standard vsftpd server on localhost port 21.

Error Handling:
	- get <wrong file name>
		Response: 550 Bad file descriptor
	- put <wrong file name>
		No such file or directory
	- cd <wrong dir>
		Response: 550 Failed to change directory.
	- !cd <wrong dir>
		No such file or directory

------------------------------------------------

To run the server, goto server directory and.
	$ ./server <control port> <data port>
	Eg - $ ./server 9000 9001


To connect to the server, goto client directory and.
	$ ./client <ip> <control port>
	Eg - $ ./client 127.0.0.1 9000

Client:
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


Sample Input-Output (Client Side).
	- In the below input output, lines with --in-- (don't type this while giving input) is input from user side.
	- RAW FTP commands are shown in line starting with Request: 
	- Response from server are shown in line starting with Response:
	- Any username password will be accepted at this point of time

	$ ./client 127.0.0.1 9000 

	Response: 220 (ftpserver0.1)

	Enter Your Username
	tmp --in--
	Request: USER tmp

	Response: 331 Please specify password

	Enter Your Pass
	tmp --in--
	Request: PASS tmp

	Response: 230 Login Successfuly

	ftp>pwd --in--
	Request: PWD

	Response: 257 /home/harshil/Acads/network/server

	ftp>ls --in--
	Request: PORT 127,0,0,1,156,68

	Response: 200 PORT command successful

	Request: LIST

	Response: 150 Here comes the directory listing.

	------------DATA---------
	Response: 150 Here comes the directory listing.
	total 68
	-rw-r--r-- 1 harshil harshil    31 Mar 17 20:46 Makefile
	-rw-r--r-- 1 harshil harshil   145 Mar 17 20:55 README.txt
	-rwxr-xr-x 1 harshil harshil 40684 Mar 17 20:56 server
	-rw-r--r-- 1 harshil harshil 16114 Mar 17 19:29 server.cpp
	-rw-r--r-- 1 harshil harshil   446 Mar 17 21:12 testfile.txt

	-------END-DATA---------
	Response: 226 Directory send OK.

	ftp>get testfile.txt --in--
	get
	Request: TYPE I

	Response: 200 Switching to Binary mode.

	Request: PORT 127,0,0,1,156,68

	Response: 200 PORT command successful

	Request: RETR testfile.txt

	Response: 150 Opening BINARY mode data connection for testfile.txt

	DATA TRANSFER
	Bytes Received : 446
	Response: 226 Transfer complete.

	ftp>cd .. --in--
	cd
	Request: CWD ..

	Response: 250 Directory successfully changed.

	ftp>pwd --in--
	Request: PWD

	Response: 257 /home/harshil/Acads/network

	ftp>!ls --in--
	.
	..
	client.cpp
	client
	Makefile
	README.txt
	testfile.txt
	ftp>!pwd
	!pwd
	Current Dir: /home/harshil/Acads/network/client
	ftp>quit --in--

