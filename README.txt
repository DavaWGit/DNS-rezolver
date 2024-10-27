David Zahálka
xzahal03
20.11.2023
DNS rezolver provádějící rezoluci záznamů A, AAAA a PTR.
Vypíše jednotlivé sekce DNS zprávy (Question section, Answer section, Authority section, Additional section).
Omezení argumentů -> nelze použít argumenty -6 a -x zároveň, více v dokumentaci.
Způsob překladu: make
Spuštění testů: make test
Příklad spuštění: ./dns -s 147.229.8.12 www.fit.vut.cz -6
-> AAAA záznam pro zjištění www.fit.vut.cz IPv6 zaslaný serveru kazi.fit.vutbr.cz (147.229.8.12)
Odevzdané soubory: manual.pdf, dns.cpp, README.txt, test.sh, Makefile