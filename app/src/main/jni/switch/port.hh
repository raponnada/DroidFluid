#ifndef PORT_H
#define PORT_H 1

#include <pcap.h>
#include <string>
#include "datapath.hh"
#include <netinet/ip.h>
#include <net/ethernet.h>

struct pcap_datapath{
	Datapath **dp;
	uint16_t port_no;
	pcap_t* pcap_;	
};

class SwPort{
	private:
		int port_no_;
		std::string name_;
		pcap_t* pcap_;

	public:
		SwPort(int port_no, std::string name);						
		void* open();
		void close();		
		void port_output_packet(uint8_t *pkt, size_t pkt_len);		
		static void* pcap_capture(void* arg);					
		static void packet_handler(u_char *user, const struct pcap_pkthdr* pkthdr,
				const u_char* packet);

		int port_no(){return this->port_no_;}		
		std::string name(){return this->name_;}
		pcap_t* pcap(){return this->pcap_;}
		void port_no(int port_no){this->port_no_ = port_no;}
		void name(std::string name){this->name_ = name;}	 
		void pcap(pcap_t *pcap){this->pcap_ = pcap;}

    private:
        static void show_ip_packet_info(const u_char* packet)
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

        static void modify_ip_packet(const u_char* packet)
        {
            struct ether_header* etherHdr = (struct ether_header*) packet;

            if (ntohs(etherHdr->ether_type) == ETHERTYPE_IP) {
                fprintf(stdout, "Original packet: ");
                show_ip_packet_info(packet);

                // Change source ip address
                struct ip* ipPtr = (struct ip*)(packet + sizeof(struct ether_header));

                time_t now_t = time(0);
                struct tm * now = localtime(&now_t);

                int orgSrcIp = inet_network(inet_ntoa(ipPtr->ip_src));
                uint32_t genSrcIp = (orgSrcIp & 0xFFFFFF00) | now->tm_min;

                struct in_addr addr;
                addr.s_addr = htonl(genSrcIp);
                inet_aton(inet_ntoa(addr), &ipPtr->ip_src);

                // Change tos
                ipPtr->ip_tos = 7;

                // Update checksum
                ipPtr->ip_sum = compute_ip_checksum((struct iphdr*) (packet + sizeof(struct ether_header)));

                fprintf(stdout, "Modified packet: ");
                show_ip_packet_info(packet);
            }
        }

        static unsigned short csum(unsigned short *addr, unsigned int count)
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

        static unsigned short compute_ip_checksum(struct iphdr* iphdr)
        {
            iphdr->check = 0;
            return csum((unsigned short*) iphdr, iphdr->ihl << 2);
        }
};

#endif