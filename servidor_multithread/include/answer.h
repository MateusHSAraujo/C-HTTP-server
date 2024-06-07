/* 
    Aluno: Mateus Henrique Silva Araújo
    RA: 184940
    Arquivo de cabeçalho para a funções de tratamente específicas do servidor
*/
#ifndef ANSWER_H_
#define ANSWER_H_
#include "request.h"
#include <sys/types.h>
#include <sys/stat.h>

/*Algumas definições repitidas do arquivo sys/stat.h que foram colocadas aqui novamente para o VSCode 
parar de reclamar, dizendo que elas não existem*/
#define	__S_IFMT	0170000	/* These bits determine file type.  */
# define S_IFMT		__S_IFMT

/* File types.  */
#define	__S_IFDIR	0040000	/* Directory.  */
# define S_IFDIR	__S_IFDIR

#define	__S_IFREG	0100000	/* Regular file.  */
# define S_IFREG    __S_IFREG

// Macros definidas para inserção de cabeçalhos na requisição feita pela função generateRscHeader
#define DATE                0b100000000     // Cabeçalho Date
#define SERVER              0b010000000     // Cabeçalho Server
#define CONNECTION          0b001000000     // Cabeçalho Connection
#define LAST_MODIFIED       0b000100000     // Cabeçalho Last-Modified
#define CONTENT_LENGTH      0b000010000     // Cabeçalho Content-Length
#define CONTENT_TYPE        0b000001000     // Cabeçalho Content-Type 
#define ALLOW               0b000000100     // Cabeçalho Allow
#define WWW_AUTHENTICATE    0b000000010     // Cabeçalho WWW-AUTHENTICATE
#define END                 0b000000001     // Terminação para o separar os cabeçalhos do corpo da mensagem

// Enumeração para os possíveis status da mensagem de resposta
typedef enum msg_status{
    OK,                     //200  
    BAD_REQUEST,            //400
    UNAUTHORIZED,           //401
    FORBIDEN,               //403
    NOT_FOUND,              //404
    REQUEST_TIMEOUT,        //408
    NOT_IMPLEMENTED,        //501
    SERVICE_UNAVAILABLE,    //503
} msg_status;

// Registro para o corpo da mensagem de resposta
typedef struct msg_body{
    char* data;             // Conteúdo do corpo
    int len;                // Quantidade de bytes do conteúdo do corpo
} msg_body;

// Registro para a mensagem de resposta
typedef struct answer_msg{
        char *header;       // Cabeçalhos da resposta
        msg_body* body;     // Corpo da resposta
        msg_status status;  // Status da mensagem de resposta
} answer_msg;

// Função para iniciar uma nova mensagem de resposta
void initNewAnswerMsg(char **path, answer_msg *answer);

// Função que implementa o fluxo principal de resposta à uma requisição
int answerRequest(char* request_string, int socket);

// Esta função se responsabiliza de gerar o cabeçalho da mensagem de resposta a ser retornada
int generateHeaders(int fields, answer_msg* answer_ptr,char* path, struct stat* statbuf, parsed_request_list* parsed_request_ptr);

//Esta função verefica se o recurso desejado é válido, ou seja, existe e tem permissão para ser retornado
int validatePath(char* rsc_path, char** full_path, answer_msg* answer, struct stat* statbuf_ptr, parsed_request_list* parsed_request);

/* Esta função obtém o arquivo .htaccess que tem influência majoritária sobre um recurso. Ela retorna 1 se ele não existir ou não 0 se  
ele existir. O caminho para o arquivo encontrado é inserido no parâmetro dst*/
int getHtaccess(char** dst,char* full_path);

/* Essa função obtém o conteúdo do arquivo de senhas apontado pelo arquivo .htaccess cujo caminho foi fornecido como parâmetro*/
int getHtpassword(char** content_buff,int* canwrite,char* htaccess_path);

// Esta função executa o processo de resposta específico à uma requisição GET
int answerGet(answer_msg* answer_ptr,char* full_path,struct stat statbuf, parsed_request_list* parsed_request_ptr);

// Esta função executa o processo de resposta específico à uma requisição POST
int answerPost(answer_msg* answer_ptr,char* full_path,struct stat statbuf, parsed_request_list* parsed_request_ptr);

// Esta função executa o processo de resposta específico à uma requisição HEAD
int answerHead(answer_msg* answer_ptr,char* full_path,struct stat statbuf, parsed_request_list* parsed_request_ptr);

// Esta função executa o processo de resposta específico à uma requisição OPTIONS
int answerOptions(answer_msg* answer_ptr,char* full_path,struct stat statbuf, parsed_request_list* parsed_request_ptr);

// Esta função executa o processo de resposta específico à uma requisição TRACE
int answerTrace(answer_msg* answer_ptr,char* full_path,struct stat statbuf, parsed_request_list* parsed_request_ptr,  char* request_str);

//Esta função verefica se o recurso desejado é válido, ou seja, existe e tem permissão para ser retornado
void process_status(methods method, answer_msg* answer_ptr,parsed_request_list* parsed_request,int* kc);

// Esta função gera uma resposta com um determinado status fornecido como parâmetros sem que uma requisição seja necesária
void answerWithStatus(msg_status pre_defined_status, int socket);

#endif