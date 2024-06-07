%{
#include "parser.tab.h"
%}
%x PrimeiroCampo SegundoCampo Corpo
%option warn reentrant bison-bridge
%option header-file="lex.yy.h"
Metodo      (GET)|(HEAD)|(POST)|(PUT)|(DELETE)|(CONNECT)|(OPTIONS)|(TRACE)
Comando     [^:\n \r]+
Param       [^\n\r,]+
Param_met   [^\n \r]+
Nl          ([\r][\n])
%%

{Metodo}                       {yylval_param->string=strdup(yytext);return(METODO);}
[ ]                            ;
{Param_met}                    {yylval_param->string=strdup(yytext);return(PARAM_MET);}
{Nl}                           {BEGIN(PrimeiroCampo); return (NL);} 
<PrimeiroCampo>{Nl}            {BEGIN(Corpo); return(NL);}
<PrimeiroCampo>{Comando}       {yylval_param->string=strdup(yytext); return (COMANDO);}
<PrimeiroCampo>[ ]             ;
<PrimeiroCampo>[:]             {BEGIN(SegundoCampo); return (':');}
<SegundoCampo>{Param}          {yylval_param->string=strdup(yytext); return (PARAM);}
<SegundoCampo>[,]              {return (',');}
<SegundoCampo>{Nl}             {BEGIN(PrimeiroCampo); return (NL);}
<Corpo>[^\n\r]+                {yylval_param->string=strdup(yytext); return (CORPO);}
%%

