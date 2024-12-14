#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>





#define BUFSZ 1024

typedef struct action {
    int type;
    int moves[100];
    int board[10][10];
} action;


int Dimensao_labirinto(FILE *file){
    int contagem = 0;
    int ch;
    while((ch = fgetc(file)) != EOF){
        if(ch == '\n' || ch == 13 || ch == 10){
            break;
        }
        if(ch != ' '){
            contagem++;
        }
    }
    rewind(file);
    return contagem;
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

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int Obtem_Posicao(int codigo, int labirinto[][10], int *posicao) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (labirinto[i][j] == codigo) {
                posicao[0] = i;
                posicao[1] = j;
                return 0;
            }
        }
    }
    return -1;
}

void liberarMatriz(int **matriz, int dimensao) {
    for (int i = 0; i < dimensao; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

void Varre_Labirinto(const char *files, int *dimensao) {
    char ch;
    int contagem = 0;
    FILE *file = fopen(files, "r");

    if (file == NULL) {
        logexit("Erro ao criar arquivo");
    }

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n' || ch == 13 || ch == 10) {
            break;
        }
        if (ch != ' ') {
            contagem++;
        }
    }

    rewind(file);
    *dimensao = contagem;

    int **matriz = (int **)malloc(contagem * sizeof(int *));
    for (int i = 0; i < contagem; i++) {
        matriz[i] = (int *)malloc(contagem * sizeof(int));
    }

    for (int i = 0; i < *dimensao; i++) {
        for (int j = 0; j < *dimensao; j++) {
            fscanf(file, "%d", &matriz[i][j]);
        }
    }
    fclose(file);
}

void Imprimi_Labirinto_Caracteres(int labirinto[][10]) {
    for (int i = 0; i < 10; i++) {
        printf("\n");
        for (int j = 0; j < 10; j++) {
            if(labirinto[i][j] != 240)
            printf("%c ", labirinto[i][j]);
        }
    }
    printf("\n");
}

void Imprimi_Labirinto_Inteiros(int labirinto[][10]) {
    for (int i = 0; i < 10; i++) {
        printf("\n");
        for (int j = 0; j < 10; j++) {
            if(labirinto[i][j] != 240)
            printf("%d ", labirinto[i][j]);
        }
    }
    printf("\n");
}

int Obtem_Valor(int linha, int coluna, int labirinto[][10]) {
    return labirinto[linha][coluna];
}

int Anda_Jogador(int *pos_jog, int labirinto[][10], int type_mov, int dim, int *win) {
    // Movimentos possíveis (cima, direita, baixo, esquerda)
    int movimentos[4][2] = {
        {-1,  0}, // Cima
        { 0,  1}, // Direita
        { 1,  0}, // Baixo
        { 0, -1}  // Esquerda
    };

    if (type_mov < 1 || type_mov > 4) {
        printf("error: movimento inválido\n");
        return 0;
    }

    // Calcula a nova posição com base no movimento
    int nova_linha = pos_jog[0] + movimentos[type_mov - 1][0];
    int nova_coluna = pos_jog[1] + movimentos[type_mov - 1][1];

    // Verifica se a nova posição está fora dos limites do labirinto
    if (nova_linha < 0 || nova_linha >= dim || nova_coluna < 0 || nova_coluna >= dim) {
        return -1;
    }

    // Verifica o valor da nova posição
    int valor = labirinto[nova_linha][nova_coluna];
    if (valor == 0) {
        return -1;
    }

    // Verifica se o jogador chegou na saída
    if (valor == 3) {
        *win = 1;
    }

    // Atualiza a posição do jogador no labirinto
    labirinto[nova_linha][nova_coluna] = 5; // Nova posição do jogador
    labirinto[pos_jog[0]][pos_jog[1]] = 1; // Marca a posição antiga como caminho livre

    // Atualiza a posição atual do jogador
    pos_jog[0] = nova_linha;
    pos_jog[1] = nova_coluna;

    return 0; // Movimento bem-sucedido
}

void Mov_possiveis(int *pos_ent, int labirinto[][10], int *opcoes) {
    //cima
    opcoes[0] = (pos_ent[0] > 0 && 
                (Obtem_Valor(pos_ent[0] - 1, pos_ent[1], labirinto) == 1 || 
                 Obtem_Valor(pos_ent[0] - 1, pos_ent[1], labirinto) == 3)) ? 1 : 0;
    // Direita
    opcoes[1] = (pos_ent[1] < 9 && 
                (Obtem_Valor(pos_ent[0], pos_ent[1] + 1, labirinto) == 1 || 
                 Obtem_Valor(pos_ent[0], pos_ent[1] + 1, labirinto) == 3)) ? 2 : 0;
    // Baixo
    opcoes[2] = (pos_ent[0] < 9 && 
                (Obtem_Valor(pos_ent[0] + 1, pos_ent[1], labirinto) == 1 || 
                 Obtem_Valor(pos_ent[0] + 1, pos_ent[1], labirinto) == 3)) ? 3 : 0;

    // Esquerda
    opcoes[3] = (pos_ent[1] > 0 && 
                (Obtem_Valor(pos_ent[0], pos_ent[1] - 1, labirinto) == 1 || 
                 Obtem_Valor(pos_ent[0], pos_ent[1] - 1, labirinto) == 3)) ? 4 : 0;
}


int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    // printf("bound to %s, waiting connections\n", addrstr);

    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);

    int csock = accept(s, caddr, &caddrlen);
    if (csock == -1) {
        logexit("accept");
    }

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("Client connected\n");

    char buf[BUFSZ];
    char buffer[BUFSZ];

    memset(buf, 0, BUFSZ);
    memset(buffer, 0, BUFSZ);

    if (argc < 5 || (strcmp(argv[3], "-i")) != 0) {
        usage(argc, argv);
    }

    const char *files = argv[4];
    int dimensao = 0;
//ABRE ARQUIVO PARA LEITURA E SALVAMENTO EM UMA MATRIZ DINAMICA
    FILE *file = fopen(files, "r");
    if (file == NULL) {
        logexit("Create file");
    }

    dimensao = Dimensao_labirinto(file);

    int **matriz = (int**)malloc(dimensao * sizeof(int *));
    for(int i = 0;  i < dimensao; i++){
        matriz[i] = (int*)malloc(dimensao * sizeof(int));
    }
    for(int i = 0; i < dimensao; i++){
        for(int j = 0; j < dimensao; j++){
            fscanf(file, "%d",&matriz[i][j]);
        }
    }

    fclose(file); 
//MODO LEITURA DO ARQUIVO - FECHADO 

    int labirinto[10][10];
    int labirinto_reset[10][10]; // contem a vesão de entrada do labirinto caso o cliente de reset no jogo



//CONVERTE A MATRIZ DE TAMANHO(DIMENSAO) PARA UM LABIRINTO DE 10 POSICÕES 
// -1 INDICA QUE NÃO EXISTE VALOR PARA AQUELA POSICAO DO LABIRINTO
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            if(j >= dimensao || i >= dimensao){
                labirinto[i][j] = 9;
                labirinto_reset[i][j] = 9;
            }else{
            labirinto[i][j] = matriz[i][j];
            labirinto_reset[i][j] = matriz[i][j];
            }

        }
    }

    for (int i = 0; i < dimensao; i++) {
        free(matriz[i]);
    }
    free(matriz);

    int movimento = 0;
    int win = 0;
    int pos_jog[2];
    int pos_ent[2];
    int op_mov[4] = {0}; // temos apenas 4 opções de movimento, eles serao setados em 1 quando permitidos

    action dados; // struct local para processamento
    action data_to_client; // struct que é enviada para o cliente

    dados.type = 0;
    memset(dados.moves, 0 , sizeof(dados.moves));
    memset(dados.board, 0, sizeof(dados.board));  

    data_to_client.type = 0;
    memset(data_to_client.moves, 0 , sizeof(data_to_client.moves));
    memset(data_to_client.board, 0, sizeof(data_to_client.board));

while (1)
{
    
    memset(buf, 0, BUFSZ);
    int received = recv_full(csock, &dados, sizeof(dados));
        if(received != 0 ){
            logexit("erro ao receber");
        }

    switch (dados.type)
    {
    case 0:  // comando recebido: START 
        printf("\nStarting new game\n");
        Obtem_Posicao( 2, labirinto, pos_ent); // obtem posicao da entrada
        Mov_possiveis(pos_ent, labirinto, op_mov); // opções de movimento
        
        data_to_client.type = 0;
        for (int i = 0; i < 4; i++)
        {
            data_to_client.moves[i] = op_mov[i];
        }

        memcpy(data_to_client.board, labirinto, sizeof(labirinto));
        labirinto[pos_ent[0]][pos_ent[1]] = 5; // atualiza o jogador para a posicao de entrada automaticamente
        send(csock, &data_to_client, sizeof(data_to_client), 0);

        break;

    case 1: // comando recebido: MOVE

       if(dados.moves[0] != 0){ // condição -> caso exista seja recebido um movimento(up, right, down, left) será feito o movimento

        Obtem_Posicao( 5, labirinto, pos_jog); // obtem posicao do jogador
        Mov_possiveis(pos_jog, labirinto, op_mov); // opções de movimento antes do jogador se movimentar 

         movimento = Anda_Jogador(pos_jog, labirinto, dados.moves[0], dimensao, &win); // avança com o jogador no tabuleiro



        Obtem_Posicao( 5, labirinto, pos_jog); // obtem a nova posicao do jogador apos o avanço
        Mov_possiveis(pos_jog, labirinto, op_mov); // adiciona no vetor op_mov[4] os movimentos possiveis pra autual posiçao 

        if(win == 1){
            data_to_client.type = 5;
        }
        else if(movimento == -1){
            data_to_client.type = -1;
        }
        else{
            data_to_client.type = 1;
        }

        for (int i = 0; i < 4; i++) // adiciona os movimentos possiveis em clinte.moves
        {
            data_to_client.moves[i] = op_mov[i];
        }

        memcpy(data_to_client.board, labirinto, sizeof(labirinto));
        send(csock, &data_to_client, sizeof(data_to_client), 0);
    
    Imprimi_Labirinto_Inteiros(labirinto);
       }
       else{

        Obtem_Posicao( 5, labirinto, pos_jog); // obtem posicao do jogador
        Mov_possiveis(pos_jog, labirinto, op_mov); // opções de movimento antes do jogador se movimentar 


        for (int i = 0; i < sizeof(data_to_client.moves); i++)
        {
            data_to_client.moves[i] = op_mov[i];
        }

        data_to_client.type = 1;
        memcpy(data_to_client.board, labirinto, sizeof(labirinto));

        send(csock, &data_to_client, sizeof(data_to_client), 0);
    Imprimi_Labirinto_Inteiros(labirinto);

       }
        
        break;

    case 2: // recebe comando -> MAP

        data_to_client.type = 2;
        memcpy(data_to_client.board, labirinto, sizeof(labirinto) );

        send(csock, &data_to_client, sizeof(data_to_client), 0);


        break;

    case 5:

    data_to_client.type = 5;
    memcpy(data_to_client.board, labirinto, sizeof(labirinto));
    send(csock, &data_to_client, sizeof(data_to_client), 0);

    break;

    case 6: // comando RESET

        data_to_client.type = 6;
        printf("\nStarting new game!\n");
        memcpy(labirinto, labirinto_reset, sizeof(labirinto_reset));

        send(csock, &data_to_client, sizeof(data_to_client), 0);

        break;

    case 7: // comendo EXIT
        
        printf("\nClient disconnected\n");
        exit(EXIT_SUCCESS);

        break;

    default:
        printf("\nComando invalido\n");
        break;
    }
}
    close(csock);
    exit(EXIT_SUCCESS);
}
