#include "MD5Hasher.h"

#include <cstdint> // uint32_t
#include <limits>
#include <utility>
#include <algorithm>
#include <cassert>


MD5Hasher::MD5Hasher() :
	m_hProv(0),
	m_hHash(0),
	m_bFinished(false)
{
	BOOL const bCACWret = ::CryptAcquireContextW(&m_hProv, NULL, MS_DEF_PROV_W, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
	if(bCACWret == 0){
		DWORD const gle = ::GetLastError();
		assert(("CryptAcquireContextW failed.", false));
	}
	assert(m_hProv != 0);

	BOOL const bCCHret = ::CryptCreateHash(m_hProv, CALG_MD5, 0, 0, &m_hHash);
	if(bCCHret == 0){
		DWORD const gle = ::GetLastError();
		assert(("CryptCreateHash failed.", false));
	}
	assert(m_hHash != 0);

	DWORD dwSizeOfSize = 0;
	BOOL const bCGHP1 = ::CryptGetHashParam(m_hHash, HP_HASHSIZE, NULL, &dwSizeOfSize, 0);
	if(bCGHP1 == 0){
		DWORD const gle = ::GetLastError();
		assert(("CryptGetHashParam failed.", false));
	}
	assert(dwSizeOfSize == 4);

	::std::uint32_t sizeOfHash = 0;
	DWORD dwSizeOfSizeOfHash = sizeof(sizeOfHash);
	BOOL const bCGHP2 = ::CryptGetHashParam(m_hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&sizeOfHash), &dwSizeOfSizeOfHash, 0);
	if(bCGHP2 == 0){
		DWORD const gle = ::GetLastError();
		assert(("CryptGetHashParam failed.", false));
	}
	assert(dwSizeOfSizeOfHash == sizeof(sizeOfHash));
	assert(sizeOfHash == sizeof(Digest));
}

MD5Hasher::MD5Hasher(MD5Hasher&& other) noexcept :
	m_hProv(0),
	m_hHash(0),
	m_bFinished(false)
{
	using ::std::swap;
	swap(*this, other);
}

MD5Hasher& MD5Hasher::operator=(MD5Hasher&& other) noexcept
{
	using ::std::swap;
	swap(*this, other);
	return *this;
}

MD5Hasher::~MD5Hasher()
{
	if(m_hProv == 0 && m_hHash == 0 && m_bFinished == false){
		return;
	}

	assert(m_bFinished); // You created hash, but not finished it, why?

	BOOL const bCDHret = ::CryptDestroyHash(m_hHash);
	if(bCDHret == 0){
		DWORD const gle = ::GetLastError();
		assert(("CryptDestroyHash failed.", false));
	}

	BOOL const bCRCret = ::CryptReleaseContext(m_hProv, 0);
	if(bCRCret == 0){
		DWORD const gle = ::GetLastError();
		assert(("CryptReleaseContext failed.", false));
	}
}

void MD5Hasher::swap(MD5Hasher& other) noexcept
{
	using ::std::swap;
	swap(m_hProv, other.m_hProv);
	swap(m_hHash, other.m_hHash);
	swap(m_bFinished, other.m_bFinished);
}

void MD5Hasher::add_data(unsigned char const* const& data, ::std::size_t const& data_size)
{
	assert(!m_bFinished); // You already finished this hash, you can not add more data to it.
	assert(data_size <= (::std::numeric_limits<DWORD>::max)()); // CryptHashData takes DWORD as data_size

	BOOL const bCHDret = ::CryptHashData(m_hHash, data, data_size, 0);
	if(bCHDret == 0){
		DWORD const gle = ::GetLastError();
		assert(("CryptHashData failed.", false));
	}
}

MD5Hasher::Digest MD5Hasher::finish()
{
	assert(!m_bFinished); // You can finish hash only once.
	m_bFinished = true;
	Digest retHash;

	DWORD dwSizeOfHash = retHash.size();
	BOOL const bCGHP3 = ::CryptGetHashParam(m_hHash, HP_HASHVAL, retHash.data(), &dwSizeOfHash, 0);
	if(bCGHP3 == 0){
		DWORD const gle = ::GetLastError();
		assert(("CryptGetHashParam failed.", false));
	}
	assert(dwSizeOfHash == retHash.size());

	return retHash;
}
