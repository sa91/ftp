//gcc -lssl -lcrypto -lpthread -g -Wall -o file final.cpp


//  You can make read synchronous by this. (StackOverflow)
// Set the socket I/O mode: In this case FIONBIO  
// enables or disables the blocking mode for the   
// socket based on the numerical value of iMode.  
// If iMode = 0, blocking is enabled;   
// If iMode != 0, non-blocking mode is enabled.
//ioctl(sockfd, FIONBIO, &iMode);  
// read and wrie are non-blocking so, 
//use poll or select to make code beter ,it workd because its in a while loop.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h> 
#include <openssl/md5.h>
#include <time.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <wait.h>
#include <sys/sendfile.h>
#define PORT1 51111
#define PORT2 5521 
#ifdef HAVE_ST_BIRTHTIME
#define mtime(x) x.st_mtime
#else
#define mtime(x) x.st_ctime
#endif
#define ffsize(x) x.st_size

void error(const char *msg)
{
	perror(msg);
	exit(1);
}
char filen[256];
char buf[256],md5[256];
char buffer[256];
char command[501];char *tag[10];
char clientname[256],data[1024];
char output[1024];
void ttime(time_t t)
{
	bzero(buf,256);
	struct tm lt;
	localtime_r(&t, &lt);
	strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &lt);
}
void md5calc(char *filename)
{
	bzero(md5,256);
	unsigned char c[MD5_DIGEST_LENGTH];
	int i;
	// printf("filename:%s\n",filename);
	FILE *inFile = fopen (filename, "rb");
	MD5_CTX mdContext;
	int bytes;
	unsigned char data[1024];

	if (inFile == NULL) {
		printf ("%s can't be opened.\n", filename);
		return ;
	}

	MD5_Init (&mdContext);
	while ((bytes = fread (data, 1, 1024, inFile)) != 0)
		MD5_Update (&mdContext, data, bytes);
	MD5_Final (c,&mdContext);
	for(i = 0; i < MD5_DIGEST_LENGTH; i++) sprintf(md5+(2*i),"%02x", c[i]);
	fclose (inFile);
}
void TCPserver(char *filename,int size)
{
	//printf("start\n");
	//fflush(stdout);
	//printf("%s\n",filename);
	int sockfd, newsockfd, portno;
	socklen_t clilen;    
	struct sockaddr_in serv_addr, cli_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = PORT2;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		error("ERROR on binding");
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) 
		error("ERROR on accept");
	//printf("%s",filename);
	FILE *inFile = fopen (filename, "rb");
	if (inFile == NULL) {
		printf ("%s can't be opened.\n", filename);
		error("file cant be read");
	}
	if(size)
	{
		bzero(data,1024);
		int sent_bytes,bytes;
		while ( (size > 0))
		{
			bzero(data,0);
			bytes = fread (data, 1, 1024, inFile);
			//printf("data %s",data);
			sent_bytes=write(newsockfd,data,bytes);    
			if(sent_bytes < 0) error("ERROR writing to file_list");
			//fprintf(stdout, "1. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, bytes, size);
			size -= sent_bytes;
		}
		//buffer_s=sendfile(newsockfd, filehandle, &offset, size);
	}
	close(newsockfd);
	close(sockfd); 

}
void UDPserver(char* filename,int size)
{
    int udpSocket, nBytes;
    struct sockaddr_in serverAddr, clientAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size, client_addr_size;
    int i;

    /*Create UDP socket*/
    udpSocket = socket(PF_INET, SOCK_DGRAM, 0);

    /*Configure settings in address struct*/
     serverAddr.sin_family = AF_INET;
     serverAddr.sin_port = htons(PORT2);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

    /*Bind socket with address struct*/
    bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    /*Initialize size variable to be used later on*/
    addr_size = sizeof serverStorage;
    FILE *inFile = fopen (filename, "rb");
    if (inFile == NULL) {
        printf ("%s can't be opened.\n", filename);
        error("file cant be read");
    }
    nBytes = recvfrom(udpSocket,data,1024,0,(struct sockaddr *)&serverStorage, &addr_size);
    bzero(data,1024);
    while ( (size > 0))
    {
            bzero(data,0);
            int bytes = fread (data, 1, 1024, inFile);
            //printf("data %s",data);
            int sent_bytes=sendto(udpSocket,data,bytes,0,(struct sockaddr *)&serverStorage,addr_size);
            if(sent_bytes < 0) error("ERROR writing to file_list");
            //fprintf(stdout, "1. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, bytes, size);
            size -= sent_bytes;
    }
    close(udpSocket);

}
void *server()
{
	char delimit[]=" \t\r\n\v\f"; 
	int buffer_s,command_s,n;  
	int sockfd, newsockfd, portno;
	socklen_t clilen;

	struct sockaddr_in serv_addr, cli_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = PORT1;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		error("ERROR on binding");
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) 
		error("ERROR on accept");
	printf("connection establised\n");
	while(1) 
	{
		int i=0;
		time_t starttime,endtime;      
		bzero(command,501);
		command_s = read(newsockfd,command,500);
		if (command_s < 0) error("ERROR reading from socket");
		//printf("Here is the command of other user: %s\n",command);
		tag[i++]=strtok(command,delimit);
		if(!bcmp("Quit",tag[0],4)) break;
		else if(!bcmp("IndexGet",tag[0],8))
		{    
			DIR *d;
			struct dirent *dir;
			d = opendir(".");
			starttime=0;endtime=time(NULL);

			tag[i++]=strtok(NULL,delimit);


			if( tag[1]!=NULL && !bcmp("shortlist",tag[1],9))
			{
				tag[i++]=strtok(NULL,delimit);tag[i++]=strtok(NULL,delimit);
				if(tag[2]==NULL || tag[3]==NULL) 
					error("ERROR time for shortlist is not given");
				struct tm st,ed;
				char *sptr1,*sptr2;
				st.tm_year = atoi(strtok_r(tag[2],".",&sptr1))-1900;ed.tm_year = atoi(strtok_r(tag[3],".",&sptr2))-1900;
				st.tm_mon = atoi(strtok_r(NULL,".",&sptr1))-1;ed.tm_mon = atoi(strtok_r(NULL,".",&sptr2))-1;
				st.tm_mday = atoi(strtok_r(NULL,".",&sptr1));ed.tm_mday = atoi(strtok_r(NULL,".",&sptr2));
				st.tm_hour = atoi(strtok_r(NULL,".",&sptr1));ed.tm_hour = atoi(strtok_r(NULL,".",&sptr2));
				st.tm_min = atoi(strtok_r(NULL,".",&sptr1));ed.tm_min = atoi(strtok_r(NULL,".",&sptr2)); 
				st.tm_sec = atoi(strtok_r(NULL,".",&sptr1));ed.tm_sec = atoi(strtok_r(NULL,".",&sptr2));
				st.tm_isdst = -1;ed.tm_isdst=-1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
				starttime = mktime(&st);endtime=mktime(&ed);
			}
			if (d)
			{
				while ((dir = readdir(d)) != NULL)
				{
					struct stat stt;
					if( stat(dir->d_name, &stt) != 0 )
						perror(dir->d_name);
					//printf("%s\n",dir->d_name);
					time_t t = mtime(stt);
					// printf("%lld %lld %s %lld\n",starttime,endtime,dir->d_name,t);
					if(t>=starttime && t<=endtime)
					{   
						bzero(data,1024);
						ttime(t);
						if(S_ISDIR(stt.st_mode)){
							sprintf(data,"FileName: %s      Last modifaication: %s      FileSize:%ld     FileType: Directory\n",dir->d_name,buf,ffsize(stt));
							buffer_s = write(newsockfd,data,strlen(data));
							if (buffer_s < 0) error("ERROR writing to file_list");
						}
						else{
							sprintf(data,"FileName: %s      Last modifaication: %s      FileSize:%ld     FileType: Regular\n",dir->d_name,buf,ffsize(stt));
							buffer_s = write(newsockfd,data,strlen(data));
							if (buffer_s < 0) error("ERROR writing to file_list");}
					}
				}
				closedir(d);
			}

		}
		else if(!bcmp("FileHash",tag[0],8))
		{
			tag[i++]=strtok(NULL,delimit);
			if(tag[1]!=NULL && !bcmp("verify",tag[1],6))
			{
				struct stat stt;
				tag[i++]=strtok(NULL,delimit);
				if( stat(tag[2], &stt) != 0 )
					perror(tag[2]);
				time_t t = mtime(stt);
				ttime(t);md5calc(tag[2]);
				bzero(data,1024);
				sprintf(data,"FileName: %s      Last modification: %s       CheckSum: %s\n",tag[2],buf,md5);
				buffer_s = write(newsockfd,data,strlen(data));
				if(buffer_s < 0) error("ERROR writing to file_list");
			}
			else
			{
				DIR *d;
				struct dirent *dir;
				d = opendir(".");
				if (d)
				{
					while ((dir = readdir(d)) != NULL)
					{
						bzero(data,1024);
						struct stat stt;
						if( stat(dir->d_name, &stt) != 0 )
							perror(dir->d_name);
						time_t t = mtime(stt);
						ttime(t);md5calc(dir->d_name);
						sprintf(data,"FileName: %s      Last modification: %s      CheckSum: %s\n",dir->d_name,buf,md5);
						buffer_s = write(newsockfd,data,strlen(data));
						if(buffer_s < 0) error("ERROR writing to file_list");
					}
				}
			}

		}
		else if(!bcmp("FileDownload",tag[0],12))
		{
		//	printf("came here\n");
		//			fflush(stdout);
			bzero(data,1024);
			tag[i++]=strtok(NULL,delimit);
			tag[i++]=strtok(NULL,delimit);
			struct stat stt;
			if( stat(tag[2], &stt) != 0 )
				perror(tag[2]);
			time_t t = mtime(stt);
			md5calc(tag[2]);ttime(t);
			if(!bcmp(tag[1],"TCP",3))
			{
                sprintf(data,"TCP FileName: %s       FileSize: %ld   Last modifaication: %s     CheckSum: %s\n",tag[2],ffsize(stt),buf,md5);
                buffer_s = write(newsockfd,data,strlen(data));
                if(buffer_s < 0) error("ERROR writing to file_list");        
		//			printf("came here\n");
		//			fflush(stdout);
					TCPserver(tag[2],ffsize(stt));// make it threaded

			}
            if(!bcmp(tag[1],"UDP",3))
            {
                sprintf(data,"UDP FileName: %s       FileSize: %ld   Last modifaication: %s     CheckSum: %s\n",tag[2],ffsize(stt),buf,md5);
                buffer_s = write(newsockfd,data,strlen(data));
                if(buffer_s < 0) error("ERROR writing to file_list");        
        //          printf("came here\n");
        //          fflush(stdout);
                    UDPserver(tag[2],ffsize(stt));// make it threaded

            }        
		}
		n = write(newsockfd,"!#(",3);
		//if (n) printf("Done");
	}
	close(newsockfd);
	close(sockfd); 
}
void TCPclient(char *filn,int size)
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	server = gethostbyname(clientname);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,server->h_length);
	serv_addr.sin_port = htons(PORT2);
	while(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))!=0); 
	bzero(output,0);
	FILE *outFile = fopen (filn, "w+");
	while ((size > 0))
	{
		int len = read(sockfd, output, 1024);
		//printf("%s\n",output);
		fwrite(output, sizeof(char), len, outFile);
		size -= len;
		//fprintf(stdout, "Receive %d bytes and we hope :- %d bytes\n", len, size);
	}
	fclose(outFile);
	close(sockfd);
}
void UDPclient(char *filn,int size)
{
    int clientSocket, portNum, nBytes;
    char buffer[1024];
    struct sockaddr_in serverAddr;
    socklen_t addr_size;

  /*Create UDP socket*/
    clientSocket = socket(PF_INET, SOCK_DGRAM, 0);

  /*Configure settings in address struct*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT2);
    serverAddr.sin_addr.s_addr = inet_addr(clientname);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  /*Initialize size variable to be used later on*/
    addr_size = sizeof serverAddr;
    int i;
    bzero(output,0);
    FILE *outFile = fopen (filn, "w+");
    if(outFile==NULL)
        error("File is not created");
    sendto(clientSocket,"start",5,0,(struct sockaddr *)&serverAddr,addr_size);
    printf("timers: %d  %d\n",(size+1023)/1024,size);
    for(i=0;i<(size+1023)/1024;i++)
    {
        nBytes = recvfrom(clientSocket,output,1024,0,NULL, NULL);
        //printf("%d %s\n",nBytes,output);
        fwrite(output, sizeof(char), nBytes, outFile);
    }
    fclose(outFile);           
    printf("Received from server:\n");
    close(clientSocket);

}
void *client()
{
	char arr[1024];
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	server = gethostbyname(clientname);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,server->h_length);
	serv_addr.sin_port = htons(PORT1);
	while(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))!=0); 
	getchar();
	while(1)
	{
		int siz;
		char *line = NULL;
		size_t len = 0;
		ssize_t bytes;
		printf(">$ ");
		   
		if((bytes = getline(&line, &len, stdin))==-1)
			error("ERROR on read command");
		//printf("line %d %s\n",bytes,line);
		n = write(sockfd,line,len);
		if (n < 0) error("ERROR writing to socket");
		bzero(output,1024);
		n = read(sockfd,output,1024);
		if(n < 0) error("ERROR reading from socket");
		printf("%s\n",output);
		sscanf(output,"%s",arr);
		
		if(!bcmp("TCP",arr,3))
		{
			sscanf(output,"%s %s %s %s %s",arr,arr,filen,arr,arr);
			siz=atoi(arr);
		//	printf("File: %s %d\n",filen,siz);
			bzero(output,1024);
			TCPclient(filen,siz); // make it threaded
		
		}
        else if(!bcmp("UDP",arr,3))
        {
            sscanf(output,"%s %s %s %s %s",arr,arr,filen,arr,arr);
            siz=atoi(arr);
        //  printf("File: %s %d\n",filen,siz);
            bzero(output,1024);
            UDPclient(filen,siz); // make it threaded
        
        }
		bzero(output,1024);
		while((n=read(sockfd,output,1024)))
		{   
			int flag=0,i;
			for(i=0;i<1022;i++)
				if(output[i]=='!' && output[i+1]=='#' && output[i+2]=='(')
					flag=1;
			printf("%s\n",output);
			if(flag)
				break;
			bzero(output,1024);    
		}

	}
	close(sockfd);
}
//Non-blocking sockets are best use,
//int x;
//x=fcntl(socket ,F_GETFL, 0);
//fcntl(socket, F_SETFL, x | O_NONBLOCK);
// this to set non-blocking socket
// Note normal sockets are not Nonblocking by nature but ,
// they return as soon as there is anything to read  which leads to improper outputs,
// better to have 1st value as size of message than use non-blocking reads till you don't get the disired output then only output.q
int main()
{

	pthread_t grp1,grp2;
	printf("Client's ip: ");
	scanf("%s",clientname);
	//printf("%s\n",clientname);
	pthread_create(&grp1,NULL,&server,NULL);
	pthread_create(&grp1,NULL,&client,NULL);
	pthread_join(grp1,NULL);
	pthread_join(grp2,NULL);
	return 0;
}
