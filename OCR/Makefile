CC = gcc 
CFLAGS = -g
CLIBS = -lm

OBJS = image.o segment.o utils.o nnet.o template.o skew.o ocr.o
all: $(OBJS) execs clean
image.o: image.c
	$(CC) -c image.c
segment.o: segment.c
	$(CC) -c segment.c
utils.o: utils.c
	$(CC) -c utils.c
nnet.o: nnet.c
	$(CC) -c nnet.c
template.o: template.c
	$(CC) -c template.c 
skew.o: skew.c
	$(CC) -c skew.c
ocr.o: ocr.c
	$(CC) -c ocr.c
execs:
	$(CC) -o web_ocr web_ocr.c $(OBJS) $(CFLAGS) $(CLIBS)
	$(CC) -o cmd_ocr cmd_ocr.c $(OBJS) $(CFLAGS) $(CLIBS)
clean:
	rm -f $(OBJS) ../web/scripts/web_ocr
	rm -f -r ../web/scripts/data
	cp web_ocr ../web/scripts
	cp -r data ../web/scripts