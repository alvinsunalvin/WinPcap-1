#include <stdio.h>
#include<ctime>
#include <iostream>
#include "pcap.h"
#include "remote-ext.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "wpcap.lib")
using namespace std;

// 以太网协议格式的定义
typedef struct ether_header {
	u_char ether_dhost[6];		// 目标地址
	u_char ether_shost[6];		// 源地址
	u_short ether_type;			// 以太网类型
}ether_header;

// 用户保存4字节的IP地址
typedef struct ip_address {
	u_char byte1;
	u_char byte2;
	u_char byte3;
	u_char byte4;
}ip_address;

// 用于保存IPV4的首部
typedef struct ip_header {
	u_char ver_ihl;		// 版本以及首部长度，各4位
	u_char tos;		// 服务质量
	u_short tlen;		// 总长度
	u_short identification;	// 身份识别
	u_short flags_offset;	// 分组偏移
	u_char ttl;		// 生命周期
	u_char proto;		// 协议类型
	u_short crc;		// 包头测验码
	ip_address saddr;	// 源IP地址
	ip_address daddr;	// 目的IP地址
	u_int op_pad;		//可选 填充字段
}ip_header;

// 保存TCP首部
typedef struct tcp_header {
	u_short sport;
	u_short dport;
	u_int sequence;		// 序列码
	u_int ack;					// 回复码

#ifdef WORDS_BIGENDIAN
	u_char offset : 4, reserved : 4;		// 偏移 预留
#else
	u_char reserved : 4, offset : 4;		// 预留 偏移
#endif
	u_char flags;				// 标志
	u_short windows;			// 窗口大小
	u_short checksum;			// 校验和
	u_short urgent_pointer;		// 紧急指针
}tcp_header;

typedef struct udp_header {
	u_int32_t sport;			// 源端口
	u_int32_t dport;			// 目标端口
	u_int8_t zero;				// 保留位
	u_int8_t proto;				// 协议标识
	u_int16_t datalen;			// UDP数据长度
}udp_header;

typedef struct icmp_header {
	u_int8_t type;				// ICMP类型
	u_int8_t code;				// 代码
	u_int16_t checksum;			// 校验和
	u_int16_t identification;	// 标识
	u_int16_t sequence;			// 序列号
	u_int32_t init_time;		// 发起时间戳
	u_int16_t recv_time;		// 接受时间戳
	u_int16_t send_time;		// 传输时间戳
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
	case 1:printf("ARP请求协议\n");break;
	case 2:printf("ARP应答协议\n");break;
	case 3:printf("RARP请求协议\n");break;
	case 4:printf("RARP应答协议\n");break;
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
	u_short ethernet_type;		// 以太网类型
	struct ether_header *ethernet_protocol;		// 以太网协议变量
	u_char *mac_string;			// 以太网地址

	ethernet_protocol = (struct ether_header*)packet_content;		// 获取以太网数据内容
	printf("Ethernet type is: ");
	ethernet_type = ntohs(ethernet_protocol->ether_type);	// 获取以太网类型
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

	// 获取以太网源地址
	printf("\t MAC Source Address: ");
	mac_string = ethernet_protocol->ether_shost;

	printf("%02x:%02x:%02x:%02x:%02x:%02x\n", *mac_string,
		*(mac_string + 1), *(mac_string + 2), *(mac_string + 3),
		*(mac_string + 4), *(mac_string + 5));

	// 获取以太网目的地址
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

	// 获得设备列表 
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
	// 跳转到该设备，打开适配器

	// 设备名，要捕捉的数据包的部分（65536保证能捕获到不同数据链路层上的每个数据包的全部内容），混杂模式，读取超时时间，错误缓冲池
	if ((adhandle = pcap_open_live(d->name, 65536, 1, 1000, errbuf)) == NULL) {
		fprintf(stderr, "\nUnable to open the adapter.%s is not supported by WinPcap\n", errbuf);
		pcap_freealldevs(alldevs);
		return -1;
	}
	// 检查数据链路层（只考虑了以太网）
	if (pcap_datalink(adhandle) != DLT_EN10MB) {
		fprintf(stderr, "\nThis program works only on Ethernet networks.\n");
		pcap_freealldevs(alldevs);
		return -1;
	}

	if (d->addresses != NULL) {
		// 获得接口的第一个地址的掩码
		netmask = ((struct sockaddr_in*)(d->addresses->netmask))->sin_addr.S_un.S_addr;
	}
	else 
		netmask = 0xffffff;
	


	while ((res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0) {
		// 请求超时
		if (0 == res) {continue;}
		// 分析数据包
		ethernet_protocol_packet_handle(NULL, header, pkt_data);
		// 将时间戳转换成可识别的格式
		local_tv_sec = header->ts.tv_sec;
		localtime_s(&ltime,&local_tv_sec);
		strftime(timestr, sizeof(timestr), "%H:%M:%S", &ltime);
		ih = (ip_header *)(pkt_data + 14); //以太网头部长度

		// 输出时间和IP信息
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
