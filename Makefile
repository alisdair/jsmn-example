CFLAGS+=-Wall -Werror -std=gnu99 -O2 -c
LDFLAGS+=`curl-config --libs`

TARGETS=twitter github
OBJS=buf.o log.o json.o jsmn.o

all: $(TARGETS) .gitignore

clean:
	$(RM) $(TARGETS)
	$(RM) $(OBJS)

twitter: $(OBJS) twitter.o
	$(CC) $(LDFLAGS) -o $@ $^

github: $(OBJS) github.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $^

.gitignore: Makefile
	echo $(TARGETS) $(OBJS) twitter.o github.o | xargs -n 1 >> .gitignore
	sort -u < .gitignore > .gitignore.sorted
	mv .gitignore.sorted .gitignore

.PHONY: all clean
