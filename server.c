#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define MAX 2048

socklen_t clnt_len;
int orig_sock, new_sock;
int* blocked_offset;
char* input;
static struct sockaddr_in clnt_adr, serv_adr;
const int PORT = 7777;

void start_socket()
{
	if ((orig_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("generate error");
	}
}

void start_bind()
{
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(PORT);

	if (bind(orig_sock, (struct sockaddr *)&serv_adr,
		sizeof(serv_adr)) < 0) {
		perror("bind error");
		close(orig_sock);
	}
}

void start_listen()
{
	listen(orig_sock, 5);
}

void start_accept()
{
	clnt_len = sizeof(clnt_adr);
	if ((new_sock = accept(orig_sock, (struct sockaddr *)&clnt_adr, &clnt_len)) < 0) {
		perror("accept error");
		close(orig_sock);
	}
}

void readNext(void* buffer, size_t bufferlen, char** data, size_t datalen)
{
    if(datalen > bufferlen){
        return;
    }
    memcpy(buffer, *data, datalen);
    *data = *data + datalen;
}

void printBuffer(void * buffer, size_t len)
{
    for(int i=0; i<len; i++){
        printf("%x", *(char *)(buffer+i));
    }
    printf("\n");
}

int readmsg(int sock, char* input, int* inputlen, int* offset, int* offsetSize)
{
    int length = 0;
    char opcode;
    if(!read(sock, &length, 4)){
        perror("connection reset by the client");
        return -1;
    }
    if (length > MAX || length <= 9) {
        printf("length<0x%x> is invalid \n", length);
        return 0;
    }
    char data[length];
    char* buffer = data;
    if(!read(sock, buffer, length)){
        perror("connection reset by the client");
        return -1;
    }
    // for debug
    // printBuffer(buffer, length);
    readNext(&opcode, 1, &buffer, 1);
    if (opcode == 0x02){
        // read input length
        readNext(inputlen, 4, &buffer, 4);
        // read input
        if (*inputlen + 9 >= length) {
            printf("length<0x%x> and inputlen<0x%x> are invalid \n", length, *inputlen);
            return 0;
        }
        readNext(input, MAX, &buffer, *inputlen);
        // read blocked offset length
        readNext(offsetSize, 4, &buffer, 4);
        if (*inputlen + *offsetSize * sizeof(int) + 9 != length){
            printf("length<0x%x>, inputlen<0x%x> and offsetSize<0x%x> are invalid \n", length, *inputlen, *offsetSize);
            return 0;
        }
        // read blocked offset
        readNext(&offset, MAX * sizeof(int), &buffer, *offsetSize);
        return 1;
    }
    printf("opcode<0x%x> is invalid \n", opcode);
    return 0;
}

void appendBuffer(void* dst, void* src, int size, int* index)
{
    int current = *index;
    memcpy(dst + current, src, size);
    *index = current + size;
}

int makeReplyMsg(double entropy, char* clnt_buf)
{
    char opcode = 0x01;
    int length = sizeof(opcode) + sizeof(double);
    int index = 0;
    appendBuffer(clnt_buf, &length, sizeof(length), &index);
    appendBuffer(clnt_buf, &opcode, sizeof(opcode), &index);
    appendBuffer(clnt_buf, &entropy, sizeof(double), &index);
    return length + sizeof(int);
}

void clean(){
    free(input);
    free(blocked_offset);
    printf("Waiting for another request\n");
}

int main(int argc, char **argv)
{
	start_socket();
	start_bind();
	start_listen();
    start_accept();
    while(1) {
        input = malloc(MAX);
        blocked_offset = malloc(MAX * sizeof(int));
        int inputlen = 0;
        int offsetSize = 0;
        int status = readmsg(new_sock, input, &inputlen, blocked_offset, &offsetSize);
        if (!status) {
            perror("Client ask me to abort");
            clean();
            if(status == -1) break;
            continue;
        }
        double entropy = 0;
        char clnt_buf[MAX];
        int len = makeReplyMsg(entropy, clnt_buf);
        write(new_sock, clnt_buf, len);
        clean();
    }
    close(new_sock);
    close(orig_sock);
	return 0;
}

