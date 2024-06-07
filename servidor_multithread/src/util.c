/* 
    Aluno: Mateus Henrique Silva Araújo
    RA: 184940
    Arquivo para funções de auxílio geral do servidor
*/
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <crypt.h>
#include "../include/server.h"
#include "../include/util.h"
#include "../include/answer.h"
#include "../include/request.h"

// Esta função adiciona o conteúdo do recurso contido em path ao final da mensagem
void appendData(msg_body* body, char* path){
    // Abre o arquivo com a função open
    int fd = open(path,O_RDONLY); 
    if(fd==-1){
        perror("Can't open resource file");
        exit(2);
    }

    // Obtém o tamanho do arquivo em bytes para definir o tamanho do vetor de conteúdo
    struct stat statbuff;
    if(fstat(fd,&statbuff)==-1){
        perror("Can't read resource stat");
        exit(2);
    }
    
    body->len  = statbuff.st_size; // A quantidade de bytes lidos deve ser igual ao tamanho do arquivo
    body->data = malloc(statbuff.st_size); // Aloca memória para guardar os bytes lidos
    //Lê todo os bytes do arquivo e insere eles no vetor de conteúdo da mensagem
    int bytes = read(fd,body->data,statbuff.st_size);

    if(bytes != body->len){
        perror("Erron in appendData(). Not all bytes of the resource file was writen in message.\n");
    }
    
    close(fd); // Fecha o arquivo aberto
    return;
}

// Esta função identifica o tipo de um arquivo a partir de sua extensão, retornando ele no formato do campo Content-Type
char* identifyRscType(char* path){
     
    // Encontra a última ocorrência do caracter / na string do caminho e atualiza type para ser igual ao nome do recurso
    char* type = strrchr(path,'/') + 1;

    // Encontra a última ocorrência do caracter . na string do nome do recuros e atualiza type para ser igual ao tipo do recurso
    type = strrchr(type,'.') + 1;

    char* aux = malloc(60);

    // Verifica qual é o tipo do arquivo a ser envia e preenche a string de auxílio com o cabeçalho adequado
    if( type==NULL || strcmp(type,"txt")==0){
        sprintf(aux,"text/plain; charset=%s",webSpace_charset);
    }
    else if (strcmp(type,"html")==0){
        strcpy(aux,"text/html");
    } 
    else if (strcmp(type,"jpeg")==0 || strcmp(type,"jpg")==0){
        strcpy(aux, "image/jpeg");
    } 
    else if (strcmp(type,"gif")==0){
        strcpy(aux, "image/gif");
    } 
    else if (strcmp(type,"png")==0){
        strcpy(aux, "image/png");
    } 
    else if (strcmp(type,"tif")==0){
        strcpy(aux, "image/tiff");
    } 
    else if (strcmp(type,"pdf")==0){
        strcpy(aux, "application/pdf");
    } 
    else if (strcmp(type,"zip")==0){
        strcpy(aux, "application/zip");
    }  
    return aux;
}

// Esta função escreve todo o conteúdo da resposta no soquete fornecido como parâmetro
void writeAnswer(int socket,answer_msg* answer){
    // Calcula a quantidade de bytes a serem escritos no soquete
    int msg_len= strlen(answer->header);    // Bytes do cabeçalho
    int body_len = answer->body->len; // Bytes do corpo
    int total_len = msg_len+body_len;    // Quantidade total 

    int bytes = write(socket,answer->header,msg_len); // Escreve o cabeçalho
    if(body_len > 0) bytes += write(socket,answer->body->data,body_len); // Escreve o corpo;
    
    // Verifica se a quantidade de bytes totais escritos bate com o total calculado
    if(bytes!=total_len) perror("Error while wrinting to socket. Not all message bytes were sent.");

    return;
}

/* Esta função combina duas strings fornecidas nos parâmetros first e second em uma única, colocando-a no apontado dst.
Ela utiliza do parâmtero preserv para liberar a memória, caso desejado, de uma das strings de parâmetro usadas*/
void strcombine(char** dst,char* first, char* second, int preserv){
    
    char* res = malloc(strlen(first)+strlen(second)+1); // Aloca espaço na memória suficiente para caber a string final
    res[0]='\0';                                        // Insere '\0'
    strcat(res,first);                                  // Insere a primeira string em res
    strcat(res,second);                                 // Concatena o resultado com a segunda string

    switch (preserv)
    {
        case 0b01: // Libera memória da string contida em first
            free(first);
            break;

        case 0b10: // Libera memória da string contida em second
            free(second);
            break;
    }
    *dst = res; // Coloca a string resultado na posição de memória destino

    return;
}

//Fução para retirar espaços no começo ou fim de parametros
char* strtrim(char* str){
    int start, stop, itr=0;

    // Conta a quantidade de espaços no começo
    while(str[itr]==' '){
        itr++;
    }
    start=itr; // Coloca a quantidade de espaços no começo em start

    // Conta a quantidade de espaços no fim
    itr=strlen(str)-1;
    while(str[itr]==' '){
        itr--;
    }
    stop=itr; // Coloca a quantidade de espaços no fim em stop
    
    char* ret = malloc((stop-start+2)*sizeof(char)); // Aloca uma string grande o suficiente para conter o conteúdo sem espaços
    ret[0]='\0';
    strncat(ret,str+start,stop-start+1); // Transfere a string sem espaços
    
    free(str); // Libera a string parâmetro utilizada
    return ret;
}

// Essa função converte o parâmetro fornecido de base64 par ascII e depois cifrar seu campo de senha 
char* convertAndCrypt(char* base){
    // Chama a função de terceiros para decodificar a base64
    char* decode_str = b64_decode(base);

    // Obtem a parte da string contendo a senha
    char* password_field = strrchr(decode_str,':') + 1;
    
    /* Assim como especificado no roteiro da atividade. O salt será sempre um valor fixo sendo igual a "EA".
    Usa-se a função crypt_r, pois, de acordo com o manual de crypt, a função crypt não é thread-safe */
    struct crypt_data data;
	data.initialized=0;

    // Cifra a senha fornecida
    char* encryptedPassword = crypt_r(password_field,"EA",&data);;
    
    // Calcula o comprimento total da string contendo o nome de usuário e a senha criptografada 
    int total_str_len = password_field - decode_str + strlen(encryptedPassword);
    char* credentials =  malloc(total_str_len+2); // Aloca uma string grande o sufuciente para conter esse par usuário:senha cifrada
    
    strncpy(credentials, decode_str , password_field - decode_str); // Transfere a string do usuário para credentials
    credentials[password_field-decode_str]='\0';                    // Insere '\0'
    strcat(credentials,encryptedPassword);                          // Concate credentials com a string contendo a senha cifrada
    credentials[total_str_len]='\n';                                // Insere '\n', pois cada linha deve conter uma senha
    credentials[total_str_len+1]='\0';                              // Insere '\0' para terminar a string
    free(decode_str);                                               // Libera memória auxiliar alocada 
    return credentials;                                             // Retorna a string contendo o campo usuário:senha cifrada
}

/* Algorítimo para conversão inversa de base64. Como foi permitido o uso de código externo pelo roteiro, o código a seguir não é de minha
   autoria */
int b64_decoded_size(char *in){
	int len , ret, i;
	len = strlen(in);
	ret = len / 4 * 3;

	for (i=len; i-->0; ) {
		if (in[i] == '=') {
			ret--;
		} else {
			break;
		}
	}

	return ret;
}

char* b64_decode(char *in){

    int b64invs[] = { 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58,
                  59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5,
                  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                  21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
                  29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
                  43, 44, 45, 46, 47, 48, 49, 50, 51 };

	size_t len, i, j;
	int    v;

	len = strlen(in);
    int decoded_size = b64_decoded_size(in);
    char* out = malloc(decoded_size+1);

	for (i=0, j=0; i<len; i+=4, j+=3) {
		v = b64invs[in[i]-43];
		v = (v << 6) | b64invs[in[i+1]-43];
		v = in[i+2]=='=' ? v << 6 : (v << 6) | b64invs[in[i+2]-43];
		v = in[i+3]=='=' ? v << 6 : (v << 6) | b64invs[in[i+3]-43];

		out[j] = (v >> 16) & 0xFF;
		if (in[i+2] != '=')
			out[j+1] = (v >> 8) & 0xFF;
		if (in[i+3] != '=')
			out[j+2] = v & 0xFF;
	}
    out[decoded_size]='\0';
	return out;
}
