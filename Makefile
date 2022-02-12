#**********************************************************
#
# File: Makefile
# Created: 2022-02-10
# Last change: 2022-02-10
# Author: David Chocholaty <xchoch09@stud.fit.vutbr.cz>
# Project: Project 1 for course IPK
# Description: Server that communicates via HTTP
#
#**********************************************************

CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
EXECUTABLE = hinfosvc
OBJS = hinfosvc.o
LOGIN = xchoch09
ZIP_FILE = $(LOGIN).zip

.PHONY: all pack run clean

all: $(EXECUTABLE)

pack: $(ZIP_FILE)

run: $(EXECUTABLE)
	./$(EXECUTABLE) $(ARGS)

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(EXECUTABLE) *.o $(ZIP_FILE)

$(ZIP_FILE): *.c *.h Makefile
	zip $@ $^