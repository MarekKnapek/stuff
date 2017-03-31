#include "SHA256Hasher.h"

#include <cstdint>
#include <limits>
#include <utility>
#include <algorithm>
#include <cassert>


SHA256Hasher::SHA256Hasher() :
	m_hProv(0),
	m_hHash(0),
	m_bFinished(false)
{
	BOOL const bCACWret = ::CryptAcquireContextW(&m_hProv, NULL, MS_ENH_RSA_AES_PROV_W, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
	if(bCACWret == 0){
		DWORD const gle = ::GetLastError();
		if(gle == NTE_KEYSET_NOT_DEF){
			BOOL const bCACWret2 = ::CryptAcquireContextW(&m_hProv, NULL, MS_ENH_RSA_AES_PROV_XP_W, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
			if(bCACWret2 == 0){
				DWORD const gle = ::GetLastError();
				assert(("CryptAcquireContextW failed.", false));
			}
		}else{
			assert(("CryptAcquireContextW failed.", false));
		}
	}
	assert(m_hProv != 0);

	BOOL const bCCHret = ::CryptCreateHash(m_hProv, CALG_SHA_256, 0, 0, &m_hHash);
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

SHA256Hasher::SHA256Hasher(SHA256Hasher&& other) noexcept :
	m_hProv(0),
	m_hHash(0),
	m_bFinished(false)
{
	using ::std::swap;
	swap(*this, other);
}

SHA256Hasher& SHA256Hasher::operator=(SHA256Hasher&& other) noexcept
{
	using ::std::swap;
	swap(*this, other);
	return *this;
}

SHA256Hasher::~SHA256Hasher()
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

void SHA256Hasher::swap(SHA256Hasher& other) noexcept
{
	using ::std::swap;
	swap(m_hProv, other.m_hProv);
	swap(m_hHash, other.m_hHash);
	swap(m_bFinished, other.m_bFinished);
}

void SHA256Hasher::add_data(unsigned char const* const& data, ::std::size_t const& data_size)
{
	assert(!m_bFinished); // You already finished this hash, you can not add more data to it.
	assert(data_size <= (::std::numeric_limits<DWORD>::max)()); // CryptHashData takes DWORD as data_size

	BOOL const bCHDret = ::CryptHashData(m_hHash, data, data_size, 0);
	if(bCHDret == 0){
		DWORD const gle = ::GetLastError();
		assert(("CryptHashData failed.", false));
	}
}

SHA256Hasher::Digest SHA256Hasher::finish()
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
