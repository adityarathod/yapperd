.PHONY: all clean

CC = clang
CFLAGS = -g -O2 -target bpf -Wall -D__BPF__

all: xdp_pass.o xdp_drop.o xdp_drop_ipv6_non_tcp.o

clean:
	rm -f *.o

xdp_pass.o: xdp_pass.c
	$(CC) $(CFLAGS) -c -o $@ $<
	
xdp_drop.o: xdp_drop.c
	$(CC) $(CFLAGS) -c -o $@ $<

xdp_drop_ipv6_non_tcp.o: xdp_drop_ipv6_non_tcp.c
	$(CC) $(CFLAGS) -c -o $@ $<
