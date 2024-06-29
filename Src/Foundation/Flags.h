#pragma once

/**
\brief Container for bitfield flag variables associated with a specific enum type.

This allows for type safe manipulation for bitfields.

<h3>Example</h3>
	// enum that defines each bit...
	struct MyEnum
	{
		enum Enum
		{
			eMAN  = 1,
			eBEAR = 2,
			ePIG  = 4,
		};
	};

	// implements some convenient global operators.
	PX_FLAGS_OPERATORS(MyEnum::Enum, PxU8);

	MxFlags<MyEnum::Enum, PxU8> myFlags;
	myFlags |= MyEnum::eMAN;
	myFlags |= MyEnum::eBEAR | MyEnum::ePIG;
	if(myFlags & MyEnum::eBEAR)
	{
		doSomething();
	}
*/

enum MxEMPTY { MxEmpty };

template<typename enumtype, typename storagetype = unsigned>
class MxFlags
	{
	public:
		typedef storagetype InternalType;

		inline	explicit	MxFlags(const MxEMPTY&) {}
		inline				MxFlags(void);
		inline				MxFlags(enumtype e);
		inline				MxFlags(const MxFlags<enumtype, storagetype> &f);
		inline	explicit	MxFlags(storagetype b);

		inline bool							 isSet(enumtype e) const;
		inline MxFlags<enumtype, storagetype> &set(enumtype e);
		inline bool                           operator==(enumtype e) const;
		inline bool                           operator==(const MxFlags<enumtype, storagetype> &f) const;
		inline bool                           operator==(bool b) const;
		inline bool                           operator!=(enumtype e) const;
		inline bool                           operator!=(const MxFlags<enumtype, storagetype> &f) const;

		inline MxFlags<enumtype, storagetype> &operator =(enumtype e);

		inline MxFlags<enumtype, storagetype> &operator|=(enumtype e);
		inline MxFlags<enumtype, storagetype> &operator|=(const MxFlags<enumtype, storagetype> &f);
		inline MxFlags<enumtype, storagetype>  operator| (enumtype e) const;
		inline MxFlags<enumtype, storagetype>  operator| (const MxFlags<enumtype, storagetype> &f) const;

		inline MxFlags<enumtype, storagetype> &operator&=(enumtype e);
		inline MxFlags<enumtype, storagetype> &operator&=(const MxFlags<enumtype, storagetype> &f);
		inline MxFlags<enumtype, storagetype>  operator& (enumtype e) const;
		inline MxFlags<enumtype, storagetype>  operator& (const MxFlags<enumtype, storagetype> &f) const;

		inline MxFlags<enumtype, storagetype> &operator^=(enumtype e);
		inline MxFlags<enumtype, storagetype> &operator^=(const MxFlags<enumtype, storagetype> &f);
		inline MxFlags<enumtype, storagetype>  operator^ (enumtype e) const;
		inline MxFlags<enumtype, storagetype>  operator^ (const MxFlags<enumtype, storagetype> &f) const;

		inline MxFlags<enumtype, storagetype>  operator~ (void) const;

		inline                                operator bool(void) const;
		inline								  operator unsigned char(void) const;
		inline                                operator unsigned short(void) const;
		inline                                operator unsigned(void) const;

		inline void                           clear(enumtype e);

	public:
		friend inline MxFlags<enumtype, storagetype> operator&(enumtype a, MxFlags<enumtype, storagetype> &b)
			{
			MxFlags<enumtype, storagetype> out;
			out.mBits = a & b.mBits;
			return out;
			}

	private:
		storagetype  mBits;
	};

#define FLAGS_OPERATORS(enumtype, storagetype)                                                                                         \
		inline MxFlags<enumtype, storagetype> operator|(enumtype a, enumtype b) { MxFlags<enumtype, storagetype> r(a); r |= b; return r; } \
		inline MxFlags<enumtype, storagetype> operator&(enumtype a, enumtype b) { MxFlags<enumtype, storagetype> r(a); r &= b; return r; } \
		inline MxFlags<enumtype, storagetype> operator~(enumtype a)             { return ~MxFlags<enumtype, storagetype>(a);             }

#define FLAGS_TYPEDEF(x, y)	typedef MxFlags<x::Enum, y> x##s;	\
	FLAGS_OPERATORS(x::Enum, y);

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype>::MxFlags(void)
	{
	mBits = 0;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype>::MxFlags(enumtype e)
	{
	mBits = static_cast<storagetype>(e);
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype>::MxFlags(const MxFlags<enumtype, storagetype> &f)
	{
	mBits = f.mBits;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype>::MxFlags(storagetype b)
	{
	mBits = b;
	}

template<typename enumtype, typename storagetype>
inline bool MxFlags<enumtype, storagetype>::isSet(enumtype e) const
	{
	return (mBits & static_cast<storagetype>(e)) == static_cast<storagetype>(e);
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> &MxFlags<enumtype, storagetype>::set(enumtype e)
	{
	mBits = static_cast<storagetype>(e);
	return *this;
	}

template<typename enumtype, typename storagetype>
inline bool MxFlags<enumtype, storagetype>::operator==(enumtype e) const
	{
	return mBits == static_cast<storagetype>(e);
	}

template<typename enumtype, typename storagetype>
inline bool MxFlags<enumtype, storagetype>::operator==(const MxFlags<enumtype, storagetype>& f) const
	{
	return mBits == f.mBits;
	}

template<typename enumtype, typename storagetype>
inline bool MxFlags<enumtype, storagetype>::operator==(bool b) const
	{
	return ((bool)*this) == b;
	}

template<typename enumtype, typename storagetype>
inline bool MxFlags<enumtype, storagetype>::operator!=(enumtype e) const
	{
	return mBits != static_cast<storagetype>(e);
	}

template<typename enumtype, typename storagetype>
inline bool MxFlags<enumtype, storagetype>::operator!=(const MxFlags<enumtype, storagetype> &f) const
	{
	return mBits != f.mBits;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> &MxFlags<enumtype, storagetype>::operator =(enumtype e)
	{
	mBits = static_cast<storagetype>(e);
	return *this;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> &MxFlags<enumtype, storagetype>::operator|=(enumtype e)
	{
	mBits |= static_cast<storagetype>(e);
	return *this;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> &MxFlags<enumtype, storagetype>::operator|=(const MxFlags<enumtype, storagetype> &f)
	{
	mBits |= f.mBits;
	return *this;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> MxFlags<enumtype, storagetype>::operator| (enumtype e) const
	{
	MxFlags<enumtype, storagetype> out(*this);
	out |= e;
	return out;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> MxFlags<enumtype, storagetype>::operator| (const MxFlags<enumtype, storagetype> &f) const
	{
	MxFlags<enumtype, storagetype> out(*this);
	out |= f;
	return out;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> &MxFlags<enumtype, storagetype>::operator&=(enumtype e)
	{
	mBits &= static_cast<storagetype>(e);
	return *this;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> &MxFlags<enumtype, storagetype>::operator&=(const MxFlags<enumtype, storagetype> &f)
	{
	mBits &= f.mBits;
	return *this;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> MxFlags<enumtype, storagetype>::operator&(enumtype e) const
	{
	MxFlags<enumtype, storagetype> out = *this;
	out.mBits &= static_cast<storagetype>(e);
	return out;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype>  MxFlags<enumtype, storagetype>::operator& (const MxFlags<enumtype, storagetype> &f) const
	{
	MxFlags<enumtype, storagetype> out = *this;
	out.mBits &= f.mBits;
	return out;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> &MxFlags<enumtype, storagetype>::operator^=(enumtype e)
	{
	mBits ^= static_cast<storagetype>(e);
	return *this;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> &MxFlags<enumtype, storagetype>::operator^=(const MxFlags<enumtype, storagetype> &f)
	{
	mBits ^= f.mBits;
	return *this;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> MxFlags<enumtype, storagetype>::operator^ (enumtype e) const
	{
	MxFlags<enumtype, storagetype> out = *this;
	out.mBits ^= static_cast<storagetype>(e);
	return out;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> MxFlags<enumtype, storagetype>::operator^ (const MxFlags<enumtype, storagetype> &f) const
	{
	MxFlags<enumtype, storagetype> out = *this;
	out.mBits ^= f.mBits;
	return out;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype> MxFlags<enumtype, storagetype>::operator~ (void) const
	{
	MxFlags<enumtype, storagetype> out;
	out.mBits = storagetype(~mBits);
	return out;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype>::operator bool(void) const
	{
	return mBits ? true : false;
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype>::operator unsigned char(void) const
	{
	return static_cast<PxU8>(mBits);
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype>::operator unsigned short(void) const
	{
	return static_cast<PxU16>(mBits);
	}

template<typename enumtype, typename storagetype>
inline MxFlags<enumtype, storagetype>::operator unsigned(void) const
	{
	return static_cast<PxU32>(mBits);
	}

template<typename enumtype, typename storagetype>
inline void MxFlags<enumtype, storagetype>::clear(enumtype e)
	{
	mBits &= ~static_cast<storagetype>(e);
	}

