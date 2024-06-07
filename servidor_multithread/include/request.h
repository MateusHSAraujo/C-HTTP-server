/* 
    Aluno: Mateus Henrique Silva Araújo
    RA: 184940
    Arquivo cabeçalho para funções de pesquisa e isolamento de campos em uma requisição
*/
#ifndef REQUEST_H_
#define REQUEST_H_

// Enumeração para os possíveis métodos de uma requisição HTTP
typedef enum methods{
    GET,
    HEAD,
    TRACE,
    OPTIONS,
    PUT,
    POST,
    CONNECT,
    DELETE,
    UNKNOWN,
} methods;

// Estrutura para um nó que contém um parâmetro de um cabeçalho da requisição
typedef struct parameter_node {     
        char* parameter_field;          // String indicando o parâmetro
        struct parameter_node* next;    // Apontador para o próximo nó de parâmetro
} parameter_node;

// Estrutura para uma lista de parâmetros de um cabeçalho da requisição
typedef struct header_parameters_list {
    parameter_node* head;               // Apontador para a cabeça da lista
} header_parameters_list;

// Estrutura para um nó que contém informações de um cabeçalho da requisição
typedef struct header_node {
    char* header_name;                          // String para o nome do cabeçalho
    header_parameters_list* parameters_list;    // Apontador para a lista de parâmetros do cabeçalho em questão
    struct header_node* next;                   // Apontador para o próximo nó de cabeçalho
} header_node;

// Estrutura para a lista ligada de cabeçalhos e parâmetros da requisição de entrada
typedef struct parsed_request_list{             
    header_node* head;                          // Apontador para a cabeça da lista de cabeçalhos
    char* body;                                 // String contendo o corpo da requisição
} parsed_request_list;

/* Esta função solicita a análise léxica/sintática da requisição fornecida no parâmetro request_string.
A lista gerada nessa análise é armazenada no parâmetro parsed_request_ptr */
void request_parse(char* request_string, parsed_request_list * parsed_request_ptr);

// Função converte a string que identifica o método em sua constante específica
methods request_getMethod(char** resource_path, parsed_request_list * parsed_request_ptr);

/*Adiciona um parametro a lista de parametros do ultimo cabeçalho da lista ligada da requisição*/
void pushParamToLastHeader(char* parameter, parsed_request_list* request_list);

/*Adiciona um comando no final da lista ligada de requisição*/
void pushHeader (char* header_name, parsed_request_list* request_list);

/*Retorna o primeiro cabeçalho da requisição (aquele que contém o método)*/
header_node* getFirstHeader(parsed_request_list* request_list);

/*Retorna o nó do cabeçalho cujo nome é igual a string fornecida como entrada*/
header_node* getHeaderNode(char* name, parsed_request_list* request_list);

// Adiciona o corpo da requisição enviada à estrutura da lista ligada da requisição
void addBody(char* body, parsed_request_list* request_list);

// Realiza a liberação da memória alocada para uma lista ligada de requisição fornecida como parâmetro
void freeParsedRequestList(parsed_request_list* request_list);


#endif