/* 
    Aluno: Mateus Henrique Silva Araújo
    RA: 184940
    Módulo com funções para solicitação de análise de requisição e estruturação da lista ligada contendo o resultado.
    Além disso, outras funções utilitárias, como obtenção de um nó específico ou liberação de memória de lista, também são fornecidas.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/answer.h"
#include "../include/util.h"
#include "../include/request.h"
#include "../include/parser.tab.h"
#include "../include/lex.yy.h"

/* Esta função solicita a análise léxica/sintática da requisição fornecida no parâmetro request_string.
A lista gerada nessa análise é armazenada no parâmetro parsed_request_ptr */
void request_parse(char* request_string, parsed_request_list * parsed_request_ptr){
    yyscan_t scanner;       //  Define variáveis necessárias para a análise reentrante
    YY_BUFFER_STATE buff;   //

    // Inicializa a estrutura de escaneamento
    yylex_init(&scanner);
    
    // Define a string lida no soquete como o alvo do escaneamento realizado pela estrutura scanner
    buff = yy_scan_string(request_string,scanner);
    
    // Realiza o escaneamento, inserindo-se o resultado no parâmetro de chamada parsed_request_ptr
    yyparse(scanner,parsed_request_ptr); 
    
    // Deleta as variáveis usadas no escaneamento, liberando memória
    yy_delete_buffer(buff,scanner);
    yylex_destroy(scanner);
}

// Função converte a string que identifica o método em sua constante específica
methods methodConverter(char* method_str){
    if(strcmp(method_str,"GET")==0){
        return GET;
    }
    else if(strcmp(method_str,"HEAD")==0){
        return HEAD;    
    }
    else if(strcmp(method_str,"OPTIONS")==0){
        return OPTIONS;
    }
    else if(strcmp(method_str,"TRACE")==0){
        return TRACE;
    }
    else if(strcmp(method_str,"PUT")==0){
        return PUT;
    }
    else if(strcmp(method_str,"POST")==0){
        return POST;
    }
    else if(strcmp(method_str,"DELETE")==0){
        return DELETE;
    }
    else if(strcmp(method_str,"CONNECT")==0){
        return CONNECT;
    }
    else return UNKNOWN;
}


//Esta função retorna o método da requisição e insere no parâmetro resource_path o caminho do recurso desejado
methods request_getMethod(char** resource_path, parsed_request_list * parsed_request_ptr){
    header_node* method_node = getFirstHeader(parsed_request_ptr);
    *resource_path = method_node->parameters_list->head->parameter_field;
    return methodConverter(method_node->header_name);
}

/*Retorna o primeiro cabeçalho da requisição (aquele que contém o método)*/
header_node* getFirstHeader(parsed_request_list* request_list){
    return request_list->head;
}

/*Retorna o nó do cabeçalho cujo nome é igual a string fornecida como entrada*/
header_node* getHeaderNode(char* name, parsed_request_list* request_list){
    // Itera sobre a lista de cabeçalhos da requisição
    for(header_node* noItr = request_list->head; noItr!=NULL; noItr=noItr->next){
        if(strcmp(name,noItr->header_name)==0){ // Se um dos cabeçalhos possuir nome igual ao parâmetro fornecido
            return noItr; // Retorna o nó deste cabeçalho
        }
    }
    return NULL;    // Caso contrário, retorna NULL
}


/*Adiciona um parametro a lista de parametros do ultimo comando da lista de comandos*/
void pushParamToLastHeader(char* parameter, parsed_request_list* request_list){
    parameter_node* new_node = malloc(sizeof(parameter_node)); // Aloca memória para um novo nó de parâmetro
    new_node->parameter_field = strtrim(parameter);            // Retira eventuais espaços no início e no fim do parâmetro e define seu valor
    new_node->next = NULL;                                     // Seta o nó sucessor deste parâmetro como nulo
    
    // Define a variável de iteração noHItr como sendo o nó de cabeçalho presente na cabeça da lista de cabeçalhos da requisição
    header_node* noHItr=request_list->head; 
    while( noHItr->next!=NULL) noHItr = noHItr->next; // Percorre a lista de cabeçalhos da requisição até a cauda
    // Ao fim dessa iteração, o nó de cabeçalho presente em noHItr é a cauda da lista de cabeçalhos da requisição

    // Obtém-se o nó de parâmetro presente na cabeça da lista de parâmetros do cabeçalho encontrado acima
    parameter_node* noItr=noHItr->parameters_list->head;
    if (noItr == NULL){ // Se esse nó for nulo...
        noHItr->parameters_list->head = new_node; // Define o novo nó como cabeça da lista de parâmetros do cabeçalho obtido
    } 
    else { // Se esse nó não for nulo...
        while( noItr->next!=NULL) noItr = noItr->next; // Percorre a lista de parâmetros até a cauda
        noItr->next = new_node; // Faz o último nó da lista de parâmetros apontar para o novo nó
    }
    
    return;
}

/*Adiciona um comando no final da lista de comandos*/
void pushHeader (char* header_name, parsed_request_list* request_list){
    header_node* new_node = malloc(sizeof(header_node));    // Aloca memória para um novo nó de cabeçalho
    new_node->header_name = header_name;                    // Define seu nome
    new_node->next = NULL;                                  // Define seu sucessor como sendo nulo
    new_node->parameters_list = malloc(sizeof(header_parameters_list)); //Aloca memória para sua lista de parâmetros
    new_node->parameters_list->head = NULL;                 // Define o nó de início da sua lista de parâmetros como nulo
    
    
    header_node* noItr=request_list->head;              // Define o nó da cabeça da lista da requisição como o nó iterado
    if (noItr == NULL) request_list->head = new_node;   // Se esse nó for nulo, torna o novo nó de cabeçalho a cabeça da lista de requisição
    else{                                               // Se esse nó não for nulo ...
        while( noItr->next!=NULL) noItr = noItr->next;  // Percorre a lista de cabeçalhos da requisição até a cauda
        noItr->next = new_node;                         // Faz o novo nó ser a nova cauda da lista
    }
    
    return;
}

// Adiciona o corpo da requisição enviada à estrutura da lista ligada da requisição
void addBody(char* body, parsed_request_list* request_list){
    request_list->body = body;
    return;
}

// Realiza a liberação da memória alocada para uma lista ligada de requisição fornecida como parâmetro
void freeParsedRequestList(parsed_request_list* request_list){
    header_node *noHItr = request_list->head, *nextNoH; /* Define uma variável de iteração, que iterará sobre a lista de cabeçalhos, como 
                                                            sendo o nó da cabeça da lista de cabeçalhos e uma variável auxiliar*/
    parameter_node *noPItr, *nextNoP;                   /* Declara uma variável de iteração para iterar sobre uma lista de parâmetros
                                                           e uma variável auxiliar*/

    while(noHItr!=NULL){  // Enquanto o nó de cabeçalho iterado não for nulo, ou seja, partindo da cabeça até a cauda da lista de cabeçalhos
        // Faz a variável auxiliar de nó de cabeçalho apontar para o nó sucessor do nó de cabeçalho iterado
        nextNoH = noHItr->next;   
        
        // Define a variável de iteração da lista de parâmetros como sendo o nó da cabeça da lista de parâmetros do cabeçalho iterado
        noPItr = noHItr->parameters_list->head; 
        
        free(noHItr->header_name);      // Libera a memória alocada para a string com o nome do cabeçalho iterado
        free(noHItr->parameters_list);  // Libera a memória alocada para a estrutura da lista de parâmetros do nó de cabeçalho iterado
        free(noHItr);                   // Libera a memória alocada para o próprio nó do cabeçalho iterado
        
        while (noPItr!=NULL) {          // Se a cabeça da lista de parâmetros não for nula
            nextNoP = noPItr->next;     // Faz a variável auxiliar de nó de parâmetro apontar para o nó sucessor do nó de parâmetro iterado 
            free(noPItr->parameter_field);  // Libera a memória alocada para a string com o valor do parâmetro
            free(noPItr);               // Libera a memória alocada para a própio nó do parâmetro iterado
            noPItr = nextNoP;           // Avança a iteração para o nó de parâmetro seguinte da lista
        }       
        
        noHItr = nextNoH;   // Avança a iteração para o nó de cabeçalho seguinte a lista
        
    }

    if (request_list->body!=NULL) free(request_list->body); // Libera a memória alocada para a string com o corpo da requisição
}