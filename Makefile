CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=c99 -pthread -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LDFLAGS = -pthread -lrt
SRN    ?= PES2UG25CS606

TARGET  = campuscompute
SRCS    = main.c input.c scheduler.c logger.c ipc.c banker.c worker.c rlimit.c
OBJS    = $(SRCS:.c=.o)
HEADERS = common.h scheduler.h logger.h ipc.h banker.h worker.h rlimit.h

.PHONY: all run demo clean zip pdf help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

demo: $(TARGET)
	./$(TARGET) --demo

clean:
	rm -f $(OBJS) $(TARGET) campuscompute.log

pdf:
	python3 makepdf.py $(SRN)

zip: clean pdf
	zip -r $(SRN).zip samples $(SRCS) $(HEADERS) Makefile makepdf.py README.md DEMO_WALKTHROUGH.md VIVA_GUIDE.md PROJECT_STUDY_GUIDE.md $(SRN).pdf "Level 3 Mini Project - Student Copy.docx"
	@echo "Verifying archive integrity:"
	unzip -t $(SRN).zip

help:
	@echo "make          build ./$(TARGET)"
	@echo "make run      start the interactive menu"
	@echo "make demo     run the integrated demonstration"
	@echo "make pdf      generate \$$SRN.pdf from screenshots and captions"
	@echo "make zip      package source, documentation, and PDF into \$$SRN.zip"
	@echo "make clean    remove objects, binary, and log"
	@echo "IPC objects are removed by the program itself (./$(TARGET) --cleanup)"
