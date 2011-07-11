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
		void ImplementSUBr(void);
		void ImplementSUBn(void);
		void ImplementSUB_HL_(void);
		void ImplementSUB_IXd_(void);
		void ImplementSUB_IYd_(void);
		void ImplementSBCAr(void);
		void ImplementSBCAn(void);
		void ImplementSBCA_HL_(void);
		void ImplementSBCA_IXd_(void);
		void ImplementSBCA_IYd_(void);
		void ImplementANDr(void);
		void ImplementANDn(void);
		void ImplementAND_HL_(void);
		void ImplementAND_IXd_(void);
		void ImplementAND_IYd_(void);
		void ImplementORr(void);
		void ImplementORn(void);
		void ImplementOR_HL_(void);
		void ImplementOR_IXd_(void);
		void ImplementOR_IYd_(void);
		void ImplementXORr(void);
		void ImplementXORn(void);
		void ImplementXOR_HL_(void);
		void ImplementXOR_IXd_(void);
		void ImplementXOR_IYd_(void);
		void ImplementCPr(void);
		void ImplementCPn(void);
		void ImplementCP_HL_(void);
		void ImplementCP_IXd_(void);
		void ImplementCP_IYd_(void);
		void ImplementINCr(void);
		void ImplementINC_HL_(void);
		void ImplementINC_IXd_(void);
		void ImplementINC_IYd_(void);
		void ImplementDECr(void);
		void ImplementDEC_HL_(void);
		void ImplementDEC_IXd_(void);
		void ImplementDEC_IYd_(void);

		//-----------------------------------------------------------------------------
		//	General-Purpose Arithmetic and CPU Control Groups
		//-----------------------------------------------------------------------------

		void ImplementDAA(void);
		void ImplementCPL(void);
		void ImplementNEG(void);
		void ImplementCCF(void);
		void ImplementSCF(void);
		void ImplementNOP(void);
		void ImplementHALT(void);
		void ImplementDI(void);
		void ImplementEI(void);
		void ImplementIM0(void);
		void ImplementIM1(void);
		void ImplementIM2(void);

		//-----------------------------------------------------------------------------
		//	16-Bit Arithmetic Group
		//-----------------------------------------------------------------------------

		void ImplementADDHLdd(void);
		void ImplementADCHLdd(void);
		void ImplementSBCHLdd(void);
		void ImplementADDIXdd(void);
		void ImplementADDIYdd(void);
		void ImplementINCdd(void);
		void ImplementINCIXdd(void);
		void ImplementINCIYdd(void);
		void ImplementDECdd(void);
		void ImplementDECIXdd(void);
		void ImplementDECIYdd(void);

		//-----------------------------------------------------------------------------
		//	Rotate and Shift Group
		//-----------------------------------------------------------------------------

		void ImplementRLCA(void);
		void ImplementRLA(void);
		void ImplementRRCA(void);
		void ImplementRRA(void);
		void ImplementRLCr(void);
		void ImplementRLC_HL_(void);
		void ImplementRLC_IXd_(void);
		void ImplementRLC_IYd_(void);
		void ImplementRLr(void);
		void ImplementRL_HL_(void);
		void ImplementRL_IXd_(void);
		void ImplementRL_IYd_(void);
		void ImplementRRCr(void);
		void ImplementRRC_HL_(void);
		void ImplementRRC_IXd_(void);
		void ImplementRRC_IYd_(void);
		void ImplementRRr(void);
		void ImplementRR_HL_(void);
		void ImplementRR_IXd_(void);
		void ImplementRR_IYd_(void);
		void ImplementSLAr(void);
		void ImplementSLA_HL_(void);
		void ImplementSLA_IXd_(void);
		void ImplementSLA_IYd_(void);
		void ImplementSRAr(void);
		void ImplementSRA_HL_(void);
		void ImplementSRA_IXd_(void);
		void ImplementSRA_IYd_(void);
		void ImplementSRLr(void);
		void ImplementSRL_HL_(void);
		void ImplementSRL_IXd_(void);
		void ImplementSRL_IYd_(void);
		void ImplementRLD(void);
		void ImplementRRD(void);

		//-----------------------------------------------------------------------------
		//	Bit Set, Reset and Test Group
		//-----------------------------------------------------------------------------

		void ImplementBITbr(void);
		void ImplementBITb_HL_(void);
		void ImplementBITb_IXd_(void);
		void ImplementBITb_IYd_(void);
		void ImplementSETbr(void);
		void ImplementSETb_HL_(void);
		void ImplementSETb_IXd_(void);
		void ImplementSETb_IYd_(void);
		void ImplementRESbr(void);
		void ImplementRESb_HL_(void);
		void ImplementRESb_IXd_(void);
		void ImplementRESb_IYd_(void);

		//-----------------------------------------------------------------------------
		//	8-Bit Load Group
		//-----------------------------------------------------------------------------

		void DecodeLDrr(uint16& address, char* pMnemonic);
		void DecodeLDrn(uint16& address, char* pMnemonic);
		void DecodeLDr_IXd_(uint16& address, char* pMnemonic);
		void DecodeLDr_IYd_(uint16& address, char* pMnemonic);
		void DecodeLD_HL_r(uint16& address, char* pMnemonic);
		void DecodeLD_IXd_r(uint16& address, char* pMnemonic);
		void DecodeLD_IYd_r(uint16& address, char* pMnemonic);
		void DecodeLD_HL_n(uint16& address, char* pMnemonic);
		void DecodeLD_IXd_n(uint16& address, char* pMnemonic);
		void DecodeLD_IYd_n(uint16& address, char* pMnemonic);
		void DecodeLDA_BC_(uint16& address, char* pMnemonic);
		void DecodeLDA_DE_(uint16& address, char* pMnemonic);
		void DecodeLDA_nn_(uint16& address, char* pMnemonic);
		void DecodeLD_BC_A(uint16& address, char* pMnemonic);
		void DecodeLD_DE_A(uint16& address, char* pMnemonic);
		void DecodeLD_nn_A(uint16& address, char* pMnemonic);
		void DecodeLDAI(uint16& address, char* pMnemonic);
		void DecodeLDAR(uint16& address, char* pMnemonic);
		void DecodeLDIA(uint16& address, char* pMnemonic);
		void DecodeLDRA(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	16-Bit Load Group
		//-----------------------------------------------------------------------------

		void DecodeLDddnn(uint16& address, char* pMnemonic);
		void DecodeLDIXnn(uint16& address, char* pMnemonic);
		void DecodeLDIYnn(uint16& address, char* pMnemonic);
		void DecodeLDHL_nn_(uint16& address, char* pMnemonic);
		void DecodeLDdd_nn_(uint16& address, char* pMnemonic);
		void DecodeLDIX_nn_(uint16& address, char* pMnemonic);
		void DecodeLDIY_nn_(uint16& address, char* pMnemonic);
		void DecodeLD_nn_HL(uint16& address, char* pMnemonic);
		void DecodeLD_nn_dd(uint16& address, char* pMnemonic);
		void DecodeLD_nn_IX(uint16& address, char* pMnemonic);
		void DecodeLD_nn_IY(uint16& address, char* pMnemonic);
		void DecodeLDSPHL(uint16& address, char* pMnemonic);
		void DecodeLDSPIX(uint16& address, char* pMnemonic);
		void DecodeLDSPIY(uint16& address, char* pMnemonic);
		void DecodePUSHqq(uint16& address, char* pMnemonic);
		void DecodePUSHIX(uint16& address, char* pMnemonic);
		void DecodePUSHIY(uint16& address, char* pMnemonic);
		void DecodePOPqq(uint16& address, char* pMnemonic);
		void DecodePOPIX(uint16& address, char* pMnemonic);
		void DecodePOPIY(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	Exchange, Block Transfer and Search Group
		//-----------------------------------------------------------------------------

		void DecodeEXDEHL(uint16& address, char* pMnemonic);
		void DecodeEXAFAFalt(uint16& address, char* pMnemonic);
		void DecodeEXX(uint16& address, char* pMnemonic);
		void DecodeEX_SP_HL(uint16& address, char* pMnemonic);
		void DecodeEX_SP_IX(uint16& address, char* pMnemonic);
		void DecodeEX_SP_IY(uint16& address, char* pMnemonic);
		void DecodeLDI(uint16& address, char* pMnemonic);
		void DecodeLDIR(uint16& address, char* pMnemonic);
		void DecodeLDD(uint16& address, char* pMnemonic);
		void DecodeLDDR(uint16& address, char* pMnemonic);
		void DecodeCPI(uint16& address, char* pMnemonic);
		void DecodeCPIR(uint16& address, char* pMnemonic);
		void DecodeCPD(uint16& address, char* pMnemonic);
		void DecodeCPDR(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	8-Bit Arithmetic Group
		//-----------------------------------------------------------------------------

		void DecodeADDAr(uint16& address, char* pMnemonic);
		void DecodeADDAn(uint16& address, char* pMnemonic);
		void DecodeADDA_IXd_(uint16& address, char* pMnemonic);
		void DecodeADDA_IYd_(uint16& address, char* pMnemonic);
		void DecodeADCAr(uint16& address, char* pMnemonic);
		void DecodeADCAn(uint16& address, char* pMnemonic);
		void DecodeADCA_IXd_(uint16& address, char* pMnemonic);
		void DecodeADCA_IYd_(uint16& address, char* pMnemonic);
		void DecodeSUBr(uint16& address, char* pMnemonic);
		void DecodeSUBn(uint16& address, char* pMnemonic);
		void DecodeSUB_IXd_(uint16& address, char* pMnemonic);
		void DecodeSUB_IYd_(uint16& address, char* pMnemonic);
		void DecodeSBCAr(uint16& address, char* pMnemonic);
		void DecodeSBCAn(uint16& address, char* pMnemonic);
		void DecodeSBCA_IXd_(uint16& address, char* pMnemonic);
		void DecodeSBCA_IYd_(uint16& address, char* pMnemonic);
		void DecodeANDr(uint16& address, char* pMnemonic);
		void DecodeANDn(uint16& address, char* pMnemonic);
		void DecodeAND_IXd_(uint16& address, char* pMnemonic);
		void DecodeAND_IYd_(uint16& address, char* pMnemonic);
		void DecodeORr(uint16& address, char* pMnemonic);
		void DecodeORn(uint16& address, char* pMnemonic);
		void DecodeOR_IXd_(uint16& address, char* pMnemonic);
		void DecodeOR_IYd_(uint16& address, char* pMnemonic);
		void DecodeXORr(uint16& address, char* pMnemonic);
		void DecodeXORn(uint16& address, char* pMnemonic);
		void DecodeXOR_IXd_(uint16& address, char* pMnemonic);
		void DecodeXOR_IYd_(uint16& address, char* pMnemonic);
		void DecodeCPr(uint16& address, char* pMnemonic);
		void DecodeCPn(uint16& address, char* pMnemonic);
		void DecodeCP_IXd_(uint16& address, char* pMnemonic);
		void DecodeCP_IYd_(uint16& address, char* pMnemonic);
		void DecodeINCr(uint16& address, char* pMnemonic);
		void DecodeINC_IXd_(uint16& address, char* pMnemonic);
		void DecodeINC_IYd_(uint16& address, char* pMnemonic);
		void DecodeDECr(uint16& address, char* pMnemonic);
		void DecodeDEC_IXd_(uint16& address, char* pMnemonic);
		void DecodeDEC_IYd_(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	General-Purpose Arithmetic and CPU Control Groups
		//-----------------------------------------------------------------------------

		void DecodeDAA(uint16& address, char* pMnemonic);
		void DecodeCPL(uint16& address, char* pMnemonic);
		void DecodeNEG(uint16& address, char* pMnemonic);
		void DecodeCCF(uint16& address, char* pMnemonic);
		void DecodeSCF(uint16& address, char* pMnemonic);
		void DecodeNOP(uint16& address, char* pMnemonic);
		void DecodeHALT(uint16& address, char* pMnemonic);
		void DecodeDI(uint16& address, char* pMnemonic);
		void DecodeEI(uint16& address, char* pMnemonic);
		void DecodeIM0(uint16& address, char* pMnemonic);
		void DecodeIM1(uint16& address, char* pMnemonic);
		void DecodeIM2(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	16-Bit Arithmetic Group
		//-----------------------------------------------------------------------------

		void DecodeADDHLdd(uint16& address, char* pMnemonic);
		void DecodeADCHLdd(uint16& address, char* pMnemonic);
		void DecodeSBCHLdd(uint16& address, char* pMnemonic);
		void DecodeADDIXdd(uint16& address, char* pMnemonic);
		void DecodeADDIYdd(uint16& address, char* pMnemonic);
		void DecodeINCdd(uint16& address, char* pMnemonic);
		void DecodeINCIXdd(uint16& address, char* pMnemonic);
		void DecodeINCIYdd(uint16& address, char* pMnemonic);
		void DecodeDECdd(uint16& address, char* pMnemonic);
		void DecodeDECIXdd(uint16& address, char* pMnemonic);
		void DecodeDECIYdd(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	Rotate and Shift Group
		//-----------------------------------------------------------------------------

		void DecodeRLCA(uint16& address, char* pMnemonic);
		void DecodeRLA(uint16& address, char* pMnemonic);
		void DecodeRRCA(uint16& address, char* pMnemonic);
		void DecodeRRA(uint16& address, char* pMnemonic);
		void DecodeRLCr(uint16& address, char* pMnemonic);
		void DecodeRLC_HL_(uint16& address, char* pMnemonic);
		void DecodeRLC_IXd_(uint16& address, char* pMnemonic);
		void DecodeRLC_IYd_(uint16& address, char* pMnemonic);
		void DecodeRLr(uint16& address, char* pMnemonic);
		void DecodeRL_HL_(uint16& address, char* pMnemonic);
		void DecodeRL_IXd_(uint16& address, char* pMnemonic);
		void DecodeRL_IYd_(uint16& address, char* pMnemonic);
		void DecodeRRCr(uint16& address, char* pMnemonic);
		void DecodeRRC_HL_(uint16& address, char* pMnemonic);
		void DecodeRRC_IXd_(uint16& address, char* pMnemonic);
		void DecodeRRC_IYd_(uint16& address, char* pMnemonic);
		void DecodeRRr(uint16& address, char* pMnemonic);
		void DecodeRR_HL_(uint16& address, char* pMnemonic);
		void DecodeRR_IXd_(uint16& address, char* pMnemonic);
		void DecodeRR_IYd_(uint16& address, char* pMnemonic);
		void DecodeSLAr(uint16& address, char* pMnemonic);
		void DecodeSLA_HL_(uint16& address, char* pMnemonic);
		void DecodeSLA_IXd_(uint16& address, char* pMnemonic);
		void DecodeSLA_IYd_(uint16& address, char* pMnemonic);
		void DecodeSRAr(uint16& address, char* pMnemonic);
		void DecodeSRA_HL_(uint16& address, char* pMnemonic);
		void DecodeSRA_IXd_(uint16& address, char* pMnemonic);
		void DecodeSRA_IYd_(uint16& address, char* pMnemonic);
		void DecodeSRLr(uint16& address, char* pMnemonic);
		void DecodeSRL_HL_(uint16& address, char* pMnemonic);
		void DecodeSRL_IXd_(uint16& address, char* pMnemonic);
		void DecodeSRL_IYd_(uint16& address, char* pMnemonic);
		void DecodeRLD(uint16& address, char* pMnemonic);
		void DecodeRRD(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	Bit Set, Reset and Test Group
		//-----------------------------------------------------------------------------

		void DecodeBITbr(uint16& address, char* pMnemonic);
		void DecodeBITb_HL_(uint16& address, char* pMnemonic);
		void DecodeBITb_IXd_(uint16& address, char* pMnemonic);
		void DecodeBITb_IYd_(uint16& address, char* pMnemonic);
		void DecodeSETbr(uint16& address, char* pMnemonic);
		void DecodeSETb_HL_(uint16& address, char* pMnemonic);
		void DecodeSETb_IXd_(uint16& address, char* pMnemonic);
		void DecodeSETb_IYd_(uint16& address, char* pMnemonic);
		void DecodeRESbr(uint16& address, char* pMnemonic);
		void DecodeRESb_HL_(uint16& address, char* pMnemonic);
		void DecodeRESb_IXd_(uint16& address, char* pMnemonic);
		void DecodeRESb_IYd_(uint16& address, char* pMnemonic);

		struct SProcessorState
		{
			uint16 m_InterruptMode : 2;
			uint16 m_IFF1 : 1;
			uint16 m_IFF2 : 1;
		};

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
			eR_AFalt = eR_R + sizeof(uint8),
			eR_BCalt = eR_AFalt + sizeof(uint16),
			eR_DEalt = eR_BCalt + sizeof(uint16),
			eR_HLalt = eR_DEalt + sizeof(uint16),
			eR_State = eR_HLalt + sizeof(uint16),
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
		SProcessorState& m_State;

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
