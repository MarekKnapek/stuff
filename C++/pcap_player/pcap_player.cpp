#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

#include <arpa/inet.h>
#include <sys/socket.h>


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


static constexpr std::uint16_t const s_target_port = 2368;


#define CHECK_RET(X) do{ if(!(X)){ std::printf("Failed in file `%s` at line %d with `%s`.\n", __FILE__, static_cast<int>(__LINE__), #X); return __LINE__; } }while(false)
#define CHECK_EXIT(X) do{ if(!(X)){ std::printf("Failed in file `%s` at line %d with `%s`.\n", __FILE__, static_cast<int>(__LINE__), #X); std::exit(__LINE__); } }while(false)


template<typename t>
t my_read(std::istream& is)
{
	t obj;
	is.read(reinterpret_cast<char*>(&obj), sizeof(obj));
	return obj;
}


int replay(int const argc, char const* const* const argv, int* const& out_packets_replayed);


int main(int const argc, char const* const* const argv)
{
	if(argc == 1)
	{
		std::printf("Example usage: ./pcap_player.elf `capture file` `target computer`\n");
		std::printf("Example usage: ./pcap_player.elf capture.pcap 127.0.0.1\n");
		return 0;
	}
	int packets_replayed;
	int const replayed = replay(argc, argv, &packets_replayed);
	CHECK_RET(replayed == 0);
	std::printf("Done, replayed %d packets.\n", packets_replayed);
	return 0;
}


int replay(int const argc, char const* const* const argv, int* const& out_packets_replayed)
{
	CHECK_RET(argc == 3);

	std::ifstream ifs{argv[1], std::ios_base::in | std::ios_base::binary};
	if(ifs.eof())
	{
		return 0;
	}

	pcap_hdr_t const hdr = my_read<pcap_hdr_t>(ifs);
	if(ifs.eof())
	{
		return 0;
	}
	CHECK_RET(hdr.magic_number == 0xa1b2c3d4);
	CHECK_RET(hdr.version_major == 2);
	CHECK_RET(hdr.version_minor == 4);

	int const sck = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	CHECK_RET(sck != -1);
	sockaddr_in si_other{};
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(s_target_port);
	int const aton = inet_aton(argv[2] , &si_other.sin_addr);
	CHECK_RET(aton != 0);

	int& packets_replayed = *out_packets_replayed;
	packets_replayed = 0;

	pcaprec_hdr_t pcaprec_hdr = my_read<pcaprec_hdr_t>(ifs);
	if(ifs.eof())
	{
		return 0;
	}
	CHECK_RET(pcaprec_hdr.incl_len > sizeof(brutal_header_t));
	std::uint32_t prev_secs = pcaprec_hdr.ts_sec;
	std::uint32_t prev_usecs = pcaprec_hdr.ts_usec;
	unsigned char buff[64 * 1024];
	ifs.read(reinterpret_cast<char*>(buff), pcaprec_hdr.incl_len);
	if(ifs.eof())
	{
		return 0;
	}

	for(;;)
	{
		auto const now = std::chrono::high_resolution_clock::now();

		int const sent = sendto(sck, buff + sizeof(brutal_header_t), pcaprec_hdr.incl_len - sizeof(brutal_header_t), 0, reinterpret_cast<sockaddr*>(&si_other), sizeof(si_other));
		CHECK_RET(sent != -1);
		++packets_replayed;


		pcaprec_hdr = my_read<pcaprec_hdr_t>(ifs);
		if(ifs.eof())
		{
			return 0;
		}
		CHECK_RET(pcaprec_hdr.incl_len > sizeof(brutal_header_t));
		ifs.read(reinterpret_cast<char*>(buff), pcaprec_hdr.incl_len);
		if(ifs.eof())
		{
			return 0;
		}

		int const secs = static_cast<int>(pcaprec_hdr.ts_sec) - static_cast<int>(prev_secs);
		int const usecs = static_cast<int>(pcaprec_hdr.ts_usec) - static_cast<int>(prev_usecs);
		prev_secs = pcaprec_hdr.ts_sec;
		prev_usecs = pcaprec_hdr.ts_usec;

		std::this_thread::sleep_until(now + (std::chrono::seconds{secs} + std::chrono::microseconds{usecs}));
	}

	return 0;
}
