#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "caputils/caputils.h"
#include "caputils/stream.h"
#include "caputils/filter.h"
#include "caputils/utils.h"
#include "caputils/marker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>

static int keep_running = 1;
static int print_content = 0;
static int save_sunnyStat = 0;
static int print_date = 0;
static int max_packets = 0;
static const char* iface = NULL;
static const char* sunnyStatFilePrefix = NULL;
	static char sunnyDagAndPort[] = "xxx\0";
	static char timeStamp_sec[] = "1340097905\0";
	static unsigned int ipId = 65536;
	static char timeStamp_psec[] = "902924299250\0";
	FILE *pfD00, *pfD01, *pfD10, *pfD11, *pfDxx;
static unsigned int *pTcpTimestamp = NULL;
static struct timeval timeout = {1,0};
static const char* program_name = NULL;
static unsigned long int winSunny;
static unsigned int winShiftSunny = 8;

void handle_paramSunnyStat(){
	fprintf(stdout, "got -s \n");
	save_sunnyStat = 1;
	sunnyStatFilePrefix = optarg;
	pfD00 = fopen("/dev/shm/qodoh.d00", "w");
	pfD01 = fopen("/dev/shm/qodoh.d01", "w");
	pfD10 = fopen("/dev/shm/qodoh.d10", "w");
	pfD11 = fopen("/dev/shm/qodoh.d11", "w");
	if (!(pfD00 && pfD01 && pfD10 && pfD11)){
		fprintf(stdout,"cannot open stat file(s).");
		exit(1);
	}
}

void handle_sigint(int signum){
	/*fclose(pfD00);*/
	/*fclose(pfD01);*/
	/*fclose(pfD10);*/
	/*fclose(pfD11);*/
	if ( keep_running == 0 ){
		fprintf(stderr, "\rGot SIGINT again, terminating.\n");
		abort();
	}
	fprintf(stderr, "\rAborting capture.\n");
	keep_running = 0;
}

static void print_tcp(FILE* dst, const struct ip* ip, const struct tcphdr* tcp){
	fprintf(dst, "TCP(HDR[%d]DATA[%0x]):\t [", 4*tcp->doff, ntohs(ip->ip_len) - 4*tcp->doff - 4*ip->ip_hl);
	winSunny = (u_int16_t)ntohs(tcp->window);
	if(tcp->syn) {
		fprintf(dst, "S");
		/* set scale info here. assume if both support win scale then no error. by sunny */
		winShiftSunny = (unsigned int) *((char*)tcp + 4*tcp->doff - 1);
		pTcpTimestamp = (u_int32_t *) ((char *)tcp + 28);
	}else{
		/* added win size scale left-shift by sunny. */
		winSunny = winSunny << winShiftSunny;
		pTcpTimestamp = (u_int32_t *) ((char *)tcp + 24);
	}
	if(tcp->fin) {
		fprintf(dst, "F");
	}
	if(tcp->ack) {
		fprintf(dst, "A");
	}
	if(tcp->psh) {
		fprintf(dst, "P");
	}
	if(tcp->urg) {
		fprintf(dst, "U");
	}
	if(tcp->rst) {
		fprintf(dst, "R");
	}
	

	fprintf(dst, "] WIN=%lu %s:%d ", winSunny, inet_ntoa(ip->ip_src), (u_int16_t)ntohs(tcp->source)); /* added WIN by sunny */
	fprintf(dst, " --> %s:%d",inet_ntoa(ip->ip_dst),(u_int16_t)ntohs(tcp->dest));
	fprintf(dst, "\n");

	ipId = (u_int16_t)ntohs(((const struct iphdr *)ip)->id);
	if( (save_sunnyStat == 1) && !(tcp->rst) && !(ipId == 0 && (u_int16_t)ntohs(((const struct iphdr *)ip)->tot_len) == 52) ){ /* this condition is hot fix, i do not know the exact reason why there are 'data' pkts without tcp payload */
		fprintf(pfDxx, "%s.", timeStamp_sec);
		fprintf(pfDxx, "%s", timeStamp_psec);
		fprintf(pfDxx, "%.9u_", ntohl(*pTcpTimestamp));
		fprintf(pfDxx, "%.5u_", ipId);
		fprintf(pfDxx, "%.10lu \n", winSunny);
	}
}

static void print_udp(FILE* dst, const struct ip* ip, const struct udphdr* udp){
	fprintf(dst, "UDP(HDR[8]DATA[%d]):\t %s:%d ",(u_int16_t)(ntohs(udp->len)-8),inet_ntoa(ip->ip_src),(u_int16_t)ntohs(udp->source));
	fprintf(dst, " --> %s:%d", inet_ntoa(ip->ip_dst),(u_int16_t)ntohs(udp->dest));
	fprintf(dst, "\n");
}

static void print_icmp(FILE* dst, const struct ip* ip, const struct icmphdr* icmp){
	fprintf(dst, "ICMP:\t %s ",inet_ntoa(ip->ip_src));
	fprintf(dst, " --> %s ",inet_ntoa(ip->ip_dst));
	fprintf(dst, "Type %d , code %d", icmp->type, icmp->code);
	if( icmp->type==0 && icmp->code==0){
		fprintf(dst, " echo reply: SEQNR = %d ", icmp->un.echo.sequence);
	}
	if( icmp->type==8 && icmp->code==0){
		fprintf(dst, " echo reqest: SEQNR = %d ", icmp->un.echo.sequence);
	}
	fprintf(dst, "\n");
}

static void print_ipv4(FILE* dst, const struct ip* ip){
	void* payload = ((char*)ip) + 4*ip->ip_hl;
	fprintf(dst, "IPv4(HDR[%d])[", 4*ip->ip_hl);
	fprintf(dst, "Len=%d:",(u_int16_t)ntohs(ip->ip_len));
	fprintf(dst, "ID=%d:",(u_int16_t)ntohs(ip->ip_id));
	fprintf(dst, "TTL=%d:",(u_int8_t)ip->ip_ttl);
	fprintf(dst, "Chk=%d:",(u_int16_t)ntohs(ip->ip_sum));

	if(ntohs(ip->ip_off) & IP_DF) {
		fprintf(dst, "DF");
	}
	if(ntohs(ip->ip_off) & IP_MF) {
		fprintf(dst, "MF");
	}

	fprintf(dst, " Tos:%0x]:\t",(u_int8_t)ip->ip_tos);

	switch( ip->ip_p ) {
	case IPPROTO_TCP:
		print_tcp(dst, ip, (const struct tcphdr*)payload);
		break;

	case IPPROTO_UDP:
		print_udp(dst, ip, (const struct udphdr*)payload);
		break;

	case IPPROTO_ICMP:
		print_icmp(dst, ip, (const struct icmphdr*)payload);
		break;

	default:
		fprintf(dst, "Unknown transport protocol: %d \n", ip->ip_p);
		break;
	}
}

static void print_ieee8023(FILE* dst, const struct llc_pdu_sn* llc){
	fprintf(dst,"dsap=%02x ssap=%02x ctrl1 = %02x ctrl2 = %02x\n", llc->dsap, llc->ssap, llc->ctrl_1, llc->ctrl_2);
}

static void print_eth(FILE* dst, const struct ethhdr* eth){
	void* payload = ((char*)eth) + sizeof(struct ethhdr);
	uint16_t h_proto = ntohs(eth->h_proto);
	uint16_t vlan_tci;

	begin:

	if(h_proto<0x05DC){
		fprintf(dst, "IEEE802.3 ");
		fprintf(dst, " %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x ",
			eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5],
			eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
		print_ieee8023(dst,(struct llc_pdu_sn*)payload);
	} else {
		switch ( h_proto ){
		case ETHERTYPE_VLAN:
			vlan_tci = ((uint16_t*)payload)[0];
			h_proto = ntohs(((uint16_t*)payload)[0]);
			payload = ((char*)eth) + sizeof(struct ethhdr);
			fprintf(dst, "802.1Q vlan# %d: ", 0x0FFF&ntohs(vlan_tci));
			goto begin;

		case ETHERTYPE_IP:
			print_ipv4(dst, (struct ip*)payload);
			break;

		case ETHERTYPE_IPV6:
			printf("ipv6\n");
			break;

		case ETHERTYPE_ARP:
			printf("arp\n");
			break;

		case 0x0810:
			fprintf(dst, "MP packet\n");
			break;

		case STPBRIDGES:
			fprintf(dst, "STP(0x%04x): (spanning-tree for bridges)\n", h_proto);
			break;

		case CDPVTP:
			fprintf(dst, "CDP(0x%04x): (CISCO Discovery Protocol)\n", h_proto);
			break;

		default:
			fprintf(dst, "Unknown ethernet protocol (0x%04x), ", h_proto);
			fprintf(dst, " %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x \n",
				eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5],
				eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
			break;
		}
	}
}

static struct option long_options[]= {
	{"content", no_argument,        0, 'c'},
	{"packets",  required_argument, 0, 'p'},
	{"iface",    required_argument, 0, 'i'},
	{"timeout",  required_argument, 0, 't'},
	{"calender", no_argument,       0, 'd'},
	{"sunnyStat",no_argument   , 0, 's'},
	{"help",     no_argument,       0, 'h'},
	{0, 0, 0, 0} /* sentinel */
};

static void show_usage(void){
	printf("%s-" VERSION_FULL "\n", program_name);
	printf("(C) 2004 Patrik Arlos <patrik.arlos@bth.se>\n");
	printf("(C) 2012 David Sveningsson <david.sveningsson@bth.se>\n");
	printf("Usage: %s [OPTIONS] STREAM\n", program_name);
	printf("  -c, --content        Write full package content as hexdump. [default=no]\n"
	       "  -i, --iface          For ethernet-based streams, this is the interface to listen\n"
	       "                       on. For other streams it is ignored.\n"
	       "  -p, --packets=N      Stop after N packets.\n"
	       "  -t, --timeout=N      Wait for N ms while buffer fills [default: 1000ms].\n"
	       "  -d, --calender       Show timestamps in human-readable format.\n"
	       "  -s, --sunnyStat      Save stat info to file. (qodoh)\n"
	       "  -h, --help           This text.\n\n");
	filter_from_argv_usage();
}

int main(int argc, char **argv){
	/* extract program name from path. e.g. /path/to/MArCd -> MArCd */
	const char* separator = strrchr(argv[0], '/');
	if ( separator ){
		program_name = separator + 1;
	} else {
		program_name = argv[0];
	}

	struct filter filter;
	if ( filter_from_argv(&argc, argv, &filter) != 0 ){
		return 0; /* error already shown */
	}

	filter_print(&filter, stderr, 0);

	int op, option_index = -1;
	while ( (op = getopt_long(argc, argv, "hcdi:p:t:", long_options, &option_index)) != -1 ){
		switch (op){
		case 0: /* long opt */
		case '?': /* unknown opt */
			break;

		case 'd':
			fprintf(stdout, "got -d \n");
			print_date = 1;
			break;

		case 'p':
			max_packets = atoi(optarg);
			break;

		case 't':
			{
				int tmp = atoi(optarg);
				timeout.tv_sec = tmp / 1000;
				timeout.tv_usec = tmp % 1000 * 1000;
			}
			break;

		case 'c':
			print_content = 1;
			break;

		case 's':
			fprintf(stdout,"-s opt arg: %s",optarg);
			handle_paramSunnyStat();
			break;

		case 'i':
			iface = optarg;
			break;

		case 'h':
			show_usage();
			return 0;

		default:
			printf ("?? getopt returned character code 0%o ??\n", op);
		}
	}
	handle_paramSunnyStat(); /* problem1: parm handle */

	int ret;

	/* Open stream(s) */
	struct stream* stream;
	if ( (ret=stream_from_getopt(&stream, argv, optind, argc, iface, "-", program_name, 0)) != 0 ) {
		return ret; /* Error already shown */
	}
	const stream_stat_t* stat = stream_get_stat(stream);
	stream_print_info(stream, stderr);

	/* handle C-c */
	signal(SIGINT, handle_sigint);

	while ( keep_running ) {
		/* A short timeout is used to allow the application to "breathe", i.e
		 * terminate if SIGINT was received. */
		struct timeval tv = timeout;

		/* Read the next packet */
		cap_head* cp;
		ret = stream_read(stream, &cp, &filter, &tv);
		if ( ret == EAGAIN ){
			continue; /* timeout */
		} else if ( ret != 0 ){
			break; /* shutdown or error */
		}

		time_t time = (time_t)cp->ts.tv_sec;
		fprintf(stdout, "[%4"PRIu64"]:%.4s:%.8s:", stat->matched, cp->nic, cp->mampid);
		sprintf(sunnyDagAndPort, "%.4s", cp->nic);
		if( print_date == 0 ) {
			fprintf(stdout, "%u.", cp->ts.tv_sec);
		} else {
			static char timeStr[25];
			struct tm tm = *gmtime(&time);
			strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm);
			fprintf(stdout, "%s.", timeStr);
		}

		fprintf(stdout, "%012"PRId64":LINK(%4d):CAPLEN(%4d):", cp->ts.tv_psec, cp->len, cp->caplen);
		if( save_sunnyStat == 1){
			if (strncmp("d00", sunnyDagAndPort, 3) == 0){
				pfDxx = pfD00;
			}else if(strncmp("d01", sunnyDagAndPort, 3) == 0){
				pfDxx = pfD01;
			}else if(strncmp("d10", sunnyDagAndPort, 3) == 0){
				pfDxx = pfD10;
			}else if(strncmp("d11", sunnyDagAndPort, 3) == 0){
				pfDxx = pfD11;
			}
			sprintf(timeStamp_sec, "%u", cp->ts.tv_sec);
			sprintf(timeStamp_psec, "%012"PRId64" ", cp->ts.tv_psec);
		}
		/* Test for libcap_utils marker packet */
		struct marker mark;
		int marker_port;
		if ( (marker_port=is_marker(cp, &mark, 0)) != 0 ){
			fprintf(stdout, "Marker [e=%d, r=%d, k=%d, s=%d, port=%d]\n",
				mark.exp_id, mark.run_id, mark.key_id, mark.seq_num, marker_port);
		} else {
			print_eth(stdout, cp->ethhdr);
		}

		if ( max_packets > 0 && stat->matched >= max_packets) {
			/* Read enough pkts lets break. */
			printf("read enought packages\n");
			break;
		}
	}

	/* if ret == -1 the stream was closed properly (e.g EOF or TCP shutdown)
	 * In addition EINTR should not give any errors because it is implied when the
	 * user presses C-c */
	if ( ret > 0 && ret != EINTR ){
		fprintf(stderr, "stream_read() returned 0x%08X: %s\n", ret, caputils_error_string(ret));
	}

	/* Write stats */
	fprintf(stderr, "%"PRIu64" packets received.\n", stat->read);
	fprintf(stderr, "%"PRIu64" packets matched filter.\n", stat->matched);

	/* Release resources */
	stream_close(stream);
	filter_close(&filter);

	fclose(pfD00);
	fclose(pfD01);
	fclose(pfD10);
	fclose(pfD11);

	return 0;
}
