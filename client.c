#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define BUFSZ 1024

struct action {
    int type;
    int moves[100];
    int board[10][10];
} typedef action;

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void clear_data(struct action *d) {
    if (d == NULL) {
        return; // Evita acessar ponteiro nulo.
    }
    memset(d, 0, sizeof(struct action));
}

int send_full(int sock_fd, const void *buffer, size_t length) {
    size_t total_sent = 0;
    const char *buf_ptr = (const char *)buffer;

    while (total_sent < length) {
        ssize_t bytes_sent = send(sock_fd, buf_ptr + total_sent, length - total_sent, 0);
        if (bytes_sent < 0) {
            if (errno == EINTR) {
                continue;  // Interrupção por sinal, tentar novamente.
            }
            perror("send");
            return -1;  // Erro irreparável.
        }
        total_sent += bytes_sent;
    }

    return 0;  // Sucesso.
}

int recv_full(int sock_fd, void *buffer, size_t length) {
    size_t total_received = 0;
    char *buf_ptr = (char *)buffer;

    while (total_received < length) {
        ssize_t bytes_received = recv(sock_fd, buf_ptr + total_received, length - total_received, 0);
        if (bytes_received < 0) {
            if (errno == EINTR) {
                continue;  // Interrupção por sinal, tentar novamente.
            }
            perror("recv");
            return -1;  // Erro irreparável.
        }
        if (bytes_received == 0) {
            // O cliente fechou a conexão.
            fprintf(stderr, "Client disconnected before all data was received.\n");
            return -1;
        }
        total_received += bytes_received;
    }

    return 0;  // Sucesso.
}

int Obtem_Valor(int linha, int coluna, int labirinto[][10]) { // Pega o elemento correspondente a uma posicao especifica da matriz
    return labirinto[linha][coluna];
}

void Anda_Jogador(int *pos_jog, int labirinto[][10], int type_mov, int dim, int *win) {

    if (type_mov == 1 && pos_jog[0] > 0 ) { // cima
        if(labirinto[pos_jog[0] - 1][pos_jog[1]] == 3){
            *win = 1; // comunica que o jogador Ganhou
        }
        labirinto[pos_jog[0] - 1][pos_jog[1]] = 5; // Atribui a nova posicao do jogador
        labirinto[pos_jog[0]][pos_jog[1]] = 1; // posição antiga do jogador recebe 1 (caminho livre)
        pos_jog[0] -= 1;
    } else if (type_mov == 2 && pos_jog[1] < dim-1) { // direita
        if(labirinto[pos_jog[0]][pos_jog[1] + 1] == 3){
            *win = 1;
        }
        labirinto[pos_jog[0]][pos_jog[1] + 1] = 5;
        labirinto[pos_jog[0]][pos_jog[1]] = 1; 
        pos_jog[1] += 1;
    } else if (type_mov == 3  && pos_jog[0] < dim-1) { // baixo
        if(labirinto[pos_jog[0] + 1][pos_jog[1]] == 3){
            *win = 1;
        }
        labirinto[pos_jog[0] + 1][pos_jog[1]] = 5;
        labirinto[pos_jog[0]][pos_jog[1]] = 1; 
        pos_jog[0] += 1;
    } else if (type_mov == 4 && pos_jog[1] > 0 ) { // esquerda
        if(labirinto[pos_jog[0]][pos_jog[1] - 1] == 3){
            *win = 1;
        }
        labirinto[pos_jog[0]][pos_jog[1] - 1] = 5;
        labirinto[pos_jog[0]][pos_jog[1]] = 1; 
        pos_jog[1] -= 1;
    } else {
        printf("Limite de borda atingido\n"); // 
    }


}

void Mov_possiveis(int *pos_ent, int labirinto[][10], int *opcoes) {
    // Identifica se é possível ir para cima
    opcoes[0] = (pos_ent[0] > 0 && Obtem_Valor(pos_ent[0] - 1, pos_ent[1], labirinto) == 1) ? 1 : 0;
    // Direita
    opcoes[1] = (pos_ent[1] < 9 && Obtem_Valor(pos_ent[0], pos_ent[1] + 1, labirinto) == 1) ? 2 : 0;
    // Baixo
    opcoes[2] = (pos_ent[0] < 9 && Obtem_Valor(pos_ent[0] + 1, pos_ent[1], labirinto) == 1) ? 3 : 0;
    // Esquerda
    opcoes[3] = (pos_ent[1] > 0 && Obtem_Valor(pos_ent[0], pos_ent[1] - 1, labirinto) == 1) ? 4 : 0;
}

void Criptografa_Labirinto(int labirinto[][10], int labirinto_criptografado[][10]){
    for(int i = 0; i < 10; i++){
        for(int j = 0; j < 10 ; j++){
            switch (labirinto[i][j])
            {
            case 0:
                labirinto_criptografado[i][j] = 35; // 35 == #   muro 
                break;
            case 1:
                labirinto_criptografado[i][j] = 95; // 95 == _   caminho livre
                break;  
            case 2:
                labirinto_criptografado[i][j] = 62; // 62 == >   entrada
                break;
            case 3:
                labirinto_criptografado[i][j] = 88; // 88 == X   saida
                break;
            case 4:
                labirinto_criptografado[i][j] = 63; // 63 == ?   nao descoberto
                break;
            case 5:
                labirinto_criptografado[i][j] = 43; // 43 == +   jogador
                break;
            default:
                labirinto_criptografado[i][j] = 240; // 240 == " " vazio
                break;
            }
        }
    }
}

void Imprimi_Labirinto(int labirinto[][10]) {
    for (int i = 0; i < 10; i++) {
        printf("\n");
        for (int j = 0; j < 10; j++) {
            if(labirinto[i][j] != 240) // 240 sinaliza locais além da dimensao do labirinto
            printf("%c ", labirinto[i][j]);
        }
    }
    printf("\n");
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) {
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    // printf("connected to %s\n", addrstr);

    char buf[BUFSZ];
    action data_to_server;
    action data;

    int win = 0;
    int start = 0;
    int labirinto_encriptado[10][10];

while (1) {

    data_to_server.type = 0;
    data.type = 0;
    memset(data_to_server.moves, 0, sizeof(data_to_server.moves));
    memset(data_to_server.board, 0, sizeof(data_to_server.board));
    memset(data.moves, 0, sizeof(data.moves));
    memset(data.board, 0, sizeof(data.board));


    memset(buf, 0, BUFSZ);
    printf("> ");
    fgets(buf, BUFSZ - 1, stdin);

    /////////////// START /////////////////
    if (0 == strcmp(buf, "start\n") ) {

        start = 1;
        data_to_server.type = 0; // sinaliza tipo start
        start = 1;

        if (send_full(s, &data_to_server, sizeof(data_to_server)) != 0) {
            fprintf(stderr, "Failed to send data to server.\n");
        }    
        int received = recv_full(s, &data, sizeof(data));
        if (received != 0) {
            logexit("erro ao receber");
        }

        printf("Possible Moves: %s %s %s %s.\n\n", 
               (data.moves[0] == 1) ? "up," : "", 
               (data.moves[1] == 2) ? "right," : "", 
               (data.moves[2] == 3) ? "down," : "", 
               (data.moves[3] == 4) ? "left," : "");
        

    } 
///////////////////  MOVE ////////////////////////
    else if(0 == strcmp(buf, "move\n")){

        if (start == 0){
            printf("error: start the game first\n\n");
            continue;
        }

        data_to_server.type = 1; // sinaliza tipo move
        data_to_server.moves[0] = 0;

        if (send_full(s, &data_to_server, sizeof(data_to_server)) != 0) {
            fprintf(stderr, "Failed to send data to server.\n");
        }    
        int received = recv_full(s, &data, sizeof(data));
        if (received != 0) {
            logexit("erro ao receber");
        }

        printf("Possible Moves: %s %s %s %s.\n\n", 
            (data.moves[0] == 1) ? "up," : "", 
            (data.moves[1] == 2) ? "right," : "", 
            (data.moves[2] == 3) ? "down," : "", 
            (data.moves[3] == 4) ? "left," : "");
    }

    /////////////// UP /////////////////
    else if (0 == strcmp(buf, "up\n") ) {
        if (start == 0){
            printf("error: start the game first\n\n");
            continue;
        }
            data_to_server.type = 1; // sinaliza tipo move
            data_to_server.moves[0] = 1; // envia comando de up = 1

        if (send_full(s, &data_to_server, sizeof(data_to_server)) != 0) {
            fprintf(stderr, "Failed to send data to server.\n");
            close(s);
            exit(EXIT_FAILURE);
        } 
        int received = recv_full(s, &data, sizeof(data));
        if (received != 0) {
            logexit("erro ao receber");
        }

        if(data.type == -1){
            printf("error: you cannot go this way\n\n");
        }
        else if (data.type == 5) { // saiu do labirinto
            win = 1;
            start = 0;
            printf("\nYou escaped\n");
                Criptografa_Labirinto(data.board, labirinto_encriptado);
                Imprimi_Labirinto(labirinto_encriptado);

            continue;

        } else {

       printf("Possible Moves: %s %s %s %s.\n\n", 
                (data.moves[0] == 1) ? "up," : "", 
                (data.moves[1] == 2) ? "right," : "", 
                (data.moves[2] == 3) ? "down," : "", 
                (data.moves[3] == 4) ? "left," : "");
    }


    }
    /////////////// RIGHT /////////////////
    else if (0 == strcmp(buf, "right\n") ) {
        if (start == 0){
            printf("error: start the game first\n\n");
            continue;
        }

            data_to_server.type = 1; // sinaliza tipo move
            data_to_server.moves[0] = 2; // envia comando de right == 2

        if (send_full(s, &data_to_server, sizeof(data_to_server)) != 0) {
            fprintf(stderr, "Failed to send data to server.\n");
            close(s);
            exit(EXIT_FAILURE);
        }    
            int received = recv_full(s, &data, sizeof(data));
            if (received != 0) {
                logexit("erro ao receber");
            }

            if(data.type == -1){
                printf("error: you cannot go this way\n\n");
            }
            else if (data.type == 5) {
            win = 1;
            start = 0;
            printf("\nYou escaped\n");
                Criptografa_Labirinto(data.board, labirinto_encriptado);
                Imprimi_Labirinto(labirinto_encriptado);

            continue;
            
            } else {
                printf("Possible Moves: %s %s %s %s.\n\n", 
                       (data.moves[0] == 1) ? "up," : "", 
                       (data.moves[1] == 2) ? "right," : "", 
                       (data.moves[2] == 3) ? "down," : "", 
                       (data.moves[3] == 4) ? "left," : "");
            }


    }
    /////////////// DOWN /////////////////
    else if (0 == strcmp(buf, "down\n") ) {
        if (start == 0){
            printf("error: start the game first\n\n");
            continue;
        }
            data_to_server.type = 1; // sinaliza tipo move
            data_to_server.moves[0] = 3;

        if (send_full(s, &data_to_server, sizeof(data_to_server)) != 0) {
            fprintf(stderr, "Failed to send data to server.\n");
            close(s);
            exit(EXIT_FAILURE);
        }    
            int received = recv_full(s, &data, sizeof(data));
            if (received != 0) {
                logexit("erro ao receber");
            }

            if(data.type == -1){
                printf("error: you cannot go this way\n\n");
            }
            else if (data.type == 5) {
            win = 1;
            start = 0;
            printf("\nYou escaped\n");
                Criptografa_Labirinto(data.board, labirinto_encriptado);
                Imprimi_Labirinto(labirinto_encriptado);

            continue;
            
            } else {

                printf("Possible Moves: %s %s %s %s.\n\n", 
                       (data.moves[0] == 1) ? "up," : "", 
                       (data.moves[1] == 2) ? "right," : "", 
                       (data.moves[2] == 3) ? "down," : "", 
                       (data.moves[3] == 4) ? "left," : "");
            }
        


        
    }
    /////////////// LEFT /////////////////
    else if (0 == strcmp(buf, "left\n") ) {
        if (start == 0){
            printf("error: start the game first\n\n");
            continue;
        }

            data_to_server.type = 1; // sinaliza tipo move
            data_to_server.moves[0] = 4;

        if (send_full(s, &data_to_server, sizeof(data_to_server)) != 0) {
            fprintf(stderr, "Failed to send data to server.\n");
            close(s);
            exit(EXIT_FAILURE);
        }    

            int received = recv_full(s, &data, sizeof(data));
            if (received != 0) {
                logexit("erro ao receber");
            }

            if(data.type == -1){
                printf("error: you cannot go this way\n\n");
            }
            else if (data.type == 5) {
            win = 1;
            start = 0;
            printf("\nYou escaped\n");
                Criptografa_Labirinto(data.board, labirinto_encriptado);
                Imprimi_Labirinto(labirinto_encriptado);

            continue;
            
            } else {

                printf("Possible Moves: %s %s %s %s.\n\n", 
                       (data.moves[0] == 1) ? "up," : "", 
                       (data.moves[1] == 2) ? "right," : "", 
                       (data.moves[2] == 3) ? "down," : "", 
                       (data.moves[3] == 4) ? "left," : "");
            }



        
    }
    /////////////// MAP /////////////////
    else if (0 == strcmp(buf, "map\n")) {
        if (start == 0){
            printf("error: start the game first\n\n");
            continue;
        }

        data_to_server.type = 2;

        if (send_full(s, &data_to_server, sizeof(data_to_server)) != 0) {
            fprintf(stderr, "Failed to send data to server.\n");
            close(s);
            exit(EXIT_FAILURE);
        }    


        int received = recv_full(s, &data, sizeof(data));
            if (received != 0) {
                logexit("erro ao receber");
            }

        Criptografa_Labirinto(data.board, labirinto_encriptado);
        Imprimi_Labirinto(labirinto_encriptado);

    }
    /////////////// HINT /////////////////
    else if (0 == strcmp(buf, "hint\n")) {
        if (start == 0){
            printf("error: start the game first\n\n");
            continue;
        }

    }
    /////////////// RESET /////////////////
    else if (0 == strcmp(buf, "reset\n")) {

        printf("\n");

        data_to_server.type = 6;
        if (send_full(s, &data_to_server, sizeof(data_to_server)) != 0) {
            fprintf(stderr, "Failed to send data to server.\n");
            close(s);
            exit(EXIT_FAILURE);
        }    
            int received = recv_full(s, &data, sizeof(data));
            if (received != 0) {
                logexit("erro ao receber");
            }       
        
        win = 0;
        start = 0;

    }
    /////////////// EXIT /////////////////
    else if (0 == strcmp(buf, "exit\n")) {

        data_to_server.type = 7;
        if (send_full(s, &data_to_server, sizeof(data_to_server)) != 0) {
            fprintf(stderr, "Failed to send data to server.\n");
            close(s);
            exit(EXIT_FAILURE);
        }    
        close(s);
        exit(EXIT_SUCCESS);

    } 
    else {
        printf("error: command not found\n\n");
    }
}


    close(s);
    exit(EXIT_SUCCESS);
}
