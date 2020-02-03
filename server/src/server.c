#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include "execution_engine.h"
#define MAX 80 
#define PORT 5000 
#define SA struct sockaddr 
  

void make_response(result res, char* message, size_t message_length)
{
    switch (res){
        case Success: sprintf(message, "%d-Command execution successful.", res); return;
        case CommandNotFound: sprintf(message, "%d-Command execution failed. Command not found.", res); return;
        case FailedExecution: sprintf(message, "%d-Command execution failed. An error occured executing command", res); return;
        default: strcpy(message, "Couldn't parse result."); return;
    }
}

void respond_to_commands(int sockfd)
{
    char buff[MAX];
    int read_result;
    result res;
    for (;;) {
        // infinite loop for handling incomming commands
        bzero(buff, MAX);

        // read incomming command
        read_result = read(sockfd, buff, sizeof(buff));

        printf("read result is: %d\n", read_result);

        if (read_result == 0){
            printf("read failed");
            break;
        }


        // print buffer 
        printf("Command To execute : %s\n", buff);

        res = execute_command(buff, sizeof(buff));

        printf("Result is %d ", res);

        bzero(buff, MAX);

        make_response(res, buff, sizeof(buff));

        printf("response is: %s\n", buff);

        write(sockfd, buff, sizeof(buff));
    }
}
  
// Driver function 
int main() 
{ 
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    while (1){

        // socket create and verification 
        sockfd = socket(AF_INET, SOCK_STREAM, 0); 
        if (sockfd == -1) { 
            printf("socket creation failed...\n"); 
            exit(0); 
        } 
        else
            printf("Socket successfully created..\n"); 
        bzero(&servaddr, sizeof(servaddr)); 
    
        // assign IP, PORT 
        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
        servaddr.sin_port = htons(PORT); 
    
        // Binding newly created socket to given IP and verification 
        if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
            printf("socket bind failed...\n"); 
            exit(0); 
        } 
        else
            printf("Socket successfully binded..\n"); 
    
        // Now server is ready to listen and verification 
        if ((listen(sockfd, 5)) != 0) { 
            printf("Listen failed...\n"); 
            exit(0); 
        } 
        else
            printf("Server listening..\n"); 
        len = sizeof(cli); 
    
        // Accept the data packet from client and verification 
        connfd = accept(sockfd, (SA*)&cli, &len); 
        if (connfd < 0) { 
            printf("server acccept failed...\n"); 
            exit(0); 
        } 
        else
            printf("server acccept the client...\n"); 
    
        // Function for chatting between client and server 
        //func(connfd); 
        // Accept commands or queries from client and respond in kind. Only exits when client closes connection.
        respond_to_commands(connfd);

        printf("closing connection");

        // After chatting close the socket 
        close(sockfd); 
    }
} 