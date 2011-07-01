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
		void ImplementLDr_HL_(void);
		void ImplementLDr_IXd_(void);
		void ImplementLDr_IYd_(void);
		void ImplementLD_HL_r(void);
		void ImplementLD_IXd_r(void);
		void ImplementLD_IYd_r(void);
		void ImplementLD_HL_n(void);
		void ImplementLD_IXd_n(void);
		void ImplementLD_IYd_n(void);
		void ImplementLDA_BC_(void);
		void ImplementLDA_DE_(void);
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
		void ImplementLDI(void);
		void ImplementLDIR(void);
		void ImplementLDD(void);
		void ImplementLDDR(void);
		void ImplementCPI(void);
		void ImplementCPIR(void);
		void ImplementCPD(void);
		void ImplementCPDR(void);

		//-----------------------------------------------------------------------------
		//	8-Bit Arithmetic Group
		//-----------------------------------------------------------------------------
		void ImplementADDAr(void);
		void ImplementADDAn(void);
		void ImplementADDA_HL_(void);
		void ImplementADDA_IXd_(void);
		void ImplementADDA_IYd_(void);
		void ImplementADCAr(void);
		void ImplementADCAn(void);
		void ImplementADCA_HL_(void);
		void ImplementADCA_IXd_(void);
		void ImplementADCA_IYd_(void);
		void ImplementSUBAr(void);
		void ImplementSUBAn(void);
		void ImplementSUBA_HL_(void);
		void ImplementSUBA_IXd_(void);
		void ImplementSUBA_IYd_(void);
		void ImplementSBCAr(void);
		void ImplementSBCAn(void);
		void ImplementSBCA_HL_(void);
		void ImplementSBCA_IXd_(void);
		void ImplementSBCA_IYd_(void);
		void ImplementANDAr(void);
		void ImplementANDAn(void);
		void ImplementANDA_HL_(void);
		void ImplementANDA_IXd_(void);
		void ImplementANDA_IYd_(void);
		void ImplementORAr(void);
		void ImplementORAn(void);
		void ImplementORA_HL_(void);
		void ImplementORA_IXd_(void);
		void ImplementORA_IYd_(void);
		void ImplementXORAr(void);
		void ImplementXORAn(void);
		void ImplementXORA_HL_(void);
		void ImplementXORA_IXd_(void);
		void ImplementXORA_IYd_(void);
		void ImplementCPAr(void);
		void ImplementCPAn(void);
		void ImplementCPA_HL_(void);
		void ImplementCPA_IXd_(void);
		void ImplementCPA_IYd_(void);
		void ImplementINCR(void);
		void ImplementINC_HL_(void);
		void ImplementINC_IXd_(void);
		void ImplementINC_IYd_(void);
		void ImplementDECm(void);

		//-----------------------------------------------------------------------------
		//	8-Bit Load Group
		//-----------------------------------------------------------------------------

		void DecodeLDrr(const uint8* pAddress, char* pMnemonic);
		void DecodeLDrn(const uint8* pAddress, char* pMnemonic);
		void DecodeLDr_IXd_(const uint8* pAddress, char* pMnemonic);
		void DecodeLDr_IYd_(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_HL_r(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_IXd_r(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_IYd_r(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_HL_n(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_IXd_n(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_IYd_n(const uint8* pAddress, char* pMnemonic);
		void DecodeLDA_BC_(const uint8* pAddress, char* pMnemonic);
		void DecodeLDA_DE_(const uint8* pAddress, char* pMnemonic);
		void DecodeLDA_nn_(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_BC_A(const uint8* pAddress, char* pMnemonic);
		void DecodeLD_DE_A(const uint8* pAddress, char* pMnemonic);
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

		//-----------------------------------------------------------------------------
		//	Exchange, Block Transfer and Search Group
		//-----------------------------------------------------------------------------

		void DecodeEXDEHL(const uint8* pAddress, char* pMnemonic);
		void DecodeEXAFAFalt(const uint8* pAddress, char* pMnemonic);
		void DecodeEXX(const uint8* pAddress, char* pMnemonic);
		void DecodeEX_SP_HL(const uint8* pAddress, char* pMnemonic);
		void DecodeEX_SP_IX(const uint8* pAddress, char* pMnemonic);
		void DecodeEX_SP_IY(const uint8* pAddress, char* pMnemonic);
		void DecodeLDI(const uint8* pAddress, char* pMnemonic);
		void DecodeLDIR(const uint8* pAddress, char* pMnemonic);
		void DecodeLDD(const uint8* pAddress, char* pMnemonic);
		void DecodeLDDR(const uint8* pAddress, char* pMnemonic);
		void DecodeCPI(const uint8* pAddress, char* pMnemonic);
		void DecodeCPIR(const uint8* pAddress, char* pMnemonic);
		void DecodeCPD(const uint8* pAddress, char* pMnemonic);
		void DecodeCPDR(const uint8* pAddress, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	8-Bit Arithmetic Group
		//-----------------------------------------------------------------------------

		void DecodeADDAr(const uint8* pAddress, char* pMnemonic);
		void DecodeADDAn(const uint8* pAddress, char* pMnemonic);
		void DecodeADDA_HL_(const uint8* pAddress, char* pMnemonic);
		void DecodeADDA_IXd_(const uint8* pAddress, char* pMnemonic);
		void DecodeADDA_IYd_(const uint8* pAddress, char* pMnemonic);
		void DecodeADCAs(const uint8* pAddress, char* pMnemonic);
		void DecodeSUBs(const uint8* pAddress, char* pMnemonic);
		void DecodeSBCas(const uint8* pAddress, char* pMnemonic);
		void DecodeANDs(const uint8* pAddress, char* pMnemonic);
		void DecodeORs(const uint8* pAddress, char* pMnemonic);
		void DecodeXORs(const uint8* pAddress, char* pMnemonic);
		void DecodeCPs(const uint8* pAddress, char* pMnemonic);
		void DecodeINCR(const uint8* pAddress, char* pMnemonic);
		void DecodeINC_HL_(const uint8* pAddress, char* pMnemonic);
		void DecodeINC_IXd_(const uint8* pAddress, char* pMnemonic);
		void DecodeINC_IYd_(const uint8* pAddress, char* pMnemonic);
		void ImplementDECm(const uint8* pAddress, char* pMnemonic);

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
			eR_BC = eR_AF + sizeof(uint16),
			eR_B = eR_BC + HI,
			eR_C = eR_BC + LO,
			eR_DE = eR_BC + sizeof(uint16),
			eR_D = eR_DE + HI,
			eR_E = eR_DE + LO,
			eR_HL = eR_DE + sizeof(uint16),
			eR_H = eR_HL + HI,
			eR_L = eR_HL + LO,
			eR_IX = eR_HL + sizeof(uint16),
			eR_IXh = eR_IX + HI,
			eR_IXl = eR_IX + LO,
			eR_IY = eR_IX + sizeof(uint16),
			eR_IYh = eR_IY + HI,
			eR_IYl = eR_IY + LO,
			eR_SP = eR_IY + sizeof(uint16),
			eR_PC = eR_SP + sizeof(uint16),
			eR_I = eR_PC + sizeof(uint8),
			eR_R = eR_I + sizeof(uint8),
			eR_IFF1 = eR_R + sizeof(uint8),
			eR_IFF2 = eR_IFF1 + sizeof(uint8),
			eR_AFalt = eR_IFF2 + sizeof(uint8),
			eR_BCalt = eR_AFalt + sizeof(uint16),
			eR_DEalt = eR_BCalt + sizeof(uint16),
			eR_HLalt = eR_DEalt + sizeof(uint16)
		};

		// Register memory is mapped by reference to the actual registers so that
		// they can be accessed in a 'natural' way.
		uint8		m_RegisterMemory[32];

		uint16& m_AF;
		uint16& m_BC;
		uint16& m_DE;
		uint16& m_HL;
		uint16& m_IX;
		uint8&	m_IXh;
		uint8&	m_IXl;
		uint16& m_IY;
		uint8&	m_IYh;
		uint8&	m_IYl;
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

		// Opcode register decode lookups
		uint8		m_16BitRegisterOffset[4];
		uint8		m_8BitRegisterOffset[8];

		uint32	m_tstate;
		uint8*	m_pMemory;
		float		m_clockSpeedMHz;

		void	IncrementR(uint8 value)		{ m_R |= ((m_R + value) & 0x7F); }

	private:
};

#endif // !defined(__Z80_H__)
