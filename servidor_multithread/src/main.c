/* 
    Aluno: Mateus Henrique Silva Araújo
    RA: 184940
    Arquivo principal da versão inicial do servidor
*/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../include/server.h"

// Implementa o fluxo de inicial do programa, fazendo algumas checagens sobre os parâmetros de entrada
int main(int argc, char *argv[]){
    // Verifica se o número de parâmetros é adequado
    if (argc != 5){
        printf("Uso: %s <caminho para a raiz do web-space> <porta> <número máximo de threads> <charset do web-space>\n", argv[0]);
        return -1;
    }

    // Verifica se é possível converte a string que corresponde ao número de porta em um inteiro
    if( atoi(argv[2])==0 ){
        printf("Parâmetro inválido fornecido para número de porta. Tente novamente.\n");
        return -1;
    } // Verifica se é possível converte a string que corresponde ao número máximo de threads em um inteiro 
    else if( atoi(argv[3])==0){ 
        printf("Parâmetro inválido fornecido para número máximo de threads. Tente novamente.\n");
        return -1;
    }

    // Imprime aviso
    printf("Avisos!\n \
            \r       - A codificação dos arquivos html/txt do servidor está sendo definida para %s;\n \
            \r       - O número máximo de threads do servidor está sendo configurado para %s;\n\
            \r       - A porta de operação do servidor está sendo definida para %s;\n",argv[4],argv[3],argv[2]);


    // Executando o servidor
    execServer(argv[1],argv[2],argv[3],argv[4]);
    return 0;
}