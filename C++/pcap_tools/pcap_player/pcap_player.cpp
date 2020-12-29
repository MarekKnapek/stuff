#include "scope_exit.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

#ifdef _MSC_VER
#include <cwchar>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif


#ifdef _MSC_VER
#define char_native wchar_t
#define main_native wmain
#else
#define char_native char
#define main_native main
#endif


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
	auto const gcount = is.gcount();
	CHECK_EXIT(gcount == sizeof(obj) || is.eof());
	return obj;
}


int replay(int const argc, char_native const* const* const argv, int* const& out_packets_replayed);


int main_native(int const argc, char_native const* const* const argv)
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


int replay(int const argc, char_native const* const* const argv, int* const& out_packets_replayed)
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

#ifdef _MSC_VER
	WORD const wVersionRequired = MAKEWORD(2, 2);
	WSADATA wsa_data;
	int const wsa_started = WSAStartup(wVersionRequired, &wsa_data);
	CHECK_RET(wsa_started == 0);
	CHECK_RET(LOBYTE(wsa_data.wVersion) == 2 && HIBYTE(wsa_data.wVersion) == 2);
	auto const wsa_stop = mk::make_scope_exit([&](){ int const wsa_stopped = WSACleanup(); CHECK_EXIT(wsa_stopped == 0); });

	SOCKET const sck = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_NO_HANDLE_INHERIT);
	CHECK_RET(sck != INVALID_SOCKET);
	auto const sck_free = mk::make_scope_exit([&](){ int const sck_freed = closesocket(sck); CHECK_EXIT(sck_freed == 0); });

	static constexpr auto const s_wchar_to_char = [](wchar_t const* const input, char* const output, int const output_len) -> bool
	{
		std::size_t const input_len_ = std::wcslen(input);
		CHECK_RET(input_len_ < 256);
		int const input_len = static_cast<int>(input_len_);
		CHECK_RET(input_len < output_len);
		bool const is_ascii = std::all_of(input, input + input_len, [](wchar_t const ch){ return static_cast<unsigned>(ch) >= 0x20 && static_cast<unsigned>(ch) <= 0x7E; });
		CHECK_RET(is_ascii);
		std::transform(input, input + input_len, output, [](wchar_t const ch){ return static_cast<char>(ch); });
		output[input_len] = '\0';
		return true;
	};

	static constexpr int const s_buff_len = 64;
	char ip_buff[s_buff_len];
	bool const converted = s_wchar_to_char(argv[2], ip_buff, s_buff_len);
	CHECK_RET(converted);

	static_assert(sizeof(std::uint32_t) == sizeof(unsigned long));
	std::uint32_t const ip = inet_addr(ip_buff);
	CHECK_RET(ip != INADDR_NONE && ip != INADDR_ANY);
	std::uint16_t const port = htons(s_target_port);
#else
	int const sck = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	CHECK_RET(sck != -1);
	sockaddr_in si_other{};
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(s_target_port);
	int const aton = inet_aton(argv[2] , &si_other.sin_addr);
	CHECK_RET(aton != 0);
#endif

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

#ifdef _MSC_VER
		static constexpr auto const s_send_to = [](SOCKET const& sck, std::uint32_t const ip, std::uint16_t const port, void* const data, int const data_len) -> bool
		{
			WSABUF wsa_buff;
			wsa_buff.len = data_len;
			wsa_buff.buf = static_cast<char*>(data);
			DWORD numberOfBytesSent;
			struct sockaddr_in target{};
			target.sin_family = AF_INET;
			target.sin_addr.s_addr = ip;
			target.sin_port = port;
			int const sent_to = WSASendTo(sck, &wsa_buff, 1, &numberOfBytesSent, 0, reinterpret_cast<sockaddr const*>(&target), sizeof(target), nullptr, nullptr);
			CHECK_RET(sent_to == 0);
			CHECK_RET(numberOfBytesSent == data_len);
			return true;
		};

		bool const sent = s_send_to(sck, ip, port, buff + sizeof(brutal_header_t), pcaprec_hdr.incl_len - sizeof(brutal_header_t));
		CHECK_RET(sent);
#else
		int const sent = sendto(sck, buff + sizeof(brutal_header_t), pcaprec_hdr.incl_len - sizeof(brutal_header_t), 0, reinterpret_cast<sockaddr*>(&si_other), sizeof(si_other));
		CHECK_RET(sent != -1);
#endif
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
