#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>

#define ETH_ALEN	6		//Octets in one ethernet address
#define ETH_HLEN	14		//Total octets in heater
#define	ETH_FRAME_LEN	1514		//Max. octets in fram sans FCS

struct arp_hdr{
    unsigned short 	hrd;		        //hardware address type
    unsigned short 	proto;			//protocol adress type
    unsigned char 	hrd_add_len;		//hardware address length
    unsigned char 	proto_add_len;		//Protokoll adress length
    unsigned short 	op;			//Operation
    unsigned char 	smac[ETH_ALEN];		//source MAC (Ethernet Address)
    unsigned char 	sip[4];			//source IP
    unsigned char 	dmac[ETH_ALEN];		//destination MAC (Ethernet Address)
    unsigned char 	dip[4];			//destination IP
    char 		pad[18];		//Padding, ARP-Requests are quite small (<64)
};

/* Global Structures */
struct interface{
    int index;
    char name[10];
    char ip[16];
    unsigned char mac[6];
};

typedef struct interface *iFace;
iFace iface = NULL;

/* Global variables */
int sockfd;
int n=0, r=0;
int sniff = 1;

/*
 * Function to handle CTRL+C for exiting
 */
void signal_callback_handler(int signum)
{
    sniff = 0;
}

/*
* This function builds arp packets and send them to all host in network.
*/
void arp_gen(iFace inf)
{
    struct arp_hdr *arp_header;                             //build up the arp packet
    char eth_frame[ETH_FRAME_LEN];				// ethernet packet
    struct ethhdr *eth_header;				//build up ethernet header, from if_ether.h
    char dmac[ETH_ALEN]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};    //Ethernet dest. Address
    struct sockaddr_ll device;
    iFace intf = inf;

    device.sll_ifindex = intf->index;
    device.sll_family = AF_PACKET;
    memcpy (device.sll_addr, intf->mac, 6 * sizeof (uint8_t));
    device.sll_halen = htons(6);

    eth_header = (struct ethhdr*)eth_frame;                 //build up the ethernet packet
    memcpy(eth_header->h_dest, dmac, ETH_ALEN);
    eth_header->h_proto=htons(0x0806);			//0x0806 for Address Resolution Packet
    memcpy(eth_header->h_source,iface->mac,6);

    arp_header = (struct arp_hdr *)(eth_frame + ETH_HLEN);  //start address in mem
    arp_header->hrd = htons(0x0001);                        //0x0001 for 802.3 Frames
    arp_header->proto = htons (0x0800);
    arp_header->hrd_add_len = ETH_ALEN;                     // 6 for eth-mac addr
    arp_header->proto_add_len = 4;                          //4 for IPv4 addr
    arp_header->op = htons(0x0001);				//0x0001 for ARP Request
    inet_pton (AF_INET, iface->ip, arp_header->sip);
    memcpy(arp_header->smac,iface->mac,6);
    memcpy(arp_header->dmac,dmac,ETH_ALEN);			//Set destination mac in arp-header
    bzero(arp_header->pad,18);				//Zero fill the packet until 64 bytes reached
    char *a, *b, *c, *d;
    char *tnet, *net, *toip;
    int i;

    net = (char *) malloc ((sizeof(char)) * 16);
    tnet = (char *) malloc ((sizeof(char)) * 16);
    toip = (char *) malloc ((sizeof(char)) * 16);

    fprintf(stdout, tnet, "%s", iface->ip);
    a = strtok (tnet, "."); /* 1st ip octect */
    b = strtok (NULL, "."); /* 2nd ip octect */
    c = strtok (NULL, "."); /* 3rd ip octect */
    d = strtok (NULL, "."); /* 4th ip octect */

    fprintf(stdout, net, "%s.%s.%s", a, b, c);

    //iterate over all arguments
    for(i = 1; i <= 255; i++){
        fprintf(stdout, toip,"%s.%i", net, i);
        inet_pton (AF_INET, toip, arp_header->dip);
        if(n = sendto(sockfd,&eth_frame, 64, 0, (struct sockaddr *) &device, sizeof(device)) <= 0){
            fprintf(stdout, "failed to send \n");
            fprintf(stdout, "error Description : %s \n", strerror(errno));
            return;
        }
        usleep(2 * 1000);
    }
    return;
}

/*
* This following funtion is responsible for
* iterating through all active interfaces and calling arp_gen.
*/
void * inject_arp(void *)
{
    //void *return_void = NULL;
    iFace p = iface;
    fprintf(stdout,"\n      Network Active Hosts Scanner\n\t Press CTRL+C to exit");

    fprintf(stdout,"\n  Index: %d\tName: %s\tip: %s\tmac: %02x:%02x:%02x:%02x:%02x:%02x\n",
           iface->index, iface->name, iface->ip, iface->mac[0], iface->mac[1],
           iface->mac[2], iface->mac[3], iface->mac[4], iface->mac[5]);
    fprintf(stdout,"________________________________________________________________________\n");
    fprintf(stdout,"   IP\t\t\tMAC\t\t\tMAC VENDOR\n");
    fprintf(stdout,"------------------------------------------------------------------------\n");
    arp_gen(p);
}
/*
 * Function to query vendor name online.
 * It requires active internet.
 */
const char *get_vendor(char *v)
{
    struct addrinfo *result, hints;
    int srvfd, rwerr = 42;
    char *request, buf[16], port[6],mac[26],*response;
    const char *delim1 = "<company>";
    char *vendor;
    const char *unknown = "unknown or no internet access";

    memset(port,0,6);
    strncpy(port,"80",2);
    memset(mac,'\0',26);

    fprintf(stdout, mac,"%s%s","/api/AvBxNyO/", v);

    memset(&hints,0,sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ( 0 != getaddrinfo("www.macvendorlookup.com", port,&hints,&result))
        return unknown;

    // Create socket after retrieving the inet protocol to use (getaddrinfo)
    srvfd = socket(result->ai_family,SOCK_STREAM,0);
    if ( srvfd < 0 )
        return unknown;

    if ( connect(srvfd,result->ai_addr,result->ai_addrlen) == -1)
        return unknown;

    request = (char *) calloc(53+23+25, 1);
    response = (char *) calloc(1024, 1);
    memset(response,'\0',1024);

    fprintf(stdout, request,"GET %s HTTP/1.1\nHost: %s\nUser-agent: simple-http client\n\n",
            mac, "www.macvendorlookup.com");

    write(srvfd,request,strlen(request));

    shutdown(srvfd,SHUT_WR);

    while ( rwerr > 0 )
    {
        rwerr = read(srvfd,response,1024);
    }

    if((vendor = strstr(response, delim1)) == NULL)
        return unknown;
    vendor+=9;
    vendor = strtok(vendor,"<");
    close(srvfd);
    return vendor;
}

/*
 * Poll wlan0 interface and gather its fllowing properties
 *             hardware address (mac)
 *             ip address
 *             interface name
 */
int pollWlan()
{
    iFace intf = NULL;
    /*int  iSocket = -1;
    if ((iSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
       perror("socket");
       return -1;
    }*/

    struct ifreq ifr;
    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) && !ifr.ifr_flags & (IFF_UP|IFF_RUNNING))
        return -1;

    intf = (struct interface *)malloc(sizeof(struct interface));
    memcpy(intf->name, "wlan0", sizeof("wlan0"));

    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
    {
        if (errno == EADDRNOTAVAIL)
        {
            fprintf(stdout,"\tN/A\n");
        }
        perror("ioctl");
        return -1;
    }
    memcpy(intf->ip, inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr), 16);

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0){
        fprintf(stdout,"ioctl SIOCGIFHWADDR failed\n");
        return -1;
    }
    memcpy(intf->mac, ifr.ifr_hwaddr.sa_data, 6);

    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0){
        fprintf(stdout, "ioctl SIOCGIFINDEX failed\n");
        return -1;
    }
    intf->index = ifr.ifr_ifindex;
    iface = (struct interface *)malloc(sizeof(struct interface));
    iface = intf;

    return 0;
}


/*
* A simple arp sniffer function.
*/
void *arp_sniff(void *)
{
    char buffer[65535];
    struct arp_hdr *arp_rply;
    char mac[20];
    struct sockaddr from;
    char vendor[20];
    int iterate = 0;
    arp_rply = (struct arp_hdr *)((struct packet*)(buffer+14));

    while(iterate < 7){
        r = recv(sockfd, buffer, sizeof(buffer), 0);
        if(((((buffer[12])<<8)+buffer[13])!=ETH_P_ARP) && ntohs(arp_rply->op)!=2)
            continue;

        fprintf(stdout, "%u.%u.%u.%u\t",
               arp_rply->sip[0], arp_rply->sip[1],
               arp_rply->sip[2], arp_rply->sip[3]);

        fprintf(stdout, mac,"%02x:%02x:%02x:%02x:%02x:%02x",
                arp_rply->smac[0], arp_rply->smac[1],
                arp_rply->smac[2], arp_rply->smac[3],
                arp_rply->smac[4], arp_rply->smac[5]);
        fprintf(stdout,"%s\t", mac);

        fprintf(stdout, vendor,"%02x%02x%02x%02x%02x%02x",
                arp_rply->smac[0], arp_rply->smac[1],
                arp_rply->smac[2], arp_rply->smac[3],
                arp_rply->smac[4], arp_rply->smac[5]);
        fprintf(stdout,"%s\n", get_vendor(vendor));
        iterate++;
    }
    close(sockfd);
    exit(0);
}


int arpScan(void)
{
    int p,s;
    pthread_t pt1, pt2;

    /* Check for uid 0 */
    if ( getuid() && geteuid() )
    {
        fprintf(stdout,"You must be root to run this.\n");
        exit(1);
    }

    signal(SIGINT, signal_callback_handler);

    if((sockfd = socket(AF_INET, SOCK_RAW, htons(IPPROTO_RAW))) < 0){
        fprintf(stdout,"socket() failed1\n");
        return -1;
    }
    if(pollWlan() < 0){
        fprintf(stdout,"wlan interface poll failed\n");
        return -1;
    }
    close(sockfd);

    if ((sockfd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
        fprintf (stdout,"socket() failed\n ");
        return -1;
    }
    s = pthread_create(&pt1, NULL, &arp_sniff, NULL);
    p = pthread_create(&pt2, NULL, &inject_arp, NULL);
    pthread_join(pt1,NULL);

    close(sockfd);
    return 0;
}
