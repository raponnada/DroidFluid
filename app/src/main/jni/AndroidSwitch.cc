#include <pcap.h>
#include <netinet/ip.h>
#include "switch/Switch.hh"

unsigned short compute_checksum(unsigned short *addr, unsigned int count)
{
    unsigned long sum = 0;

    while (count > 1) {
        sum += *addr++;
        count -= 2;
    }
    if (count > 0) {
        sum += ((*addr) & htons(0xFF00));
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    sum = ~sum;

    return ((unsigned short) sum);
}

unsigned short compute_ip_checksum(struct iphdr* iphdr)
{
    iphdr->check = 0;
    return compute_checksum((unsigned short*) iphdr, iphdr->ihl << 2);
}

void show_ip_packet_info(const u_char* packet)
{
    struct ether_header* etherHdr = (struct ether_header*) packet;

    fprintf(stdout, "ethernet MacSrc: %s"
            , ether_ntoa((const struct ether_addr*)etherHdr->ether_shost));
    fprintf(stdout, "  MacDst: %s"
            , ether_ntoa((const struct ether_addr*)etherHdr->ether_dhost));

    struct ip* ipPtr = (struct ip*)(packet + sizeof(struct ether_header));

    fprintf(stdout, " IpSrc: %s", inet_ntoa(ipPtr->ip_src));
    fprintf(stdout, " IpDst: %s", inet_ntoa(ipPtr->ip_dst));
    fprintf(stdout, " Sum: %d", ipPtr->ip_sum);
    fprintf(stdout, " ToS: %d", ipPtr->ip_tos);

    fprintf(stdout, "\n");
    fflush(stdout);
}

u_int16_t handle_ethernet(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char* packet)
{
    struct ether_header* etherHdr = (struct ether_header*) packet;

    if (ntohs(etherHdr->ether_type) == ETHERTYPE_IP) {
        fprintf(stdout, "Change ToS and source IP address\n");
        fprintf(stdout, "Original packet: ");
        show_ip_packet_info(packet);

        struct ip* ipPtr = (struct ip*)(packet + sizeof(struct ether_header));

        int orgSrcIp = inet_network(inet_ntoa(ipPtr->ip_src));

        time_t now_t = time(0);
        struct tm * now = localtime(&now_t);

        uint32_t genSrcIp = (orgSrcIp & 0xFFFFFF00) | now->tm_min;

        struct in_addr addr;
        addr.s_addr = htonl(genSrcIp);

        // Update Tos, Source ip, and checksum
        ipPtr->ip_tos = 7;
        inet_aton(inet_ntoa(addr), &ipPtr->ip_src);
        ipPtr->ip_sum = compute_ip_checksum((struct iphdr*) (packet + sizeof(struct ether_header)));

        fprintf(stdout, "Modified packet: ");
        show_ip_packet_info(packet);
    }

    return etherHdr->ether_type;
}

void packet_handler_callback(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char* packet)
{
    handle_ethernet(args, pkthdr, packet);
    fflush(stdout);
}

int main( int argc, char* argv[])
{
    std::vector<std::string> ports;
    ports.push_back("wlan0");

    Switch *sw = new Switch(0, "127.0.0.1", 6653, ports);

    sw->start();
    wait_for_sigint();
    sw->stop();

    delete sw;
    return 0;
//    char errbuf[PCAP_ERRBUF_SIZE];
//
//    pcap_t *descr = pcap_open_live("wlan0", BUFSIZ, -1, -1, errbuf);
//
//    if (descr == NULL) {
//        fprintf(stdout, "Error: %s", errbuf);
//        return 1;
//    }
//
//    pcap_loop(descr, -1, packet_handler_callback, NULL);
//
//    return 0;
}
