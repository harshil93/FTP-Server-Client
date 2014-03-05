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

using namespace std;

#define PR(x) cout << #x " = " << x << "\n";
#define DEBUG 1
#define LOG 1
#define BACKLOG 100             // No. of backlog reqeusts
#define BUFSIZE 2048			// BufferSize



/**
 * server_listen - bind to the supplied port and listen
 * @param  char* port - a string
 * @return int the fd if no error otherwise <0 value which indicates error
 */
int server_listen(char *port){
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
    	PR(bytesSent)
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
	while(1){
		char buf[10001];
		int bytesRead;
		if((bytesRead = recv(serverfd,buf,10000,0)) >0){
			result = string(buf,buf+bytesRead);
			return 0;
		}else{
			return -1;
		}
	}
}

int recvonce(int serverfd, string& result){
	
	char buf[1001];
	int bytesRead;
	if((bytesRead = recv(serverfd,buf,1000,0)) >0){
		PR(bytesRead)
		result = string(buf,buf+bytesRead);
		PR(result)
		cout<<"andar"<<endl;
		return 0;
	}else{
		cerr<<"returning -1"<<endl;
		cerr<<errno<<endl;
		cout<<"errno"<<endl;
		return -1;
	}
	
}


int main(int argc, char **argv){

	if(argc!=3){
		cout<<"The correct format is $./client <host> <port>";
		return 0;
	}
	int serverfd;
	if( (serverfd = make_client_connection(argv[1],argv[2]) ) > 0 ){

		string    command[] =    //these are the commands we send to ftp server
		{
		        "USER harshil\r\n",   
		        "PASS omnamo\r\n",
		        "PORT 127,0,0,1,35,40\r\n",             //passive mode for firewalls
		        "LIST\r\n"         //put the file onto the server
		};
		string res;
		recvonce(serverfd,res);
		cout<<res<<endl;
		send_all(serverfd,command[0].c_str(),command[0].length());
		recvonce(serverfd,res);
		cout<<res<<endl;
		send_all(serverfd,command[1].c_str(),command[1].length());
		recvonce(serverfd,res);
		cout<<res<<endl;

		int pid = fork();

		if(pid != 0){
			// int *sta = (int *) malloc(sizeof(int));
			wait(NULL);
			// cout<<"asdfasdfasdfasdfasd "<<(*sta)<<endl;
			recvonce(serverfd,res);
			cout<<"sdfsdf"<<endl;
			cout<<res<<endl;
			return 0;
			//parent
		}else{
			//child which will receive data
			cout<<"CHILD"<<endl;
			char port[] = "9000";
			int dataportserverfd = server_listen(port);
			PR(dataportserverfd)
			send_all(serverfd,command[2].c_str(),command[2].length());
			cout<<"after send_all"<<endl;
			recvonce(serverfd,res);
			cout<<"223"<<endl;
			cout<<res<<endl;
			send_all(serverfd,command[3].c_str(),command[3].length());
			recvonce(serverfd,res);
			cout<<res<<endl;
			cout<<"@##@"<<endl;
			int dataportclientfd = accept_connection(dataportserverfd);
			recvall(dataportclientfd,res);
			cout<<"------------DATA---------"<<endl;
			cout<<res<<endl;
			cout<<"-------END-DATA---------"<<endl;
			close(dataportclientfd);
			close(dataportserverfd);

			return 0;
		}
		//server should be created
		

		
	}else{
		cerr<<"Cannot connect to server"<<endl;
	}

	return 0;

}



