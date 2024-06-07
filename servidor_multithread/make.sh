#!/bin/bash
# Script para compilar o aplicativo do servidor

gcc src/main.c \
    src/lex.yy.c \
    src/parser.tab.c \
    src/request.c \
    src/answer.c \
    src/server.c \
    src/util.c \
    -I./include -ly -lfl -lcrypt -w -o servidor