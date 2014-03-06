#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> // inet_pton
#include <unistd.h> // close()
#include <errno.h>
#include <sys/wait.h>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
using namespace std;

#define PR(x) cout << #x " = " << x << "\n";
#define DEBUG 1
#define LOG 1
#define BACKLOG 100             // No. of backlog reqeusts
#define BUFSIZE 2048			// BufferSize

void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}
// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

/**
 * server_listen - bind to the supplied port and listen
 * @param  char* port - a string
 * @return int the fd if no error otherwise <0 value which indicates error
 */
int server_listen(const char *port){
	// Create address structs
	struct addrinfo hints, *res;
	int sock_fd;
	// Load up address structs
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int addr_status = getaddrinfo(NULL, port, &hints, &res);
	if (addr_status != 0)
	{
	  fprintf(stderr, "Cannot get info\n");
	  return -1;
	}

	// Loop through results, connect to first one we can
	struct addrinfo *p;
	for (p = res; p != NULL; p = p->ai_next)
	{
	  // Create the socket
	  sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	  if (sock_fd < 0)
	  {
	    perror("server: cannot open socket");
	    continue;
	  }

	  // Set socket options
	  int yes = 1;
	  int opt_status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	  if (opt_status == -1)
	  {
	    perror("server: setsockopt");
	    exit(1);
	  }

	  // Bind the socket to the port
	  int bind_status = bind(sock_fd, res->ai_addr, res->ai_addrlen);
	  if (bind_status != 0)
	  {
	    close(sock_fd);
	    perror("server: Cannot bind socket");
	    continue;
	  }

	  // Bind the first one we can
	  break;
	}

	// No binds happened
	if (p == NULL)
	{
	  fprintf(stderr, "server: failed to bind\n");
	  return -2;
	}

	// Don't need the structure with address info any more
	freeaddrinfo(res);

	// Start listening
	if (listen(sock_fd, BACKLOG) == -1) {
	  perror("listen");
	  exit(1);
	}

	return sock_fd;
}

/**
 * A function wrapper to wrap both IPv4 and IPv6
 * @param  struct sockaddr *sa
 */
void *get_in_addr (struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
 * Accepts a client connection. The server fd is passed as a para
 * @param  server_fd
 * @return client_fd
 */
int accept_connection(int server_fd){
	struct sockaddr_storage their_addr; // connector's address information
	char s[INET6_ADDRSTRLEN];
	socklen_t sin_size = sizeof their_addr;

	// Accept connections
	int client_fd = accept(server_fd, (struct sockaddr *)&their_addr, &sin_size);
	if (client_fd == -1)
	{
	  perror("accept");
	  return -1;
	}

	// Print out IP address
	inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
	printf("server: got connection from %s\n", s);
	// Setting Timeout
	struct timeval tv;
	tv.tv_sec = 120;  /* 120 Secs Timeout */
	tv.tv_usec = 0;  // Not init'ing this can cause strange errors
	setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
	return client_fd;
}

/**
 * Creates socket, connects to remote host
 * @param const char *host  - Host's domain name or IP address 
 * @param const char *port - The port to which we have to make connection. 
 * @returns fd of socket, <0 if error
 */
int make_client_connection (const char *host, const char *port)
{
  // Create address structs
  struct addrinfo hints, *res;
  int sock_fd;

  // Load up address structs
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  //fprintf(stderr, "%s %s\n", host, port);
  int addr_status = getaddrinfo(host, port, &hints, &res);
  if (addr_status != 0)
  {
    fprintf(stderr, "Cannot get address info\n");
    return -1;
  }

  // Loop through results, connect to first one we can
  struct addrinfo *p;
  for (p = res; p != NULL; p = p->ai_next)
  {
    // Create the socket
    sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd < 0)
    {
      perror("client: cannot open socket");
      continue;
    }

    // Make connection
    int connect_status = connect(sock_fd, p->ai_addr, p->ai_addrlen);
    if (connect_status < 0)
    {
      close(sock_fd);
      perror("client: connect");
      continue;
    }

    // Bind the first one we can
    break;
  }

  // No binds happened
  if (p == NULL)
  {
    fprintf(stderr, "client: failed to connect\n");
    return -2;
  }

  // Print out IP address
  char s[INET6_ADDRSTRLEN];
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  //fprintf(stderr, "client: connecting to %s\n", s);

  // Don't need the structure with address info any more
  freeaddrinfo(res);
  PR(sock_fd)
  return sock_fd;
}

/**
 * A wrapper function on send() socket all which tries to send all the data that is in the buffer
 * @param  int socket
 * @param  const void *buffer
 * @param  size_t length
 * @return
 */
int send_all(int socket,const void *buffer, size_t length) {
    size_t i = 0;
    for (i = 0; i < length;){
    	int bytesSent = send(socket, buffer, length - i,MSG_NOSIGNAL);
    	if(bytesSent==-1){
    		return errno;
    	}else{
    		i+=bytesSent;
    	}
    }
    return 0;
}
/**
 * It receives the data from serverfd till /r/n
 * @param  serverfd 
 * @param  result   sets the output to result
 * @return          status code
 */

int recvall(int serverfd, string& result){
	int len=0;
	while(1){
		char buf[10001];
		int bytesRead;
		if((bytesRead = recv(serverfd,buf,10000,0)) >0){
			result = string(buf,buf+bytesRead);
			len+=bytesRead;
		}else if(bytesRead<0){
			return -1;
		}else{
			return len;
		}
	}
}

int recvallbinary(int serverfd, FILE *fd){
	unsigned char buf[10001];
	int bytesRead=0;
	int len=0;
	while((bytesRead = recv(serverfd,buf,10000,0)) >0){
		cout<<"CALLED"<<endl;
		len+=bytesRead;
		fwrite(buf,1,bytesRead,fd);
	}
	if(bytesRead < 0){
		cerr<<"Error Occurred";
		return -1;
	}else{
		
		return len;
	}
}

string remBuf;

int recvoneline(int serverfd, string& result){
	char buf[1001];
	int bytesRead=0;
	result = remBuf;
	do{
		result+=string(buf,buf+bytesRead);
		int pos = result.find("\r\n");
		if(pos!=string::npos){
			//found
			remBuf=result.substr(pos+2);
			result=result.substr(0,pos+2);
			break;
		}
	}while((bytesRead = recv(serverfd,buf,1000,0)) >0);

	if(bytesRead < 0){
		cerr<<"Error Occurred";
		return -1;
	}else{
		return 0;
	}
	
}

void getportstring(string& portstr, string& port){
	portstr = "PORT 127,0,0,1,35,40\r\n";
	port="9000";
}

int main(int argc, char **argv){
	remBuf="";
	if(argc!=3){
		cout<<"The correct format is $./client <host> <port>";
		return 0;
	}
	int serverfd;
	if( (serverfd = make_client_connection(argv[1],argv[2]) ) > 0 ){

		string    command[] =    //these are the commands we send to ftp server
		{
		        "USER harshil\r\n",   
		        "PASS omnamo\r\n"
		};

		string res;
		recvoneline(serverfd,res);
		cout<<res<<endl;
		send_all(serverfd,command[0].c_str(),command[0].length());
		recvoneline(serverfd,res);
		cout<<res<<endl;
		send_all(serverfd,command[1].c_str(),command[1].length());
		recvoneline(serverfd,res);
		cout<<res<<endl;


		while(1){
			cout<<"ftp>";
			string userinput;
			getline(std::cin,userinput);
			ltrim(userinput);
			if(userinput.compare(0,strlen("put"),"put") == 0){
					cout<<"put"<<endl;
			}else if(userinput.compare(0,strlen("get"),"get") == 0){
				cout<<"get"<<endl;
				int pid = fork();
				if(pid != 0){
					int stat;
					wait(&stat);
					recvoneline(serverfd,res);
					cout<<res<<endl;
					
				}else{
					string typei = "TYPE I\r\n";
					send_all(serverfd,typei.c_str(),typei.size());
					recvoneline(serverfd,res);
					cout<<res<<endl;
					
					string portstr,port;
					getportstring(portstr,port);
					
					int dataportserverfd = server_listen(port.c_str());
					PR(dataportserverfd)
					send_all(serverfd,portstr.c_str(),portstr.size());
					recvoneline(serverfd,res);
					cout<<res<<endl;

					string path = userinput.substr(3);
					path = trim(path);
					PR(path)
					string getstr = "RETR "+path+"\r\n";
					send_all(serverfd,getstr.c_str(),getstr.size());
					recvoneline(serverfd,res);
					cout<<res<<endl;

					int dataportclientfd = accept_connection(dataportserverfd);
					
					FILE * filew;
					int numw;

					filew=fopen(path.c_str(),"wb");
					int len = recvallbinary(dataportclientfd,filew);
					cout<<"Bytes Received : "<<len<<endl;
					fclose(filew);
					close(dataportclientfd);
					close(dataportserverfd);
					return 0;

				}
			}else if(userinput.compare(0,strlen("ls"),"ls") == 0){
				int pid = fork();
				if(pid != 0){
					int stat;
					wait(&stat);
					recvoneline(serverfd,res);
					cout<<res<<endl;
					

				}else{
					//child which will receive data
					string portstr,port;
					getportstring(portstr,port);
					
					int dataportserverfd = server_listen(port.c_str());
					PR(dataportserverfd)
					send_all(serverfd,portstr.c_str(),portstr.size());
					recvoneline(serverfd,res);
					cout<<res<<endl;
					send_all(serverfd,"LIST\r\n",strlen("LIST\r\n"));
					recvoneline(serverfd,res);
					cout<<res<<endl;
					int dataportclientfd = accept_connection(dataportserverfd);
					recvall(dataportclientfd,res);
					cout<<"------------DATA---------"<<endl;
					cout<<res<<endl;
					cout<<"-------END-DATA---------"<<endl;
					close(dataportclientfd);
					close(dataportserverfd);
					return 0;

				}
			}else if(userinput.compare(0,strlen("cd"),"cd") == 0){
					cout<<"cd"<<endl;
					string path = userinput.substr(2);
					path = trim(path);
					string cwdstr = "CWD "+path+"\r\n";
					send_all(serverfd,cwdstr.c_str(),cwdstr.size());
					recvoneline(serverfd,res);
					cout<<res<<endl;
			}else if(userinput.compare(0,strlen("pwd"),"pwd") == 0){
					string pwdstr = "PWD\r\n";
					send_all(serverfd,pwdstr.c_str(),pwdstr.size());
					recvoneline(serverfd,res);
					cout<<res<<endl;
			}else if(userinput.compare(0,strlen("!ls"),"!ls") == 0){
					cout<<"!ls"<<endl;
			}else if(userinput.compare(0,strlen("!cd"),"!cd") == 0){
					cout<<"!cd"<<endl;
			}else if(userinput.compare(0,strlen("!pwd"),"!pwd") == 0){
					cout<<"!pwd"<<endl;
			}else if(userinput.compare(0,strlen("quit"),"quit") == 0){
					cout<<"quit"<<endl;
			}/*else{
					send_all(serverfd,(userinput+"\r\n").c_str(),userinput.size() + 2);
					recvoneline(serverfd,res);
					cout<<res<<endl;	
			}*/
		}
		

		
		//server should be created
		

		
	}else{
		cerr<<"Cannot connect to server"<<endl;
	}

	return 0;

}



