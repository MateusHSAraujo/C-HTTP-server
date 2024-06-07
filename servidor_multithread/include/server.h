/* 
    Aluno: Mateus Henrique Silva Araújo
    RA: 184940
    Arquivo de cabeçalho para funções do servidor
*/
#ifndef SERVER_H_
#define SERVER_H_

#define DBUFF_INITIAL_SIZE 2048     // Macro que define o tamanho inicial do buffer dinâmico
#define TIMEOUT_SECS 5              // Macro que define o tempo inicial de timeout após uma requisição com keep-conection

// Buffer dinâmico usado para a leitura de requisições do soquete
typedef struct DynamicCharBuff{
    char* data; // Buffer para armazenar conteúdo lido
    int size;   // Tamanho do buffer
    int len;    // Comprimento do conteúdo lido
} d_buff;

extern char* webSpace_path;     // Declaração para a variável global que guarda o caminho do web-space
extern char* webSpace_charset;  // Declaração para a variável global que guarda o charset do web-space

// Função principal que executa o servidor
int execServer(char* webSpace,char* port, char* max_threads, char* charset);

/* Essa função implementa o processo de leitura e resposta à requisições enviadas por soquete que deve ser feita pelas threads geradas*/
void* thread_manageConnection(void* msg_socket_ptr);

// Esta função faz a inicialização de um buffer dinâmico
void initDbuff(d_buff* in);

// Função para ler uma mensagem enviada pelo soquete, convertê-la em string (por intermédio de um buffer dinâmico)
int readToDbuff(d_buff* buff, int fd);

// Libera a memória do vetor dinâmico que armazena a requisição
void freeDbuff(d_buff* in);

// Função que realiza a expansão do vetor dinâmico quando uma leitura excede seu limite
void expandDbuff(d_buff* buff);

// Função de CallBack para ser invocada quando o programa receber um sinal de terminação
void kill_server();
#endif
