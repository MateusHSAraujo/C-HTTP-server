/* 
    Aluno: Mateus Henrique Silva Araújo
    RA: 184940
    Arquivo de cabeçalho para funções de auxílio geral do servidor
*/
#ifndef UTIL_H_
#define UTIL_H_

#include "answer.h"

// Esta função adiciona o conteúdo do recurso contido em path ao final da mensagem
void appendData(msg_body* content, char* path);

// Esta função identifica o tipo de um arquivo a partir de sua extensão, retornando ele no formato do campo Content-Type
char* identifyRscType(char* path);

// Esta função escreve todo o conteúdo da resposta no soquete fornecido como parâmetro
void writeAnswer(int socket, answer_msg* answer);

/* Esta função combina duas strings fornecidas nos parâmetros first e second em uma única, colocando-a no apontador dst.
Ela utiliza do parâmtero preserv para liberar a memória, caso desejado, de uma das strings de parâmetro usadas*/
void strcombine(char** dst,char* first, char* second, int preserv);

// Função para retirar espacos no comeco ou fim de parametros
char* strtrim(char* str);

// Essa função converte o parâmetro fornecido de base64 par ascII e depois cifrar seu campo de senha
char* convertAndCrypt(char* base);

// Funções de terceiros obtidos na internet para realizar a decodificação de base64
int b64_decoded_size(char *in);
char* b64_decode(char *in);
#endif