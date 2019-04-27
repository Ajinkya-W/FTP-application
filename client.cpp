//essential headers
#include <stdio.h>
#include<iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include<sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include<fcntl.h>
using namespace std;

void nexit()
{//normal exit
    exit(0);
}
int main(int argc, char *argv[])
{	
    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, portno, n;
    //checking if enough input is notentered as command for client
    if (argc < 3) 
    {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       nexit();
    }
    //converting arg2 from string to integer
    portno = atoi(argv[2]);

    //creating an endpoint for communication
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //negative value returned means error as sockets are always positive
    if (sockfd < 0) 
    {
        perror("Error!! While opening socket");            
        nexit();
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) 
    {
        fprintf(stderr,"ERROR, no such host\n");
        nexit();
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    //setting address family as ipv4
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);
    //initiating a connection on a socket
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    {
        perror("Error!! While connecting");
        nexit();
    }
    //display screen
    cout<<"\e[1;1H\e[2J";
    cout<<"-----------------------------Welcome to Client Window-----------------------------";
    cout<<endl;
    cout<<"Enter either of the following commands:"<<endl;
    cout<<"1.ls 2.cd 3.chmod 4.lls 5.lcd 6.lchmod 7.put 8.get 9.close"<<endl;
    
    //creating buffer for saving entered input 
    char buffer[8192];
    bzero(buffer,8192);
    //until client runs endlessly untilclose command is entered
    while(1)
    {
        cout<<endl<<">> ";
        fgets(buffer,255,stdin);
        int i=0,argc=0,k=0;
        char** parameter = new char * [20];

        char var[256];
        int a;
        while(i<strlen(buffer))
        {//breaking input string into different parameters
            a=(int)buffer[i];
            //checking ascii for space and newline
            if(a==32 || a==10)
            {
                parameter[argc] = new char [20];
                strcpy(parameter[argc], var);
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
        //put command for uploading file to server
        if(!strncmp("put",parameter[0],3))
        {
            int fd1,n1;
            write(sockfd,buffer,strlen(buffer));
            //opening a file from clientwhich we wish to send
            fd1=open(parameter[1],O_RDONLY);
            if(fd1 == -1)
                perror("error with the input file");
           
            while(1)
            {
                bzero(buffer,8192);
                //reading from file descriptor to buffer
                n=read(fd1, buffer, 8192);
                //writing from buffer to socket for server
                if(write(sockfd, buffer, n)!=n)
                {   
                    perror("problem in writing to buffer");
                    break;
                }
                if(n==-1)
                {
                    perror("problem in reading from file");
                    break;
                }
                else if(n<8192)
                {
                    cout<<"put executed successfully";
                    break;
                }
            }
            close(fd1);
        }
        //get command for downloading file from server
        else if(!strncmp("get",parameter[0],3))
        {
            int fd2,bytescount,n1;
            //sending complete input command strings to server
            write(sockfd,buffer,strlen(buffer));
            //creataing new file
            fd2=open(parameter[1], O_CREAT | O_WRONLY | O_EXCL , 0700);
            //cout<<"after fd2"<<endl;
            while(1)                
            {   
                bzero(buffer,8192);
                bytescount = read(sockfd, buffer, sizeof(buffer));
                n1=write(fd2, buffer,bytescount);
                if(bytescount==-1)
                    perror("problem in reading from server socket");
                else if(bytescount<8192)
                {
                    cout<<"File received successfully";
                    break;
                }
            }
        }
        //listing of files on client
        else if(!strcmp("lls" , parameter[0]))
        {
            pid_t pid =fork();
            if (pid < 0)
            {
                perror("Error!! fork failed");
                nexit();
            }
            else if(pid==0)
            {
                execlp("/bin/ls","ls",NULL);   
            }
            else 
                wait(NULL);
        }
        //changing directory of client
        else if(!strncmp("lcd" , parameter[0], 3))
        {
            char* path = new char[1000];
            int k = chdir(parameter[1]);
        
            if (k!=0)
                cout<<"Error!! while changing the directory"<<endl;
            else
            {
                path = getcwd(path,1000);
                cout<<"Current working dir: "<<path<<"\n";
            }
 
        }
        //changing access of files of client
        else if(!strncmp("lchmod" , parameter[0], 6))
        {
            if(fork()==0)
            {

                int e=0;
                for(i=1;i<argc;i++)
                {
                    argv[i]=parameter[i];
                }

                argv[i]=NULL;
                argv[0]=(char*)"/bin/chmod";

                e=execvp(argv[0],argv);
                if(e==-1)
                {   
                    cout<<"Error!! while changing mod"<<endl;
                    nexit();
                }
            }
            else wait(NULL);
        }
        //closing the client socket
        else if(!strncmp("close",parameter[0],5))
            break;
        //server side commands sent from here
        else
        {
            n = write(sockfd,buffer,strlen(buffer));
            //cout<<n<<endl;
            if (n < 0) 
            {
                perror("Error!! While writing to socket");nexit();
            }
            bzero(buffer,8192);
            n = read(sockfd,buffer,8191);
            if (n < 0) 
            {
                perror("Error!! While reading from socket");nexit();
            }
            printf("%s\n",buffer);
            bzero(buffer,8192);
        }
    }
    close(sockfd);
    
    return 0;
}