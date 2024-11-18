#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define BUFSZ 1024

struct action
{
    int type;
    int moves[100];
    int board[10][10];
}typedef action;


 char * serialize_int( char *buffer, int value)
{
  /* Write big-endian int value into buffer; assumes 32-bit int and 8-bit char. */
  buffer[0] = value >> 24;
  buffer[1] = value >> 16;
  buffer[2] = value >> 8;
  buffer[3] = value;
  return buffer + 4;
}

 char * serialize_vetor( char *buffer, int *valor){
    int t = 0;
    for (int i = 0; i < 100; i++)
    {
        buffer[t] = valor[i] >> 24;
        buffer[t+1] = valor[i] >> 16;
        buffer[t+2] = valor[i] >> 8;
        buffer[t+3] = valor[i];

        t = t + 4;

    }
    return buffer + t;
    
}

 char * serialize_matriz( char *buffer, int value[][10]) {
    int t = 0; // índice no buffer
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            // Serializa o inteiro value[i][j] em formato big-endian
            buffer[t] = value[i][j] >> 24;       // byte mais significativo
            buffer[t+1] = value[i][j] >> 16;     // próximo byte
            buffer[t+2] = value[i][j] >> 8;      // segundo byte menos significativo
            buffer[t+3] = value[i][j];           // byte menos significativo
            t += 4; // Incrementa o índice do buffer para o próximo inteiro
        }
    }
    return buffer + t; // Retorna o buffer atualizado
}


 char * serialize_temp( char *buffer, struct action *value)
{
  buffer = serialize_int(buffer, value->type);
  buffer = serialize_vetor(buffer, value->moves);
  buffer = serialize_matriz(buffer, value->board);
  return buffer;
}

 char * deserialize_int( char *buffer, int *value) {
    *value = (buffer[0] << 24) |  // Byte mais significativo
             (buffer[1] << 16) |
             (buffer[2] << 8)  |
             (buffer[3]);      // Byte menos significativo
    return buffer + 4; // Retorna o buffer atualizado
}

 char *deserialize_action( char *buffer, struct action *data) {
    // Desserializa o campo 'type' (1 inteiro)
    buffer = deserialize_int(buffer, &data->type);

    // Desserializa o array 'move' (100 inteiros)
    for (int i = 0; i < 100; i++) {
        buffer = deserialize_int(buffer, &data->moves[i]);
    }

    // Desserializa a matriz 'board' (10x10 inteiros)
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            buffer = deserialize_int(buffer, &data->board[i][j]);
        }
    }

    return buffer; // Retorna o ponteiro atualizado
}


void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void Envia_Comando(char *buf, int s, struct action *data_to_server , struct action *data){
	    memset(buf, 0, BUFSZ);
        printf("mensagem> ");
        fgets(buf, BUFSZ - 1, stdin);


        if(0 == strcmp(buf,  "start\n")){
            data_to_server->type = 0; // sinaliza tipo start
            memset(data_to_server->moves, 0 , sizeof(data_to_server->moves));
            memset(data_to_server->board, 0, sizeof(data_to_server->board));
        }
        else if (0 == strcmp(buf,  "move\n"))
        {
            data_to_server->type = 1; // sinaliza tipo move  
            memset(data_to_server->moves, 0 , sizeof(data_to_server->moves));
            memset(data_to_server->board, 0, sizeof(data_to_server->board));         
        }
        else if (0 == strcmp(buf,  "map\n"))
        {
            data_to_server->type = 2; // sinaliza tipo map
            memset(data_to_server->moves, 0 , sizeof(data_to_server->moves));
            memset(data_to_server->board, 0, sizeof(data_to_server->board));            
        }
        else if (0 == strcmp(buf,  "hint\n"))
        {
            data_to_server->type = 3; // sinaliza tipo hint
            memset(data_to_server->moves, 0 , sizeof(data_to_server->moves));
            memset(data_to_server->board, 0, sizeof(data_to_server->board));
        }        
        else if (0 == strcmp(buf,  "reset\n"))
        {
            data_to_server->type = 4; // sinaliza tipo reset
            memset(data_to_server->moves, 0 , sizeof(data_to_server->moves));
            memset(data_to_server->board, 0, sizeof(data_to_server->board));
 
        }
        else if (0 == strcmp(buf,  "exit\n"))
        {
            data_to_server->type = 5; // sinaliza tipo exit
            memset(data_to_server->moves, 0 , sizeof(data_to_server->moves));
            memset(data_to_server->board, 0, sizeof(data_to_server->board));
 
        }
        else{
            data_to_server->type = 9;  // 9 indica que foi inserido uma ação invalida
            memset(data_to_server->moves, 0 , sizeof(data_to_server->moves));
            memset(data_to_server->board, 0, sizeof(data_to_server->board));
            printf("Ação invalida");
        }
        serialize_temp(buf, data_to_server);

        size_t count = send(s, buf, strlen(buf) + 1, 0);
        if (count != strlen(buf) + 1) {
            logexit("send");
        }

	    memset(buf, 0, BUFSZ);
        
        count = recv(s, buf, BUFSZ, 0);
        deserialize_action(buf, data);
        printf("received: %d bytes\n", (int)count);
        puts(buf);

}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) {
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    printf("connected to %s\n", addrstr);

     char buf[BUFSZ];
    

    
    action data_to_server;
    action data;

while(1){
		Envia_Comando(buf,s, &data_to_server, &data);
}


    close(s);
    exit(EXIT_SUCCESS);
}
