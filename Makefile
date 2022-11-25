SRCS	:=	mini_serv.c

.PHONY: all
all:
	cc $(SRCS) -o mini_serv

.PHONY: clean
clean:
	rm -f mini_serv
