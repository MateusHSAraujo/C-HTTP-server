/*
    Aluno: Mateus Henrique Silva Araújo
    RA: 184940
    Arquivo para funções de configuração de soquete de comunicação
*/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdint.h>
#include "../include/server.h"
#include "../include/answer.h"
#include "../include/request.h"
#include "../include/parser.tab.h"

static int server_socket, len_addr; // Variáveis usadas para configuração da comunicação por soquete
struct sockaddr_in server,client;   //
char* webSpace_path;                // Variável global que contém o caminho do web-space roteado
char* webSpace_charset;             // Variável global que contém o charset usado no web-space
pthread_mutex_t thread_number_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex global que protege a variável global thread_number
static int thread_number = 0;       // Variável global que armazena a quantidade de threads atualmente em operação
int max_threads;                    // Variável que contém o número máximo de threads

// Função para configuração do servidor
int configServer(int port){
    len_addr = sizeof(client);

    // Inicia o soquete do servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){ // Verifica se a inicialização foi bem-sucedida
        // Caso a inicialização tenha falhado, imprime status de erro no log e finaliza 
        perror("Error while trying to initiate server socket");
        exit(errno);
    }

    // Configura o soquete do servidor
    server.sin_family = AF_INET;            // Família INET
    server.sin_port   = htons(port);        // Porta fornecida pelo usuário
    server.sin_addr.s_addr = INADDR_ANY;    //  Qualquer endereço disponível

    // Tenta realizar associação entre endereço de IP e porta à um soquete
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0){
        // Em caso de falha, imprime status de erro no log e finaliza
        perror("Error while trying to bind server to its socket");
        exit(errno);
    }

    // Tenta iniciar escuta de conexões no soquete do servidor
    if (listen(server_socket, 5) != 0){
        // Em caso de falha, imprime status de erro no log e finaliza
        perror("Error while trying to listen to server socket");
        exit(errno);
    }

    // Tenta reconfigurar a função de tratamento do sinal SIGINT para a função kill_server
    if (signal(SIGINT, kill_server) == SIG_ERR){
        // Em caso de falha, imprime status de erro no log e finaliza
        exit(errno);
    }
    
    // Configurações finalizadas
}

// Função principal que executa o servidor
int execServer(char* webSpace,char* port, char* max_threads_num, char* charset){
    // Verifica se o caminho para o web-space fornecido é válido dentro do sistema
    if (access(webSpace,X_OK | R_OK)){
        perror("access syscall fails on web-space path");
        exit(errno); // Se não for, finaliza o programa
    }
    webSpace_path = webSpace;               // Configura variável global que contém o caminho do web-space
    webSpace_charset = charset;             // Configura a variável global que contém o charset do web-space 
    max_threads = atoi(max_threads_num);    // Configura a variável global que contém o número máximo de threads
    // Chamada para a função de configuração do servidor
    configServer( (unsigned short) atoi(port));

    int msg_socket; // Define um inteiro para guardar o soquete de mensagem
    printf(">> Server ready to recieve connections!\n");
    while(1){
        // Este loop permite que o servidor permaneça constantemente checando se à uma requisição de conexão
        msg_socket = accept(server_socket, (struct sockaddr*)&client, &len_addr);

        /* Se uma conexão ocorre, o soquete de mensagem retorna um valor de descritor de arquivo válido. Se a chamada accept houver sido interrompida
            por algum sinal recebido, msg_socket == -1 e errno == EINTR, então o programa não entra na condicional. */
        if(msg_socket!= -1 && errno !=EINTR){ 
            
            /* Este semáforo governará o acesso a variável global thread_number, a qual indica o número atual de thread existentes. 
                Ele é necessário, pois as demais threads poderão acessar essa variável para decrementá-la quando finalizarem. */
            pthread_mutex_lock(&thread_number_mutex); 
            if(thread_number < max_threads){ // Verifica se o processo original possui o número máximo de threads permitdo
                // Se não possui...
                thread_number++; // Incrementa o número de filhos
                pthread_mutex_unlock(&thread_number_mutex); // Libera o semáforo
            
                pthread_t newThread;
                pthread_create(&newThread, NULL, thread_manageConnection, (void *)(intptr_t)msg_socket);
                // thread_manageConnection((void *) (intptr_t) msg_socket); // Para debbug
                
            } else {
                pthread_mutex_unlock(&thread_number_mutex); // Antes de qualquer ação, libera o semáfaro
                answerWithStatus(SERVICE_UNAVAILABLE, msg_socket); // Gera uma resposta com erro de timeout 
                close(msg_socket);
            }

        }
    }

}

/* Essa função implementa o processo de leitura e resposta à requisições enviadas por soquete que deve ser feita pelas threads*/
void* thread_manageConnection(void* msg_socket_ptr){
    int socket = (intptr_t) msg_socket_ptr;
    
    // Variáveis que serão usadas durante cada requisição
    d_buff in;
    in.data=NULL;
    int keep_connection=0;
    int timeout = 0;

    while (1){
        // Realizando a configuração do buffer intermediário para leitura da mensagem do cliente
        initDbuff(&in);
        // Realizando a leitura da mensagem do cliente e configuração da entrada do analisador léxico
        timeout = readToDbuff(&in, socket);
        if(timeout==0){ // Somente caso a requisição seja válida, essa condicional verifica
            keep_connection = answerRequest(in.data,socket); // Gera a resposta da requisição em forma de uma string

        } else if (timeout==1){
            answerWithStatus(REQUEST_TIMEOUT,socket); // Gera uma resposta com erro de timeout 
        }
        
        freeDbuff(&in);
        // Verifica se houve timeout (timeout!=0) ou se o cliente deseja encerrrar a conexão (keep_connection==0)
        if (keep_connection == 0 || timeout != 0) break;
    }
    
    // Fecha a conexão com o cliente
    close(socket);

    // Decrementa o número de threads existentes
    pthread_mutex_lock(&thread_number_mutex); 
    thread_number--;
    pthread_mutex_unlock(&thread_number_mutex);
    
    // Finaliza a thread
    pthread_exit(0); // Comente para debbugar

}

// Esta função faz a inicialização do buffer dinâmico
void initDbuff(d_buff* in){
    in->data= malloc(DBUFF_INITIAL_SIZE);
    in->size = DBUFF_INITIAL_SIZE;
    in->len = 0;
}

// Liberação da memória do vetor dinâmico que armazena a requisição enviada pelo soquete
void freeDbuff(d_buff* in){
    if(in->data!=NULL) free(in->data);
    return;
}   

// Função para ler uma mensagem enviada pelo soquete, convertê-la em string (por intermédio de um buffer dinâmico)
int readToDbuff(d_buff* buff, int fd){
    // Variáveis usadas para chamada de sistema poll()
    int n;
    struct pollfd fds;
    fds.fd=fd;
    fds.events = POLLIN;

    /* Define o tempo de tolerancia para fechar a conexão.*/
    int tolerance = TIMEOUT_SECS * 1000;
    
    
    n = poll(&fds, 1, tolerance);
    // Se poll retornou um valor maior que 0 e revents == POLLIN, há possibilidade de leitura
    if(n > 0 && ((fds.revents & POLLIN) || (fds.revents & POLLHUP))) {
        int bytes;
        if (buff->size - buff->len < DBUFF_INITIAL_SIZE+1) expandDbuff(buff); // Verifica se o buffer precisa ser expandido para comportar mais dados
        bytes = read(fd, buff->data, DBUFF_INITIAL_SIZE/2); //Realiza leitura do soquete
        buff->len+=bytes;   // Atualiza quantidade de elementos do buffer

        /* Se nenhum dado tiver sido lido, retorna -1.
           Nos sistemas Linux a chamada de systema poll() retorna 0 e seta revents == POLLIN mesmo se não há nenhum dado a ser lido, mas há
           possibilidade de leitura */
        if(bytes == 0) return -1; ; 
        
        // Processo para ler dados restantes caso a primeira leitura não tiver lido todos
        while(bytes>0){
            if (buff->size - buff->len < DBUFF_INITIAL_SIZE+1) expandDbuff(buff); // Verifica necessidade de aumento de tamanho do buffer
            bytes = recv(fd, buff->data+buff->len, DBUFF_INITIAL_SIZE/2,MSG_DONTWAIT); // Lê os dados disponíveis (leitura não bloqueante)
            if (bytes>0) buff->len+=bytes;  // Atualiza quantidade de elementos do buffer
        }

        /* Tranforma o buffer em uma string (todas as leituras são feitas de modo que sempre sobre um espaço no final do buffer para
           a inserção do elemento '\0')*/
        buff->data[buff->len]='\0'; 

        return 0; // Retorna 0

    } else if(n==0)  return 1; // Caso tenha ocorrido timeout, retorna 1
    else if (fds.revents & POLLNVAL) return -1;
    else{
        // Caso ocorra algum valor anômalo
        perror("Error in poll() syscall");
        exit(errno);
    }

}

// Função que realiza a expansão do vetor dinâmico quando uma leitura excede seu limite
void expandDbuff(d_buff* buff){
    char* n_data = malloc(buff->size*2);
    for(int i=0; i < buff->len;i++){
        n_data[i] = buff->data[i];
    }
    free(buff->data);
    buff->data = n_data;
    buff->size *= 2; 
}

// Função de CallBack para ser invocada quando o programa receber um sinal de terminação
void kill_server(){
    printf("\nTermination signal recieved! Server closed...\n");
    exit(errno);
}

