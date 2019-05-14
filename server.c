#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define MAX 1024

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

char* readNext(char* buffer, size_t size)
{
    char* tmp = malloc(size);
    memcpy(tmp, buffer, size);
    buffer = buffer + size;
    return tmp;
}

int readmsg(int sock, char* input, int* offset)
{
    int length = 0;
    int inputlen = 0;
    int offsetSize = 0;
    read(sock, &length, 4);
    if (length > MAX) {
        printf("length<%d> is larger than MAX \n", length);
        return 0;
    }
    char* buffer = malloc(length);
    char* orig_buff = buffer;
    read(sock, &buffer, length);
    char* opcode = readNext(buffer, 1);
    if (*opcode == 0x03){
        free(opcode);
        free(orig_buff);
        return 0;
    }
    if (*opcode == 0x02){
        // read input length
        char* inputlen_raw = readNext(buffer, 4);
        memcpy(&inputlen, inputlen_raw, 4);
        free(inputlen_raw);
        // read input
        input = readNext(buffer, inputlen);
        // read blocked offset length
        char* offsetSize_raw = readNext(buffer, 4);
        memcpy(&offsetSize, offsetSize_raw, 4);
        free(offsetSize_raw);
        // read blocked offset
        offset = (int *)readNext(buffer, offsetSize);
        if (inputlen + offsetSize + 9 != length){
            free(opcode);
            free(orig_buff);
            return 0;
        }
    }
    free(opcode);
    free(orig_buff);
    return 1;
}

int makeReplyMsg(double entropy, char* clnt_buf, int length)
{
    memset(clnt_buf, 0xAA, length);
}

void clean(){
    free(input);
    free(blocked_offset);
    close(new_sock);
    printf("Waiting for another connection\n");
}

int main(int argc, char **argv)
{
	start_socket();
	start_bind();
	start_listen();

    while(1) {
        start_accept();
        int status = readmsg(new_sock, input, blocked_offset);
        if (!status) {
            perror("Client ask me to abort");
        close(new_sock);
        printf("Waiting for another connection\n");
            continue;
        }
        double entropy = 0;
        int clnt_size = 9;
        char clnt_buf[clnt_size];
        makeReplyMsg(entropy, clnt_buf, clnt_size);
        write(new_sock, clnt_buf, clnt_size);
        clean();
    }
    close(orig_sock);
	return 0;
}

