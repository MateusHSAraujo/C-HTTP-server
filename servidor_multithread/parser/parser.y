%define api.pure full
%lex-param{yyscan_t scanner}
%parse-param {yyscan_t scanner}{parsed_request_list* parsed_request_ptr}

%code top{
    #include "../include/lex.yy.h"
}

%code requires {
    typedef void * yyscan_t;
    #include "../include/parser.tab.h"
    #include "../include/request.h"
}

%union{ 
    char* string;
    }
%token <string>COMANDO
%token <string>PARAM
%token <string>METODO
%token <string>PARAM_MET
%token <string>CORPO
%token NL

%%
requisicao: primeira_linha linhas NL CORPO {addBody($4,parsed_request_ptr);}
          | primeira_linha linhas NL;
          ;
primeira_linha: metodo param_metodo NL
              ;
metodo: METODO {pushHeader($1,parsed_request_ptr);}
      ;
param_metodo: param_metodo PARAM_MET {pushParamToLastHeader($2,parsed_request_ptr);}
            | PARAM_MET {pushParamToLastHeader($1,parsed_request_ptr);}
            ;
linhas: linhas linha
      | linha
      ;
linha: primeiro_campo ':' segundo_campo NL
     ;
primeiro_campo : COMANDO {pushHeader($1,parsed_request_ptr);} //Ao reconhecer um comando, adiciona ele na lista de comandos
               ;
segundo_campo : segundo_campo ',' PARAM {pushParamToLastHeader($3,parsed_request_ptr);} //Ao reconhecer um parametro, adiciona ele na lista de parametros do ultimo no da lista de comandos
              | PARAM {pushParamToLastHeader($1,parsed_request_ptr);}
              ;

%%