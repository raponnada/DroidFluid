#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
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
FILE *fp;

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
    //fprintf(stdout, "Before ip print");

    sprintf(tnet, "%s", iface->ip);
    a = strtok (tnet, "."); /* 1st ip octect */
    b = strtok (NULL, "."); /* 2nd ip octect */
    c = strtok (NULL, "."); /* 3rd ip octect */
    d = strtok (NULL, "."); /* 4th ip octect */

    sprintf( net, "%s.%s.%s", a, b, c);

    //iterate over all arguments
    for(i = 2; i <= 254; i++){

        if (i == atoi(d)){
            continue;
        }
        sprintf(toip,"%s.%i", net, i);
        //fprintf(stdout, "ipaddress: %s\n", toip);
        inet_pton (AF_INET, toip, arp_header->dip);
        if(n = sendto(sockfd,&eth_frame, 64, 0, (struct sockaddr *) &device, sizeof(device)) <= 0){
            fprintf(stdout, "failed to send \n");
           // fprintf(stdout, "error Description : %s \n", strerror(errno));
            return;
        }
        //usleep(2 * 1000);
        //fprintf(stdout, "Iterate over all arguments");
    }
    return;
}

/*
* This following funtion is responsible for
* iterating through all active interfaces and calling arp_gen.
*/
void * inject_arp(void *)
{
    iFace p = iface;
    fprintf(stdout,"\n  Index: %d\tName: %s\tip: %s\tmac: %02x:%02x:%02x:%02x:%02x:%02x\n",
           iface->index, iface->name, iface->ip, iface->mac[0], iface->mac[1],
           iface->mac[2], iface->mac[3], iface->mac[4], iface->mac[5]);
    arp_gen(p);

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
    char ip_addrs[16];
    struct sockaddr from;
    int iterate = 0;

    fp = fopen("/sdcard/Dfluid/devices.txt","w");
    arp_rply = (struct arp_hdr *)((struct packet*)(buffer+14));
    sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",
                    iface->mac[0], iface->mac[1],
                    iface->mac[2], iface->mac[3],
                    iface->mac[4], iface->mac[5]);
    fprintf(fp, "%s %s\n",iface->ip, mac);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while(iterate < 5){

        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof tv))
        { perror("setsockopt"); }

        r = recv(sockfd, buffer, sizeof(buffer), 0);
        if (r < 0) {
            iterate++;
            continue;
        }

        if(((((buffer[12])<<8)+buffer[13])!=ETH_P_ARP) && ntohs(arp_rply->op)!=2)
            continue;
        sprintf (ip_addrs, "%u.%u.%u.%u",arp_rply->sip[0], arp_rply->sip[1],
                 arp_rply->sip[2], arp_rply->sip[3]);
        fprintf(stdout, "%s \t", ip_addrs);
        sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",
                arp_rply->smac[0], arp_rply->smac[1],
                arp_rply->smac[2], arp_rply->smac[3],
                arp_rply->smac[4], arp_rply->smac[5]);
        fprintf(stdout,"%s\n", mac);
        if(ip_addrs != (iface->ip)){
            fprintf(fp, "%s %s\n",ip_addrs, mac);
        }
        iterate++;
    }

    fclose(fp);
    close(sockfd);
    exit(0);
}


int main(int argc, char* argv[])
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

    if ((sockfd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
        fprintf (stdout,"socket() failed\n ");
        return -1;
    }

    p = pthread_create(&pt2, NULL, &inject_arp, NULL);
    s = pthread_create(&pt1, NULL, &arp_sniff, NULL);

    pthread_join(pt1,NULL);

    //close(sockfd);
    return 0;
}
