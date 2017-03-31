#pragma once
#ifndef SHA256Hasher_h_included
#define SHA256Hasher_h_included


#include <array>

#include <windows.h>
#include <wincrypt.h>


class SHA256Hasher
{
public:
	typedef ::std::array<unsigned char, 256 / 8> Digest; // 32 bytes
public:
	SHA256Hasher();
	SHA256Hasher(SHA256Hasher const&) = delete;
	SHA256Hasher(SHA256Hasher&& other) noexcept;
	SHA256Hasher& operator=(SHA256Hasher const&) = delete;
	SHA256Hasher& operator=(SHA256Hasher&& other) noexcept;
	~SHA256Hasher();
public:
	void swap(SHA256Hasher& other) noexcept;
public:
	void add_data(unsigned char const* const& data, ::std::size_t const& data_size);
	Digest finish();
private:
	HCRYPTPROV m_hProv;
	HCRYPTHASH m_hHash;
	bool m_bFinished;
};

inline void swap(SHA256Hasher& a, SHA256Hasher& b) noexcept { a.swap(b); }


#endif
