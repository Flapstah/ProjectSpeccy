#if !defined(__Z80_H__)
#define __Z80_H__

#include "common/platform_types.h"

#define LITTLE_ENDIAN

class CZ80
{
	public:
		CZ80(uint8* pMemory, float clockSpeedMHz);

	protected:
		const char* Get8BitRegisterString(uint8 threeBits);
		const char* Get16BitRegisterString(uint8 twoBits);

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

		//-----------------------------------------------------------------------------
		//	Exchange, Block Transfer and Search Group
		//-----------------------------------------------------------------------------

		void ImplementEXDEHL(void);
		void ImplementEXAFAF(void);
		void ImplementEXX(void);
		void ImplementEX_SP_HL(void);
		void ImplementEX_SP_IX(void);
		void ImplementEX_SP_IY(void);

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



		enum eConstants
		{
#if defined(LITTLE_ENDIAN)
			LO = 0,
			HI = 1
#else
			HI = 0,
			LO = 1
#endif // defined(LITTLE_ENDIAN)
		};

		enum eRegisters
		{
			// 8 bit register offsets (3 bits) (6 represents (HL))
			B = 0,
			C = 1,
			D = 2,
			E = 3,
			H = 4,
			L = 5,
			A = 7,

			// 16 bit register offsets (2 bits)
			BC = 0,
			DE = 1,
			HL = 2,
			SP = 3,

			// Register memory mapping
			eR_AF = 0,
			eR_A = eR_AF + HI,
			eR_F = eR_AF + LO,
			eR_BC = 2,
			eR_B = eR_BC + HI,
			eR_C = eR_BC + LO,
			eR_DE = 4,
			eR_D = eR_DE + HI,
			eR_E = eR_DE + LO,
			eR_HL = 6,
			eR_H = eR_HL + HI,
			eR_L = eR_HL + LO,
			eR_IX = 8,
			eR_IY = 10,
			eR_SP = 12,
			eR_PC = 14,
			eR_I = 16,
			eR_R = 17,
			eR_IFF1 = 18,
			eR_IFF2 = 19,
			eR_AFalt = 20,
			eR_BCalt = 22,
			eR_DEalt = 24,
			eR_HLalt = 26
		};

		// Register memory is mapped by reference to the actual registers so that
		// they can be accessed in a 'natural' way.
		uint8 m_RegisterMemory[32];

		uint8 m_16BitRegisterOffset[4];
		uint8 m_8BitRegisterOffset[8];

		uint16& m_AF;
		uint16& m_BC;
		uint16& m_DE;
		uint16& m_HL;
		uint16& m_IX;
		uint16& m_IY;
		uint16& m_SP;
		uint16& m_PC;
		uint16& m_AFalt;
		uint16& m_BCalt;
		uint16& m_DEalt;
		uint16& m_HLalt;
		uint8&	m_A;
		uint8&	m_F;
		uint8&	m_B;
		uint8&	m_C;
		uint8&	m_D;
		uint8&	m_E;
		uint8&	m_H;
		uint8&	m_L;
		uint8&	m_I;
		uint8&	m_R;
		uint8&	m_IFF1; // Only the bit that corresponds to the eF_PV flag is used
		uint8&	m_IFF2; // Only the bit that corresponds to the eF_PV flag is used

		uint32 m_tstate;
		uint8* m_Memory;

#if defined(LITTLE_ENDIAN)
#define REGISTER_ORDER(_hi_, _lo_)	\
		struct													\
		{																\
			uint8 m_ ## _lo_;							\
			uint8 m_ ## _hi_;							\
		};
#define PTR_PADDING_HI_LO(_hi_, _lo_)
#define PTR_PADDING_REG(_reg_)
#else
#define REGISTER_ORDER(_hi_, _lo_)	\
		struct													\
		{																\
			uint8 m_ ## _hi_;							\
			uint8 m_ ## _lo_;							\
		};
#define PTR_PADDING_HI_LO(_hi_, _lo_) uint8 m_ ## _hi_ ## _lo_ ## padding[sizeof(uint8*) - sizeof(uint16)]
#define PTR_PADDING_REG(_reg_) uint8 m_ ## _reg_ ## padding[sizeof(uint8*) - sizeof(uint16)]
#endif // defined(LITTLE_ENDIAN)

#define REGISTER_PAIR(_hi_, _lo_)					\
		union																	\
		{																			\
			uint16				m_ ## _hi_ ## _lo_;		\
			REGISTER_ORDER(_hi_, _lo_);					\
		};

#define REGISTER_16BIT(_reg_)											\
		union																					\
		{																							\
			uint16				m_ ## _reg_;									\
			REGISTER_ORDER(_reg_ ## _hi, _reg_ ## _lo);	\
		};

		// Semantic equivalent of REGISTER_16BIT, but syntactically 'nicer' when
		// used as a temporary 16 bit register, e.g. see ImplementLDA_nn_()
#define NON_MEMBER_REGISTER_16BIT(_reg_)	\
		union URegister16Bit									\
		{																			\
			uint16				m_val;								\
			REGISTER_ORDER(hi, lo);							\
		} _reg_;

		void	IncrementR(uint8 value)		{ m_R |= ((m_R + value) & 0x7F); }

	private:
};

#endif // !defined(__Z80_H__)
