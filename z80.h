#if !defined(__Z80_H__)
#define __Z80_H__

#include "common/platform_types.h"

#if !defined(LITTLE_ENDIAN)
#define LITTLE_ENDIAN
#endif

class CZ80
{
	public:
		CZ80(uint8* pMemory, float clockSpeedMHz);

		void Reset(void);
		float Update(float milliseconds);
		float SingleStep(void);

		bool GetEnableDebug(void) const;
		void SetEnableDebug(bool set);
		bool GetEnableUnattendedDebug(void) const;
		void SetEnableUnattendedDebug(bool set);
		bool GetEnableOutputStatus(void) const;
		void SetEnableOutputStatus(bool set);
		bool GetEnableBreakpoints(void) const;
		void SetEnableBreakpoints(bool set);
		bool GetEnableProgramFlowBreakpoints(void) const;
		void SetEnableProgramFlowBreakpoints(bool set);

	protected:
		void OutputStatus(void);
		void OutputInstruction(uint16 address);
		void HitBreakpoint(const char* type);
		void HandleIllegalOpcode(void);

		uint32 Step(void);
		void Decode(uint16& address, char* pMnemonic);
		uint8 HandleArithmeticAddFlags(uint16 source1, uint16 source2);
		uint8 HandleArithmeticSubtractFlags(uint16 source1, uint16 source2);

		bool WriteMemory(uint16 address, uint8 byte);
		bool ReadMemory(uint16 address, uint8& byte);
		bool WritePort(uint16 address, uint8 byte);
		bool ReadPort(uint16 address, uint8& byte);
		
		const char* Get8BitRegisterString(uint8 threeBits);
		const char* Get16BitRegisterString(uint8 twoBits);
		const char* GetConditionString(uint8 threeBits);
		bool IsConditionTrue(uint8 threeBits);

		//-----------------------------------------------------------------------------
		//	8-Bit Load Group
		//-----------------------------------------------------------------------------

		uint32 ImplementLDrr(void);
		uint32 ImplementLDrn(void);
		uint32 ImplementLDr_HL_(void);
		uint32 ImplementLDr_IXd_(void);
		uint32 ImplementLDr_IYd_(void);
		uint32 ImplementLD_HL_r(void);
		uint32 ImplementLD_IXd_r(void);
		uint32 ImplementLD_IYd_r(void);
		uint32 ImplementLD_HL_n(void);
		uint32 ImplementLD_IXd_n(void);
		uint32 ImplementLD_IYd_n(void);
		uint32 ImplementLDA_BC_(void);
		uint32 ImplementLDA_DE_(void);
		uint32 ImplementLDA_nn_(void);
		uint32 ImplementLD_BC_A(void);
		uint32 ImplementLD_DE_A(void);
		uint32 ImplementLD_nn_A(void);
		uint32 ImplementLDAI(void);
		uint32 ImplementLDAR(void);
		uint32 ImplementLDIA(void);
		uint32 ImplementLDRA(void);

		//-----------------------------------------------------------------------------
		//	16-Bit Load Group
		//-----------------------------------------------------------------------------

		uint32 ImplementLDddnn(void);
		uint32 ImplementLDIXnn(void);
		uint32 ImplementLDIYnn(void);
		uint32 ImplementLDHL_nn_(void);
		uint32 ImplementLDdd_nn_(void);
		uint32 ImplementLDIX_nn_(void);
		uint32 ImplementLDIY_nn_(void);
		uint32 ImplementLD_nn_HL(void);
		uint32 ImplementLD_nn_dd(void);
		uint32 ImplementLD_nn_IX(void);
		uint32 ImplementLD_nn_IY(void);
		uint32 ImplementLDSPHL(void);
		uint32 ImplementLDSPIX(void);
		uint32 ImplementLDSPIY(void);
		uint32 ImplementPUSHqq(void);
		uint32 ImplementPUSHAF(void);
		uint32 ImplementPUSHIX(void);
		uint32 ImplementPUSHIY(void);
		uint32 ImplementPOPqq(void);
		uint32 ImplementPOPAF(void);
		uint32 ImplementPOPIX(void);
		uint32 ImplementPOPIY(void);

		//-----------------------------------------------------------------------------
		//	Exchange, Block Transfer and Search Group
		//-----------------------------------------------------------------------------

		uint32 ImplementEXDEHL(void);
		uint32 ImplementEXAFAF(void);
		uint32 ImplementEXX(void);
		uint32 ImplementEX_SP_HL(void);
		uint32 ImplementEX_SP_IX(void);
		uint32 ImplementEX_SP_IY(void);
		uint32 ImplementLDI(void);
		uint32 ImplementLDIR(void);
		uint32 ImplementLDD(void);
		uint32 ImplementLDDR(void);
		uint32 ImplementCPI(void);
		uint32 ImplementCPIR(void);
		uint32 ImplementCPD(void);
		uint32 ImplementCPDR(void);

		//-----------------------------------------------------------------------------
		//	8-Bit Arithmetic Group
		//-----------------------------------------------------------------------------
		uint32 ImplementADDAr(void);
		uint32 ImplementADDAn(void);
		uint32 ImplementADDA_HL_(void);
		uint32 ImplementADDA_IXd_(void);
		uint32 ImplementADDA_IYd_(void);
		uint32 ImplementADCAr(void);
		uint32 ImplementADCAn(void);
		uint32 ImplementADCA_HL_(void);
		uint32 ImplementADCA_IXd_(void);
		uint32 ImplementADCA_IYd_(void);
		uint32 ImplementSUBr(void);
		uint32 ImplementSUBn(void);
		uint32 ImplementSUB_HL_(void);
		uint32 ImplementSUB_IXd_(void);
		uint32 ImplementSUB_IYd_(void);
		uint32 ImplementSBCAr(void);
		uint32 ImplementSBCAn(void);
		uint32 ImplementSBCA_HL_(void);
		uint32 ImplementSBCA_IXd_(void);
		uint32 ImplementSBCA_IYd_(void);
		uint32 ImplementANDr(void);
		uint32 ImplementANDn(void);
		uint32 ImplementAND_HL_(void);
		uint32 ImplementAND_IXd_(void);
		uint32 ImplementAND_IYd_(void);
		uint32 ImplementORr(void);
		uint32 ImplementORn(void);
		uint32 ImplementOR_HL_(void);
		uint32 ImplementOR_IXd_(void);
		uint32 ImplementOR_IYd_(void);
		uint32 ImplementXORr(void);
		uint32 ImplementXORn(void);
		uint32 ImplementXOR_HL_(void);
		uint32 ImplementXOR_IXd_(void);
		uint32 ImplementXOR_IYd_(void);
		uint32 ImplementCPr(void);
		uint32 ImplementCPn(void);
		uint32 ImplementCP_HL_(void);
		uint32 ImplementCP_IXd_(void);
		uint32 ImplementCP_IYd_(void);
		uint32 ImplementINCr(void);
		uint32 ImplementINC_HL_(void);
		uint32 ImplementINC_IXd_(void);
		uint32 ImplementINC_IYd_(void);
		uint32 ImplementDECr(void);
		uint32 ImplementDEC_HL_(void);
		uint32 ImplementDEC_IXd_(void);
		uint32 ImplementDEC_IYd_(void);

		//-----------------------------------------------------------------------------
		//	General-Purpose Arithmetic and CPU Control Groups
		//-----------------------------------------------------------------------------

		uint32 ImplementDAA(void);
		uint32 ImplementCPL(void);
		uint32 ImplementNEG(void);
		uint32 ImplementCCF(void);
		uint32 ImplementSCF(void);
		uint32 ImplementNOP(void);
		uint32 ImplementHALT(void);
		uint32 ImplementDI(void);
		uint32 ImplementEI(void);
		uint32 ImplementIM0(void);
		uint32 ImplementIM1(void);
		uint32 ImplementIM2(void);

		//-----------------------------------------------------------------------------
		//	16-Bit Arithmetic Group
		//-----------------------------------------------------------------------------

		uint32 ImplementADDHLdd(void);
		uint32 ImplementADCHLdd(void);
		uint32 ImplementSBCHLdd(void);
		uint32 ImplementADDIXdd(void);
		uint32 ImplementADDIYdd(void);
		uint32 ImplementINCdd(void);
		uint32 ImplementINCIX(void);
		uint32 ImplementINCIY(void);
		uint32 ImplementDECdd(void);
		uint32 ImplementDECIX(void);
		uint32 ImplementDECIY(void);

		//-----------------------------------------------------------------------------
		//	Rotate and Shift Group
		//-----------------------------------------------------------------------------

		uint32 ImplementRLCA(void);
		uint32 ImplementRLA(void);
		uint32 ImplementRRCA(void);
		uint32 ImplementRRA(void);
		uint32 ImplementRLCr(void);
		uint32 ImplementRLC_HL_(void);
		uint32 ImplementRLC_IXd_(void);
		uint32 ImplementRLC_IYd_(void);
		uint32 ImplementRLr(void);
		uint32 ImplementRL_HL_(void);
		uint32 ImplementRL_IXd_(void);
		uint32 ImplementRL_IYd_(void);
		uint32 ImplementRRCr(void);
		uint32 ImplementRRC_HL_(void);
		uint32 ImplementRRC_IXd_(void);
		uint32 ImplementRRC_IYd_(void);
		uint32 ImplementRRr(void);
		uint32 ImplementRR_HL_(void);
		uint32 ImplementRR_IXd_(void);
		uint32 ImplementRR_IYd_(void);
		uint32 ImplementSLAr(void);
		uint32 ImplementSLA_HL_(void);
		uint32 ImplementSLA_IXd_(void);
		uint32 ImplementSLA_IYd_(void);
		uint32 ImplementSRAr(void);
		uint32 ImplementSRA_HL_(void);
		uint32 ImplementSRA_IXd_(void);
		uint32 ImplementSRA_IYd_(void);
		uint32 ImplementSRLr(void);
		uint32 ImplementSRL_HL_(void);
		uint32 ImplementSRL_IXd_(void);
		uint32 ImplementSRL_IYd_(void);
		uint32 ImplementRLD(void);
		uint32 ImplementRRD(void);

		//-----------------------------------------------------------------------------
		//	Bit Set, Reset and Test Group
		//-----------------------------------------------------------------------------

		uint32 ImplementBITbr(void);
		uint32 ImplementBITb_HL_(void);
		uint32 ImplementBITb_IXd_(void);
		uint32 ImplementBITb_IYd_(void);
		uint32 ImplementSETbr(void);
		uint32 ImplementSETb_HL_(void);
		uint32 ImplementSETb_IXd_(void);
		uint32 ImplementSETb_IYd_(void);
		uint32 ImplementRESbr(void);
		uint32 ImplementRESb_HL_(void);
		uint32 ImplementRESb_IXd_(void);
		uint32 ImplementRESb_IYd_(void);

		//-----------------------------------------------------------------------------
		//	Jump Group
		//-----------------------------------------------------------------------------

		uint32 ImplementJPnn(void);
		uint32 ImplementJPccnn(void);
		uint32 ImplementJRe(void);
		uint32 ImplementJRCe(void);
		uint32 ImplementJRNCe(void);
		uint32 ImplementJRZe(void);
		uint32 ImplementJRNZe(void);
		uint32 ImplementJP_HL_(void);
		uint32 ImplementJP_IX_(void);
		uint32 ImplementJP_IY_(void);
		uint32 ImplementDJNZe(void);

		//-----------------------------------------------------------------------------
		//	Call and Return Group
		//-----------------------------------------------------------------------------

		uint32 ImplementCALLnn(void);
		uint32 ImplementCALLccnn(void);
		uint32 ImplementRET(void);
		uint32 ImplementRETcc(void);
		uint32 ImplementRETI(void);
		uint32 ImplementRETN(void);
		uint32 ImplementRSTp(void);

		//-----------------------------------------------------------------------------
		//	Input and Output Group
		//-----------------------------------------------------------------------------

		uint32 ImplementINA_n_(void);
		uint32 ImplementINr_C_(void);
		uint32 ImplementINI(void);
		uint32 ImplementINIR(void);
		uint32 ImplementIND(void);
		uint32 ImplementINDR(void);
		uint32 ImplementOUT_n_A(void);
		uint32 ImplementOUT_C_r(void);
		uint32 ImplementOUTI(void);
		uint32 ImplementOTIR(void);
		uint32 ImplementOUTD(void);
		uint32 ImplementOTDR(void);

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
		void DecodePUSHAF(uint16& address, char* pMnemonic);
		void DecodePUSHIX(uint16& address, char* pMnemonic);
		void DecodePUSHIY(uint16& address, char* pMnemonic);
		void DecodePOPqq(uint16& address, char* pMnemonic);
		void DecodePOPAF(uint16& address, char* pMnemonic);
		void DecodePOPIX(uint16& address, char* pMnemonic);
		void DecodePOPIY(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	Exchange, Block Transfer and Search Group
		//-----------------------------------------------------------------------------

		void DecodeEXDEHL(uint16& address, char* pMnemonic);
		void DecodeEXAFAF(uint16& address, char* pMnemonic);
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
		void DecodeINCIX(uint16& address, char* pMnemonic);
		void DecodeINCIY(uint16& address, char* pMnemonic);
		void DecodeDECdd(uint16& address, char* pMnemonic);
		void DecodeDECIX(uint16& address, char* pMnemonic);
		void DecodeDECIY(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	Rotate and Shift Group
		//-----------------------------------------------------------------------------

		void DecodeRLCA(uint16& address, char* pMnemonic);
		void DecodeRLA(uint16& address, char* pMnemonic);
		void DecodeRRCA(uint16& address, char* pMnemonic);
		void DecodeRRA(uint16& address, char* pMnemonic);
		void DecodeRLCr(uint16& address, char* pMnemonic);
		void DecodeRLC_IXd_(uint16& address, char* pMnemonic);
		void DecodeRLC_IYd_(uint16& address, char* pMnemonic);
		void DecodeRLr(uint16& address, char* pMnemonic);
		void DecodeRL_IXd_(uint16& address, char* pMnemonic);
		void DecodeRL_IYd_(uint16& address, char* pMnemonic);
		void DecodeRRCr(uint16& address, char* pMnemonic);
		void DecodeRRC_IXd_(uint16& address, char* pMnemonic);
		void DecodeRRC_IYd_(uint16& address, char* pMnemonic);
		void DecodeRRr(uint16& address, char* pMnemonic);
		void DecodeRR_IXd_(uint16& address, char* pMnemonic);
		void DecodeRR_IYd_(uint16& address, char* pMnemonic);
		void DecodeSLAr(uint16& address, char* pMnemonic);
		void DecodeSLA_IXd_(uint16& address, char* pMnemonic);
		void DecodeSLA_IYd_(uint16& address, char* pMnemonic);
		void DecodeSRAr(uint16& address, char* pMnemonic);
		void DecodeSRA_IXd_(uint16& address, char* pMnemonic);
		void DecodeSRA_IYd_(uint16& address, char* pMnemonic);
		void DecodeSRLr(uint16& address, char* pMnemonic);
		void DecodeSRL_IXd_(uint16& address, char* pMnemonic);
		void DecodeSRL_IYd_(uint16& address, char* pMnemonic);
		void DecodeRLD(uint16& address, char* pMnemonic);
		void DecodeRRD(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	Bit Set, Reset and Test Group
		//-----------------------------------------------------------------------------

		void DecodeBITbr(uint16& address, char* pMnemonic);
		void DecodeBITb_IXd_(uint16& address, char* pMnemonic);
		void DecodeBITb_IYd_(uint16& address, char* pMnemonic);
		void DecodeSETbr(uint16& address, char* pMnemonic);
		void DecodeSETb_IXd_(uint16& address, char* pMnemonic);
		void DecodeSETb_IYd_(uint16& address, char* pMnemonic);
		void DecodeRESbr(uint16& address, char* pMnemonic);
		void DecodeRESb_IXd_(uint16& address, char* pMnemonic);
		void DecodeRESb_IYd_(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	Jump Group
		//-----------------------------------------------------------------------------

		void DecodeJPnn(uint16& address, char* pMnemonic);
		void DecodeJPccnn(uint16& address, char* pMnemonic);
		void DecodeJRe(uint16& address, char* pMnemonic);
		void DecodeJRCe(uint16& address, char* pMnemonic);
		void DecodeJRNCe(uint16& address, char* pMnemonic);
		void DecodeJRZe(uint16& address, char* pMnemonic);
		void DecodeJRNZe(uint16& address, char* pMnemonic);
		void DecodeJP_HL_(uint16& address, char* pMnemonic);
		void DecodeJP_IX_(uint16& address, char* pMnemonic);
		void DecodeJP_IY_(uint16& address, char* pMnemonic);
		void DecodeDJNZe(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	Call and Return Group
		//-----------------------------------------------------------------------------

		void DecodeCALLnn(uint16& address, char* pMnemonic);
		void DecodeCALLccnn(uint16& address, char* pMnemonic);
		void DecodeRET(uint16& address, char* pMnemonic);
		void DecodeRETcc(uint16& address, char* pMnemonic);
		void DecodeRETI(uint16& address, char* pMnemonic);
		void DecodeRETN(uint16& address, char* pMnemonic);
		void DecodeRSTp(uint16& address, char* pMnemonic);

		//-----------------------------------------------------------------------------
		//	Input and Output Group
		//-----------------------------------------------------------------------------

		void DecodeINA_n_(uint16& address, char* pMnemonic);
		void DecodeINr_C_(uint16& address, char* pMnemonic);
		void DecodeINI(uint16& address, char* pMnemonic);
		void DecodeINIR(uint16& address, char* pMnemonic);
		void DecodeIND(uint16& address, char* pMnemonic);
		void DecodeINDR(uint16& address, char* pMnemonic);
		void DecodeOUT_n_A(uint16& address, char* pMnemonic);
		void DecodeOUT_C_r(uint16& address, char* pMnemonic);
		void DecodeOUTI(uint16& address, char* pMnemonic);
		void DecodeOTIR(uint16& address, char* pMnemonic);
		void DecodeOUTD(uint16& address, char* pMnemonic);
		void DecodeOTDR(uint16& address, char* pMnemonic);

		//=============================================================================

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
			eR_PCh = eR_PC + HI,
			eR_PCl = eR_PC + LO,
			eR_I = eR_PC + sizeof(uint16),
			eR_R = eR_I + sizeof(uint8),
			eR_AFalt = eR_R + sizeof(uint8),
			eR_BCalt = eR_AFalt + sizeof(uint16),
			eR_DEalt = eR_BCalt + sizeof(uint16),
			eR_HLalt = eR_DEalt + sizeof(uint16),
			eR_State = eR_HLalt + sizeof(uint16),
			eR_Address = eR_State + sizeof(uint16),
			eR_Addressh = eR_Address + HI,
			eR_Addressl = eR_Address + LO
		};

		//=============================================================================

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
		uint8&	m_PCh;
		uint8&	m_PCl;
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
		uint16&	m_address;
		uint8&	m_addresshi;
		uint8&	m_addresslo;

		// Opcode register decode lookups
		uint8		m_16BitRegisterOffset[4];
		uint8		m_8BitRegisterOffset[8];

		uint8*	m_pMemory;
		float		m_clockSpeedMHz;
		float		m_reciprocalClockSpeedMHz;
		bool		m_enableDebug;
		bool		m_enableUnattendedDebug;
		bool		m_enableOutputStatus;
		bool		m_enableBreakpoints;
		bool		m_enableProgramFlowBreakpoints;

		//=============================================================================

		void	IncrementR(uint8 value)		{ m_R = (m_R & 0x80) | ((m_R + value) & 0x7F); }

		//=============================================================================

	private:
};

#endif // !defined(__Z80_H__)
