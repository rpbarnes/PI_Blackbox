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
#include <ctype.h>

// compile with:
//    gcc -pthread -o send_console send_console.c

// note in the code below the IP address 192.168.250.108 is the actual address of the receiver of the message (PI IP address)
// the PI must be set to this address to receive correctly

#define PORT_NUMBER	2020    // arbitrary software port, must be the same for sender and receiver
#define MESSAGE_LENGTH 100

typedef struct
{
  long msg_type;
  char msg_text[1];
}print_msg;

typedef struct
{
  unsigned char buf[MESSAGE_LENGTH];
  unsigned long long count;
}packet_t;

#define packet_size sizeof(packet_t)

char *server = "192.168.250.108";  /* change this to use a different server */
struct tm * timeinfo;

packet_t the_packet;          // object containing string message and packet count
char console_output[200];
char  packet_count_string[30];
struct sockaddr_in my_IP_address, remote_address;
int socket_FD;
int address_string_length = sizeof(remote_address);
char send_buffer[MESSAGE_LENGTH];

//function declarations
void Setup_Socket();
void Trim_WhiteSpace(char * str);
void* SendPacket(void* id);

void Setup_Socket()
{
    // create socket
    if ((socket_FD=socket(AF_INET, SOCK_DGRAM, 0))==-1)    	// AF_INET == Ip address family. SOCK_DGRAM == socket datagram service
          printf("socket created\n");


    memset((char *)&my_IP_address, 0, sizeof(my_IP_address));   // clear memory location to 0
    my_IP_address.sin_family = AF_INET;
    my_IP_address.sin_addr.s_addr = htonl(INADDR_ANY);
    my_IP_address.sin_port = htons(0);

    // bind to all local addresses and a specific port number, "PORT_NUMBER"
    if (bind(socket_FD, (struct sockaddr *)&my_IP_address, sizeof(my_IP_address)) < 0)
    {
         perror("bind failed");
         return;
    }

    // assign remote_address, the destination address
    // inet_aton converts numeric ip address to binary representation
    memset((char *) &remote_address, 0, sizeof(remote_address));  // clear memory location to 0
    remote_address.sin_family = AF_INET;
    remote_address.sin_port = htons(PORT_NUMBER);
    if (inet_aton(server, &remote_address.sin_addr)==0) {
          fprintf(stderr, "inet_aton() failed\n");
          exit(1);
    }

    the_packet.count = 0;   // initialize packet count to 0
}

void* SendPacket(void* id) // void* is a generic pointer, not associated with any particular type. Must be typecast to be dereferenced
{
        int receive_length;          // # bytes in acknowledgement message
        size_t characters;
      	size_t bufsize = 128;
        char* keyboard_input_buffer;

      	keyboard_input_buffer = (char *)malloc(bufsize * sizeof(char));		// allocate memory for the buffer to store keyboard input

      	if( keyboard_input_buffer == NULL)
       	{
      		 perror("Unable to allocate buffer");
      		 exit(1);
       	}

        int packet_index = 0;
        printf("Sending packets to %s port# %d\n", server, PORT_NUMBER);
        printf("Text entered here sent over socket\n");
        while(1)
        {
        		printf("\n>>  ");
            characters = getline(&keyboard_input_buffer,&bufsize,stdin);
        		keyboard_input_buffer[characters-1] = '\0';	// null terminate the string
        		characters -= 1; // subtracting one because last character read is newline
        		sprintf(send_buffer, "%s, [packet# %d]", keyboard_input_buffer, packet_index);  // copy text read from keyboard to send buffer
            send_buffer[strlen(send_buffer)] = '\0';

            /* compose the packet message */
            for(int i = 0; i < MESSAGE_LENGTH; i++)
            {
                if(i<strlen(send_buffer))
                  the_packet.buf[i] = send_buffer[i];    // copying datetime response to packet buffer
                else
                  the_packet.buf[i] = '\0';
            }

            /* print the packet message */
            printf("SEND:\t\t");
            for(int z = 0; z < MESSAGE_LENGTH; z++)
            {
                printf("%c", the_packet.buf[z]);
            }
            printf("\n");	// print a newline after each packet


            if(sendto(socket_FD, (void*)&the_packet, packet_size, 0, (struct sockaddr *)&remote_address, address_string_length)==-1)
            {
                perror("sendto");
                exit(1);
            }
        		++packet_index;
            ++the_packet.count;
        }
        close(socket_FD);
  return NULL;
}

void Trim_WhiteSpace(char * str)    // removes trailing whitespace (spaces and newlines)
{

  int stringLen = strlen(str);
  while(stringLen > 0 && isspace (str[stringLen -1])) // step backwards through string subtractin whitespace characters
  {
    stringLen--;
  }
  str[stringLen] = '\0';
}

int main(void)
{
  pthread_t send_thread;
  int send_rc;
  long send_t = 0;

  Setup_Socket();
  send_rc = pthread_create(&send_thread, NULL, SendPacket, (void *)send_t);


  pthread_exit(NULL);

  return 0;
}
