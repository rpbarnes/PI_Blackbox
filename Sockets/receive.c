#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>


// compile with:
//    gcc -pthread -o receive receive.c

#define PORT_NUMBER	2020    // must match between sender and receiver
#define BUFLEN 100

typedef struct
{
  long msg_type;
  char msg_text[1];
}print_msg;

typedef struct
{
  unsigned char buf[BUFLEN];
  unsigned long long count;
}packet_t;

#define packet_size sizeof(packet_t)
time_t rawtime;
struct tm * timeinfo;


void* ReadPacket(void* id)
{
    struct sockaddr_in my_IP_address;             // our address
    struct sockaddr_in remote_address;            // remote address
    socklen_t addrlen = sizeof(remote_address);   // length of addresses
    int receive_length;                           // # bytes received
    int socket_FD;                                // our socket
    int message_count = 0;                        // count # of messages we received
    packet_t the_packet;                          // receive buffer
    int i = 0;

    print_msg the_message;
    key_t key;
    int message_ID;

    key = ftok("progfile", 65);
    message_ID = msgget(key, 0666 | IPC_CREAT);
    the_message.msg_type = 1;

    /* create a UDP socket */
    if ((socket_FD = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
           perror("cannot create socket\n");
           return 0;
    }

    // setup socket parameters
    memset((char *)&my_IP_address, 0, sizeof(my_IP_address));  // set memory location to 0
    my_IP_address.sin_family = AF_INET;
    my_IP_address.sin_addr.s_addr = htonl(INADDR_ANY);
    my_IP_address.sin_port = htons(PORT_NUMBER);

    /* bind the socket to any valid IP address and a specific port */
    if (bind(socket_FD, (struct sockaddr *)&my_IP_address, sizeof(my_IP_address)) < 0)
    {
          perror("bind failed");
          return 0;
    }

    printf("Listening on port %d.....\n", PORT_NUMBER);
    // loop indefintely, receiving and printing what's received
    while(1)
    {

           receive_length = recvfrom(socket_FD, (void*)&the_packet, packet_size, 0, (struct sockaddr *)&remote_address, &addrlen);
           if (receive_length > 0)
           {
                printf("RECEIVED: \t\t\"%s\" (%d bytes)\n", the_packet.buf, receive_length, the_packet.count);

            }
            else
              printf("uh oh - something went wrong!\n");
    }

  return NULL;
}

int main(void)
{
  pthread_t receive_thread;
  int receive_rc;
  long receive_t = 1;
  receive_rc = pthread_create(&receive_thread, NULL, ReadPacket, (void *)receive_t);

  pthread_join(receive_thread, NULL);
  pthread_exit(NULL);

  return 0;
}
