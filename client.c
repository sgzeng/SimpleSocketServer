#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define PORT 7777
#define MAX_LINE 2048
const char* IP = "127.0.0.1";

void appendBuffer(void* dst, void* src, int size, int* index)
{
    int current = *index;
    memcpy(dst + current, src, size);
    *index = current + size;
}

int makeRequestMsg(void* clnt_buf)
{
    char opcode = 0x02;
    int inputlen = 5;
    char input[] = {0x0, 0x1, 0x2, 0x3, 0x4};
    int offsetSize = 3;
    int blocked_offset[] = {0, 1, 2};
    // length = 1 + 4 + 5 + 4 + 3*4 = 
    int length = sizeof(opcode) + sizeof(inputlen) + sizeof(input) + sizeof(offsetSize) + sizeof(blocked_offset);
    int index = 0;
    appendBuffer(clnt_buf, &length, sizeof(length), &index);
    appendBuffer(clnt_buf, &opcode, sizeof(opcode), &index);
    appendBuffer(clnt_buf, &inputlen, sizeof(inputlen), &index);
    appendBuffer(clnt_buf, input, sizeof(input), &index);
    appendBuffer(clnt_buf, &offsetSize, sizeof(offsetSize), &index);
    appendBuffer(clnt_buf, blocked_offset, sizeof(blocked_offset), &index);
    return length;
}

void printBuffer(void * buffer, size_t len)
{
    for(int i=0; i<len; i++){
        printf("%x", *(char *)(buffer+i));
    }
    printf("\n");
}

/* msg handling */
void str_cli(int sockfd)
{
	/*send or recieve*/
	char sendline[MAX_LINE] , recvline[MAX_LINE], command[5];
	while(fgets(command , 5 , stdin) != NULL)	
	{
        if(strncmp(command, "send", 4)){
            bzero(command , 5);
            continue;
        }
        int len = makeRequestMsg(sendline);
		write(sockfd , sendline , len + sizeof(int));
        read(sockfd, recvline, 13);
        printBuffer(recvline, 13);
		bzero(sendline , MAX_LINE);
        bzero(recvline , MAX_LINE);
        bzero(command , 5);
	}
}

int main(int argc , char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;

    if((sockfd = socket(AF_INET , SOCK_STREAM , 0)) == -1)
    {
        perror("socket error");
        exit(1);
    }

    bzero(&servaddr , sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(IP);

    if(connect(sockfd , (struct sockaddr *)&servaddr , sizeof(servaddr)) < 0)
    {
        perror("connect error");
        exit(1);
    }

	str_cli(sockfd);	
	exit(0);
}
