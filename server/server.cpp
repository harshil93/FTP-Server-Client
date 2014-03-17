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
#include <sys/stat.h>
#include <dirent.h>
#include <sstream>
using namespace std;

#define PR(x) cout << #x " = " << x << "\n";
#define DEBUG 1
#define LOG 1
#define BACKLOG 100             // No. of backlog reqeusts
#define BUFSIZE 2048			// BufferSize

char *FTP_SERVER_CONTROL_PORT;
char *FTP_SERVER_DATA_PORT;

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

int bindsocket(const char *port){
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


int make_client_connection_with_sockfd (int sock_fd,const char *host, const char *port)
{
	struct addrinfo hints, *res;

	// first, load up address structs with getaddrinfo():

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(host, port, &hints, &res);
	// connect!
	int stat = connect(sock_fd, res->ai_addr, res->ai_addrlen);
	if(stat==-1){
		cout<<"Connect Error "<<strerror(errno)<<endl;
		return -1;
	}
  
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


int sendallbinary(int serverfd, FILE *fd,int size){
	unsigned char buf[100001];
	int bytesSent=0;
	while(size>0){
		int bytesRead = fread(buf,1,100000,fd);
		int stat = send_all(serverfd,buf,bytesRead);
		if(stat != 0 ){
			cout<<"ERROR IN SENDING"<<endl;
			return -1;
		}
		size = size - bytesRead;
	}
	return 0;	
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

int reverseportstring(string &in,string &ipstr,string &portstr){
	int cnt=0,pos;;
	string ip = in;
	for (int i = 0; i < in.size(); ++i)
	{
		if(ip[i]==','){
			ip[i] = '.';
			cnt++;
			if(cnt==4){
				pos = i;
				break;
			}
		}
	}
	
	if(cnt!=4) return -1;
	ipstr = ip.substr(0,pos);
	string port = ip.substr(pos+1);
	int val=0;
	int i=0;

	while(i<port.size()){
		if(port[i] == ',') break;
		val = 10*val +  (port[i] - '0');
		i++;
	}
	val = 256*val;
	int portval = val;
	val = 0;
	i++;
	while(i<port.size()){
		val = 10*val + (port[i] - '0');
		i++;
	}

	portval = portval + val;
	stringstream ss;
	ss<<portval;
	portstr = ss.str();
	return 0;

}

string exec(const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    pclose(pipe);
    return result;
}

void doftp(int clientControlfd){
	string progver = "220 (ftpserver0.1)\r\n";
	send_all(clientControlfd,progver.c_str(),progver.size());
	string username;
	recvoneline(clientControlfd,username);
	if(username.compare(0,strlen("USER"),"USER") == 0){
		//do authentication
		string res = "331 Please specify password\r\n";
		send_all(clientControlfd,res.c_str(),res.size());
		string password;
		recvoneline(clientControlfd,password);
		// do pass authentication.
		res = "230 Login Successfuly\r\n";
		send_all(clientControlfd,res.c_str(),res.size());
	}else{
		string res = "430 Invalid Username\r\n";
		send_all(clientControlfd,res.c_str(),res.size());
	}
	int clientDatafd=0,datasocket=0;
	int binarymode = 0;
	while(1){
		string command;
		recvoneline(clientControlfd,command);
		if(command.compare(0,strlen("SYST"),"SYST") == 0  ){
			string res = "215 UNIX Type: L8\r\n";
			send_all(clientControlfd,res.c_str(),res.size());
		}else if(command.compare(0,strlen("PORT"),"PORT") == 0){
			string portstr = command.substr(4);
			portstr = trim(portstr);
			string ip,port;
			reverseportstring(portstr,ip,port);
			datasocket = bindsocket(FTP_SERVER_DATA_PORT);
			clientDatafd = make_client_connection_with_sockfd(datasocket,ip.c_str(),port.c_str());
			string res = "200 PORT command successful\r\n";
			send_all(clientControlfd,res.c_str(),res.size());
		}else if(command.compare(0,strlen("LIST"),"LIST") == 0){
			if(clientDatafd>0){
				string res = "150 Here comes the directory listing.\r\n";
				send_all(clientControlfd,res.c_str(),res.size());
				res = exec("ls -l");
				send_all(clientDatafd,res.c_str(),res.size());
				close(clientDatafd);
				close(datasocket);
				clientDatafd = 0;
				datasocket  = 0;
				res = "226 Directory send OK.\r\n";
				send_all(clientControlfd,res.c_str(),res.size());
			}else{
				string res = "425 Use PORT\r\n";
				send_all(clientControlfd,res.c_str(),res.size());
			}
		}else if(command.compare(0,strlen("PWD"),"PWD") == 0){
			char cwd[1024];
	      	getcwd(cwd, sizeof(cwd)) ;
	      	string res(cwd);
	      	res = "257 "+res+"\r\n";
			send_all(clientControlfd,res.c_str(),res.size());	        
		}else if(command.compare(0,strlen("CWD"),"CWD") == 0){
			string path = command.substr(3);
			path = trim(path);
			int stat = chdir(path.c_str());
			if(stat==0)
			{
				string res = "250 Directory successfully changed.\r\n";
				send_all(clientControlfd,res.c_str(),res.size());
			}
			else{
				string res = "550 Failed to change directory.\r\n";
				send_all(clientControlfd,res.c_str(),res.size());
			}

		}else if(command.compare(0,strlen("TYPE I"),"TYPE I") == 0){
			string res = "200 Switching to Binary mode.\r\n";
			binarymode = 1;
			send_all(clientControlfd,res.c_str(),res.size());
		}else if(command.compare(0,strlen("RETR"),"RETR") == 0){
			if(clientDatafd<=0){
				string res = "425 Use PORT first\r\n";
				send_all(clientControlfd,res.c_str(),res.size());
				continue;
			}

			string path = command.substr(4);
			path = trim(path);

			struct stat st;
			int statcode = stat(path.c_str(), &st);
			int size = st.st_size;
			if(statcode == -1){
				close(clientDatafd);
				close(datasocket);
				clientDatafd = 0;
				datasocket  = 0;
				string res = "550 "+string(strerror(errno))+"\r\n";
				send_all(clientControlfd,res.c_str(),res.size());
				binarymode=0;
				continue;
			}

			string res = "150 Opening BINARY mode data connection for "+path+"\r\n";
			send_all(clientControlfd,res.c_str(),res.size());

			
			FILE * filew;
			int numw;
			filew=fopen(path.c_str(),"rb");
			int len = sendallbinary(clientDatafd,filew,size);
			cout<<"Bytes Sent : "<<size<<endl;
			fclose(filew);
			close(clientDatafd);
			close(datasocket);
			clientDatafd = 0;
			datasocket  = 0;
			res = "226 Transfer complete.\r\n";
			send_all(clientControlfd,res.c_str(),res.size());
			binarymode=0;
		}else if(command.compare(0,strlen("STOR"),"STOR") == 0){
			if(clientDatafd<=0){
				string res = "425 Use PORT first\r\n";
				send_all(clientControlfd,res.c_str(),res.size());
				continue;
			}

			string path = command.substr(4);
			path = trim(path);

			string res = "150 Ok to send data.\r\n";
			send_all(clientControlfd,res.c_str(),res.size());

			FILE * filew;
			int numw;

			filew=fopen(path.c_str(),"wb");
			int len = recvallbinary(clientDatafd,filew);
			cout<<"Bytes Received : "<<len<<endl;
			fclose(filew);

			close(clientDatafd);
			close(datasocket);
			clientDatafd = 0;
			datasocket  = 0;
			res = "226 Transfer complete.\r\n";
			send_all(clientControlfd,res.c_str(),res.size());
			binarymode=0;
		}else if(command.compare(0,strlen("QUIT"),"QUIT") == 0){
			string res = "221 Goodbye.\r\n";
			send_all(clientControlfd,res.c_str(),res.size());
			close(clientDatafd);
			close(datasocket);
			close(clientControlfd);
			return;

		}else{
			string res = "500 Unknown command.\r\n";
			send_all(clientControlfd,res.c_str(),res.size());
		}
	}

}

int main(int argc, char **argv){
	if(argc == 3){
		FTP_SERVER_CONTROL_PORT = argv[1];
		FTP_SERVER_DATA_PORT= argv[2];
	}else{
		cout<<"The format is ./server <control port> <data port>"<<endl;
		cout<<"If you are using ports <1024 , use sudo ./server <control port> <data port>"<<endl;
		exit(0);
	}
	

	// make server bind and listen to the supplied port
	int server_fd;
	if((server_fd = server_listen(FTP_SERVER_CONTROL_PORT)) <0 ){
		fprintf(stderr, "%s\n", "Error listening to given port");
		return 0;
	}

	cout<<"Server Listening at "<<FTP_SERVER_CONTROL_PORT<<endl;
	// now start accept incoming connections:

	while(1){
		int clientControlfd;
		if((clientControlfd = accept_connection(server_fd)) <0){
			fprintf(stderr, "%s\n", "Error Accepting Connections");
		}else{
			int pid = fork();

			if(pid==0){
				//child
				doftp(clientControlfd);
				close(clientControlfd);
				return 0;
			}
		}

	}
	return 0;

}
