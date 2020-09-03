#include "scope_exit.h" // make_scope_exit

#include <sys/types.h>
#include <sys/socket.h> // socket, AF_INET, SOCK_DGRAM, bind, socklen_t, recvfrom

#include <netinet/in.h> // sockaddr_in, INADDR_ANY
#include <netinet/ip.h>

#include <arpa/inet.h> // htons, htonl
#include <sys/signalfd.h> // signalfd

#include <poll.h> // poll, pollfd, POLLIN
#include <signal.h> // sigemptyset, sigaddset, sigprocmask
#include <unistd.h> // close, read

#include <cassert> // assert
#include <chrono> // chrono
#include <cstdint> // uint16_t, uint32_t, int32_t
#include <cstdio> // printf
#include <fstream> // ofstream


struct pcap_hdr_t
{
	std::uint32_t magic_number; /* magic number */
	std::uint16_t version_major; /* major version number */
	std::uint16_t version_minor; /* minor version number */
	std::int32_t thiszone; /* GMT to local correction */
	std::uint32_t sigfigs; /* accuracy of timestamps */
	std::uint32_t snaplen; /* max length of captured packets, in octets */
	std::uint32_t network; /* data link type */
};
static_assert(sizeof(pcap_hdr_t) == 24);

struct pcaprec_hdr_t
{
	std::uint32_t ts_sec; /* timestamp seconds */
	std::uint32_t ts_usec; /* timestamp microseconds */
	std::uint32_t incl_len; /* number of octets of packet saved in file */
	std::uint32_t orig_len; /* actual length of packet */
};
static_assert(sizeof(pcaprec_hdr_t) == 16);

struct brutal_header_t
{
	unsigned char eth_ig_1[3];
	unsigned char eth_addr_1[3];
	unsigned char eth_ig_2[3];
	unsigned char eth_addr_2[3];
	unsigned char eth_type[2];
	unsigned char ip_hdr_len[1];
	unsigned char ip_dsfield_enc[1];
	unsigned char ip_len[2];
	unsigned char ip_id[2];
	unsigned char ip_frag_offset[2];
	unsigned char ip_ttl[1];
	unsigned char ip_proto[1];
	unsigned char ip_checksum[2];
	unsigned char ip_src[4];
	unsigned char ip_dst[4];
	unsigned char udp_src_port[2];
	unsigned char udp_dst_port[2];
	unsigned char udp_len[2];
	unsigned char udp_checksum[2];
};
static_assert(sizeof(brutal_header_t) == 42);


static constexpr std::uint16_t const s_listening_port = 2368;
static constexpr int const s_ip_header_overhead = 28;
static constexpr int const s_udp_header_overhead = 8;


#define CHECK_RET(X, R) do{ if(X){}else{ std::printf("Failed in file `%s` on line %d with `%s`.\n", __FILE__, __LINE__, #X); return R; } }while(false)
#define CHECK_RET_VOID(X) do{ if(X){}else{ std::printf("Failed in file `%s` on line %d with `%s`.\n", __FILE__, __LINE__, #X); return; } }while(false)
#define CHECK_RET_LINE(X) CHECK_RET(X, __LINE__)


int capture(int const argc, char const* const* const argv, int* const& out_packets_captured);


int main(int const argc, char const* const* const argv)
{
	if(argc == 1)
	{
		std::printf("Example usage: ./pcap_capture.elf `capture file`\n");
		std::printf("Example usage: ./pcap_capture.elf capture.pcap\n");
		return 0;
	}
	int packets_captured;
	int const captured = capture(argc, argv, &packets_captured);
	CHECK_RET_LINE(captured == 0);
	std::printf("Done, captured %d packets.\n", packets_captured);
	return 0;
}


int capture(int const argc, char const* const* const argv, int* const& out_packets_captured)
{
	assert(out_packets_captured);

	CHECK_RET_LINE(argc == 2);

	int const sck = socket(AF_INET, SOCK_DGRAM, 0);
	CHECK_RET_LINE(sck != -1);

	sockaddr_in sck_addr_in_server{};
	sck_addr_in_server.sin_family = AF_INET;
	sck_addr_in_server.sin_port = htons(s_listening_port);
	sck_addr_in_server.sin_addr.s_addr = htonl(INADDR_ANY);
	int const bound = bind(sck, reinterpret_cast<sockaddr const*>(&sck_addr_in_server), sizeof(sck_addr_in_server));
	CHECK_RET_LINE(bound == 0);
	auto const fn_close_sck = mk::make_scope_exit([&](){ int const closed = close(sck); CHECK_RET_VOID(closed == 0); });

	std::ofstream ofs{argv[1], std::ios_base::out | std::ios_base::binary | std::ios_base::trunc};

	pcap_hdr_t pcap_hdr;
	pcap_hdr.magic_number = 0xa1b2c3d4;
	pcap_hdr.version_major = 2;
	pcap_hdr.version_minor = 4;
	pcap_hdr.thiszone = 0;
	pcap_hdr.sigfigs = 0;
	pcap_hdr.snaplen = 64 * 1024;
	pcap_hdr.network = 1;
	ofs.write(reinterpret_cast<char const*>(&pcap_hdr), sizeof(pcap_hdr));

	sigset_t signal_set;
	int const signal_set_initialized = sigemptyset(&signal_set);
	CHECK_RET_LINE(signal_set_initialized == 0);
	int const signal_set_added = sigaddset(&signal_set, SIGINT);
	CHECK_RET_LINE(signal_set_added == 0);

	int const signal_fd = signalfd(-1, &signal_set, SFD_NONBLOCK);
	CHECK_RET_LINE(signal_fd != -1);
	auto const fn_close_signal_fd = mk::make_scope_exit([&](){ int const closed = close(signal_fd); CHECK_RET_VOID(closed == 0); });

	int const signal_disabled = sigprocmask(SIG_BLOCK, &signal_set, nullptr);
	CHECK_RET_LINE(signal_disabled == 0);

	int& packets_captured = *out_packets_captured;
	packets_captured = 0;
	auto const baseline_time = std::chrono::high_resolution_clock::now();
	for(;;)
	{
		struct signalfd_siginfo signal_fd_info;
		auto const signal_fd_read = read(signal_fd, &signal_fd_info, sizeof(signal_fd_info));
		CHECK_RET_LINE((signal_fd_read == -1 && errno == EAGAIN) || signal_fd_read == sizeof(signal_fd_info));
		if(signal_fd_read == sizeof(signal_fd_info))
		{
			break;
		}

		pollfd pfd{};
		pfd.fd = sck;
		pfd.events = POLLIN;
		int const polled = poll(&pfd, 1, 100); // 10 times per second
		CHECK_RET_LINE(polled != -1);
		if(polled == 0)
		{
			continue;
		}
		assert(polled == POLLIN);

		unsigned char buff[64 * 1024];
		sockaddr_in sck_addr_in_client;
		socklen_t sck_addr_in_client_len = sizeof(sck_addr_in_client);
		auto const rcvd = recvfrom(sck, buff + sizeof(brutal_header_t), sizeof(buff) - sizeof(brutal_header_t), 0, reinterpret_cast<sockaddr*>(&sck_addr_in_client), &sck_addr_in_client_len);
		CHECK_RET_LINE(rcvd != -1);
		CHECK_RET_LINE(sck_addr_in_client_len == sizeof(sck_addr_in_client));
		if(rcvd == 0)
		{
			break;
		}

		auto const now = std::chrono::high_resolution_clock::now();
		auto const diff_time = now - baseline_time;
		auto const seconds = std::chrono::duration_cast<std::chrono::seconds, std::int64_t>(diff_time);
		auto const micros = std::chrono::duration_cast<std::chrono::microseconds, std::int64_t>(diff_time - seconds);
		pcaprec_hdr_t pcaprec_hdr;
		pcaprec_hdr.ts_sec = static_cast<std::uint32_t>(seconds.count());
		pcaprec_hdr.ts_usec = static_cast<std::uint32_t>(micros.count());
		pcaprec_hdr.incl_len = sizeof(brutal_header_t) + rcvd;
		pcaprec_hdr.orig_len = sizeof(brutal_header_t) + rcvd;
		ofs.write(reinterpret_cast<char const*>(&pcaprec_hdr), sizeof(pcaprec_hdr));

		std::uint16_t const len_ip = rcvd + s_ip_header_overhead;
		std::uint16_t const len_udp = rcvd + s_udp_header_overhead;
		brutal_header_t& brutal_header = *reinterpret_cast<brutal_header_t*>(buff + 0);
		brutal_header.eth_ig_1[0] = 0xFF;
		brutal_header.eth_ig_1[1] = 0xFF;
		brutal_header.eth_ig_1[2] = 0xFF;
		brutal_header.eth_addr_1[0] = 0xFF;
		brutal_header.eth_addr_1[1] = 0xFF;
		brutal_header.eth_addr_1[2] = 0xFF;
		brutal_header.eth_ig_2[0] = 0x00;
		brutal_header.eth_ig_2[1] = 0x00;
		brutal_header.eth_ig_2[2] = 0x00;
		brutal_header.eth_addr_2[0] = 0x00;
		brutal_header.eth_addr_2[1] = 0x00;
		brutal_header.eth_addr_2[2] = 0x00;
		brutal_header.eth_type[0] = 0x08;
		brutal_header.eth_type[1] = 0x00;
		brutal_header.ip_hdr_len[0] = 0x45;
		brutal_header.ip_dsfield_enc[0] = 0x00;
		brutal_header.ip_len[0] = (len_ip >> (1 * 8)) & 0xFF;
		brutal_header.ip_len[1] = (len_ip >> (0 * 8)) & 0xFF;
		brutal_header.ip_id[0] = 0x00;
		brutal_header.ip_id[1] = 0x00;
		brutal_header.ip_frag_offset[0] = 0x40;
		brutal_header.ip_frag_offset[1] = 0x00;
		brutal_header.ip_ttl[0] = 0xFF;
		brutal_header.ip_proto[0] = 0x11;
		brutal_header.ip_checksum[0] = 0x00; // ???
		brutal_header.ip_checksum[1] = 0x00; // ???
		brutal_header.ip_src[0] = (sck_addr_in_client.sin_addr.s_addr >> (0 * 8)) & 0xFF;
		brutal_header.ip_src[1] = (sck_addr_in_client.sin_addr.s_addr >> (1 * 8)) & 0xFF;
		brutal_header.ip_src[2] = (sck_addr_in_client.sin_addr.s_addr >> (2 * 8)) & 0xFF;
		brutal_header.ip_src[3] = (sck_addr_in_client.sin_addr.s_addr >> (3 * 8)) & 0xFF;
		brutal_header.ip_dst[0] = 0xFF;
		brutal_header.ip_dst[1] = 0xFF;
		brutal_header.ip_dst[2] = 0xFF;
		brutal_header.ip_dst[3] = 0xFF;
		brutal_header.udp_src_port[0] = (sck_addr_in_client.sin_port >> (0 * 8)) & 0xFF;
		brutal_header.udp_src_port[1] = (sck_addr_in_client.sin_port >> (1 * 8)) & 0xFF;
		brutal_header.udp_dst_port[0] = (s_listening_port >> (1 * 8)) & 0xFF;
		brutal_header.udp_dst_port[1] = (s_listening_port >> (0 * 8)) & 0xFF;
		brutal_header.udp_len[0] = (len_udp >> (1 * 8)) & 0xFF;
		brutal_header.udp_len[1] = (len_udp >> (0 * 8)) & 0xFF;
		brutal_header.udp_checksum[0] = 0x00;
		brutal_header.udp_checksum[1] = 0x00;

		ofs.write(reinterpret_cast<char const*>(buff), sizeof(brutal_header_t) + rcvd);
		++packets_captured;
	}

	return 0;
}
