CC = gcc
FLAGS = -Wall -g


stnc: stnc.o netcom.h client server ipv4_tcp ipv4_udp ipv6_tcp ipv6_udp uds_dgram uds_stream cleantxt mmap pipe
	$(CC) $(FLAGS) -pthread -o stnc stnc.o

client: client.o netcom.h
	$(CC) $(FLAGS) -o client client.o

server: server.o netcom.h
	$(CC) $(FLAGS) -o server server.o -lssl -lcrypto

ipv4_tcp: ipv4_tcp.o netcom.h
	$(CC) $(FLAGS) -o ipv4_tcp ipv4_tcp.o -lssl -lcrypto

ipv4_udp: ipv4_udp.o netcom.h
	$(CC) $(FLAGS) -o ipv4_udp ipv4_udp.o -lssl -lcrypto

ipv6_tcp: ipv6_tcp.o netcom.h
	$(CC) $(FLAGS) -o ipv6_tcp ipv6_tcp.o -lssl -lcrypto

ipv6_udp: ipv6_udp.o netcom.h
	$(CC) $(FLAGS) -o ipv6_udp ipv6_udp.o -lssl -lcrypto

uds_dgram: uds_dgram.o netcom.h
	$(CC) $(FLAGS) -o uds_dgram uds_dgram.o -lssl -lcrypto

uds_stream: uds_stream.o netcom.h
	$(CC) $(FLAGS) -o uds_stream uds_stream.o -lssl -lcrypto

mmap: mmap.o netcom.h
	$(CC) $(FLAGS) -o mmap mmap.o -lssl -lcrypto

pipe: pipe.o netcom.h
	$(CC) $(FLAGS) -o pipe pipe.o -lssl -lcrypto

stnc.o: stnc.c netcom.h
	$(CC) $(FLAGS) -c stnc.c

client.o: client.c netcom.h
	$(CC) $(FLAGS) -c client.c

server.o: server.c netcom.h
	$(CC) $(FLAGS) -c server.c

ipv4_tcp.o: ipv4_tcp.c netcom.h
	$(CC) $(FLAGS) -c ipv4_tcp.c

ipv4_udp.o: ipv4_udp.c netcom.h
	$(CC) $(FLAGS) -c ipv4_udp.c

ipv6_tcp.o: ipv6_tcp.c netcom.h
	$(CC) $(FLAGS) -c ipv6_tcp.c

ipv6_udp.o: ipv6_udp.c netcom.h
	$(CC) $(FLAGS) -c ipv6_udp.c

uds_dgram.o: uds_dgram.c netcom.h
	$(CC) $(FLAGS) -c uds_dgram.c

uds_stream.o: uds_stream.c netcom.h
	$(CC) $(FLAGS) -c uds_stream.c

mmap.o: mmap.c netcom.h
	$(CC) $(FLAGS) -c mmap.c

pipe.o: pipe.c netcom.h
	$(CC) $(FLAGS) -c pipe.c


cleantxt:
	rm -f *.txt 
clean:
	rm -f *.o stnc client server ipv4_tcp ipv4_udp ipv6_tcp ipv6_udp uds_dgram uds_stream mmap pipe *.txt