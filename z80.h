#if !defined(__Z80_H__)
#define __Z80_H__

#include "common/platform_types.h"

#define LITTLE_ENDIAN

class CZ80
{
	public:
	protected:
		//-----------------------------------------------------------------------------
		//	8-Bit Load Group
		//-----------------------------------------------------------------------------

		void ImplementLDrr(void);
		void ImplementLDrn(void);
		void ImplementLDrHL(void);
		void ImplementLDrIXd(void);
		void ImplementLDrIYd(void);
		void ImplementLDHLr(void);
		void ImplementLDIXdr(void);
		void ImplementLDIYdr(void);
		void ImplementLDHLn(void);
		void ImplementLDIXdn(void);
		void ImplementLDIYdn(void);
		void ImplementLDABC(void);
		void ImplementLDADE(void);
		void ImplementLDA_nn_(void);
		void ImplementLDBCA(void);
		void ImplementLDDEA(void);
		void ImplementLD_nn_A(void);
		void ImplementLDAI(void);
		void ImplementLDAR(void);
		void ImplementLDIA(void);
		void ImplementLDRA(void);

		//-----------------------------------------------------------------------------
		//	16-Bit Load Group
		//-----------------------------------------------------------------------------

		void ImplementLDddnn(void);
		void ImplementLDIXnn(void);
		void ImplementLDIYnn(void);
		void ImplementLDHL_nn_(void);
		void ImplementLDdd_nn_(void);
		void ImplementLDIX_nn_(void);
		void ImplementLDIY_nn_(void);
		void ImplementLD_nn_HL(void);
		void ImplementLD_nn_dd(void);
		void ImplementLD_nn_IX(void);
		void ImplementLD_nn_IY(void);
		void ImplementLDSPHL(void);
		void ImplementLDSPIX(void);
		void ImplementLDSPIY(void);
		void ImplementPUSHqq(void);
		void ImplementPUSHIX(void);
		void ImplementPUSHIY(void);
		void ImplementPOPqq(void);
		void ImplementPOPIX(void);
		void ImplementPOPIY(void);

/*
		//-----------------------------------------------------------------------------
		//	Exchange, Block Transfer and Search Group
		//-----------------------------------------------------------------------------

		void ImplementEXDEHL(void);

		//-----------------------------------------------------------------------------
		//	8-Bit Load Group
		//-----------------------------------------------------------------------------

		void DecodeLD(const uint8* pAddress, char* pMnemonic);
		void DecodeLDn(const uint8* pAddress, char* pMnemonic);
		void DecodeLDrIXd(const uint8* pAddress, char* pMnemonic);
		void DecodeLDrIYd(const uint8* pAddress, char* pMnemonic);
		void DecodeLDHLr(const uint8* pAddress, char* pMnemonic);
		void DecodeLDIXdr(const uint8* pAddress, char* pMnemonic);
		void DecodeLDIYdr(const uint8* pAddress, char* pMnemonic);
		void DecodeLDHLn(const uint8* pAddress, char* pMnemonic);
		void DecodeLDIXdn(const uint8* pAddress, char* pMnemonic);
		void DecodeLDIYdn(const uint8* pAddress, char* pMnemonic);
		void DecodeLDABC(const uint8* pAddress, char* pMnemonic);
		void DecodeLDADE(const uint8* pAddress, char* pMnemonic);
		void DecodeLDA_nn_(const uint8* pAddress, char* pMnemonic);
		void DecodeLDBCA(const uint8* pAddress, char* pMnemonic);
		void DecodeLDDEA(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_nn_A(const uint8* pAddress, char* pMnemonic);
		void DecodeLDAI(const uint8* pAddress, char* pMnemonic);
		void DecodeLDAR(const uint8* pAddress, char* pMnemonic);
		void DecodeLDIA(const uint8* pAddress, char* pMnemonic);
		void DecodeLDRA(const uint8* pAddress, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	16-Bit Load Group
		//-----------------------------------------------------------------------------

		void DecodeLDddnn(const uint8* pAddress, char* pMnemonic);
		void DecodeLDIXnn(const uint8* pAddress, char* pMnemonic);
		void DecodeLDIYnn(const uint8* pAddress, char* pMnemonic);
		void DecodeLDHL_nn_(const uint8* pAddress, char* pMnemonic);
		void DecodeLDdd_nn_(const uint8* pAddress, char* pMnemonic);
		void DecodeLDIX_nn_(const uint8* pAddress, char* pMnemonic);
		void DecodeLDIY_nn_(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_nn_HL(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_nn_dd(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_nn_IX(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_nn_IY(const uint8* pAddress, char* pMnemonic);
		void DecodeLDSPHL(const uint8* pAddress, char* pMnemonic);
		void DecodeLDSPIX(const uint8* pAddress, char* pMnemonic);
		void DecodeLDSPIY(const uint8* pAddress, char* pMnemonic);
		void DecodePUSHqq(const uint8* pAddress, char* pMnemonic);
		void DecodePUSHIX(const uint8* pAddress, char* pMnemonic);
		void DecodePUSHIY(const uint8* pAddress, char* pMnemonic);
		void DecodePOPqq(const uint8* pAddress, char* pMnemonic);
		void DecodePOPIX(const uint8* pAddress, char* pMnemonic);
		void DecodePOPIY(const uint8* pAddress, char* pMnemonic);

		*/
		enum eFlags
		{
			eF_C =	1 << 0,	// Carry
			eF_N =	1 << 1,	// Add/Subtract flag
			eF_PV =	1 << 2,	// Parity/Overflow
			eF_X =	1 << 3,	// A copy of bit 3 of the result
			eF_H =	1 << 4,	// Half-carry
			eF_Y =	1 << 5,	// A copy of bit 5 of the result
			eF_Z =	1 << 6,	// Zero
			eF_S =	1 << 7	// Sign

				// N.B. The only way to read X, Y and N is PUSH AF
		};

		void	IncrementR(uint8 value)		{ m_R |= ((m_R + value) & 0x7F); }

		struct SRegister16Bit
		{
			uint16	m_register;
			uint8*					operator&(void) const				{ return reinterpret_cast<uint8*>(m_register);											}
			uint8						operator*(void) const				{ return *(reinterpret_cast<uint8*>(m_register));										}
			SRegister16Bit	operator++(void)						{	SRegister16Bit temp; temp.m_register = m_register++; return temp;	}
			SRegister16Bit	operator--(void)						{	SRegister16Bit temp; temp.m_register = m_register--; return temp;	}
			SRegister16Bit&	operator++(int)							{ ++m_register; return *this;																				}
			SRegister16Bit&	operator--(int)							{ --m_register; return *this;																				}
			SRegister16Bit&	operator=(uint16 value)			{ m_register = value; return *this;																	}
											operator uint16(void) const	{ return m_register;																								}
			void						SetRegisterPair(uint8& hi, uint8& lo) const { uint8* p = reinterpret_cast<uint8*>(m_register); lo = *p; hi = *++p; }
		};

#if defined(LITTLE_ENDIAN)
#define REGISTER_ORDER(_hi_, _lo_)	\
		struct													\
		{																\
			uint8 m_ ## _lo_;							\
			uint8 m_ ## _hi_;							\
		};
#else
#define REGISTER_ORDER(_hi_, _lo_)	\
		struct													\
		{																\
			uint8 m_ ## _hi_;							\
			uint8 m_ ## _lo_;							\
		};
#endif // defined(LITTLE_ENDIAN)

#define REGISTER_PAIR(_hi_, _lo_)					\
		union																	\
		{																			\
			SRegister16Bit m_ ## _hi_ ## _lo_;	\
			REGISTER_ORDER(_hi_, _lo_);					\
		};

		uint8&	Get8BitRegister(uint8 threeBits);
		const char* Get8BitRegisterString(uint8 threeBits);

		SRegister16Bit&	Get16BitRegister(uint8 twoBits);
		const char* Get16BitRegisterString(uint8 twoBits);

		REGISTER_PAIR(A,F);
		REGISTER_PAIR(B,C);
		REGISTER_PAIR(D,E);
		REGISTER_PAIR(H,L);
		SRegister16Bit m_IX;
		SRegister16Bit m_IY;
		REGISTER_PAIR(I,R);
		SRegister16Bit m_SP;
		SRegister16Bit m_PC;
		uint16 _AF_;
		uint16 _BC_;
		uint16 _DE_;
		uint16 _HL_;
		uint8 IFF1; // Only the bit that corresponds to the eF_PV flag is used
		uint8 IFF2; // Only the bit that corresponds to the eF_PV flag is used

		REGISTER_PAIR(T,M); // Temporary register pair used for nn/(nn) type instructions
		uint32 m_tstate;

	private:
};

#endif // !defined(__Z80_H__)
