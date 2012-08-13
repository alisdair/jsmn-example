CFLAGS+=-Wall -Werror -std=gnu99 -O2 -c
LDFLAGS+=`curl-config --libs`

TARGETS=twitter
OBJS=buf.o log.o jsmn.o twitter.o

all: $(TARGETS) .gitignore

clean:
	$(RM) $(TARGETS)
	$(RM) $(OBJS)

twitter: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $^

.gitignore: Makefile
	echo $(TARGETS) $(OBJS) | xargs -n 1 >> .gitignore
	sort -u < .gitignore > .gitignore.sorted
	mv .gitignore.sorted .gitignore

.PHONY: all clean
