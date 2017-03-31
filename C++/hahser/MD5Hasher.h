#pragma once
#ifndef MD5Hasher_h_included
#define MD5Hasher_h_included


#include <array>

#include <windows.h>
#include <wincrypt.h>


class MD5Hasher
{
public:
	typedef ::std::array<unsigned char, 128 / 8> Digest; // 16 bytes
public:
	MD5Hasher();
	MD5Hasher(MD5Hasher const&) = delete;
	MD5Hasher(MD5Hasher&& other) noexcept;
	MD5Hasher& operator=(MD5Hasher const&) = delete;
	MD5Hasher& operator=(MD5Hasher&& other) noexcept;
	~MD5Hasher();
public:
	void swap(MD5Hasher& other) noexcept;
public:
	void add_data(unsigned char const* const& data, ::std::size_t const& data_size);
	Digest finish();
private:
	HCRYPTPROV m_hProv;
	HCRYPTHASH m_hHash;
	bool m_bFinished;
};

inline void swap(MD5Hasher& a, MD5Hasher& b) noexcept { a.swap(b); }


#endif
