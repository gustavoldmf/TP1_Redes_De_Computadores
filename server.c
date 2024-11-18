#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

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
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void Recebe_Dados(char *buf, int csock, struct action *data,struct action *data_to_client, int opcoes[], int labirinto[][10]) {
    
    memset(buf, 0, BUFSZ);
    size_t count = recv(csock, buf, BUFSZ - 1, 0);

    deserialize_action(buf, data);

    switch (data->type)
    {
    case 0:
        printf("\nJogo iniciado:\n");
        break;
    case 1:
        printf("\nMovimentos possiveis:\n");

        break;
    case 2:

        break;
    case 3:

        break;
    default:
        printf("\nComando invalido\n");

        break;
    }

    memset(buf, 0, BUFSZ);
    sprintf(buf, "ACK ");
    serialize_temp(buf, data_to_client);
    count = send(csock, buf, strlen(buf) + 1, 0);
    if (count != strlen(buf) + 1) {
        logexit("send");
    }
}

int Obtem_Posicao( int codigo, int labirinto[][10], int posicao[], int * dimensao) {
    for (int i = 0; i < *dimensao; i++) {
        for (int j = 0; j < *dimensao; j++) {
            if (labirinto[i][j] == codigo) {
                posicao[0] = i;
                posicao[1] = j;
                return 0;
            }
        }
    }
    return -1;
}

void liberarMatriz(int **matriz, int  dimensao) {
    for (int i = 0; i < dimensao; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

void Varre_Labirinto(const char *files, int * dimensao) {
    char ch;
    int contagem = 0;
    FILE *file = fopen(files, "r");

    if (file == NULL) {
        logexit("Erro ao criar arquivo");
    }

    while((ch = fgetc(file)) != EOF){
        if(ch == '\n' || ch == 13 || ch == 10){
            break;
        }
        if(ch != ' '){
            contagem++;
        }
    }

    rewind(file);
    *dimensao = contagem;

    int **matriz;
    matriz = (int**)malloc(contagem * sizeof(int *));
    for(int i = 0;  i < contagem; i++){
        matriz[i] = (int*)malloc(contagem * sizeof(int));
    }

    for(int i = 0; i < *dimensao; i++){
        for(int j = 0; j < *dimensao; j++){
            fscanf(file, "%d",&matriz[i][j]);
        }
    }
    fclose(file); 
}

int  Obtem_Valor(int linha, int coluna, int labirinto[][10]) {
    return labirinto[linha][coluna];
}

void Start(int *pos_ent, int labirinto[][10], int *opcoes) {
    // Identifica se é possível ir para cima
    if (pos_ent[0] > 0 && Obtem_Valor(pos_ent[0]-1, pos_ent[1], labirinto) == 1) { // 49 (ASCII) equivale a 1
        opcoes[0] = 1;
    }
    // Identifica se é possível ir para a direita
    if (pos_ent[1] < 9 && Obtem_Valor(pos_ent[0], pos_ent[1] + 1, labirinto) == 1) {
        opcoes[1] = 1;
    }
    // Identifica se é possível ir para baixo
    if (pos_ent[0] < 9 && Obtem_Valor(pos_ent[0] + 1, pos_ent[1], labirinto) == 1) {
        opcoes[2] = 1;
    }
    // Identifica se é possível ir para a esquerda
    if (pos_ent[1] > 0 && Obtem_Valor(pos_ent[0], pos_ent[1] - 1, labirinto) == 1) {
        opcoes[3] = 1;
    }
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
    printf("bound to %s, waiting connections\n", addrstr);

    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);

    int csock = accept(s, caddr, &caddrlen);
    if (csock == -1) {
        logexit("accept");
    }

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

     char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    if (argc < 5 || (strcmp(argv[3], "-i")) != 0) {
        usage(argc, argv);
    }

    int dimensao = 0;
    char ch;
    int linhas = 0;
    int colunas = 0;
    int contagem = 0;

    const char *files = argv[4];

//ABRE ARQUIVO PARA LEITURA E SALVAMENTO EM UMA MATRIZ DINAMICA
    FILE *file = fopen(files, "r");
    if (file == NULL) {
        logexit("Create file");
    }

    while((ch = fgetc(file)) != EOF){
        if(ch == '\n' || ch == 13 || ch == 10){
            break;
        }
        if(ch != ' '){
            contagem++;
        }
    }
    rewind(file);
    dimensao = contagem;

    int **matriz = (int**)malloc(contagem * sizeof(int *));

    for(int i = 0;  i < contagem; i++){
        matriz[i] = (int*)malloc(contagem * sizeof(int));
    }

    for(int i = 0; i < dimensao; i++){
        for(int j = 0; j < dimensao; j++){
            fscanf(file, "%d",&matriz[i][j]);
        }
    }

    fclose(file); 
//MODO LEITURA DO ARQUIVO - FECHADO 


    int labirinto[10][10];


//CONVERTE A MATRIZ DE TAMANHO(DIMENSAO) PARA UM LABIRINTO DE 10 POSICÕES 
// -1 INDICA QUE NÃO EXISTE VALOR PARA AQUELA POSICAO DO LABIRINTO
        printf("\nLabirinto:\n");
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            if(j >= dimensao || i >= dimensao){
                labirinto[i][j] = 9;
            }else{
            labirinto[i][j] = matriz[i][j];
            }

        }
    }
//IMPRIMI LABIRINTO
    for (int i = 0; i < 10; i++)
    {
        printf("\n");
        for (int j = 0; j < 10; j++)
        {
            printf("%d ",labirinto[i][j]);
        }
    }
    printf("\n");

    for (int i = 0; i < dimensao; i++) {
        free(matriz[i]);
    }
    free(matriz);


    int pos_jog[2];
    int pos_ent[2];
    int pos_saida[2];

//PEGA A POSICÃO DE CADA ELEMENTO DO LABIRINTO
    Obtem_Posicao( 5, labirinto, pos_jog, &dimensao); // jogador
    Obtem_Posicao( 2, labirinto, pos_ent, &dimensao); // entrada
    Obtem_Posicao( 3, labirinto, pos_saida, &dimensao);     // saída

    printf("A entrada do labirinto está na posição [%d][%d]\n",pos_ent[0], pos_ent[1]);

    int op_mov[4] = {0};
    Start(pos_ent, labirinto, op_mov); // opções de movimento

    printf("Movimentos permitidos> %s %s %s %s\n",
           op_mov[0] ? "cima," : "", op_mov[1] ? "direita," : "", 
           op_mov[2] ? "baixo," : "", op_mov[3] ? "esquerda," : "");

    //LIBERA ESPAÇO ALOCADO PARA A MATRIZ




    /*struct action -> types (server):
    update - 4
    win - 5
    */
    action dados; 
    action data_to_client;

while (1)
{
    Recebe_Dados(buf, csock, &dados, &data_to_client, op_mov, labirinto );
}



    
    


    close(csock);

    exit(EXIT_SUCCESS);
}
