/* 
    Aluno: Mateus Henrique Silva Araújo
    RA: 184940
    Arquivo para funções de tratamento específicas do servidor
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <crypt.h>
#include "../include/answer.h"
#include "../include/util.h"
#include "../include/request.h"
#include "../include/server.h"

// Função para iniciar uma nova mensagem de resposta
void initNewAnswerMsg(char **path, answer_msg *answer){
    answer->status=NOT_FOUND; // Inicializa o status como Not Found
    answer->header = malloc(1);  // Aloca uma memória inicial para a mensagem
    answer->header[0]='\0';      // Insere '\0'
    answer->body = malloc(sizeof(msg_body));  // Aloca memória para a estrutura que armazena o corpo
    answer->body->data = NULL;   // Inicializa com NULL o campo de dados do corpo
    answer->body->len = 0;       // Define como 0 o comprimento inicial do corpo
    return;
}

// Função que implementa o fluxo principal de resposta à uma requisição
int answerRequest(char* request_string, int socket){
    char* full_path;                                    // String para armazenar o caminho completo até o recurso requisitado
    struct stat statbuf;                                // Buffer para inserir informações de arquivo do recurso a ser enviado na resposta
    answer_msg answer;                                  // Mensagem de resposta
    parsed_request_list* parsed_request = malloc(sizeof(parsed_request_list));    // Lista encadeada de cabeçalhos e parâmetros da mensagem de requisição
    parsed_request->head = NULL;                        // Inicializa a lista como sendo nula
    parsed_request->body = NULL;                        // Inicializa o corpo da requisição como sendo nulo
    int keep_conection = 0;                             // Parâmetro que indica se a conexão dever ser mantida ou fechada

    //Iniciando nova mesagem de resposta
    initNewAnswerMsg(&full_path,&answer);

    // Realizando a análise léxica e sintática da requisição
    request_parse(request_string ,parsed_request);

    // Isolando valores adequados da requisição
    char* resource_path;
    methods method = request_getMethod(&resource_path, parsed_request); // Obtém o método da requisição

    if(validatePath(resource_path, &full_path, &answer, &statbuf, parsed_request)){ // Se a função que valida o caminho retornar 1, então a requisição pode ser respondida
        switch (method){
            case GET: keep_conection = answerGet(&answer,full_path,statbuf, parsed_request); // Chama a função que trata uma requisição GET
                break;
            case HEAD: keep_conection = answerHead(&answer,full_path,statbuf, parsed_request); // Chama a função que trata uma requisição HEAD
                break;
            case OPTIONS: keep_conection = answerOptions(&answer,full_path,statbuf, parsed_request); // Chama a função que trata uma requisição OPTIONS
                break;
            case TRACE: keep_conection = answerTrace(&answer,full_path,statbuf, parsed_request, request_string); // Chama a função que trata uma requisição TRACE
                break;
            case POST: keep_conection = answerPost(&answer,full_path,statbuf, parsed_request); // Chama a função que trata uma requisição Post
                break;
            case UNKNOWN: answer.status = BAD_REQUEST;  // Caso o método não pertença protocolo HTTP. Gera o erro Bad Request
                break;
            default:  answer.status=NOT_IMPLEMENTED; // Deverão entrar aqui os casos de métodos PUT, POST DELETE e CONNECT
                break;
        }
    }

    // Processa status e escreve a mensagem de resposta
    process_status(method,&answer, parsed_request, &keep_conection);
    writeAnswer(socket,&answer);
    
    // Libera estruturar alocadas
    freeParsedRequestList(parsed_request);
    free(parsed_request);
    free(answer.header);
    if(answer.body->data!=NULL) free(answer.body->data);
    free(answer.body);
    free(full_path);
    
    // Retorna 0 se deve-se encerrar a conexão com cliente ou 1 se deve-se mantê-la
    return keep_conection;
}


// Esta função se responsabiliza de gerar o cabeçalho da mensagem de resposta a ser retornada
int generateHeaders(int fields, answer_msg* answer_ptr,char* path, struct stat* statbuf, parsed_request_list* parsed_request_ptr){
    char aux[200]; // Buffer auxiliar
    char buffer[80]; // Buffer para strings de tempo
    time_t curtime;  // Struct para valores de tempo   
    struct tm *info; // Struct para informações de valores de tempo
    int request_kc;

    // Campo Date
    if (fields & DATE){
        time(&curtime);                         // Obtém o tempo em segundos desde a EPOCH
        info = localtime(&curtime);             // Converte para um padrão associado a timezone do usuário
        strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %Z", info); // Gera uma string padronizada com as informações de tempo
        sprintf(aux,"Date: %s\r\n",buffer);     // Transfere a string com informações de tempo para o buffer auxiliar
        strcombine(&answer_ptr->header,answer_ptr->header,aux,0b01); // Insere no cabeçalho final da requisição de resposta
    }
    
    // Campo Server
    if (fields & SERVER){
        // Insere informação sobre o servidor no cabeçalho final da requisição de resposta
        strcombine(&answer_ptr->header,answer_ptr->header,"Server: Servidor HTTP ver. final de Mateus Araujo\r\n",0b01);
    }

    // Campo Allow
    if(fields & ALLOW){
        // Insere informação sobre os métodos HTTP respondidos pelo servidor  no cabeçalho final da requisição de resposta
        strcombine(&answer_ptr->header,answer_ptr->header,"Allow: GET, HEAD, POST, OPTIONS, TRACE\r\n",0b01);
    }

    // Campo Connection:
    if (fields & CONNECTION){
        char* connection_type;

        if(parsed_request_ptr != NULL){ // Se foi fornecida uma lista ligada contendo a requisição...
            // Obtém o campo "Connection" da requisição do cliente por meio da lista ligada
            header_node * node = getHeaderNode("Connection", parsed_request_ptr); 

            /* Se a requisição apresentar este campo, define connection_type como sendo igual a ele. Caso não, define o campo como 
            tendo valor "close". Em seguida, seta request_kc como 0 se connection_type for "close" ou como 1 se connection_type for
            keep-alive*/
            connection_type = (node==NULL)? "close": node->parameters_list->head->parameter_field;
            request_kc = (strcmp(connection_type,"close")==0)? 0:1;
    
        } else{ // Se não foi fornecida uma lista ligada contendo a requisição...
            request_kc = 0;              
            connection_type = "close";  
        } 

        sprintf(aux, "Connection: %s\r\n",connection_type); // Insere o cabeçalho na string do buffer auxiliar
        strcombine(&answer_ptr->header,answer_ptr->header,aux,0b01); // Insere a string contida no buffer auxiliar no cabeçalho final

        if(request_kc){ // Se a conezão for do tipo "keep-alive"
                sprintf(aux, "Keep-Alive: timeout=%d\r\n",TIMEOUT_SECS);     // Especifica a quantidade de segundos até timeout
                strcombine(&answer_ptr->header,answer_ptr->header,aux,0b01); // Insere a informação no cabeçalho final
        }
    }

    // Campo Last-Modified
    if(fields &  LAST_MODIFIED){
        info = localtime(&statbuf->st_mtime);                               // Obtém o tempo da última modificação do recurso a ser enviado
        strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %Z", info); // Converte em uma string formatada
        sprintf(aux, "Last-Modified: %s\r\n",buffer);                       // Gera cabeçalho "Last-Modified" segundo padrão estabelecido
        strcombine(&answer_ptr->header,answer_ptr->header,aux,0b01);        // Insere cabeçalho gerado no cabeçalho final
    }
    
    // Campo Content-Lenght
    if(fields & CONTENT_LENGTH){
        // Obtém o tamanho em bytes do recurso a ser enviado e gera o cabeçalho "Content-Length" segundo padrão estabelecido
        sprintf(aux,"Content-Length: %ld\r\n",statbuf->st_size);     
        strcombine(&answer_ptr->header,answer_ptr->header,aux,0b01);       // Insere cabeçalho gerado no cabeçalho final 
    }
    
    // Campo Content-type
    if (fields &  CONTENT_TYPE){
        char* content_type_str = identifyRscType(path);              // Identifica o tipo do recurso a ser enviado
        sprintf(aux,"Content-Type: %s\r\n",content_type_str);        // Gera o cabeçalho "Content-Type" segundo padrão estabelecido
        strcombine(&answer_ptr->header,answer_ptr->header,aux,0b01); // Insere cabeçalho gerado no cabeçalho final
        free(content_type_str);                                      // Libera memória usada na string do tipo do recurso
    }

    // Campo WWW-Authenticate
    if(fields & WWW_AUTHENTICATE){
        // Gera o cabeçalho "WWW-Authenticate" segundo padrão estabelecido
        sprintf(aux,"WWW-Authenticate: Basic realm=\"Espaco Protegido\"\r\n"); 
        strcombine(&answer_ptr->header,answer_ptr->header,aux,0b01); // Insere cabeçalho gerado no cabeçalho final
    }

    //Terminação do cabeçalho
    if (fields & END){
        strcombine(&answer_ptr->header,answer_ptr->header,"\r\n",0b01); // Adiciona separção do cabeçalho final ao corpo da mensagem
    }

    // Retorna o valor indicando se a requisição o cliente deseja manter a conexão (1) ou não (0)
    return request_kc; 
}

// Esta função executa o processo de resposta específico à uma requisição GET
int answerGet(answer_msg* answer_ptr,char* full_path,struct stat statbuf, parsed_request_list* parsed_request_ptr){
    // Gera cabeçalho
    int kc = generateHeaders( DATE|SERVER|CONNECTION|LAST_MODIFIED|CONTENT_LENGTH|CONTENT_TYPE|END, // Cabeçalhos desejados na resposta
                              answer_ptr, full_path, &statbuf, parsed_request_ptr); // Demais parâmetros
    appendData(answer_ptr->body,full_path); //Insere conteúdo
    return kc; // Retorna o valor indicando se a requisição o cliente deseja manter a conexão (1) ou não (0)
}

// Esta função executa o processo de resposta específico à uma requisição POST
int answerPost(answer_msg* answer_ptr,char* full_path,struct stat statbuf, parsed_request_list* parsed_request_ptr){
    //Inicializa uma para armazenar o valor de Keep-connection da requisição
    int kc;

    // Verifica se o tipo de arquivo enviado pela requisição é um formulário
    header_node* content_type_field = getHeaderNode("Content-Type", parsed_request_ptr);
    if(content_type_field == NULL || strcmp(content_type_field->parameters_list->head->parameter_field,"application/x-www-form-urlencoded")!=0){
        // Como o servidor não sabe responder requisições que enviam outros tipos de corpo, retorna status Not Implemented
        answer_ptr->status = NOT_IMPLEMENTED;
        return 0; 
    }
    
    // Antes de realizar outros processamentos, obtemos os valores dos campos do formulário enviado.
    char *username = strstr(parsed_request_ptr->body,"=") + 1, // Campo com nome de usuário
         *pass = strstr(username,"=") + 1,                     // Campo com senha antiga
         *new_pass1 = strstr(pass,"=") + 1 ,                   // Campo com nova senha
         *new_pass2 = strstr(new_pass1,"=") + 1,               // Campo com confirmação de nova senha
         *send_btn = strstr(new_pass2,"=") + 1;                // Campo com botão de enviar

    // Garante que o formulário apresenta os campos adequados e transforma cada um deles em uma string
    if (username!=NULL && pass!=NULL && new_pass1!=NULL && send_btn!=NULL){
        // Tranforma cada campo obtido em uma string única se aproveitando do divisor "&" que existe entre eles
        *strchr(username,'&')='\0';
        *strchr(pass,'&')='\0';
        *strchr(new_pass1,'&')='\0';
        *strchr(new_pass2,'&')='\0';
    } else {
        // Se algum dos campos necessário não for fornecido, retorna uma página indicando problema no formulário enviado
        stat("html/bad_form.html",&statbuf);    // Lê informações de arquivo da pagina
        kc = generateHeaders( DATE|SERVER|CONNECTION|LAST_MODIFIED|CONTENT_LENGTH|CONTENT_TYPE|END, 
                              answer_ptr, "html/bad_form.html", &statbuf, parsed_request_ptr);      // Gera cabeçalho de resposta
        appendData(answer_ptr->body,"html/bad_form.html"); //Insere conteúdo
        return kc; // retorna
    }

    // Verifica se as duas senhas novas fornecidas são rigorosamente iguais
    if(strcmp(new_pass1,new_pass2)){
        // Retorna página html indicando que as senhas foram preenchidas erroneamente
        stat("html/password_not_eq.html",&statbuf); // Lê informações de arquivo da página
        kc = generateHeaders( DATE|SERVER|CONNECTION|LAST_MODIFIED|CONTENT_LENGTH|CONTENT_TYPE|END,
                              answer_ptr, "html/password_not_eq.html", &statbuf, parsed_request_ptr);  // Gera cabeçalho de resposta
        appendData(answer_ptr->body,"html/password_not_eq.html"); //Insere conteúdo
        return kc; // retorna
    }

    char* htaccess_path;
    // Obtém-se o caminho para o arquivo .htaccess mais próximo ao recurso indicado pela requisição
    if(getHtaccess(&htaccess_path,full_path)){
        // Caso ocorra algum problema na obtenção do caminho do .htaccess que atua no recurso, envia página de erro
        stat("html/htpass_error.html",&statbuf);                // Lê informações de arquivo da página
        kc = generateHeaders( DATE|SERVER|CONNECTION|LAST_MODIFIED|CONTENT_LENGTH|CONTENT_TYPE|END,
                              answer_ptr, "html/htpass_error.html", &statbuf, parsed_request_ptr); // Gera cabeçalho de resposta
        appendData(answer_ptr->body,"html/htpass_error.html");  //Insere conteúdo
        return kc; // retorna
    }

    /* Agora temos, em htaccess_path, o caminho para o arquivo .htaccess que influência o formulário indicado pela requisição.
    Precisamos obter o conteúdo do arquivo apontado por ele.*/
    char *htpassword_content;   // Buffer para armazenar o conteúdo do arquivo de senhas
    int  canwrite = 0;          // Variável para indicar a possibilidade de escrita sobre o arquivo de senhas
    /*Obtém o conteúdo e o descritor de arquivo do arquivo htpassword. É necessário que o arquivo seja mantido aberto durante todo o
    restante do processamento, pois, como poderão ser feitas alterações nesse arquivo, outras threads lerem seu conteúdo poderá produ
    zir condições de corrida*/
    int fd =  getHtpassword(&htpassword_content, &canwrite, htaccess_path);

    if( fd<0 || canwrite==0){ // Verifica se foi possível abrir o arquivo e se ele apresenta permissão de escrita
        /* Caso ocorra algum problema na leitura do .htaccess que atua no recurso, ou na abertura e leitura do arquivo de senhas,
        envia página de erro*/
        close(fd);                                              // Fecha o arquivo .htpassword aberto
        stat("html/htpass_error.html",&statbuf);                // Lê informações de arquivo da página
        kc = generateHeaders( DATE|SERVER|CONNECTION|LAST_MODIFIED|CONTENT_LENGTH|CONTENT_TYPE|END,
                              answer_ptr, "html/htpass_error.html", &statbuf, parsed_request_ptr); // Gera cabeçalho de resposta
        appendData(answer_ptr->body,"html/htpass_error.html"); //Insere conteúdo
        return kc; // retorna
    }

    // Inicializa uma variável para indicar falha na verificação
    int verificationFailed = 0;
    
    // Inicializa estrutura para ser usada com função crypt_r
    struct crypt_data data;
	data.initialized=0;

    // Configura uma variável auxiliar para se procurar o nome de usuário no arquivo
    char* aux = malloc(strlen(username)+2);
    sprintf(aux,"%s:",username);
    
    // Procura o nome de usuário fornecido no conteúdo do arquivo de senhas
    char* credentials = strstr(htpassword_content,aux);

    if(credentials!=NULL){ // Se houver ao menos um usuário com o nome fornecido no formulário...
        char* curr_pass = strchr(credentials,':') + 1; // Obtém a senha atual presente no conteúdo do arquivo de senhas
        char* nl_delim = strchr(curr_pass,'\n');       // Encontra a próxima ocorrência de um fim de linha
        // Insere '\0' na próxima ocorrência da quebra de linha, isolando curr_pass em uma string contendo somente a senha atual
        *nl_delim = '\0'; 

        // Cifra a senha enviada como senha antiga no formulário
        // Segundo o manual, a string gerada por crypt serve como chave para gerar uma nova string encriptada de mesmo sal.
        char* crypted_pass = crypt_r(pass,curr_pass,&data);
        
        // Confere se a senha atual e a senha antiga fornecida no formulário, ambas cifradas, são identicas
        if(strcmp(crypted_pass,curr_pass)==0){ // Se forem idênticas...
            // Cifra a nova senha
            crypted_pass = crypt_r(new_pass1,curr_pass,&data);

            //Substitui ela no arquivo
            lseek(fd,curr_pass-htpassword_content,SEEK_SET); // Desloca o offset para corresponder a posição da senha antiga (CHECAR)
            write(fd,crypted_pass,strlen(crypted_pass));     // Escreve por cima da antiga senha
        } else{ // Se não forem idênticas...
            verificationFailed = 1; // Seta a variável que indica erro na verificação
        }
    } else{ // Se não houver nenhum usuário com o nome fornecido no formulário...
        verificationFailed = 1; // Seta a variável que indica erro na verificação
    }
    close(fd);  // Fecha o arquivo, permitindo que outra thread opere sobre ele

    if(verificationFailed){ // Se não existis um usuário com o nome fornecido ou a senha fornecida não bater com a senha atual..
        // Configura o retorno de uma página de erro
        stat("html/verification_failed.html",&statbuf); // Lê informações de arquivo da página
        kc = generateHeaders( DATE|SERVER|CONNECTION|LAST_MODIFIED|CONTENT_LENGTH|CONTENT_TYPE|END, 
                              answer_ptr, "html/verification_failed.html", &statbuf, parsed_request_ptr); // Gera cabeçalho de resposta
        appendData(answer_ptr->body,"html/verification_failed.html"); //Insere conteúdo
        
    } else { // Se a verificação foi bem-sucedida e a senha foi alterada
        // Configura o retorno de uma página de successo
        stat("html/success.html",&statbuf); // Lê informações de arquivo da página
        kc = generateHeaders( DATE|SERVER|CONNECTION|LAST_MODIFIED|CONTENT_LENGTH|CONTENT_TYPE|END, 
                              answer_ptr, "html/success.html", &statbuf, parsed_request_ptr);  // Gera cabeçalho de resposta
        appendData(answer_ptr->body,"html/success.html"); //Insere conteúdo
    }

    // Libera memória de variáveis alocadas
    free(aux);
    free(htpassword_content);

    return kc; // Retorna o valor indicando se a requisição o cliente deseja manter a conexão (1) ou não (0)
}

// Esta função executa o processo de resposta específico à uma requisição HEAD
int answerHead(answer_msg* answer_ptr,char* full_path,struct stat statbuf, parsed_request_list* parsed_request_ptr){
    // Gera Header e retorna indicador de conexão
    return generateHeaders( DATE|SERVER|CONNECTION|LAST_MODIFIED|CONTENT_LENGTH|CONTENT_TYPE|END, // Cabeçalhos desejados na resposta 
                            answer_ptr, full_path, &statbuf, parsed_request_ptr);                 
}

// Esta função executa o processo de resposta específico à uma requisição OPTIONS
int answerOptions(answer_msg* answer_ptr,char* full_path,struct stat statbuf, parsed_request_list* parsed_request_ptr){
    // Gera Header e retorna indicador de conexão
    return generateHeaders( DATE|SERVER|CONNECTION|LAST_MODIFIED|CONTENT_LENGTH|CONTENT_TYPE|ALLOW|END, // Cabeçalhos desejados na resposta
                            answer_ptr, full_path, &statbuf, parsed_request_ptr);
}

// Esta função executa o processo de resposta específico à uma requisição TRACE
int answerTrace(answer_msg* answer_ptr,char* full_path,struct stat statbuf, parsed_request_list* parsed_request_ptr, char* request_str){
    // Gera cabeçalho inical
    int kc = generateHeaders(DATE|SERVER|CONNECTION|LAST_MODIFIED, answer_ptr, full_path, &statbuf, parsed_request_ptr);
    
    // Gera a requisição como uma string para adicioná-la ao corpo da resposta
    answer_ptr->body->data = malloc(strlen(request_str)+1); // Aloca o espaço nos dados do corpo
    strcpy(answer_ptr->body->data,request_str);           // Transfere a requisição para o corpo
    answer_ptr->body->len = strlen(request_str)+1;          // Atualiza o tamanho do corpo

    // Adiciona detalhes adicionais sobre requisição recebida
    char aux[100];
    sprintf(aux, "Content-Length: %ld\r\n",strlen(request_str)+1);
    strcombine(&answer_ptr->header,answer_ptr->header,aux,0b01);
    strcombine(&answer_ptr->header,answer_ptr->header,"Content-type: message/http\r\n",0b01);
    strcombine(&answer_ptr->header,answer_ptr->header,"\r\n",0b01);
    
    // Retorna indicador de conexão
    return kc;
}

/* Esta função verifica a profundidade no web-space de um recurso, retornando 1 caso ele esteja fora do web-space.
Ela faz isso ao percorrer a string do caminho completo componente a componente e, sempre que encontra um diretório "..", 
o remove da string junto com seu diretório antecessor (que seria saltado)*/
int checkInvalidDepth(char* full_path){
    if(strcmp(full_path,webSpace_path)==0) return 0;  //Para o caso do recurso ser a raiz

    // Define uma variável para guardar o resultado da função e uma para guardar o offset até a parte iterada da string
    int res, offset = strlen(webSpace_path);            

    /* Define apontadores para a string que conterá o caminho completo, a parte iterada da string com caminho completo
    e para a próxima barra na string iterada*/
    char *final_path = strdup(full_path), *itr, *next_slash;     

    /* Incremente a variável iteradora de modo a fazer a iteração começar no primeiro caractere após a primeira barra após o
    caminho do web-space*/
    itr = final_path + offset+1;
    
    // Fica no laço até que a string toda seja percorrida
    while(1){
        // Encontra a posição da próxima barra a partir da parte iterada atual 
        next_slash = strchr(itr,'/');

        // Se não houver mais barras na parte iterada, significa que toda a string de caminho foi percorrida
        if(next_slash==NULL){
            /* Verifica se o caminho do webspace está presente no início da string alterada do caminho. Se sim, o recurso
            é interno ao web-space, e a função retorna 0. Se não, o recurso está fora do web-space, portanto a função retor
            na 1*/
            res = (strncmp(final_path,webSpace_path,strlen(webSpace_path))==0)? 0:1; 
            free(final_path);    // Libera memória alocada para a string com o caminho final
            return res; 
        }

        /* Faz a próxima barra a partir do início da parte iterada ser igual '\0'. Isso transforma o componente do caminho 
        iterado em uma string*/
        *next_slash = '\0';

        // Verifica se esse componente do caminho iterado é ".."
        if(strcmp(itr,"..") == 0){
            *next_slash='/';    // Desfaz a alteração feita anteriormente

            /* Transforma a barra localizada no caracter anterior ao começo do componente do caminho iterado no caracter nulo,
            tornando essa parte da string inalcançável para as funções da biblioteca string.h*/
            *(itr-1) = '\0';    

            /*Com o componente iterado "retirado" da string de caminho final, encontra a última ocorrência do caracter '/' nessa
            string (que deve ficar logo antes do nome do diretório anterior ao componente iterado) e a transforma em '\0'. Isto
            elimina o diretório antecessor ao componente ".." da string do caminho final.*/
            *(strrchr(final_path,'/'))='\0';
            offset = strlen(final_path);    // Calcula o tamanho da nova string sem o diretório saltado pelo ".."
            
            /*Copia no caminho final, o restante da string ainda não iterada, sem o "..", na string final, sem o diretório que
            foi saltado por ".."*/
            strcat(final_path,next_slash);  

            /*Atualiza a posição para a nova iteração começar no componente do caminho logo após o ".."*/
            itr = final_path + offset + 1;
        } else{
            *next_slash='/';    // Desfaz a alteração feita anteriormente
            /*Atualiza a posição para a nova iteração começar no próximo componente do caminho*/
            itr = next_slash+1; 
        }
    }
}

/* Esta função obtém o arquivo .htaccess que tem influência majoritária sobre um recurso. Ela retorna 1 se ele não existir ou não 0 se  
ele existir. O caminho para o arquivo encontrado é inserido no parâmetro dst*/
int getHtaccess(char** dst,char* full_path){
    char* lastDir, *full_path_cpy;
    int len = strlen(full_path);
    
    // Realiza procedimento adicional para tratar arquivos padrões dentro de recursos diretórios
    full_path_cpy = malloc(strlen(full_path)+2);    // Aloca uma string para armazenar o caminho completo e mais dois elementos
    strcpy(full_path_cpy,full_path);                // Copia o caminho completo para a string alocada
    if(full_path_cpy[len-1]!='/') full_path_cpy[len]='/';   // Se o caminho completo não terminar com uma barra, insere uma barra
    full_path_cpy[len+1]='\0';                      // Insere a terminação para string

    while(strcmp(full_path_cpy,webSpace_path)!=0){  // Enquanto a cópia alterada do caminho completo não for igual ao caminho do web-space
        lastDir = strrchr(full_path_cpy,'/');  // Encontra a últica ocorrência de uma / no caminho do recurso
        full_path_cpy[strlen(full_path_cpy) - strlen(lastDir)] = '\0';    // Retira o nome o último diretório do caminho do recurso
        
        // Constrói o caminho para o .htaccess no diretório mais próximo do último diretório avaliado
        strcombine(dst,full_path_cpy,"/.htaccess",0b11);     
        if (access(*dst,F_OK) == 0) break; // Verifica se esse arquivo existe e, se sim, termina o laço
        
        // Se o arquivo não existe...
        free(*dst);        // Libera a memória da string utilizada;
    }
    
    /* Se webSpace_path e full_path_cpy são iguai, não foi encontrado nenhum .htaccess nos diretórios até o recurso. Mas ainda
       é possível que exista um no diretório raiz */
    if(strcmp(full_path_cpy,webSpace_path)==0){
        strcombine(dst,full_path_cpy,"/.htaccess",0b11);
        
        // Checa se existe .htaccess na raiz. Se não existir o recurso é desprotegido e pode ser retornado
        if (access(*dst,F_OK) != 0){
            free(*dst);
            free(full_path_cpy);
            return 1; 
        } 
    } 
    free(full_path_cpy);
    
    return 0;
}

/* Essa função obtém o conteúdo do arquivo de senhas apontado pelo arquivo .htaccess cujo caminho foi fornecido como parâmetro*/
int getHtpassword(char** content_buff,int* canwrite,char* htaccess_path){
    struct stat infobuf;
    if(access(htaccess_path,R_OK)) return -1; // Verifica se o arquivo htaccess pode ser lido. Se não pode, retorna -1

    // Abre o arquivo .htaccess encontrado e obtém o caminho para as senhas
    stat(htaccess_path,&infobuf);               // Checa o tamanho do arquivo
    char* auxBuff = malloc(infobuf.st_size+1);  // Cria um buffer para armazenar o caminho para as senhas
    int fd = open(htaccess_path, O_RDONLY);     // Abre o arquivo
    

    free(htaccess_path);                    // Libera a memória alocada para a string
    int bytes_read = read(fd, auxBuff, infobuf.st_size);  // Lê o conteúdo do arquivo e armazena em auxBuff
    close(fd);                                      // Fecha o arquivo para não atrapalhar outras threads
    auxBuff[bytes_read] = '\0';                     // Transforma auxBuff em uma string
    
    /* Como o enunciado garantiu que o único conteúdo no arquivo .htaccess será o caminho para o arquivo de senhas. 
    Esse caminho se encontra em auxBuff. Dessa forma, podemos abrir o arquivo de senhas e lê-lo*/

    /* Checa o tamanho, a existência do arquivo e se ele pode ser lido e escrito. Se ele não existir ou não puder ser lido
    retorna -1*/
    if (stat(auxBuff,&infobuf)==-1 || (S_IRUSR & infobuf.st_mode)==0){
        free(auxBuff);
        return -1;
    } 

    if((S_IWUSR & infobuf.st_mode)!=0 && canwrite!=NULL){
        *canwrite = 1;               // Indica se é possível realizar escrita sobre o arquivo de senhas
        fd = open(auxBuff,O_RDWR); // Abre o arquivo com permissão de leitura e escrita
    } 
    else fd = open(auxBuff,O_RDONLY); // Abre o arquivo com permissão só de leitura
    
    auxBuff = (char *) realloc(auxBuff,infobuf.st_size + 1);               // Ajusta a quantidade de memória do buffer pra comportar todo o arquivo
    bytes_read = read(fd, auxBuff, infobuf.st_size);  // Lê o conteúdo do arquivo e armazena em auxBuff
    // close(fd);    // Como é possível que a thread deseje alterar o conteúdo do arquivo, não devemos fechá-lo, pois isso permitiria que outra thread
                     // o acessasse
    auxBuff[bytes_read] = '\0';                       // Transforma auxBuff em uma string
    
    if(content_buff!=NULL) *content_buff = auxBuff; // Guarda o conteúdo lido na variável de saída
    return fd;
}

/* Esta função deve verificar a necessidade de credenciais para um certo recurso e, caso haja, verificar se as credenciais enviadas
são compatíveis*/
int checkAuthorization(char* full_path, parsed_request_list* parsed_request){
    char *closest_htaccess_path;
    struct stat infobuf;
    
    // Chama a função getHtaccess para obter o arquivo .htaccess que protege o recurso
    if(getHtaccess(&closest_htaccess_path,full_path)){
        // Se essa função retorna 0, então não existe um recurso .htaccess protegendo o recurso
        return 0; // Retorna 0 para que o recurso seja entregue
    }
    
    // A partir daqui, existe um arquivo .htaccess protegendo o recurso, cujo caminho se encontra na string closest_htaccess_path
    
    // Verificando se a requisição tem o campo necessário
    header_node* authorization_field = getHeaderNode("Authorization", parsed_request);
    if(authorization_field == NULL){
        /* Há um .htaccess no caminho do recurso e a requisição não tem o campo com usuário e senha.
        Nesse caso, o servidor retorna com status UNAUTHORIZED   */
        free(closest_htaccess_path);
        return 1;
    } 

    // Obtém o valor do campo, isola a parte contendo a senha, converte de volta da base64 e encritografa a parte da senha
    char* credentials = convertAndCrypt(strrchr(authorization_field->parameters_list->head->parameter_field,' ') + 1);

    /* Obtendo o conteúdo do arquivo de senhas. Se, por qualquer motivo, esse arquivo não puder ser aberto, retorna 1 para que a 
    o servidor retone o status UNAUTHORIZED*/
    char* auxBuff;
    int res , fd = getHtpassword(&auxBuff,NULL,closest_htaccess_path); 
    if (fd<0) return 1;
    
    /*O buffer auxiliar agora contém todo o arquivo de senhas fornecido. Se a busca pela string credentials (que contém o usuário e a
    senha criptografada fonecidos na requisição) retornar NULL, então esse usuário e senha não existem e o servidor retorna com status 
    UNAUTHORIZED. Caso o retorno seja diferente de NULL, esse usuário e senha existem e o servidor pode continuar com a resposta à re
    quisição*/ 
    if(strstr(auxBuff,credentials)==NULL) res= 1;
    else res= 0;

    close(fd);                                       // Fecha o arquivo, permitindo que outra thread opere sobre ele
    free(credentials);                              // Libera a memória gasta na string de credenciais
    free(auxBuff);                                  // Libera a memória gasta no buffer auxiliar
    return res;
}

//Esta função verefica se o recurso desejado é válido, ou seja, existe e tem permissão para ser retornado
int validatePath(char* rsc_path, char** full_path, answer_msg* answer, struct stat* statbuf_ptr, parsed_request_list* parsed_request){

    //Combinar o caminho do espaço web com o recurso solicitado
    if (strcmp(rsc_path,"/")==0) *full_path = strdup(webSpace_path);
    else{
        strcombine(full_path,webSpace_path,rsc_path,0b11);
    } 

    // Verifica se o recurso requisitado não se encontra fora do web-space definido
    if(checkInvalidDepth(*full_path)){
        answer->status = FORBIDEN;
        return 0;
    }

    // Verifica se é necessário autenticação para se acessar esse recurso
    if(checkAuthorization(*full_path,parsed_request)){
        answer->status = UNAUTHORIZED;
        return 0;
    }

    if(stat(*full_path, statbuf_ptr) == -1){ 
        switch (errno){
            case EACCES:
                // Caso o recurso exista mas um dos diretórios até ele não tem permissão de execução, status="Forbiden"
                answer->status = FORBIDEN; 
                break;
            default:
                // Caso o recurso não exista, status="Not Found"
                answer->status = NOT_FOUND; 
                break;
        }
        return 0;
    } 

    if(access(*full_path,R_OK)!=0){// Verificar se o recurso possui permissão de leitura. Caso não possua, status="Forbiden"
        answer->status = FORBIDEN;
        return 0; 
    }

    switch (statbuf_ptr->st_mode & S_IFMT)  { // Verificar se o recurso é um arquivo ou diretório
        
        case S_IFREG : // Caso seja um arquivo
            answer->status = OK;
            break;
        
        case S_IFDIR : // Caso seja um diretório
            if(access(*full_path,X_OK)!=0){// Verificar se o ele possui permição de varredura. Caso não possua, status="Forbiden"
                answer->status = FORBIDEN;
                return 0; //Retorna 0 pois não pode ser aberto
            }
            
            //Se o caminho termina com uma barra, retira-se a barra
            if((*full_path)[strlen(*full_path)-1]=='/') (*full_path)[strlen(*full_path)-1]= '\0';

            //Caso ele possua permissão de varredura. Verificar se existe o arquivo index.html ou welcome.html
            //Como não foi informada uma preferência sobre qual desses dois arquivos escolher caso ambos existam, será dada preferência ao arquivo index.html
            char* index;
            strcombine(&index,*full_path,"/index.html",0b11); 
            char* welcome;
            strcombine(&welcome,*full_path,"/welcome.html",0b11);
            int index_exist = 0, index_readable = 0;        // Define que, por padrão, o arquivo index não existe
            int welcome_exist = 0, welcome_readable = 0;    // Define que, por padrão, o arquivo welcome não existe
            if (stat(index,statbuf_ptr)==0){                   // Caso o arquivo index exista
                index_exist = 1;                            // Altera a variável que indica sua existência
                index_readable = (access(index,R_OK)==0);   // Checa se ele pode ser lido
            }
            if (stat(welcome,statbuf_ptr)==0){                     // Caso o arquivo welcome exista
                welcome_exist = 1;                              // Altera a variável que indica sua existência
                welcome_readable = (access(welcome,R_OK)==0);   // Checa se ele pode ser lido
            }


            if( index_exist && index_readable){ // Se index existir e puder se lido, abre ele com a chamada open e imprime seu contúdo na tela com a chamada write
                stat(index,statbuf_ptr); // Reinicia a variável statbuf com os valores adequados
                free(*full_path);    // Libera a memória alocada anteriormente 
                free(welcome);  
                *full_path = index;  // Atualiza a variaável global com o caminho adequado ao recurso como o contido em index
                answer->status = OK; // status = OK
            } // Caso index não exista ou não puder ser lido
            else if( welcome_exist && welcome_readable){ // Se welcome existir e puder se lido, abre ele com a chamada open e imprime seu contúdo na tela com a chamada write
                stat(welcome,statbuf_ptr); // Reinicia a variável statbuf com os valores adequados
                free(index); 
                free(*full_path);      // Libera a memória alocada anteriormente
                *full_path = welcome; // Atualiza a variaável global com o caminho adequado ao recurso como o contido em welcome
                answer->status = OK; // status = OK
            }
            else{
                free(welcome);
                free(index);

                if(!index_exist && !welcome_exist){ // Caso ambos não existam, status="Not Found"
                    answer->status = NOT_FOUND;
                    return 0; 
                } 
                else{ // Caso um ou mais deles existam mas ambos estão sem permissão de leitura, status=“Forbiden”
                    answer->status = FORBIDEN;
                    return 0; 
                } 
            }
            break;
    }
    return 1; // Se o programa chegar a esse ponto, o caminho agora indica um recurso válido, a variável statbuf contém suas informações e o recurso pode ser lido
}


//Esta função processa o status da resposta
void process_status(methods method, answer_msg* answer_ptr,parsed_request_list* parsed_request,int* kc_ptr){
    char* error_path;
    struct stat error_statbuf;

    switch (answer_ptr->status){
        case OK: // Em caso de suscesso
            // Insere o cabeçalho OK
            strcombine(&answer_ptr->header,"HTTP/1.1 200 OK\r\n", answer_ptr->header,0b10);
            return; // retorna

        case BAD_REQUEST: // Em caso de erro 400
            strcombine(&answer_ptr->header,"HTTP/1.1 400 Bad Request\r\n", answer_ptr->header,0b10);  // Insere cabeçalho de erro
            error_path = "html/bad_request.html"; // Define o caminho adequado para a página html deste erro
            stat(error_path,&error_statbuf);      // Lê as informações de arquivo desta página
            break;                                // Quebra o bloco switch

        case UNAUTHORIZED:  // Em caso de erro 401
            strcombine(&answer_ptr->header,"HTTP/1.1 401 Unauthorized\r\n", answer_ptr->header,0b10);  // Insere cabeçalho de erro
            error_path = "html/unauthorized.html";   // Define o caminho adequado para a página html deste erro
            stat(error_path,&error_statbuf);         // Lê as informações de arquivo desta página
            generateHeaders( DATE|WWW_AUTHENTICATE|SERVER|CONNECTION|LAST_MODIFIED|CONTENT_LENGTH|CONTENT_TYPE|END, 
                             answer_ptr,error_path,&error_statbuf,parsed_request);  // Gera cabeçalhos de resposta
            *kc_ptr = 0;                             // Define o indicador de conexão como sendo zero (deve-se fechar conexão com cliente)
            appendData(answer_ptr->body,error_path); // Adiciona conteúdo do arquivo de erro
            return;                                  // retorna

        case FORBIDEN: // Em caso de erro 403
            strcombine(&answer_ptr->header,"HTTP/1.1 403 Forbiden\r\n", answer_ptr->header,0b10);  // Insere cabeçalho de erro
            error_path = "html/forbiden.html";    // Define o caminho adequado para a página html deste erro
            stat(error_path,&error_statbuf);      // Lê as informações de arquivo desta página
            break;                                // Quebra o bloco switch

        case NOT_FOUND:
            strcombine(&answer_ptr->header,"HTTP/1.1 404 Not Found\r\n", answer_ptr->header,0b10);  // Insere cabeçalho de erro
            error_path = "html/not_found.html";   // Define o caminho adequado para a página html deste erro
            stat(error_path,&error_statbuf);      // Lê as informações de arquivo desta página
            break;                                // Quebra o bloco switch

        case REQUEST_TIMEOUT:
            strcombine(&answer_ptr->header,"HTTP/1.1 408 Request Timeout\r\n", answer_ptr->header,0b10);  // Insere cabeçalho de erro
            error_path = "html/request_timeout.html";   // Define o caminho adequado para a página html deste erro
            stat(error_path,&error_statbuf);            // Lê as informações de arquivo desta página
            break;                                      // Quebra o bloco switch

        case NOT_IMPLEMENTED:
            strcombine(&answer_ptr->header,"HTTP/1.1 501 Not Implemented\r\n", answer_ptr->header,0b10);  // Insere cabeçalho de erro
            error_path = "html/not_implemented.html";   // Define o caminho adequado para a página html deste erro
            stat(error_path,&error_statbuf);            // Lê as informações de arquivo desta página
            break;                                      // Quebra o bloco switch

        case SERVICE_UNAVAILABLE:
            strcombine(&answer_ptr->header,"HTTP/1.1 503 Service Unavailable\r\n", answer_ptr->header,0b10);  // Insere cabeçalho de erro
            error_path = "html/service_unavailable.html";   // Define o caminho adequado para a página html deste erro
            stat(error_path,&error_statbuf);                // Lê as informações de arquivo desta página
            break;                                          // Quebra o bloco switch
    }

    /* Somente respostas com status de erro (exceto 401) chegam até aqui. Dessa forma, usa-se as funções já definidas anteriormente 
    para finalizar-se a geração da resposta*/
    if(method==GET) *kc_ptr = answerGet(answer_ptr,error_path,error_statbuf,parsed_request);
    if(method==HEAD) *kc_ptr = answerHead(answer_ptr,error_path,error_statbuf,parsed_request);

}

// Esta função gera uma resposta com um determinado status fornecido como parâmetros sem que uma requisição seja necesária
void answerWithStatus(msg_status pre_defined_status, int socket){
    answer_msg answer;      // Mensagem de resposta
    int keep_conection;     // Indicador de conexão

    //Iniciando nova mesagem de resposta
    answer.status=NOT_FOUND;
    answer.header = malloc(1);
    answer.header[0]='\0';
    answer.body = malloc(sizeof(msg_body));
    answer.body->len = 0;

    // Define o status da mensagem como o passado por argumento
    answer.status = pre_defined_status;

    // Processa o status de erro
    process_status(GET,&answer, NULL,&keep_conection);

    // Escreve a resposta no soquete fornecido
    writeAnswer(socket,&answer);
    
    //Libera campos de mensagem
    free(answer.header);
    free(answer.body->data);
    free(answer.body);
    
    return;
}

