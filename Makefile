CFLAGS+=-Wall -Werror -std=gnu99 -O2 
LDFLAGS+=`curl-config --libs`

TARGETS=twitter github
OBJS=buf.o log.o json.o jsmn.o

all: $(TARGETS) .gitignore

clean:
	$(RM) $(TARGETS)
	$(RM) $(OBJS)

twitter: $(OBJS) twitter.o
	$(CC) -o $@ $^ $(LDFLAGS)

github: $(OBJS) github.o
	$(CC) -o $@ $^ $(LDFLAGS) 

.PHONY: all clean
