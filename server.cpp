#include<stdlib.h>
#include<iostream>
#include<string.h>
#include<unistd.h>
#include<sys/types.h> 
#include<sys/wait.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<fstream>
#include<cstdio>
#include<memory>
#include<fcntl.h>
using namespace std;

//execute function 
string execute(const char* cmd) {
    string result;
    char buffer[256];
    //using unique ptr to directly store the command output of popen in defined buffer
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    for (;fgets(buffer, 256, pipe.get()) != nullptr;) 
        result =result+ buffer;
    return result;
}
void errorexit()
{
	exit(1);
}
void nexit()
{
	exit(0);
}
int main(int argc, char *argv[])
{
	cout<<"\e[1;1H\e[2J";
	cout<<"*------------------Server Started-------------------* ";
	cout<<endl;	
     int sockfd, newsockfd, portno;
     socklen_t cliAddlen;
     
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     //checking for no. of arguments
     if (argc < 2) 
     {
        fprintf(stderr,"Error!! Insufficient arguments entered.\n");
        errorexit();
     }
     //creating a endpoint for communication
     sockfd = socket(AF_INET, SOCK_STREAM, 0);

     if (sockfd < 0) 
     { 
     	perror("Error!!  While opening socket"); 
     	nexit();
     }
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     //setting address family as ipv4
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     int sizeofservadd=0;

     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              {perror("Error!! failure in binding");nexit();}
     char buffer[8192];
     listen(sockfd,5);
     cliAddlen = sizeof(cli_addr);
     pid_t pid;

     //server will be  in infinite loop.
     while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cliAddlen);
        if (newsockfd < 0)
            {
            	perror("Error!! accept request failed");
            	nexit();
            }
        //forking to run every client in its own child process so that
        //client will be independent to eachother    
        pid = fork();
        
        if (pid < 0)
            {
            	perror("Error!! Fork failed");
            	nexit();
            }
        if (pid == 0)
        {
            close(sockfd);
            //infinite loop for handling every command for each client
            while(1)
            {
	            n = read(newsockfd,buffer,255);
	            if (n < 0) {perror("Error!! While reading from socket");nexit();}

	            // parsing buffer in the following codes and saving it to parameter array
	            int i=0,argc=0,k=0;
		        char** parameter = new char * [20];

		        char var[256];
		        int a;
		        while(i<strlen(buffer))
		        {
		            a=(int)buffer[i];
		            if(a==32 || a==10)
		            {
		                //cout<<var<<endl;
		                parameter[argc] = new char [20];
		                strcpy(parameter[argc], var);
		                //cout<<argc<<parameter[argc]<<endl;
		                bzero(var,256);
		                k=0;
		                argc++;
		            }
		            else 
		            {
		                var[k]=buffer[i];
		                k++;
		            }
		            i++;
		        }
		        //saving a copy of buffer
		        char* orig= new char[1000];
		        strcpy(orig,buffer);
		        //null terminating
		        orig[strlen(orig)-1] = '\0';
		        //to redirect stdout and stderr to same buffer
		        orig=strcat(orig,(char*)" 2>&1");
				//emptying buffer each time before next use
		        bzero(buffer,8192);

		        //ls: listing files or folders of the current directory in server side.
		        //chmod: changing access mode for files in server side
		         if(!strcmp("ls" , parameter[0]) || !strcmp("chmod" , parameter[0]) || !strcmp("pwd" , parameter[0])){
		         	string s = execute(orig);
		         	//if string is not empty that means either error or returned output will be printed
		         	if(s!="")
		         	{
		         		strcpy(buffer,s.c_str());
			         	n = write(newsockfd,buffer,strlen(buffer));
			            if (n < 0) {perror("Error!! While writing to socket");nexit();}
		         	}
		         	// if no returning output that means command executed
		         	else 
		         	{
		         		n = write(newsockfd,"command successfully executed\n",50);
			            if (n < 0) 
			            {
			           		perror("Error!! While writing to socket");
			           		nexit();
			           	}
		         	}
		            bzero(buffer,8192);
		         }
		         //cd:changing directory in server side
		         else if( !strcmp("cd" , parameter[0]))
		         {
		         	int k = chdir(parameter[1]);
					//k==-1, error 
		            if (k!=0)
		            {
		                n = write(newsockfd,"Error! while changing the directory",50);
						if (n < 0) 
						{
							perror("Error!! While writing to socket");
						 	nexit();}
						}
		            else
		            {
		                getcwd(buffer,8192);
		              
		                n = write(newsockfd,"command successfully executed",200);

		                if (n < 0)
		                { 
		                	perror("Error!! While writing to socket");nexit();
		                }
		               	bzero(buffer,8192);
		            }
		         }
		         //putting files from client to server
		         else if(!strcmp("put",parameter[0]))
		         {
		         	int fd2,bytescount,n1;
		         	//creating a file with same name of client file
		         	fd2=open(parameter[1], O_CREAT | O_WRONLY | O_EXCL , 0700);
					cout<<"";
					while(1)
					{
					bzero(buffer,8192);
					bytescount = read(newsockfd, buffer, sizeof(buffer));
					n1=write(fd2, buffer,bytescount);
					if(bytescount==-1)
						perror("??problem in reading");
					else if(bytescount<8192)
						break;
					}
		         }
		         //getting a file from server to client
		         else if(!strcmp("get",parameter[0]))
		         {
			         	int fd3,n1;
	            write(newsockfd,buffer,strlen(buffer));
	            //opening the file
	            fd3=open(parameter[1],O_RDONLY);
	            if(fd3 == -1)
	                perror("??error with the input file descriptor");
	           
	            while(1)
	            {
	                bzero(buffer,8192);
	                n=read(fd3, buffer, 8192);
	                if(write(newsockfd, buffer, n)!=n)
	                {   
	                    perror("??problem in writing");
	                    break;
	                }
	                if(n==-1)
	                {
	                    perror("??problem in reading");
	                    break;
	                }
	                else if(n<8192)
	                    break;
            }
            close(fd3);
        
		         }
	             else{
	             	n = write(newsockfd,"Command is not listed",23);
	            	if (n < 0){ perror("Error!! While writing to socket");nexit();} 
	             }
	             bzero(buffer,8192);
        	}
        }
        else
            close(newsockfd);
    } 
return 0; 
}
