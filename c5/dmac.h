#ifndef CRYPTOPP_DMAC_H
#define CRYPTOPP_DMAC_H

#include "cbcmac.h"

NAMESPACE_BEGIN(CryptoPP)

template <class T>
class CRYPTOPP_NO_VTABLE DMAC_Base : public SameKeyLengthAs<T>, public MessageAuthenticationCode
{
public:
	static std::string StaticAlgorithmName() {return std::string("DMAC(") + T::StaticAlgorithmName() + ")";}

	enum {DIGESTSIZE=T::BLOCKSIZE};

	DMAC_Base() {}

	void CheckedSetKey(void *, Empty empty, const byte *key, unsigned int length, const NameValuePairs &params);
	void Update(const byte *input, unsigned int length);
	void TruncatedFinal(byte *mac, unsigned int size);
	unsigned int DigestSize() const {return DIGESTSIZE;}

private:
	byte *GenerateSubKeys(const byte *key, unsigned int keylength);

	unsigned int m_subkeylength;
	SecByteBlock m_subkeys;
	CBC_MAC<T> m_mac1;
	typename T::Encryption m_f2;
	unsigned int m_counter;
};

//! DMAC
/*! Based on "CBC MAC for Real-Time Data Sources" by Erez Petrank
	and Charles Rackoff. T should be BlockTransformation class.
*/
template <class T>
class DMAC : public MessageAuthenticationCodeFinal<DMAC_Base<T> >
{
public:
	DMAC() {}
	DMAC(const byte *key, unsigned int length=DMAC_Base<T>::DEFAULT_KEYLENGTH)
		{this->SetKey(key, length);}
};

template <class T>
void DMAC_Base<T>::CheckedSetKey(void *, Empty empty, const byte *key, unsigned int length, const NameValuePairs &params)
{
	m_subkeylength = T::StaticGetValidKeyLength(T::BLOCKSIZE);
	m_subkeys.resize(2*STDMAX((unsigned int)T::BLOCKSIZE, m_subkeylength));
	m_mac1.SetKey(GenerateSubKeys(key, length), m_subkeylength, params);
	m_f2.SetKey(m_subkeys+m_subkeys.size()/2, m_subkeylength, params);
	m_counter = 0;
	m_subkeys.resize(0);
}

template <class T>
void DMAC_Base<T>::Update(const byte *input, unsigned int length)
{
	m_mac1.Update(input, length);
	m_counter = (m_counter + length) % T::BLOCKSIZE;
}

template <class T>
void DMAC_Base<T>::TruncatedFinal(byte *mac, unsigned int size)
{
	ThrowIfInvalidTruncatedSize(size);

	byte pad[T::BLOCKSIZE];
	byte padByte = byte(T::BLOCKSIZE-m_counter);
	memset(pad, padByte, padByte);
	m_mac1.Update(pad, padByte);
	m_mac1.TruncatedFinal(mac, size);
	m_f2.ProcessBlock(mac);
}

template <class T>
byte *DMAC_Base<T>::GenerateSubKeys(const byte *key, unsigned int keylength)
{
	typename T::Encryption cipher(key, keylength);
	memset(m_subkeys, 0, m_subkeys.size());
	cipher.ProcessBlock(m_subkeys);
	m_subkeys[m_subkeys.size()/2 + T::BLOCKSIZE - 1] = 1;
	cipher.ProcessBlock(m_subkeys+m_subkeys.size()/2);
	return m_subkeys;
}

NAMESPACE_END

#endif
