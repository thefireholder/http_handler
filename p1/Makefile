#!/bin/bash                                                                                                                                                                                                      
CC = gcc

default: http_server.c
	$(CC) http_server.c -o http_server

clean:
	-rm http_server

dist: Makefile http_server.c README
	-rm http_server
	tar -czf project1.tar.gz Makefile README http_server.c lab_report.docx