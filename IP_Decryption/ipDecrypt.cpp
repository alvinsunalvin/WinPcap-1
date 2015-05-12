#include <stdio.h>
#include<ctime>
#include <iostream>
#include "pcap.h"
#include "remote-ext.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "wpcap.lib")
using namespace std;

// ��̫��Э���ʽ�Ķ���
typedef struct ether_header {
	u_char ether_dhost[6];		// Ŀ���ַ
	u_char ether_shost[6];		// Դ��ַ
	u_short ether_type;			// ��̫������
}ether_header;

// �û�����4�ֽڵ�IP��ַ
typedef struct ip_address {
	u_char byte1;
	u_char byte2;
	u_char byte3;
	u_char byte4;
}ip_address;

// ���ڱ���IPV4���ײ�
typedef struct ip_header {
	u_char ver_ihl;		// �汾�Լ��ײ����ȣ���4λ
	u_char tos;		// ��������
	u_short tlen;		// �ܳ���
	u_short identification;	// ���ʶ��
	u_short flags_offset;	// ����ƫ��
	u_char ttl;		// ��������
	u_char proto;		// Э������
	u_short crc;		// ��ͷ������
	ip_address saddr;	// ԴIP��ַ
	ip_address daddr;	// Ŀ��IP��ַ
	u_int op_pad;		//��ѡ ����ֶ�
}ip_header;

// ����TCP�ײ�
typedef struct tcp_header {
	u_short sport;
	u_short dport;
	u_int sequence;		// ������
	u_int ack;					// �ظ���

#ifdef WORDS_BIGENDIAN
	u_char offset : 4, reserved : 4;		// ƫ�� Ԥ��
#else
	u_char reserved : 4, offset : 4;		// Ԥ�� ƫ��
#endif
	u_char flags;				// ��־
	u_short windows;			// ���ڴ�С
	u_short checksum;			// У���
	u_short urgent_pointer;		// ����ָ��
}tcp_header;

typedef struct udp_header {
	u_int32_t sport;			// Դ�˿�
	u_int32_t dport;			// Ŀ��˿�
	u_int8_t zero;				// ����λ
	u_int8_t proto;				// Э���ʶ
	u_int16_t datalen;			// UDP���ݳ���
}udp_header;

typedef struct icmp_header {
	u_int8_t type;				// ICMP����
	u_int8_t code;				// ����
	u_int16_t checksum;			// У���
	u_int16_t identification;	// ��ʶ
	u_int16_t sequence;			// ���к�
	u_int32_t init_time;		// ����ʱ���
	u_int16_t recv_time;		// ����ʱ���
	u_int16_t send_time;		// ����ʱ���
}icmp_header;

typedef struct arp_header {
	u_int16_t arp_hardware_type;
	u_int16_t arp_protocol_type;
	u_int8_t arp_hardware_length;
	u_int8_t arp_protocol_length;
	u_int16_t arp_operation_code;
	u_int8_t arp_source_ethernet_address[6];
	u_int8_t arp_source_ip_address[4];
	u_int8_t arp_destination_ethernet_address[6];
	u_int8_t arp_destination_ip_address[4];
}arp_header;

void tcp_protocol_packet_handle(u_char *argument,const struct pcap_pkthdr *packet_header,const u_char *packet_content) {
	struct tcp_header *tcp_protocol;
	u_short sport;
	u_short dport;
	int header_length;
	u_short windows;
	u_short urgent_pointer;
	u_int sequence;
	u_int acknowledgement;
	u_short checksum;
	u_char flags;

	printf("\t\t TCP Protocol \n");

	tcp_protocol = (struct tcp_header*)(packet_content + 14 + 20);
	sport = ntohs(tcp_protocol->sport);
	dport = ntohs(tcp_protocol->dport);
	header_length = tcp_protocol->offset * 4;
	sequence = ntohl(tcp_protocol->sequence);
	acknowledgement = ntohl(tcp_protocol->ack);
	windows = ntohs(tcp_protocol->windows);
	urgent_pointer = ntohs(tcp_protocol->urgent_pointer);
	flags = tcp_protocol->flags;
	checksum = ntohs(tcp_protocol->checksum);

	printf( "\t\t Header Length:%d, Source Port:%d, Destination Port:%d Flags:%c, Windows:%d", header_length, sport, dport, flags, windows);

	if (flags & 0x08) printf("PSH");
	if (flags & 0x10) printf("ACK");
	if (flags & 0x02) printf("SYN");
	if (flags & 0x20) printf("URG");
	if (flags & 0x01) printf("FIN");
	if (flags & 0x04) printf("RST");
	printf("\n");
}

void udp_protocol_packet_handle(u_char *argument,const struct pcap_pkthdr *packet_header,const u_char *packet_content) {
	struct udp_header* udp_protocol;
	u_short sport;
	u_short dport;
	u_short datalen;

	printf("\t\t UDP Protocol \n");
	udp_protocol = (struct udp_header*)(packet_content + 14 + 20);
	sport = ntohs(udp_protocol->sport);
	dport = ntohs(udp_protocol->dport);
	datalen = ntohs(udp_protocol->datalen);

	printf("\t\t Data Length:%d, Source Port:%d, Destination Port:%d\n", datalen, sport, dport);
}

void arp_protocol_packet_handle(u_char *argument,const struct pcap_pkthdr *packet_header,const u_char *packet_content) {
	struct arp_header *arp_protocol;
	u_short protocol_type;
	u_short hardware_type;
	u_short operation_code;
	u_char hardware_length;
	u_char protocol_length;
	struct tm ltime;
	char timestr[16];
	time_t local_tv_sec;
	local_tv_sec = packet_header->ts.tv_sec;
	int err = localtime_s(&ltime, &local_tv_sec);
	strftime(timestr, sizeof(timestr), "%H:%M:%S", &ltime);

	printf("\t\t ARP Protocol Decryption: ");
	arp_protocol = (struct arp_header*)(packet_content + 14);
	hardware_type = ntohs(arp_protocol->arp_hardware_type);
	protocol_type = ntohs(arp_protocol->arp_protocol_type);
	operation_code = ntohs(arp_protocol->arp_operation_code);
	hardware_length = arp_protocol->arp_hardware_length;
	protocol_length = arp_protocol->arp_protocol_length;

	switch (operation_code){
	case 1:printf("ARP����Э��\n");break;
	case 2:printf("ARPӦ��Э��\n");break;
	case 3:printf("RARP����Э��\n");break;
	case 4:printf("RARPӦ��Э��\n");break;
	default:break;
	}
	printf("\tProtocol Length : %d\t Time : %s\t", protocol_length, timestr);
}

void icmp_protocol_packet_handle(u_char *argument,const struct pcap_pkthdr *packet_header,const u_char *packet_content) {
	struct icmp_header *icmp_protocol;
	u_short type;
	u_short datalen;
	u_int init_time;
	u_int recv_time;
	u_int send_time;

	icmp_protocol = (struct icmp_header*)(packet_content + 14 + 20);
	datalen = sizeof(icmp_protocol);
	type = icmp_protocol->type;
	init_time = icmp_protocol->init_time;
	recv_time = icmp_protocol->recv_time;
	send_time = icmp_protocol->send_time;

	printf("\t\t ICMP Protocol Decryption:\t ");
	switch (icmp_protocol->type) {
	case 8:printf("Echo Request\n"); break;
	case 0:printf("Echo Reply\n"); break;
	default:break;
	}
	printf("\t\t Data Length:%d\t Type:%c\t Init Time:%d\t Receive\t Time:%d\t send time:%d\n", datalen, type, init_time, recv_time, send_time);
}

void ip_protocol_packet_handle(u_char *argument,const struct pcap_pkthdr *packet_header,const u_char *packet_content) {
	struct ip_header *ip_protocol;
	u_int header_length;
	u_char tos;
	u_short checksum;

	ip_address saddr;
	ip_address daddr;
	u_char ttl;
	u_short tlen;
	u_char proto;   //protocol type
	u_short identification;
	u_short offset;

	printf("\t IP Protocol Decryption:\n");

	ip_protocol = (struct ip_header*)(packet_content + 14);
	checksum = ntohs(ip_protocol->crc);
	tos = ip_protocol->tos;
	proto = ip_protocol->proto;
	offset = ntohs(ip_protocol->flags_offset);

	saddr = ip_protocol->saddr;
	daddr = ip_protocol->daddr;
	ttl = ip_protocol->ttl;
	identification = ip_protocol->identification;
	tlen = ip_protocol->tlen;
	offset = ip_protocol->flags_offset;

	printf("\t Source Address: %d.%d.%d.%d\n\t Destination Address: %d.%d.%d.%d\n",saddr.byte1,saddr.byte2,saddr.byte3,saddr.byte4,
		daddr.byte1,daddr.byte2,daddr.byte3,daddr.byte4);
	printf("\t TTL:%d\t Total Length:%d\t Offset:%d\n",ttl, tlen, offset);

	switch (ip_protocol->proto) {
	case 1:icmp_protocol_packet_handle(argument, packet_header, packet_content); break;
	case 2:printf("\t\t IGMP Protocol\n"); break;
	case 6:tcp_protocol_packet_handle(argument, packet_header, packet_content);break;
	case 17:udp_protocol_packet_handle(argument, packet_header, packet_content);break;
	default:
		break;
	}
}


void ethernet_protocol_packet_handle(u_char *argument,const struct pcap_pkthdr *packet_header,const u_char *packet_content) {
	u_short ethernet_type;		// ��̫������
	struct ether_header *ethernet_protocol;		// ��̫��Э�����
	u_char *mac_string;			// ��̫����ַ

	ethernet_protocol = (struct ether_header*)packet_content;		// ��ȡ��̫����������
	printf("Ethernet type is: ");
	ethernet_type = ntohs(ethernet_protocol->ether_type);	// ��ȡ��̫������
	//printf("	%04x     ", ethernet_type);

	switch (ethernet_type) {
	case 0x0800:printf("IPv4 protocol\n");break;
	case 0x86DD:printf("IPv6 protocol\n"); break;
	case 0x880B:printf("PPP protocol\n"); break;
	case 0x814C:printf("SNMP protocol\n"); break;
	case 0x0806:printf("ARP protocol\n");break;
	case 0x8863:printf("PPPoE protocol<Discovery Stage>\n"); break;
	case 0x8864:printf("PPPoE protocol<Session Stage>\n"); break;
	default:printf("UKNOWN protocol\n");
	}

	// ��ȡ��̫��Դ��ַ
	printf("\t MAC Source Address: ");
	mac_string = ethernet_protocol->ether_shost;

	printf("%02x:%02x:%02x:%02x:%02x:%02x\n", *mac_string,
		*(mac_string + 1), *(mac_string + 2), *(mac_string + 3),
		*(mac_string + 4), *(mac_string + 5));

	// ��ȡ��̫��Ŀ�ĵ�ַ
	printf("\t MAC Target Address: ");
	mac_string = ethernet_protocol->ether_dhost;
	printf("%02x:%02x:%02x:%02x:%02x:%02x\n",*mac_string,
		*(mac_string + 1),*(mac_string + 2),*(mac_string + 3),
		*(mac_string + 4),*(mac_string + 5));

	
	switch (ethernet_type) {
	case 0x0800:ip_protocol_packet_handle(argument, packet_header, packet_content);break;
	case 0x0806:arp_protocol_packet_handle(argument, packet_header, packet_content); break;
	default:
		break;
	}
}

int main() {
	pcap_if_t *alldevs;
	pcap_if_t *d;
	pcap_t *adhandle;
	char errbuf[PCAP_ERRBUF_SIZE];
	int inum;
	int i = 0;
	u_int netmask;
	int res;
	struct pcap_pkthdr *header;
	struct tm ltime;
	const u_char *pkt_data;
	time_t local_tv_sec;
	char timestr[16];
	ip_header *ih;

	// ����豸�б� 
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1) {
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		exit(1);
	}

	for (d = alldevs; d; d = d->next) {
		printf("%d. %s", ++i, d->name);
		if (d->description) {
			printf("(%s)\n", d->description);
		}
		else {
			printf("No description available\n");
		}
	}

	if (0 == i) {
		printf("\nNo interface found! Make sure WinPcap is installed\n");
		return -1;
	}

	printf("Enter the interface number(1-%d):", i);
	scanf_s("%d", &inum);
	if (inum < 1 || inum > i) {
		printf("\nInterface number out of range.\n");
		pcap_freealldevs(alldevs);
		return -1;
	}

	for (d = alldevs, i = 0; i < inum - 1; d = d->next, i++);
	// ��ת�����豸����������

	// �豸����Ҫ��׽�����ݰ��Ĳ��֣�65536��֤�ܲ��񵽲�ͬ������·���ϵ�ÿ�����ݰ���ȫ�����ݣ�������ģʽ����ȡ��ʱʱ�䣬���󻺳��
	if ((adhandle = pcap_open_live(d->name, 65536, 1, 1000, errbuf)) == NULL) {
		fprintf(stderr, "\nUnable to open the adapter.%s is not supported by WinPcap\n", errbuf);
		pcap_freealldevs(alldevs);
		return -1;
	}
	// ���������·�㣨ֻ��������̫����
	if (pcap_datalink(adhandle) != DLT_EN10MB) {
		fprintf(stderr, "\nThis program works only on Ethernet networks.\n");
		pcap_freealldevs(alldevs);
		return -1;
	}

	if (d->addresses != NULL) {
		// ��ýӿڵĵ�һ����ַ������
		netmask = ((struct sockaddr_in*)(d->addresses->netmask))->sin_addr.S_un.S_addr;
	}
	else 
		netmask = 0xffffff;
	


	while ((res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0) {
		// ����ʱ
		if (0 == res) {continue;}
		// �������ݰ�
		ethernet_protocol_packet_handle(NULL, header, pkt_data);
		// ��ʱ���ת���ɿ�ʶ��ĸ�ʽ
		local_tv_sec = header->ts.tv_sec;
		localtime_s(&ltime,&local_tv_sec);
		strftime(timestr, sizeof(timestr), "%H:%M:%S", &ltime);
		ih = (ip_header *)(pkt_data + 14); //��̫��ͷ������

		// ���ʱ���IP��Ϣ
		printf("\t Time: %s\n\t Header Length:%d \n", timestr, header->len);

		printf("\t Source Address: %d.%d.%d.%d\n\t Destination Address: %d.%d.%d.%d\n",
			ih->saddr.byte1,ih->saddr.byte2,ih->saddr.byte3,ih->saddr.byte4,
			ih->daddr.byte1,ih->daddr.byte2,ih->daddr.byte3,ih->daddr.byte4);
		printf("\n\n");
	}

	if (-1 == res) {
		printf("Error reading the packet:%s\n", pcap_geterr(adhandle));
		return -1;
	}
	pcap_freealldevs(alldevs);

	fclose(stdin);
	return 0;
}
