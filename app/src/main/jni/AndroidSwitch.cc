#include <pcap.h>
#include "switch/Switch.hh"

void get_leader_ip(char *ip, int len)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in oracle;

    oracle.sin_family = AF_INET;
    oracle.sin_port = htons(9000);
    oracle.sin_addr.s_addr = inet_addr("127.0.0.1");

    char sendBuff[] = "LEADER?";

    socklen_t slen = sizeof(oracle);

    sendto(sockfd, sendBuff, sizeof(sendBuff), 0, (struct sockaddr *)&oracle, sizeof(oracle));
    int recvCount = recvfrom(sockfd, ip, len, 0,  (struct sockaddr *)&oracle, &slen);

    fprintf(stdout, "%s\n", ip);
    fflush(stdout);
}

int main(int argc, char* argv[])
{
    char leader_ip[1024];

    get_leader_ip(leader_ip, 1024);

    std::vector<std::string> ports;
    ports.push_back("wlan0");

    Switch *sw = new Switch(0, &leader_ip, 6653, ports);

    sw->start();
    wait_for_sigint();
    sw->stop();

    delete sw;
    return 0;
}
