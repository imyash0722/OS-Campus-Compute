CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=c99 -pthread -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LDFLAGS = -pthread -lrt
SRN_PAIR = PES2UG25CS615_PES2UG25CS606

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
	python3 makepdf.py PES2UG25CS606
	python3 makepdf.py PES2UG25CS615

zip: clean pdf
	rm -f $(SRN_PAIR).zip
	zip -r $(SRN_PAIR).zip samples $(SRCS) $(HEADERS) Makefile makepdf.py README.md PROJECT_STUDY_GUIDE.md "Level 3 Mini Project - Student Copy.docx" PES2UG25CS606.pdf PES2UG25CS615.pdf shots/
	@echo "Verifying archive integrity:"
	unzip -t $(SRN_PAIR).zip

help:
	@echo "make          build ./$(TARGET)"
	@echo "make run      start the interactive menu"
	@echo "make demo     run the integrated demonstration"
	@echo "make pdf      generate PES2UG25CS606.pdf and PES2UG25CS615.pdf"
	@echo "make zip      package all project material into $(SRN_PAIR).zip"
	@echo "make clean    remove objects, binary, and log"
	@echo "IPC objects are removed by the program itself (./$(TARGET) --cleanup)"
