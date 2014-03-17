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
#include <sstream>
#include <dirent.h>
using namespace std;

#define PR(x) cout << #x " = " << x << "\n";
#define LOG 1
#define BACKLOG 100             // No. of backlog reqeusts
#define BUFSIZE 2048			// BufferSize
int debug;
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
	// printf("server: got connection from %s\n", s);
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
			result += string(buf,buf+bytesRead);
			len+=bytesRead;
		}else if(bytesRead<0){
			return -1;
		}else{
			return len;
		}
	}
}

/**
 * Receives data in binary mode and writes it to fd
 * @param  int serverfd 
 * @param  FILE *fd       
 * @return  -1 error, 0 success 
 * 
 * */
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

/**
 * Sends the to serverfd using binary mode reading from fd and 
 * @param  serverfd 
 * @param  FILE *fd       
 * @param  int size - size of the file    
 * @return -1 error, 0 success
 */
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


string remBuf; // to handle buffer
/**
 * Receives one line from serverfd and stores it in result
 * @param  serverfd 
 * @param  result   
 * @return          -1 error, 0 success
 */
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
/**
 * To execute any system command
 * @param  cmd - command to be executed
 * @return  output of the command
 */
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
/**
 * Converts int to string
 * @param  k - supplied integer
 * @return   string
 */
string int2str(int k){
	stringstream ss;
	ss<<k;
	return ss.str();
}
/**
 * Gives the ip of the current system on which this code is running
 * @param  m_sd - the socket descriptor
 * @return - ip string 
 */
string getownip(int m_sd){
	struct sockaddr_in localAddress;
	socklen_t addressLength = sizeof(localAddress);
	getsockname(m_sd, (struct sockaddr*)&localAddress, &addressLength);
	return string(inet_ntoa( localAddress.sin_addr));
}

/**
 * Selects a random port and gives its port string by assigning portstr and also giving port in port
 * @param ownip   - the supplied ip (Eg. 127.0.0.1 )
 * @param portstr - returned PORT command string. (Eg- PORT 127,0,0,1,35,40)
 * @param port    - returned port (Eg - 9000)
 */
void getportstring(string ownip,string& portstr, string& port){
	for (int i = 0; i < ownip.size(); ++i)
	{
		if(ownip[i] == '.') ownip[i] = ',';
	}
	int portnum = 40001 + rand() % 10;
	string p1 = int2str(portnum/256);
	string p2 = int2str(portnum%256);
	portstr = "PORT "+ownip+","+p1+","+p2+"\r\n";
	//portstr = "PORT 127,0,0,1,"+p1+","+p2+"\r\n";
	port = int2str(portnum);
}

int main(int argc, char **argv){
	remBuf="";
	debug = 0;
	if(argc<3){
		cout<<"The correct format is $./client <host> <port>";
		return 0;
	}
	if(argc == 4){
		string msg = string(argv[3]);
		if(msg == "-d"){
			// Debug Mode
			debug = 1;
		}
	}
	int serverfd;
	if( (serverfd = make_client_connection(argv[1],argv[2]) ) > 0 ){
		// Sending out Authentication Information.
		string res,user,pass;
		recvoneline(serverfd,res);
		cout<<"Response: "<<res<<endl;
		cout<<"Enter Your Username"<<endl;
		getline(std::cin,user);
		string userstr = "USER "+user+"\r\n";
		if(debug == 1) cout<<"Request: "<<userstr<<endl;
		send_all(serverfd,userstr.c_str(),userstr.size());
		recvoneline(serverfd,res);
		cout<<"Response: "<<res<<endl;
		cout<<"Enter Your Pass"<<endl;
		getline(std::cin,pass);
		string passstr = "PASS "+pass+"\r\n";
		if(debug == 1) cout<<"Request: "<<passstr<<endl;
		send_all(serverfd,passstr.c_str(),passstr.size());
		recvoneline(serverfd,res);
		cout<<"Response: "<<res<<endl;
		//TODO - handle auth

		while(1){
			cout<<"ftp>";
			string userinput;
			getline(std::cin,userinput);
			ltrim(userinput);
			if(userinput.compare(0,strlen("put"),"put") == 0){
				//cout<<"put"<<endl;
				int pid = fork();
				if(pid != 0){
					int stat;
					wait(&stat);
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;
					
				}else{
					string path = userinput.substr(3); // Gets the filename from put command
					path = trim(path);

					// Gets the file size
					struct stat st;
					int statcode = stat(path.c_str(), &st);
					int size = st.st_size;
					if(statcode == -1){
						cout<<strerror(errno)<<endl;
						continue;
					}
					// Switching to Binary Mode
					string typei = "TYPE I\r\n";
					if(debug == 1) cout<<"Request: "<<typei<<endl;
					send_all(serverfd,typei.c_str(),typei.size());
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;

					// Getting a random port and corresponding PORT command
					string portstr,port;
					getportstring(getownip(serverfd),portstr,port);
					
					// Listening to data server
					int dataportserverfd = server_listen(port.c_str());
					if(debug == 1) cout<<"Request: "<<portstr<<endl;
					send_all(serverfd,portstr.c_str(),portstr.size());
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;

					// Sending out the STOR command
					string storstr = "STOR "+path+"\r\n";
					if(debug == 1) cout<<"Request: "<<storstr<<endl;
					send_all(serverfd,storstr.c_str(),storstr.size());
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;

					int dataportclientfd = accept_connection(dataportserverfd);
					
					// Opening file and sending Data
					FILE * filew;
					int numw;
					filew=fopen(path.c_str(),"rb");
					cout<<"DATA TRANSFER"<<endl;
					int len = sendallbinary(dataportclientfd,filew,size);
					
					fclose(filew);
					close(dataportclientfd);
					close(dataportserverfd);
					return 0;

				}
			}else if(userinput.compare(0,strlen("get"),"get") == 0){
				cout<<"get"<<endl;
				int pid = fork();
				if(pid != 0){
					int stat;
					wait(&stat);
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;
					
				}else{
					// Switching to Binary Mode

					string typei = "TYPE I\r\n";
					if(debug == 1) cout<<"Request: "<<typei<<endl;
					send_all(serverfd,typei.c_str(),typei.size());
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;

					// Getting a random port and corresponding PORT command
					string portstr,port;
					getportstring(getownip(serverfd),portstr,port);
					
					// Listening to data server
					int dataportserverfd = server_listen(port.c_str());
					if(debug == 1) cout<<"Request: "<<portstr<<endl;
					send_all(serverfd,portstr.c_str(),portstr.size());
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;

					string path = userinput.substr(3); // Gets the filename from get command
					path = trim(path);
					

					string getstr = "RETR "+path+"\r\n";
					if(debug == 1) cout<<"Request: "<<getstr<<endl;
					send_all(serverfd,getstr.c_str(),getstr.size());
					recvoneline(serverfd,res);


					cout<<"Response: "<<res<<endl;
					if(res.compare(0,strlen("550"),"550") == 0){
						close(dataportserverfd);
						continue;
					}
					int dataportclientfd = accept_connection(dataportserverfd);
					
					FILE * filew;
					int numw;
					// Receiving data and storing it in file.
					filew=fopen(path.c_str(),"wb");
					cout<<"DATA TRANSFER"<<endl;
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
					cout<<"Response: "<<res<<endl;
					

				}else{
					//child which will receive data
					string portstr,port;
					getportstring(getownip(serverfd),portstr,port);
					int dataportserverfd = server_listen(port.c_str());
					if(debug == 1) cout<<"Request: "<<portstr<<endl;
					send_all(serverfd,portstr.c_str(),portstr.size());
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;
					if(debug == 1) cout<<"Request: "<<"LIST\r\n"<<endl;
					send_all(serverfd,"LIST\r\n",strlen("LIST\r\n"));
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;
					int dataportclientfd = accept_connection(dataportserverfd);
					recvall(dataportclientfd,res);
					cout<<"------------DATA---------"<<endl;
					cout<<"Response: "<<res<<endl;
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
					if(debug == 1) cout<<"Request: "<<cwdstr<<endl;
					send_all(serverfd,cwdstr.c_str(),cwdstr.size());
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;
			}else if(userinput.compare(0,strlen("pwd"),"pwd") == 0){
					string pwdstr = "PWD\r\n";
					if(debug == 1) cout<<"Request: "<<pwdstr<<endl;
					send_all(serverfd,pwdstr.c_str(),pwdstr.size());
					recvoneline(serverfd,res);
					cout<<"Response: "<<res<<endl;
			}else if(userinput.compare(0,strlen("!ls"),"!ls") == 0){
					DIR *dp;
					struct dirent *ep;     
					dp = opendir ("./");
					if (dp != NULL)
					  {
					    while (ep = readdir (dp))
					      cout<<ep->d_name<<endl;
					    closedir (dp);
					  }
					  else
					    perror ("Couldn't open the directory");
			}else if(userinput.compare(0,strlen("!cd"),"!cd") == 0){
					string path = userinput.substr(3);
					path = trim(path);
					int stat = chdir(path.c_str());
					if(stat==0)
						cout<<"Directory Successfully Changed"<<endl;
					else
						cout<<strerror(errno)<<endl;
			}else if(userinput.compare(0,strlen("!pwd"),"!pwd") == 0){
					cout<<"!pwd"<<endl;
					char cwd[1024];
			       if (getcwd(cwd, sizeof(cwd)) != NULL)
			           cout<<"Current Dir: "<<cwd<<endl;
			       else
			           perror("getcwd() error");

			}else if(userinput.compare(0,strlen("quit"),"quit") == 0){
					close(serverfd);
					exit(0);
			}else if(userinput.compare(0,strlen("help"),"help") == 0){
					cout<<"Look at the README file supplied"<<endl;
			}			else{
				cout<<"UNKNOWN COMMAND"<<endl;	
			}
		}
				
	}else{
		cerr<<"Cannot connect to server"<<endl;
	}

	return 0;

}



