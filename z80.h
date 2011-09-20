#if !defined(__Z80_H__)
#define __Z80_H__

#include "common/platform_types.h"
#include "imemory.h"

#if !defined(LITTLE_ENDIAN)
#define LITTLE_ENDIAN
#endif

class CZ80
{
	public:
		CZ80(IMemory* pMemory);

		void Reset(void);
		uint32 SingleStep(void);
		uint32 ServiceInterrupts(void);

		void LoadSNA(uint8* regs);

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

		void HitBreakpoint(const char* type) const;

	protected:
		void OutputStatus(void) const;
		void OutputInstruction(uint16 address) const;
		void HandleIllegalOpcode(void) const;

		uint32 Step(void);
		void Decode(uint16& address, char* pMnemonic) const;
		uint8 HandleArithmeticAddFlags(uint16 source1, uint16 source2, bool withCarry);
		uint8 HandleArithmeticSubtractFlags(uint16 source1, uint16 source2, bool withCarry);
		void HandleLogicalFlags(uint8 source);
		uint16 Handle16BitArithmeticAddFlags(uint32 source1, uint32 source2, bool withCarry);
		uint16 Handle16BitArithmeticSubtractFlags(uint32 source1, uint32 source2, bool withCarry);

		void WriteMemory(uint16 address, uint8 byte);
		uint8 ReadMemory(uint16 address) const;
		inline void WritePort(uint16 address, uint8 byte)	{ m_pMemory->WritePort(address, byte); }
		inline uint8 ReadPort(uint16 address) const				{ return m_pMemory->ReadPort(address); }
		
		const char* Get8BitRegisterString(uint8 threeBits) const;
		const char* Get8BitIXRegisterString(uint8 threeBits) const;
		const char* Get8BitIYRegisterString(uint8 threeBits) const;
		const char* Get16BitRegisterString(uint8 twoBits) const;
		const char* GetConditionString(uint8 threeBits) const;
		bool IsConditionTrue(uint8 threeBits) const;

		//-----------------------------------------------------------------------------
		//	8-Bit Load Group
		//-----------------------------------------------------------------------------

		uint32 ImplementLDrr(void);
		uint32 ImplementLDrn(void);
		uint32 ImplementLDIXhn(void);
		uint32 ImplementLDIXhr(void);
		uint32 ImplementLDIXln(void);
		uint32 ImplementLDIXlr(void);
		uint32 ImplementLDIYhn(void);
		uint32 ImplementLDIYhr(void);
		uint32 ImplementLDIYln(void);
		uint32 ImplementLDIYlr(void);
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
		uint32 ImplementADDAIXh(void);
		uint32 ImplementADDAIXl(void);
		uint32 ImplementADDAIYh(void);
		uint32 ImplementADDAIYl(void);
		uint32 ImplementADDAn(void);
		uint32 ImplementADDA_HL_(void);
		uint32 ImplementADDA_IXd_(void);
		uint32 ImplementADDA_IYd_(void);
		uint32 ImplementADCAr(void);
		uint32 ImplementADCAIXh(void);
		uint32 ImplementADCAIXl(void);
		uint32 ImplementADCAIYh(void);
		uint32 ImplementADCAIYl(void);
		uint32 ImplementADCAn(void);
		uint32 ImplementADCA_HL_(void);
		uint32 ImplementADCA_IXd_(void);
		uint32 ImplementADCA_IYd_(void);
		uint32 ImplementSUBr(void);
		uint32 ImplementSUBIXh(void);
		uint32 ImplementSUBIXl(void);
		uint32 ImplementSUBIYh(void);
		uint32 ImplementSUBIYl(void);
		uint32 ImplementSUBn(void);
		uint32 ImplementSUB_HL_(void);
		uint32 ImplementSUB_IXd_(void);
		uint32 ImplementSUB_IYd_(void);
		uint32 ImplementSBCAr(void);
		uint32 ImplementSBCAIXh(void);
		uint32 ImplementSBCAIXl(void);
		uint32 ImplementSBCAIYh(void);
		uint32 ImplementSBCAIYl(void);
		uint32 ImplementSBCAn(void);
		uint32 ImplementSBCA_HL_(void);
		uint32 ImplementSBCA_IXd_(void);
		uint32 ImplementSBCA_IYd_(void);
		uint32 ImplementANDr(void);
		uint32 ImplementANDIXh(void);
		uint32 ImplementANDIXl(void);
		uint32 ImplementANDIYh(void);
		uint32 ImplementANDIYl(void);
		uint32 ImplementANDn(void);
		uint32 ImplementAND_HL_(void);
		uint32 ImplementAND_IXd_(void);
		uint32 ImplementAND_IYd_(void);
		uint32 ImplementORr(void);
		uint32 ImplementORIXh(void);
		uint32 ImplementORIXl(void);
		uint32 ImplementORIYh(void);
		uint32 ImplementORIYl(void);
		uint32 ImplementORn(void);
		uint32 ImplementOR_HL_(void);
		uint32 ImplementOR_IXd_(void);
		uint32 ImplementOR_IYd_(void);
		uint32 ImplementXORr(void);
		uint32 ImplementXORIXh(void);
		uint32 ImplementXORIXl(void);
		uint32 ImplementXORIYh(void);
		uint32 ImplementXORIYl(void);
		uint32 ImplementXORn(void);
		uint32 ImplementXOR_HL_(void);
		uint32 ImplementXOR_IXd_(void);
		uint32 ImplementXOR_IYd_(void);
		uint32 ImplementCPr(void);
		uint32 ImplementCPIXh(void);
		uint32 ImplementCPIXl(void);
		uint32 ImplementCPIYh(void);
		uint32 ImplementCPIYl(void);
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

		void DecodeLDrr(uint16& address, char* pMnemonic) const;
		void DecodeLDrn(uint16& address, char* pMnemonic) const;
		void DecodeLDIXhn(uint16& address, char* pMnemonic) const;
		void DecodeLDIXhr(uint16& address, char* pMnemonic) const;
		void DecodeLDIXln(uint16& address, char* pMnemonic) const;
		void DecodeLDIXlr(uint16& address, char* pMnemonic) const;
		void DecodeLDIYhn(uint16& address, char* pMnemonic) const;
		void DecodeLDIYhr(uint16& address, char* pMnemonic) const;
		void DecodeLDIYln(uint16& address, char* pMnemonic) const;
		void DecodeLDIYlr(uint16& address, char* pMnemonic) const;
		void DecodeLDr_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeLDr_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeLD_HL_r(uint16& address, char* pMnemonic) const;
		void DecodeLD_IXd_r(uint16& address, char* pMnemonic) const;
		void DecodeLD_IYd_r(uint16& address, char* pMnemonic) const;
		void DecodeLD_HL_n(uint16& address, char* pMnemonic) const;
		void DecodeLD_IXd_n(uint16& address, char* pMnemonic) const;
		void DecodeLD_IYd_n(uint16& address, char* pMnemonic) const;
		void DecodeLDA_BC_(uint16& address, char* pMnemonic) const;
		void DecodeLDA_DE_(uint16& address, char* pMnemonic) const;
		void DecodeLDA_nn_(uint16& address, char* pMnemonic) const;
		void DecodeLD_BC_A(uint16& address, char* pMnemonic) const;
		void DecodeLD_DE_A(uint16& address, char* pMnemonic) const;
		void DecodeLD_nn_A(uint16& address, char* pMnemonic) const;
		void DecodeLDAI(uint16& address, char* pMnemonic) const;
		void DecodeLDAR(uint16& address, char* pMnemonic) const;
		void DecodeLDIA(uint16& address, char* pMnemonic) const;
		void DecodeLDRA(uint16& address, char* pMnemonic) const;

		//-----------------------------------------------------------------------------
		//	16-Bit Load Group
		//-----------------------------------------------------------------------------

		void DecodeLDddnn(uint16& address, char* pMnemonic) const;
		void DecodeLDIXnn(uint16& address, char* pMnemonic) const;
		void DecodeLDIYnn(uint16& address, char* pMnemonic) const;
		void DecodeLDHL_nn_(uint16& address, char* pMnemonic) const;
		void DecodeLDdd_nn_(uint16& address, char* pMnemonic) const;
		void DecodeLDIX_nn_(uint16& address, char* pMnemonic) const;
		void DecodeLDIY_nn_(uint16& address, char* pMnemonic) const;
		void DecodeLD_nn_HL(uint16& address, char* pMnemonic) const;
		void DecodeLD_nn_dd(uint16& address, char* pMnemonic) const;
		void DecodeLD_nn_IX(uint16& address, char* pMnemonic) const;
		void DecodeLD_nn_IY(uint16& address, char* pMnemonic) const;
		void DecodeLDSPHL(uint16& address, char* pMnemonic) const;
		void DecodeLDSPIX(uint16& address, char* pMnemonic) const;
		void DecodeLDSPIY(uint16& address, char* pMnemonic) const;
		void DecodePUSHqq(uint16& address, char* pMnemonic) const;
		void DecodePUSHAF(uint16& address, char* pMnemonic) const;
		void DecodePUSHIX(uint16& address, char* pMnemonic) const;
		void DecodePUSHIY(uint16& address, char* pMnemonic) const;
		void DecodePOPqq(uint16& address, char* pMnemonic) const;
		void DecodePOPAF(uint16& address, char* pMnemonic) const;
		void DecodePOPIX(uint16& address, char* pMnemonic) const;
		void DecodePOPIY(uint16& address, char* pMnemonic) const;

		//-----------------------------------------------------------------------------
		//	Exchange, Block Transfer and Search Group
		//-----------------------------------------------------------------------------

		void DecodeEXDEHL(uint16& address, char* pMnemonic) const;
		void DecodeEXAFAF(uint16& address, char* pMnemonic) const;
		void DecodeEXX(uint16& address, char* pMnemonic) const;
		void DecodeEX_SP_HL(uint16& address, char* pMnemonic) const;
		void DecodeEX_SP_IX(uint16& address, char* pMnemonic) const;
		void DecodeEX_SP_IY(uint16& address, char* pMnemonic) const;
		void DecodeLDI(uint16& address, char* pMnemonic) const;
		void DecodeLDIR(uint16& address, char* pMnemonic) const;
		void DecodeLDD(uint16& address, char* pMnemonic) const;
		void DecodeLDDR(uint16& address, char* pMnemonic) const;
		void DecodeCPI(uint16& address, char* pMnemonic) const;
		void DecodeCPIR(uint16& address, char* pMnemonic) const;
		void DecodeCPD(uint16& address, char* pMnemonic) const;
		void DecodeCPDR(uint16& address, char* pMnemonic) const;

		//-----------------------------------------------------------------------------
		//	8-Bit Arithmetic Group
		//-----------------------------------------------------------------------------

		void DecodeADDAr(uint16& address, char* pMnemonic) const;
		void DecodeADDAIXh(uint16& address, char* pMnemonic) const;
		void DecodeADDAIXl(uint16& address, char* pMnemonic) const;
		void DecodeADDAIYh(uint16& address, char* pMnemonic) const;
		void DecodeADDAIYl(uint16& address, char* pMnemonic) const;
		void DecodeADDAn(uint16& address, char* pMnemonic) const;
		void DecodeADDA_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeADDA_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeADCAr(uint16& address, char* pMnemonic) const;
		void DecodeADCAIXh(uint16& address, char* pMnemonic) const;
		void DecodeADCAIXl(uint16& address, char* pMnemonic) const;
		void DecodeADCAIYh(uint16& address, char* pMnemonic) const;
		void DecodeADCAIYl(uint16& address, char* pMnemonic) const;
		void DecodeADCAn(uint16& address, char* pMnemonic) const;
		void DecodeADCA_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeADCA_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeSUBr(uint16& address, char* pMnemonic) const;
		void DecodeSUBIXh(uint16& address, char* pMnemonic) const;
		void DecodeSUBIXl(uint16& address, char* pMnemonic) const;
		void DecodeSUBIYh(uint16& address, char* pMnemonic) const;
		void DecodeSUBIYl(uint16& address, char* pMnemonic) const;
		void DecodeSUBn(uint16& address, char* pMnemonic) const;
		void DecodeSUB_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeSUB_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeSBCAr(uint16& address, char* pMnemonic) const;
		void DecodeSBCAIXh(uint16& address, char* pMnemonic) const;
		void DecodeSBCAIXl(uint16& address, char* pMnemonic) const;
		void DecodeSBCAIYh(uint16& address, char* pMnemonic) const;
		void DecodeSBCAIYl(uint16& address, char* pMnemonic) const;
		void DecodeSBCAn(uint16& address, char* pMnemonic) const;
		void DecodeSBCA_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeSBCA_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeANDr(uint16& address, char* pMnemonic) const;
		void DecodeANDIXh(uint16& address, char* pMnemonic) const;
		void DecodeANDIXl(uint16& address, char* pMnemonic) const;
		void DecodeANDIYh(uint16& address, char* pMnemonic) const;
		void DecodeANDIYl(uint16& address, char* pMnemonic) const;
		void DecodeANDn(uint16& address, char* pMnemonic) const;
		void DecodeAND_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeAND_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeORr(uint16& address, char* pMnemonic) const;
		void DecodeORIXh(uint16& address, char* pMnemonic) const;
		void DecodeORIXl(uint16& address, char* pMnemonic) const;
		void DecodeORIYh(uint16& address, char* pMnemonic) const;
		void DecodeORIYl(uint16& address, char* pMnemonic) const;
		void DecodeORn(uint16& address, char* pMnemonic) const;
		void DecodeOR_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeOR_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeXORr(uint16& address, char* pMnemonic) const;
		void DecodeXORIXh(uint16& address, char* pMnemonic) const;
		void DecodeXORIXl(uint16& address, char* pMnemonic) const;
		void DecodeXORIYh(uint16& address, char* pMnemonic) const;
		void DecodeXORIYl(uint16& address, char* pMnemonic) const;
		void DecodeXORn(uint16& address, char* pMnemonic) const;
		void DecodeXOR_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeXOR_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeCPr(uint16& address, char* pMnemonic) const;
		void DecodeCPIXh(uint16& address, char* pMnemonic) const;
		void DecodeCPIXl(uint16& address, char* pMnemonic) const;
		void DecodeCPIYh(uint16& address, char* pMnemonic) const;
		void DecodeCPIYl(uint16& address, char* pMnemonic) const;
		void DecodeCPn(uint16& address, char* pMnemonic) const;
		void DecodeCP_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeCP_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeINCr(uint16& address, char* pMnemonic) const;
		void DecodeINC_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeINC_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeDECr(uint16& address, char* pMnemonic) const;
		void DecodeDEC_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeDEC_IYd_(uint16& address, char* pMnemonic) const;

		//-----------------------------------------------------------------------------
		//	General-Purpose Arithmetic and CPU Control Groups
		//-----------------------------------------------------------------------------

		void DecodeDAA(uint16& address, char* pMnemonic) const;
		void DecodeCPL(uint16& address, char* pMnemonic) const;
		void DecodeNEG(uint16& address, char* pMnemonic) const;
		void DecodeCCF(uint16& address, char* pMnemonic) const;
		void DecodeSCF(uint16& address, char* pMnemonic) const;
		void DecodeNOP(uint16& address, char* pMnemonic) const;
		void DecodeHALT(uint16& address, char* pMnemonic) const;
		void DecodeDI(uint16& address, char* pMnemonic) const;
		void DecodeEI(uint16& address, char* pMnemonic) const;
		void DecodeIM0(uint16& address, char* pMnemonic) const;
		void DecodeIM1(uint16& address, char* pMnemonic) const;
		void DecodeIM2(uint16& address, char* pMnemonic) const;

		//-----------------------------------------------------------------------------
		//	16-Bit Arithmetic Group
		//-----------------------------------------------------------------------------

		void DecodeADDHLdd(uint16& address, char* pMnemonic) const;
		void DecodeADCHLdd(uint16& address, char* pMnemonic) const;
		void DecodeSBCHLdd(uint16& address, char* pMnemonic) const;
		void DecodeADDIXdd(uint16& address, char* pMnemonic) const;
		void DecodeADDIYdd(uint16& address, char* pMnemonic) const;
		void DecodeINCdd(uint16& address, char* pMnemonic) const;
		void DecodeINCIX(uint16& address, char* pMnemonic) const;
		void DecodeINCIY(uint16& address, char* pMnemonic) const;
		void DecodeDECdd(uint16& address, char* pMnemonic) const;
		void DecodeDECIX(uint16& address, char* pMnemonic) const;
		void DecodeDECIY(uint16& address, char* pMnemonic) const;

		//-----------------------------------------------------------------------------
		//	Rotate and Shift Group
		//-----------------------------------------------------------------------------

		void DecodeRLCA(uint16& address, char* pMnemonic) const;
		void DecodeRLA(uint16& address, char* pMnemonic) const;
		void DecodeRRCA(uint16& address, char* pMnemonic) const;
		void DecodeRRA(uint16& address, char* pMnemonic) const;
		void DecodeRLCr(uint16& address, char* pMnemonic) const;
		void DecodeRLC_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeRLC_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeRLr(uint16& address, char* pMnemonic) const;
		void DecodeRL_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeRL_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeRRCr(uint16& address, char* pMnemonic) const;
		void DecodeRRC_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeRRC_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeRRr(uint16& address, char* pMnemonic) const;
		void DecodeRR_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeRR_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeSLAr(uint16& address, char* pMnemonic) const;
		void DecodeSLA_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeSLA_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeSRAr(uint16& address, char* pMnemonic) const;
		void DecodeSRA_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeSRA_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeSRLr(uint16& address, char* pMnemonic) const;
		void DecodeSRL_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeSRL_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeRLD(uint16& address, char* pMnemonic) const;
		void DecodeRRD(uint16& address, char* pMnemonic) const;

		//-----------------------------------------------------------------------------
		//	Bit Set, Reset and Test Group
		//-----------------------------------------------------------------------------

		void DecodeBITbr(uint16& address, char* pMnemonic) const;
		void DecodeBITb_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeBITb_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeSETbr(uint16& address, char* pMnemonic) const;
		void DecodeSETb_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeSETb_IYd_(uint16& address, char* pMnemonic) const;
		void DecodeRESbr(uint16& address, char* pMnemonic) const;
		void DecodeRESb_IXd_(uint16& address, char* pMnemonic) const;
		void DecodeRESb_IYd_(uint16& address, char* pMnemonic) const;

		//-----------------------------------------------------------------------------
		//	Jump Group
		//-----------------------------------------------------------------------------

		void DecodeJPnn(uint16& address, char* pMnemonic) const;
		void DecodeJPccnn(uint16& address, char* pMnemonic) const;
		void DecodeJRe(uint16& address, char* pMnemonic) const;
		void DecodeJRCe(uint16& address, char* pMnemonic) const;
		void DecodeJRNCe(uint16& address, char* pMnemonic) const;
		void DecodeJRZe(uint16& address, char* pMnemonic) const;
		void DecodeJRNZe(uint16& address, char* pMnemonic) const;
		void DecodeJP_HL_(uint16& address, char* pMnemonic) const;
		void DecodeJP_IX_(uint16& address, char* pMnemonic) const;
		void DecodeJP_IY_(uint16& address, char* pMnemonic) const;
		void DecodeDJNZe(uint16& address, char* pMnemonic) const;

		//-----------------------------------------------------------------------------
		//	Call and Return Group
		//-----------------------------------------------------------------------------

		void DecodeCALLnn(uint16& address, char* pMnemonic) const;
		void DecodeCALLccnn(uint16& address, char* pMnemonic) const;
		void DecodeRET(uint16& address, char* pMnemonic) const;
		void DecodeRETcc(uint16& address, char* pMnemonic) const;
		void DecodeRETI(uint16& address, char* pMnemonic) const;
		void DecodeRETN(uint16& address, char* pMnemonic) const;
		void DecodeRSTp(uint16& address, char* pMnemonic) const;

		//-----------------------------------------------------------------------------
		//	Input and Output Group
		//-----------------------------------------------------------------------------

		void DecodeINA_n_(uint16& address, char* pMnemonic) const;
		void DecodeINr_C_(uint16& address, char* pMnemonic) const;
		void DecodeINI(uint16& address, char* pMnemonic) const;
		void DecodeINIR(uint16& address, char* pMnemonic) const;
		void DecodeIND(uint16& address, char* pMnemonic) const;
		void DecodeINDR(uint16& address, char* pMnemonic) const;
		void DecodeOUT_n_A(uint16& address, char* pMnemonic) const;
		void DecodeOUT_C_r(uint16& address, char* pMnemonic) const;
		void DecodeOUTI(uint16& address, char* pMnemonic) const;
		void DecodeOTIR(uint16& address, char* pMnemonic) const;
		void DecodeOUTD(uint16& address, char* pMnemonic) const;
		void DecodeOTDR(uint16& address, char* pMnemonic) const;

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

		IMemory*	m_pMemory;
		mutable bool		m_enableDebug;
		mutable bool		m_enableUnattendedDebug;
		mutable bool		m_enableBreakpoints;
		bool		m_enableOutputStatus;
		bool		m_enableProgramFlowBreakpoints;

		//=============================================================================

		void	IncrementR(uint8 value)		{ m_R = (m_R & 0x80) | ((m_R + value) & 0x7F); }

		//=============================================================================

	private:
};

#endif // !defined(__Z80_H__)
