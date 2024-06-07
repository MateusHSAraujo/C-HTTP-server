Aluno:  Mateus Henrique Silva Araújo
RA:     184940
Data:   01/12/2023
Versão final do programa servidor


- Para compilar o programa servidor, basta executar o script make.sh fornecido no direório raiz do projeto.
- Caso queira usar o web-space fornecido para testes, lembre-se de antes executar o script chmod_webspace para reconfigurar as permissões dos
arquivos
- Não é necessário recompilar os analisadores léxico e sintático, haja vista que seus arquivos já estão inclusos nos diretórios do programa.
Também não é recomendado que o faça, pois o arquivo de header gerado pelo flex é incompleto, fazendo-se necessário a adição de um include
para o arquivo "parser.tab.h" manualmente nele (sugiro que na linha #38).
- Lembre-se que o uso do programa é o seguinte: 
,                    ./servidor <caminho para web-space> <porta> <número máximo de threads> <charset do web-space>
onde <caminho para web-space> é o caminho para o web-space a ser roteado, <porta> é a porta de conexão que se deseja ligar o servidor,
<número máximo de threads> é o número máximo de threads ativas que serão usadas para responder requisições e <charset do web-space> é
a codificação dos arquivos txt roteados.