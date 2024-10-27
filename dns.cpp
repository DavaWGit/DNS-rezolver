/*
    Projekt: DNS rezolver
    Autor: David Zahálka
    Login: xzahal03
    Datum: 20.11.2023
*/

#include <iostream>
#include <iomanip>
#include <string.h>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*
    Hlavička DNS
*/
struct DNS_header {
    uint16_t DNS_ID;
    uint16_t DNS_FLAGS;
    uint16_t DNS_QDCOUNT;
    uint16_t DNS_ANCOUNT;
    uint16_t DNS_NSCOUNT;
    uint16_t DNS_ARCOUNT;
};

/*
    Tělo otázky DNS
*/
struct DNS_question {
    std::string QNAME;
    uint16_t QTYPE;
    uint16_t QCLASS;
};

/*
    Struktura pro uložení hodnot na výstup
*/
struct DNS_Record {
    std::string name;
    uint16_t type;
    uint16_t dnsclass;
    uint32_t ttl;
    std::string rdata;
};

/**
    Konstruktor hlavičky
    @param header - Hlavička pro kterou se mají vyplnit hodnoty
*/
void header_constr(DNS_header* header)
{
    header->DNS_ID = htons(70);
    header->DNS_FLAGS = htons(0b0000000100000000);
    header->DNS_QDCOUNT = htons(1);
    header->DNS_ANCOUNT = htons(0);
    header->DNS_NSCOUNT = htons(0);
    header->DNS_ARCOUNT = htons(0);
}

/**
    Konstruktor těla otázky
    @param question - Struktura otázky k vyplnění
    @param name - Dotazovaný domain name (nebo IPv4/IPv6)
*/
void question_constr(DNS_question* question, std::string& name)
{
    question->QNAME = name;
    question->QTYPE = htons(1);
    question->QCLASS = htons(1);
}

/**
    Převedení domain name na tvar vhodný pro DNS otázku
    @param address - Převáděná adresa
    @return - Adresa v korektním tvaru
*/
char* Convert_question(std::string& address)
{
    char* temp_add = new char[address.size() + 1];
    int len = 0;
    int label = 0;
    int i = 0;
    for(char c : address)
    {
        if(c == '.')
        {
            temp_add[i] = static_cast<char>(len); // zapsání délky segmentu před segment
            i++;
            for(int j = 0; j < len; j++)
            {
                temp_add[i] = address[label + j];
                i++;
            }
            label += len + 1;
            len = 0;
        }
        else
        {
            len++;
        }
    }
    // poslední label po poslední tečce
    temp_add[i] = static_cast<char>(len);
    i++;
    for(int j = 0; j < len; j++)
    {
        temp_add[i] = address[label + j];
        i++;
    }
    temp_add[i] = '\0'; //přidání nulového bitu na konec stringu
    return temp_add;
}

/**
    Rozšíření IPv6 do její celé formy
    @param ip - IPv6 adresa k rozšíření
    @return - Kompletní verze IPv6
 */
std::string extend_ipv6(const std::string& ip)
{
    std::string expanded;
    int colonCount = 0;
    bool doubleColonFound = false;

    // počítání dvoj teček a kontrola použití shorthand notace (:)
    for (size_t i = 0; i < ip.size(); ++i) {
        if (ip[i] == ':') {
            colonCount++;
            if (i > 0 && ip[i - 1] == ':') {
                doubleColonFound = true;
            }
        }
    }

    // počítání chybějících hex sekcí
    int missingSections = 8 - colonCount - 1 + (doubleColonFound ? 1 : 0);
    size_t lastColon = -1;

    for (size_t i = 0; i < ip.size(); ++i) {
        if (ip[i] == ':') {
            if (i == lastColon + 1) {
                while (missingSections-- > 0) {
                    expanded += "0000:";
                }
            }
            lastColon = i;
        } else if (lastColon == i - 1 || i == 0) {
            size_t nextColon = ip.find(':', i);
            if (nextColon == std::string::npos) {
                nextColon = ip.size();
            }

            std::string section = ip.substr(i, nextColon - i);
            expanded += std::string(4 - section.size(), '0') + section;
            i = nextColon - 1;

            if (i < ip.size() - 1) {
                expanded += ":";
            }
        }
    }

    // jestli adresa končí shorthand notací tak za ní doplnit nuly
    if (doubleColonFound && lastColon == ip.size() - 1) {
        while (missingSections-- > 0) {
            expanded += ":0000";
        }
    }

    return expanded;
}

/**
    Obrácení IPv4 adresy pro PTR záznam
    @param ip - Adresa kterou je třeba obrátit
    @return - Obrácená adresa
*/
std::string reverse_address(const std::string& ip)
{
    std::string reversed_ip;
    int len = ip.length();
    int start = len;

    for(int i = len - 1; i >= -1; i--) 
    {
        if(i == -1 || ip[i] == '.') 
        {
            if(!reversed_ip.empty()) 
            {
                reversed_ip += ".";
            }
            reversed_ip += ip.substr(i + 1, start - (i + 1));
            start = i;
        }
    }

    return reversed_ip += ".in-addr.arpa";
}

/**
    Obrácení IPv6 adresy pro PTR záznam
    @param ip - IPv6 adresa kterou je třeba obrátit
    @return - Obrácená IPv6 adresa
*/
std::string reverse_ipv6_address(const std::string& ip)
{
    std::string reversed_ip, tmp;
    tmp = extend_ipv6(ip);
    int len = tmp.length();

    for(int i = len - 1; i >= 0; i--)
    {
        if(tmp[i] != ':')
        {
            reversed_ip += tmp[i];
            reversed_ip += ".";
        }
    }
    return reversed_ip += "ip6.arpa";
}

/**
    Funkce na výběr verze IP adresy (4/6)
    @param ip - Adresa kterou je třeba obrátit
    @return - Obrácená adresa
*/
std::string get_ip_version(const std::string& ip)
{
    int cnt = 0;
    for(char c : ip) 
    {
        if (c == ':') 
        {
            cnt++;
        }
    }
    if(cnt > 0) {

        return reverse_ipv6_address(ip);
    } 
    else 
    {
        return reverse_address(ip);
    }
}

/**
    Funkce na provedení DNS rezoluce
    @param server_ip - Adresa serveru
    @param server_name - Dotazovaná adresa
    @param port - Port na kterém se provede rezoluce
    @param header - Hlavička DNS
    @param reverse - Zda je třeba provést PTR záznam
    @param isServer - Jestli se jedná o rezoluci serveru
    @param quadA - Zda je potřeba provést AAAA záznam
    @param recursion - Zda se má rezoluce provést rekurzivně
    @return - Buffer s obsahem celé odpovědi od serveru
*/
std::vector<char> DNS_query(const std::string& server_ip, std::string& server_name, uint16_t port, DNS_header& header, bool& reverse, bool& isServer, bool& quadA, bool& recursion)
{
    if(reverse == true && quadA == true)
    {
        std::cerr << "Neplatná kombinace přepínačů (-x a -6)." << std::endl;
        exit(EXIT_FAILURE);
    }
    // zjištění serveru se provádí vždy rekurzivně
    if(recursion == false && isServer == false)
    {
        header.DNS_FLAGS &= ~(1 << 8);
        header.DNS_FLAGS = htons(header.DNS_FLAGS);
    }
    DNS_question question;
    if(reverse == true && isServer == false && quadA == false)
    {
        server_name = get_ip_version(server_name);
    }
    question_constr(&question, server_name);
    if(reverse == true && isServer == false && quadA == false)
    {
        question.QTYPE = htons(12);
    }
    if(quadA == true && isServer == false && reverse == false)
    {
        question.QTYPE = htons(28);
    }
    // dva sockety a buffery, jeden pro IPv6, jeden pro IPv4
    struct sockaddr_in server_socket;
    struct sockaddr_in6 server_socket6;
    memset(&server_socket, 0, sizeof(server_socket));
    memset(&server_socket6, 0, sizeof(server_socket6));
    struct in_addr tmp_buffer;
    struct in6_addr tmp_buffer6;

    size_t buffer_size = 1024;
    std::vector<char> buffer(buffer_size);
    char* current_position = buffer.data();

    // vložení celé hlavičky a dotazu do bufferu
    memcpy(current_position, &header, sizeof(DNS_header));
    current_position += sizeof(DNS_header);
    memcpy(current_position, Convert_question(question.QNAME), question.QNAME.size() + 2);
    current_position += question.QNAME.size() + 2;
    memcpy(current_position, &question.QTYPE, sizeof(uint16_t));
    current_position += sizeof(uint16_t);
    memcpy(current_position, &question.QCLASS, sizeof(uint16_t));
    current_position += sizeof(uint16_t);

    int user_socket;
    
    // kontrola, jestli hledaná adresa je IPv6
    if(inet_pton(AF_INET6, server_ip.c_str(), &tmp_buffer6) == 1)
    {
        server_socket6.sin6_family = AF_INET6;
        server_socket6.sin6_port = htons(port);
        server_socket6.sin6_addr = tmp_buffer6;
        user_socket = socket(AF_INET6, SOCK_DGRAM, 0);
        if(user_socket == -1)
        {
            std::cerr << "UDP socket se nepodařil vytvořit." << std::endl;
            exit(EXIT_FAILURE);
        }
        if(connect(user_socket, (struct sockaddr*)&server_socket6, sizeof(server_socket6)) == -1)
        {
            std::cerr << "Navázání spojení bylo neúspěšné." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    // nebo jestli je IPv4
    else if(inet_pton(AF_INET, server_ip.c_str(), &tmp_buffer) == 1)
    {
        server_socket.sin_family = AF_INET;
        server_socket.sin_port = htons(port);
        server_socket.sin_addr = tmp_buffer;
        user_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if(user_socket == -1)
        {
            std::cerr << "UDP socket se nepodařil vytvořit." << std::endl;
            exit(EXIT_FAILURE);
        }
        if(connect(user_socket, (struct sockaddr*)&server_socket, sizeof(server_socket)) == -1)
        {
            std::cerr << "Navázání spojení bylo neúspěšné." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    int i = send(user_socket, buffer.data(), (current_position - buffer.data()), 0);
    if(i == -1)
    {
        std::cerr << "Data se neposlala." << std::endl;
    }
    std::vector<char> answer(buffer_size);
    // nastavení timeoutu v případě čekání na data
    struct timeval time;
    time.tv_sec = 5;
    time.tv_usec = 0;
    if(setsockopt(user_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&time, sizeof(time)) == -1)
    {
        std::cerr << "Nepodařilo se nastavit timeout";
    }
    if((i = recv(user_socket, answer.data(), buffer_size, 0)) == -1)
    {
        std::cerr << "Žádná data nebyla obdržena." << std::endl;
        exit(EXIT_FAILURE);
    }
    answer.resize(i);
    
    return answer;
}

/**
    Funkce na zpracování ukazatelů v DNS odpovědi
    @param reader - Aktuální pozice buffer v odpovědi
    @param buffer - Buffer s celou odpovědí
    @return - Zpracovaný domain name
*/
std::string read_domain_name(char*& reader, const std::vector<char>& buffer)
{
    std::string domainName;
    char* endReader = nullptr; // použito ke skoku zpět na prvotní lokaci

    while (true)
    {
        uint8_t length = static_cast<uint8_t>(*reader);

        if (length == 0)
        {
            reader++; // přeskočení null bytu
            break;
        }
        // pointer (je potřeba udělat skok)
        else if ((length & 0xC0) == 0xC0)
        {
            if (!endReader)
            {
                endReader = reader + 2;
            }
            // vypočítání offsetu
            uint16_t offset = (length & 0x3F) << 8 | static_cast<uint8_t>(*(reader + 1));
            reader = const_cast<char*>(buffer.data()) + offset; // provedení skoku na pozici offsetu
        }
        else
        {
            if (!domainName.empty())
            {
                domainName += '.';
            }
            domainName.append(reader + 1, reader + length + 1);
            reader += length + 1;
        }
    }

    if (endReader) {
        reader = endReader; // skočení na prvotní lokaci pokud došlo ke skoku
    }

    return domainName;
}

/**
    Funkce na zpracování odpovědi na výstup
    @param reader - Aktuální místo v bufferu
    @param buffer - Celý buffer z odpovědí
    @return - Struktura DNS_Record s hodnotami, které se mají vypsat na výstup
*/
DNS_Record parseDNS_Record(char*& reader, const std::vector<char>& buffer)
{
    DNS_Record record;

    record.name = read_domain_name(reader, buffer);

    // uložení všech hodnot potřebných pro výpis do struktury record
    memcpy(&record.type, reader, 2);
    record.type = ntohs(record.type);
    reader += 2;

    memcpy(&record.dnsclass, reader, 2);
    record.dnsclass = ntohs(record.dnsclass);
    reader += 2;

    memcpy(&record.ttl, reader, 4);
    record.ttl = ntohl(record.ttl);
    reader += 4;

    uint16_t data_len;
    memcpy(&data_len, reader, 2);
    data_len = ntohs(data_len);
    reader += 2;

    // A záznam
    if (record.type == 1 && data_len == 4)
    {
        struct in_addr addr;
        memcpy(&addr, reader, data_len);
        record.rdata = inet_ntoa(addr);
        reader += data_len;
    }
    // AAAA záznam
    else if (record.type == 28 && data_len == 16)
    {
        char ipv6_address[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, reader, ipv6_address, INET6_ADDRSTRLEN);
        record.rdata = ipv6_address;
        reader += data_len;
    }
    // PTR, CNAME a NS záznamy
    else if (record.type == 2 || record.type == 12 || record.type == 5)
    {
        record.rdata = read_domain_name(reader, buffer);
    }
    else
    {
        reader += data_len;
    }

    return record;
}

/**
    Funkce na zmenšení všech písmen v argumentech
    @param arg - Argument ze vstupu
*/
void lowerArg(char* arg)
{
    for(int i = 0; arg[i]; i++)
    {
        arg[i] = std::tolower(arg[i]);
    }
}

int main(int argc, char *argv[]) {

    // přepínače
    bool arg_recursion = false;
    bool arg_reverse = false;
    bool arg_quadA = false;
    bool arg_port = false;
    bool has_server = false;

    DNS_header header;
    header_constr(&header);

    std::string ip_name, server_name;
    int ip_port = 53;

    // zpracování argumentů
    for(int i = 1; i < argc; i++)
    {
        lowerArg(argv[i]);

        if(strcmp(argv[i], "-r") == 0)
        {
            if(arg_recursion == true)
            {
                std::cerr << "Argument -r již byl použit." << std::endl;
                exit(EXIT_FAILURE);
            }
            arg_recursion = true;
        }
        else if(strcmp(argv[i], "-x") == 0)
        {
            if(arg_reverse == true)
            {
                std::cerr << "Argument -x již byl použit." << std::endl;
                exit(EXIT_FAILURE);
            }
            arg_reverse = true;
        }
        else if(strcmp(argv[i], "-6") == 0)
        {
            if(arg_quadA == true)
            {
                std::cerr << "Argument -6 již byl použit." << std::endl;
                exit(EXIT_FAILURE);
            }
            arg_quadA = true;
        }
        else if(strcmp(argv[i], "-s") == 0)
        {
            if(i + 1 < argc)
            {
                i++;
                server_name = argv[i];
                has_server = true;
            }
            else
            {
                std::cerr << "Nebyl zadán server." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if(strcmp(argv[i], "-p") == 0)
        {
            if(arg_port == true)
            {
                std::cerr << "Argument -p již byl použit." << std::endl;
                exit(EXIT_FAILURE);
            }
            arg_port = true;
            if(i + 1 < argc)
            {
                i++;
                ip_port = atoi(argv[i]);
            }
            else
            {
                std::cerr << "Nebyl zadán port." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            ip_name = argv[i];
        }
    }
    if(ip_name.empty() == true)
    {
        std::cerr << "Není nastavena žádná adresa k rezoluci.";
        exit(EXIT_FAILURE);
    }
    if(has_server == false)
    {
        std::cerr << "Není nastaven žádný DNS server.";
        exit(EXIT_FAILURE);
    }
    // server_name je třeba rezolvovat (je ve tvaru domain name)
    bool isServer = true;
    struct in_addr tmp_buffer;
    struct in6_addr tmp_buffer6;
    std::vector<char> response;
    DNS_header* dnsResponse;
    char* reader;
    if(inet_pton(AF_INET, server_name.c_str(), &tmp_buffer) != 1 && inet_pton(AF_INET6, server_name.c_str(), &tmp_buffer6) != 1)
    {
        response = DNS_query("1.1.1.1", server_name, ip_port, header, arg_reverse, isServer, arg_quadA, arg_recursion);
        dnsResponse = reinterpret_cast<DNS_header*>(response.data());
        reader = response.data() + sizeof(DNS_header);
        while (*reader != 0) reader++;
        reader += 5;
        DNS_Record record;
        for(int ans = 0; ans < ntohs(dnsResponse->DNS_ANCOUNT); ans++) 
        {
            record = parseDNS_Record(reader, response);
        }
        server_name = record.rdata;
    }
    isServer = false;

    // rezoluce hledané adresy
    std::vector<char> response2 = DNS_query(server_name, ip_name, ip_port, header, arg_reverse, isServer, arg_quadA, arg_recursion);

    DNS_header* resp_header = reinterpret_cast<DNS_header*>(response2.data());

    // zpracování flagů authority, recursive a truncated pro výstup
    bool aa = ntohs(resp_header->DNS_FLAGS) & (1 << 10);
    bool tc = ntohs(resp_header->DNS_FLAGS) & (1 << 9);
    bool rd = ntohs(resp_header->DNS_FLAGS) & (1 << 8);

    std::cout << "Authoritative: " << (aa ? "Yes" : "No") << ", ";
    std::cout << "Recursive: " << (rd ? "Yes" : "No") << ", ";
    std::cout << "Truncated: " << (tc ? "Yes" : "No") << std::endl;


    dnsResponse = reinterpret_cast<DNS_header*>(response2.data());
    reader = response2.data() + sizeof(DNS_header);

    // výpis question sectionu
    char* header_reader = response2.data();
    header_reader += 4;
    uint32_t numQ;
    memcpy(&numQ,header_reader, 2);
    numQ = ntohs(numQ);
    std::cout << "Question section (" << numQ << ")" << std::endl;
    header_reader += 2;

    uint32_t numA;
    memcpy(&numA, header_reader, 2);
    numA = ntohs(numA);
    header_reader += 2;

    uint32_t numAut;
    memcpy(&numAut, header_reader, 2);
    numAut = ntohs(numAut);
    header_reader += 2;

    uint32_t numAdd;
    memcpy(&numAdd, header_reader, 2);
    numAdd = ntohs(numAdd);

    while(*reader != 0) reader++;
    reader += 1;
    uint32_t typeQ;
    memcpy(&typeQ, reader, 2);
    typeQ = ntohs(typeQ);
    reader += 2;

    uint32_t classQ;
    memcpy(&classQ, reader, 2);
    classQ = ntohs(classQ);
    reader += 2;

    std::cout << "  " << ip_name << ".";
    switch(typeQ)
    {
        case 1: std::cout << ", A";
        break;
        case 12: std::cout << ", PTR";
        break;
        case 28: std::cout << ", AAAA";
        break;
    }
    switch(classQ)
    {
        case 1: std::cout << ", IN";
        break;
    }
    std::cout << std::endl;

    // výpis answer sectionu
    std::cout << "Answer section (" << numA << ")" << std::endl;

    for(uint32_t ans = 0; ans < numA; ans++) 
    {
        DNS_Record record = parseDNS_Record(reader, response2);

        std::cout << "  " << record.name << ".";
        switch(record.type)
        {
            case 1: std::cout << ", A";
            break;
            case 2: std::cout << ", NS";
            break;
            case 5: std::cout << ", CNAME";
            break;
            case 6: std::cout << ", SOA";
            break;
            case 12: std::cout << ", PTR";
            break;
            case 15: std::cout << ", MX";
            break;
            case 28: std::cout << ", AAAA";
            break;
        }
        switch(record.dnsclass)
        {
            case 1: std::cout << ", IN";
            break;
        }
        std::cout << ", " << std::dec << record.ttl;
        if(record.type == 2 || record.type == 5 || record.type == 12)
        {
            std::cout << ", " << record.rdata << "." << std::endl;
        }
        else
        {
            std::cout << ", " << record.rdata << std::endl;
        }
    }

    // výpis authority sectionu
    std::cout << "Authority section (" << numAut << ")" << std::endl;

    for(uint32_t ans = 0; ans < numAut; ans++) 
    {
        DNS_Record record = parseDNS_Record(reader, response2);

        std::cout << "  " << record.name << ".";
        switch(record.type)
        {
            case 1: std::cout << ", A";
            break;
            case 2: std::cout << ", NS";
            break;
            case 5: std::cout << ", CNAME";
            break;
            case 6: std::cout << ", SOA";
            break;
            case 12: std::cout << ", PTR";
            break;
            case 15: std::cout << ", MX";
            break;
            case 28: std::cout << ", AAAA";
            break;
        }
        switch(record.dnsclass)
        {
            case 1: std::cout << ", IN";
            break;
        }
        std::cout << ", " << std::dec << record.ttl;
        if(record.type == 2 || record.type == 5 || record.type == 12)
        {
            std::cout << ", " << record.rdata << "." << std::endl;
        }
        else
        {
            std::cout << ", " << record.rdata << std::endl;
        }
    
    }

    // výpis additional sectionu
    std::cout << "Additional section (" << numAdd << ")" << std::endl;

    for(uint32_t ans = 0; ans < numAdd; ans++) 
    {
        DNS_Record record = parseDNS_Record(reader, response2);

        std::cout << "  " << record.name << ".";
        switch(record.type)
        {
            case 1: std::cout << ", A";
            break;
            case 2: std::cout << ", NS";
            break;
            case 5: std::cout << ", CNAME";
            break;
            case 6: std::cout << ", SOA";
            break;
            case 12: std::cout << ", PTR";
            break;
            case 15: std::cout << ", MX";
            break;
            case 28: std::cout << ", AAAA";
            break;
        }
        switch(record.dnsclass)
        {
            case 1: std::cout << ", IN";
            break;
        }
        std::cout << ", " << std::dec << record.ttl;
        if(record.type == 2 || record.type == 5 || record.type == 12)
        {
            std::cout << ", " << record.rdata << "." << std::endl;
        }
        else
        {
            std::cout << ", " << record.rdata << std::endl;
        }
    }

    return 0;


}