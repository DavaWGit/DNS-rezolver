all: dns

dns: dns.cpp
	g++ -Wall -Wextra -pedantic -std=c++14 -lm -pthread dns.cpp -o dns

clean:
	rm -f *.o dns

test: dns
	bash test.sh 