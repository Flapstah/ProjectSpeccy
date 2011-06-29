#include <stdio.h>
#include <string.h>

#include "z80.h"

//=============================================================================
//	All information contained herein is based on Zilog's "Z80 Family CPU User
//	Manual".
//
//	Minor timing corrections have been made for instruction execution time where
//	each T State is assumed to be 0.25 nanoseconds (based on the vast majority
//	of the instructions) based on a 4MHz clock.
//=============================================================================

//=============================================================================

CZ80::CZ80(uint8* pMemory, float clockSpeedMHz)
	// Map registers to register memory
	: m_AF(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_AF])))
	, m_BC(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_BC])))
	, m_DE(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_DE])))
	, m_HL(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_HL])))
	, m_IX(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_IX])))
	, m_IY(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_IY])))
	, m_SP(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_SP])))
	, m_PC(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_PC])))
	, m_AFalt(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_AFalt])))
	, m_BCalt(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_BCalt])))
	, m_DEalt(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_DEalt])))
	, m_HLalt(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_HLalt])))
	, m_A(m_RegisterMemory[eR_A])
	, m_F(m_RegisterMemory[eR_F])
	, m_B(m_RegisterMemory[eR_B])
	, m_C(m_RegisterMemory[eR_C])
	, m_D(m_RegisterMemory[eR_D])
	, m_E(m_RegisterMemory[eR_E])
	, m_H(m_RegisterMemory[eR_H])
	, m_L(m_RegisterMemory[eR_L])
	, m_I(m_RegisterMemory[eR_I])
	, m_R(m_RegisterMemory[eR_R])
	, m_IFF1(m_RegisterMemory[eR_IFF1])
	, m_IFF2(m_RegisterMemory[eR_IFF2])
{
	// Easy decoding of opcodes to 16 bit registers
	m_16BitRegisterOffset[BC] = eR_BC;
	m_16BitRegisterOffset[DE] = eR_DE;
	m_16BitRegisterOffset[HL] = eR_HL;
	m_16BitRegisterOffset[SP] = eR_SP;

	// Easy decoding of opcodes to 8 bit registers (still need special handling
	// for (HL))
	memset(m_8BitRegisterOffset, 0, sizeof(m_8BitRegisterOffset));
	m_8BitRegisterOffset[B] = eR_B;
	m_8BitRegisterOffset[C] = eR_C;
	m_8BitRegisterOffset[D] = eR_D;
	m_8BitRegisterOffset[E] = eR_E;
	m_8BitRegisterOffset[H] = eR_H;
	m_8BitRegisterOffset[L] = eR_L;
	m_8BitRegisterOffset[A] = eR_A;
}

//=============================================================================

const char* CZ80::Get8BitRegisterString(uint8 threeBits)
{
	switch (threeBits & 0x07)
	{
		case 0: return "B";			break;
		case 1: return "C";			break;
		case 2: return "D";			break;
		case 3: return "E";			break;
		case 4: return "H";			break;
		case 5: return "L";			break;
		case 6: return "(HL)";	break;
		case 7: return "A";			break;
	}
}

//=============================================================================

const char* CZ80::Get16BitRegisterString(uint8 twoBits)
{
	switch (twoBits & 0x03)
	{
		case 0: return "BC";		break;
		case 1: return "DE";		break;
		case 2: return "HL";		break;
		case 3: return "SP";		break;
	}
}

//=============================================================================

//-----------------------------------------------------------------------------
//	8-Bit Load Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::ImplementLDrr(void)
{
	//
	// Operation:	r <- r'
	// Op Code:		LD
	// Operands:	r, r'
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|d|d|d|s|s|s|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where ddd and sss are any of:
	//								B						000
	//								C						001
	//								D						010
	//								E						011
	//								H						100
	//								L						101
	//								A						111
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	Get8BitRegister(*m_pPC >> 3) = Get8BitRegister(*m_pPC);
	++m_PC;
	m_tstate += 4;
}

//=============================================================================

void CZ80::ImplementLDrn(void)
{
	//
	// Operation:	r <- n
	// Op Code:		LD
	// Operands:	r, n
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|r|r|r|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where rrr is any of:
	//								B						000
	//								C						001
	//								D						010
	//								E						011
	//								H						100
	//								L						101
	//								A						111
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75
	//
	IncrementR(1);
	uint8 opcode = *m_pPC++;
	Get8BitRegister(opcode) = *m_pPC++;
	m_tstate += 7;
}

//=============================================================================

void CZ80::ImplementLDrHL(void)
{
	//
	// Operation:	r <- (HL)
	// Op Code:		LD
	// Operands:	r, (HL) 
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|r|r|r|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where rrr is any of:
	//								B						000
	//								C						001
	//								D						010
	//								E						011
	//								H						100
	//								L						101
	//								A						111
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75
	//
	IncrementR(1);
	Get8BitRegister(*m_pPC >> 3) = Get8BitRegister(*m_pPC);
	++m_PC;
	m_tstate += 7;
}

//=============================================================================

void CZ80::ImplementLDrIXd(void)
{
	//
	// Operation:	r <- (IX+d)
	// Op Code:		LD
	// Operands:	r, (IX+d) 
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|r|r|r|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where rrr is any of:
	//								B						000
	//								C						001
	//								D						010
	//								E						011
	//								H						100
	//								L						101
	//								A						111
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	uint8 opcode = *(++m_pPC);
	Get8BitRegister(opcode >> 3) = *(m_pIX + *(++m_pPC)++);
	m_tstate += 19;
}

//=============================================================================

void CZ80::ImplementLDrIYd(void)
{
	//
	// Operation:	r <- (IY+d)
	// Op Code:		LD
	// Operands:	r, (IY+d) 
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|r|r|r|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where rrr is any of:
	//								B						000
	//								C						001
	//								D						010
	//								E						011
	//								H						100
	//								L						101
	//								A						111
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	uint8 opcode = *++m_pPC;
	Get8BitRegister(opcode >> 3) = *(&m_IY + *(++m_pPC)++);
	m_tstate += 19;
}

//=============================================================================

void CZ80::ImplementLDHLr(void)
{
	//
	// Operation:	(HL) <- r
	// Op Code:		LD
	// Operands:	(HL), r
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|1|1|0|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where rrr is any of:
	//								B						000
	//								C						001
	//								D						010
	//								E						011
	//								H						100
	//								L						101
	//								A						111
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75
	//
	IncrementR(1);
	*m_pHL = Get8BitRegister(*m_pPC++);
	m_tstate += 7;
}

//=============================================================================

void CZ80::ImplementLDIXdr(void)
{
	//
	// Operation:	(IX+d) <- r
	// Op Code:		LD
	// Operands:	(IX+d), r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|1|1|0|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where rrr is any of:
	//								B						000
	//								C						001
	//								D						010
	//								E						011
	//								H						100
	//								L						101
	//								A						111
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	uint8 opcode = *(++m_pPC);
	*(m_pIX + *(++m_pPC)++) = Get8BitRegister(opcode);
	m_tstate += 19;
}

//=============================================================================

void CZ80::ImplementLDIYdr(void)
{
	//
	// Operation:	(IY+d) <- r
	// Op Code:		LD
	// Operands:	(IY+d), r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|1|1|0|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where rrr is any of:
	//								B						000
	//								C						001
	//								D						010
	//								E						011
	//								H						100
	//								L						101
	//								A						111
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	uint8 opcode = *(++m_pPC);
	*(m_pIY + *(++m_pPC)++) = Get8BitRegister(opcode);
	m_tstate += 19;
}

//=============================================================================

void CZ80::ImplementLDHLn(void)
{
	//
	// Operation:	(HL) <- n
	// Op Code:		LD
	// Operands:	(HL), n
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|1|1|0| 36
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						10 (4,3,3)				2.50
	//
	IncrementR(1);
	*m_pHL = *(++m_pPC)++;
	m_tstate += 10;
}

//=============================================================================

void CZ80::ImplementLDIXdn(void)
{
	//
	// Operation:	(IX+d) <- n
	// Op Code:		LD
	// Operands:	(IX+d), n
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|1|1|0| 36
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	*(m_pIX + *m_pPC) = *(m_pPC + 1);
	++++m_PC;
	m_tstate += 19;
}

//=============================================================================

void CZ80::ImplementLDIYdn(void)
{
	//
	// Operation:	(IY+d) <- n
	// Op Code:		LD
	// Operands:	(IY+d), n
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|1|1|0| 36
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	*(m_pIY + *m_pPC) = *(m_pPC + 1);
	++++m_PC;
	m_tstate += 19;
}

//=============================================================================

void CZ80::ImplementLDABC(void)
{
	//
	// Operation:	A <- (BC)
	// Op Code:		LD
	// Operands:	A, (BC) 
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|1|0|1|0| 0A
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75
	//
	IncrementR(1);
	m_A = *m_pBC;
	++m_PC;
	m_tstate += 7;
}

//=============================================================================

void CZ80::ImplementLDADE(void)
{
	//
	// Operation:	A <- (DE)
	// Op Code:		LD
	// Operands:	A, (DE) 
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|1|0|1|0| 1A
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75
	//
	IncrementR(1);
	m_A = *m_pDE;
	++m_PC;
	m_tstate += 7;
}

//=============================================================================

void CZ80::ImplementLDA_nn_(void)
{
	//
	// Operation:	A <- (nn)
	// Op Code:		LD
	// Operands:	A, (nn) 
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|1|0|1|0| 3A
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						13 (4,3,3,3)			3.25
	//
	IncrementR(1);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++m_pPC);
	temp.m_hi = *(++m_pPC);
	m_A = *temp.m_ptr;
	++m_PC;
	m_tstate += 13;
}

//=============================================================================

void CZ80::ImplementLDBCA(void)
{
	//
	// Operation:	(BC) <- A
	// Op Code:		LD
	// Operands:	(BC), A
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|0|0|1|0| 02
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75
	//
	IncrementR(1);
	*m_pBC = m_A;
	++m_PC; 
	m_tstate += 7;
}

//=============================================================================

void CZ80::ImplementLDDEA(void)
{
	//
	// Operation:	(DE) <- A
	// Op Code:		LD
	// Operands:	(DE), A
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|0|0|1|0| 12
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75
	//
	IncrementR(1);
	*m_pDE = m_A;
	++m_PC; 
	m_tstate += 7;
}

//=============================================================================

void CZ80::ImplementLD_nn_A(void)
{
	//
	// Operation:	(nn) <- A
	// Op Code:		LD
	// Operands:	(nn), A
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|0|0|1|0| 12
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						13 (4,3,3,3)			3.25
	//
	IncrementR(1);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++m_pPC);
	temp.m_hi = *(++m_pPC);
	*temp.m_ptr = m_A;
	++m_PC;
	m_tstate += 13;
}

//=============================================================================

void CZ80::ImplementLDAI(void)
{
	//
	// Operation:	A <- I
	// Op Code:		LD
	// Operands:	A, I 
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|0|1|0|1|1|1| 57
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						9 (4,5)						2.25
	//
	IncrementR(2);
	m_A = m_I;
	m_F &= ~(eF_S | eF_Z | eF_H | eF_N);
	m_F |= (m_A & eF_S) | (eF_Z & (m_A == 0)) | (eF_PV & IFF2);
	++++m_PC;
	m_tstate += 9;
}

//=============================================================================

void CZ80::ImplementLDAR(void)
{
	//
	// Operation:	A <- R
	// Op Code:		LD
	// Operands:	A, R 
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|0|1|1|1|1|1| 5F
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						9 (4,5)						2.25
	//
	IncrementR(2);
	m_A = m_R;
	m_F &= ~(eF_S | eF_Z | eF_H | eF_N);
	m_F |= (m_A & eF_S) | (eF_Z & (m_A == 0)) | (eF_PV & IFF2);
	++++m_PC;
	m_tstate += 9;
}

//=============================================================================

void CZ80::ImplementLDIA(void)
{
	//
	// Operation:	I <- A
	// Op Code:		LD
	// Operands:	I, A 
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|0|0|0|1|1|1| 47
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						9 (4,5)						2.25
	//
	IncrementR(2);
	m_I = m_A;
	++++m_PC;
	m_tstate += 9;
}

//=============================================================================

void CZ80::ImplementLDRA(void)
{
	//
	// Operation:	R <- A
	// Op Code:		LD
	// Operands:	R, A 
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|0|0|1|1|1|1| 4F
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						9 (4,5)						2.25
	//
	IncrementR(2);
	m_R = m_A;
	++++m_PC;
	m_tstate += 9;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	16-Bit Load Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::ImplementLDddnn(void)
{
	//
	// Operation:	dd <- nn
	// Op Code:		LD
	// Operands:	dd, nn
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|d|d|0|0|0|1|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								HL					10
	//								SP					11
	//
	//							M Cycles		T States					MHz E.T.
	//								3						10 (4,3,3)				2.50
	//
	IncrementR(1);
	uint8 opcode = *m_pPC;
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	Get16BitRegister(opcode >> 4) = temp.m_val;
	m_tstate += 10;
}

//=============================================================================

void CZ80::ImplementLDIXnn(void)
{
	//
	// Operation:	IX <- nn
	// Op Code:		LD
	// Operands:	IX, nn
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|0|0|1| 21
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						14 (4,4,3,3)			3.50
	//
	IncrementR(2);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	m_IX = temp.m_val;
	m_tstate += 14;
}

//=============================================================================

void CZ80::ImplementLDIYnn(void)
{
	//
	// Operation:	IY <- nn
	// Op Code:		LD
	// Operands:	IY, nn
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|0|0|1| 21
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						14 (4,4,3,3)			3.50
	//
	IncrementR(2);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	m_IY = temp.m_val;
	m_tstate += 14;
}

//=============================================================================

void CZ80::ImplementLDHL_nn_(void)
{
	// Operation:	H <- (nn+1), L <- (nn)
	// Op Code:		LD
	// Operands:	HL, (nn)
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|1|0|1|0| 2A
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						16 (4,3,3,3,3)		4.00
	//
	IncrementR(1);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	m_L = *temp.m_ptr;
	m_H = *(++temp.m_ptr);
	m_tstate += 16;
}

//=============================================================================

void CZ80::ImplementLDdd_nn_(void)
{
	//
	// Operation:	ddh <- (nn+1), ddl <- (nn)
	// Op Code:		LD
	// Operands:	dd, (nn)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|d|d|1|0|1|1|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								HL					10
	//								SP					11
	//
	//							M Cycles		T States					MHz E.T.
	//								6						20 (4,4,3,3,3,3)	5.00
	//
	IncrementR(2);
	uint8 opcode = *(++m_pPC);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	uint16 value = *temp.m_ptr;
	value |= *(++temp.m_ptr) << 8;
	Get16BitRegister(opcode >> 4) = value;
	m_tstate += 20;
}

//=============================================================================

void CZ80::ImplementLDIX_nn_(void)
{
	//
	// Operation:	IXh <- (nn+1), IXl <- (nn)
	// Op Code:		LD
	// Operands:	IX, (nn)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|1|0|1|0|1|0| 2A
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						20 (4,4,3,3,3,3)	5.00
	//
	IncrementR(2);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	m_IX_hi = *temp.m_ptr;
	m_IX_lo = *(++temp.m_ptr);
	m_tstate += 20;
}

//=============================================================================

void CZ80::ImplementLDIY_nn_(void)
{
	//
	// Operation:	IYh <- (nn+1), IYl <- (nn)
	// Op Code:		LD
	// Operands:	IY, (nn)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|1|0|1|0|1|0| 2A
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						20 (4,4,3,3,3,3)	5.00
	//
	IncrementR(2);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	m_IY_hi = *temp.m_ptr;
	m_IY_lo = *(++temp.m_ptr);
	m_tstate += 20;
}

//=============================================================================

void CZ80::ImplementLD_nn_HL(void)
{
	// Operation:	(nn+1) <- H, (nn) <- L
	// Op Code:		LD
	// Operands:	(nn), HL
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|0|1|0| 22
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						16 (4,3,3,3,3)		4.00
	//
	IncrementR(1);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	*temp.m_ptr = m_L;
	*(++temp.m_ptr) = m_H;
	m_tstate += 16;
}

//=============================================================================

void CZ80::ImplementLD_nn_dd(void)
{
	//
	// Operation:	(nn+1) <- ddh, (nn) <- ddl
	// Op Code:		LD
	// Operands:	(nn), dd
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|d|d|0|0|1|1|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								HL					10
	//								SP					11
	//
	//							M Cycles		T States					MHz E.T.
	//								6						20 (4,4,3,3,3,3)	5.00
	//
	IncrementR(2);
	uint8 opcode = *(++m_pPC);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	uint16 value = Get16BitRegister(opcode >> 4);
	*temp.m_ptr = value & 0xFF;
	*(++temp.m_ptr) = value >> 8;
	m_tstate += 20;
}

//=============================================================================

void CZ80::ImplementLD_nn_IX(void)
{
	//
	// Operation:	(nn+1) <- IXh, (nn) <- IXl
	// Op Code:		LD
	// Operands:	(nn), IX
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|1|0|0|0|1|0| 22
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						20 (4,4,3,3,3,3)	5.00
	//
	IncrementR(2);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	*temp.m_ptr = m_IX_lo;
	*(++temp.m_ptr) = m_IX_hi;
	m_tstate += 20;
}

//=============================================================================

void CZ80::ImplementLD_nn_IY(void)
{
	//
	// Operation:	(nn+1) <- IYh, (nn) <- IYl
	// Op Code:		LD
	// Operands:	(nn), IY
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|1|0|0|0|1|0| 22
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						20 (4,4,3,3,3,3)	5.00
	//
	IncrementR(2);
	NON_MEMBER_REGISTER_16BIT(temp);
	temp.m_lo = *(++++m_pPC);
	temp.m_hi = *(++m_pPC)++;
	*temp.m_ptr = m_IY_lo;
	*(++temp.m_ptr) = m_IY_hi;
	m_tstate += 20;
}

//=============================================================================

void CZ80::ImplementLDSPHL(void)
{
	// Operation:	SP <- HL
	// Op Code:		LD
	// Operands:	SP, HL
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|0|0|1| F9
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						6									1.50
	//
	IncrementR(1);
	m_SP = m_HL;
	++m_PC;
	m_tstate += 6;
}

//=============================================================================

void CZ80::ImplementLDSPIX(void)
{
	// Operation:	SP <- IX
	// Op Code:		LD
	// Operands:	SP, IX
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|0|0|1| F9
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						10 (4,6)					2.50
	//
	IncrementR(2);
	m_SP = m_IX;
	++m_PC;
	m_tstate += 10;
}

//=============================================================================

void CZ80::ImplementLDSPIY(void)
{
	// Operation:	SP <- IY
	// Op Code:		LD
	// Operands:	SP, IY
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|0|0|1| F9
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						10 (4,6)					2.50
	//
	IncrementR(2);
	m_SP = m_IY;
	++m_PC;
	m_tstate += 10;
}

//=============================================================================

void CZ80::ImplementPUSHqq(void)
{
	// Operation:	(SP-2) <- qql, (SP-1) <- qqh
	// Op Code:		PUSH
	// Operands:	qq
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|q|q|0|1|0|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								HL					10
	//								AF					11
	//
	//							M Cycles		T States					MHz E.T.
	//								3						11 (5,3,3)				2.75
	//
	IncrementR(1);
	uint16 value = Get16BitRegister(*m_pPC++ >> 4);
	*(--m_pSP) = value >> 8;
	*(--m_pSP) = value & 0xFF;
	m_tstate += 11;
}

//=============================================================================

void CZ80::ImplementPUSHIX(void)
{
	// Operation:	(SP-2) <- IXl, (SP-1) <- IXh
	// Op Code:		PUSH
	// Operands:	IX
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|0|1|0|1| E5
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,5,3,3)			3.75
	//
	IncrementR(2);
	*(--m_pSP) = m_IX_hi;
	*(--m_pSP) = m_IX_lo;
	++++m_PC;
	m_tstate += 15;
}

//=============================================================================

void CZ80::ImplementPUSHIY(void)
{
	// Operation:	(SP-2) <- IYl, (SP-1) <- IYh
	// Op Code:		PUSH
	// Operands:	IY
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|0|1|0|1| E5
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,5,3,3)			3.75
	//
	IncrementR(2);
	*(--m_pSP) = m_IY_hi;
	*(--m_pSP) = m_IY_lo;
	++++m_PC;
	m_tstate += 15;
}

//=============================================================================

void CZ80::ImplementPOPqq(void)
{
	// Operation:	qqh <- (SP+1), qql <- (SP)
	// Op Code:		POP
	// Operands:	qq
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|q|q|0|0|0|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								HL					10
	//								AF					11
	//
	//							M Cycles		T States					MHz E.T.
	//								3						10 (4,3,3)				2.75
	//
	IncrementR(1);
	uint16 value = *m_pSP;
	value |= *(++m_pSP) << 8;
	Get16BitRegister(*m_pPC >> 4) = value;
	++m_PC;
	m_tstate += 10;
}

//=============================================================================

void CZ80::ImplementPOPIX(void)
{
	// Operation:	IXh <- (SP+1), IXl <- (SP)
	// Op Code:		POP
	// Operands:	IX
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|0|0|0|1| E1
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						14 (4,4,3,3)			3.50
	//
	IncrementR(2);
	m_IX_lo = *m_pSP;
	m_IX_hi = *(++m_pSP);
	++++m_PC;
	m_tstate += 14;
}

//=============================================================================

void CZ80::ImplementPOPIY(void)
{
	// Operation:	IYh <- (SP+1), IYh <- (SP)
	// Op Code:		POP
	// Operands:	IY
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|0|0|0|1| E1
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						14 (4,4,3,3)			3.50
	//
	IncrementR(2);
	m_IY_lo = *m_pSP;
	m_IY_hi = *(++m_pSP);
	++++m_PC;
	m_tstate += 14;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Exchange, Block Transfer and Search Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::ImplementEXDEHL(void)
{
	//
	// Operation:	DE <-> HL
	// Op Code:		EX
	// Operands:	DE, HL
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|0|1|1| EB
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	m_DE ^= m_HL;
	m_HL ^= m_DE;
	m_DE ^= m_HL;
	++m_PC;
	m_tstate += 4;
}

//=============================================================================

void CZ80::ImplementEXAFAF(void)
{
	//
	// Operation:	AF <-> AF'
	// Op Code:		EX
	// Operands:	AF, AF'
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|1|0|0|0| 08
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	m_AF ^= _AF_;
	_AF_ ^= m_AF;
	m_AF ^= _AF_;
	++m_PC;
	m_tstate += 4;
}

//=============================================================================

void CZ80::ImplementEXX(void)
{
	//
	// Operation:	BC <-> BC', DE <-> DE', HL <-> HL'
	// Op Code:		EX
	// Operands:	BC, BC', DE, DE', HL, HL'
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|0|0|1| D9
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	m_BC ^= _BC_;
	_BC_ ^= m_BC;
	m_BC ^= _BC_;

	m_DE ^= _DE_;
	_DE_ ^= m_DE;
	m_DE ^= _DE_;

	m_HL ^= _HL_;
	_HL_ ^= m_HL;
	m_HL ^= _HL_;
	++m_PC;
	m_tstate += 4;
}

//=============================================================================

void CZ80::ImplementEX_SP_HL(void)
{
	// Operation:	H <-> (SP+1), L <-> (SP)
	// Op Code:		EX
	// Operands:	(SP), HL
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|0|0|1|1| E3
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,3,4,3,5)		4.75
	//
	IncrementR(1);
	m_L ^= *(m_pSP);
	*(m_pSP) ^= m_L;
	m_L ^= *(m_pSP)++;

	m_H ^= *(m_pSP);
	*(m_pSP) ^= m_H;
	m_H ^= *(m_pSP);
	++m_PC;
	m_tstate += 19;
}

//=============================================================================

void CZ80::ImplementEX_SP_IX(void)
{
	// Operation:	IXh <-> (SP+1), IXl <-> (SP)
	// Op Code:		EX
	// Operands:	(SP), IX
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|0|0|1|1| E3
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,4,3,5)	5.75
	//
	IncrementR(2);
	m_IX_lo ^= *(m_pSP);
	*(m_pSP) ^= m_IX_lo;
	m_IX_lo ^= *(m_pSP)++;

	m_IX_hi ^= *(m_pSP);
	*(m_pSP) ^= m_IX_hi;
	m_IX_hi ^= *(m_pSP);
	++++m_PC;
	m_tstate += 23;
}

//=============================================================================

void CZ80::ImplementEX_SP_IY(void)
{
	// Operation:	IYh <-> (SP+1), IYl <-> (SP)
	// Op Code:		EX
	// Operands:	(SP), IY
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|0|0|1|1| E3
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,4,3,5)	5.75
	//
	IncrementR(2);
	m_IY_lo ^= *(m_pSP);
	*(m_pSP) ^= m_IY_lo;
	m_IY_lo ^= *(m_pSP)++;

	m_IY_hi ^= *(m_pSP);
	*(m_pSP) ^= m_IY_hi;
	m_IY_hi ^= *(m_pSP);
	++++m_PC;
	m_tstate += 23;
}

//=============================================================================


//=============================================================================


//=============================================================================


//=============================================================================


//=============================================================================


//=============================================================================


//=============================================================================


//=============================================================================


































//-----------------------------------------------------------------------------
//	8-Bit Load Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeLD(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,%s", Get8BitRegisterString(*pAddress >> 3), Get8BitRegisterString(*(pAddress + 1)));
}

//=============================================================================

void CZ80::DecodeLDn(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,0x%02X", Get8BitRegisterString(*pAddress >> 3), *(pAddress + 1));
}

//=============================================================================

void CZ80::DecodeLDrIXd(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,(IX+0x%02X)", Get8BitRegisterString(*(pAddress + 1) >> 3), *(pAddress + 2));
}

//=============================================================================

void CZ80::DecodeLDrIYd(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,(IY+0x%02X)", Get8BitRegisterString(*(pAddress + 1) >> 3), *(pAddress + 2));
}

//=============================================================================

void CZ80::DecodeLDHLr(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (HL),%s", Get8BitRegisterString(*pAddress));
}

//=============================================================================

void CZ80::DecodeLDIXdr(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (IX+0x%02X),%s", *(pAddress + 2), Get8BitRegisterString(*(pAddress + 1) >> 3));
}

//=============================================================================

void CZ80::DecodeLDIYdr(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (IY+0x%02X),%s", *(pAddress + 2), Get8BitRegisterString(*(pAddress + 1) >> 3));
}

//=============================================================================

void CZ80::DecodeLDHLn(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (HL),0x%02X", *(pAddress + 1));
}

//=============================================================================

void CZ80::DecodeLDIXdn(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (IX+0x%02X),0x%02X", *(pAddress + 2), *(pAddress + 3));
}

//=============================================================================

void CZ80::DecodeLDIYdn(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (IY+0x%02X),0x%02X", *(pAddress + 2), *(pAddress + 3));
}

//=============================================================================

void CZ80::DecodeLDABC(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD A,(BC)");
}

//=============================================================================

void CZ80::DecodeLDADE(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD A,(DE)");
}

//=============================================================================

void CZ80::DecodeLDA_nn_(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD A,(0x%02X%02X)", *(pAddress + 2), *(pAddress + 1));
}

//=============================================================================

void CZ80::DecodeLDBCA(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (BC),A");
}

//=============================================================================

void CZ80::DecodeLDDEA(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (DE),A");
}

//=============================================================================

void CZ80::DecodeLD_nn_A(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (0x%02X%02X),A", *(pAddress + 2), *(pAddress + 1));
}

//=============================================================================

void CZ80::DecodeLDAI(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD A,I");
}

//=============================================================================

void CZ80::DecodeLDAR(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD A,R");
}

//=============================================================================

void CZ80::DecodeLDIA(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD I,A");
}

//=============================================================================

void CZ80::DecodeLDRA(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD R,A");
}

//=============================================================================

//-----------------------------------------------------------------------------
//	16-Bit Load Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeLDddnn(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,0x%02X%02X", Get16BitRegisterString(*pAddress >> 4), *(pAddress + 2), *(pAddress + 1));
}

//=============================================================================

void CZ80::DecodeLDIXnn(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD IX,0x%02X%02X", *(pAddress + 3), *(pAddress + 2));
}

//=============================================================================

void CZ80::DecodeLDIYnn(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD IY,0x%02X%02X", *(pAddress + 3), *(pAddress + 2));
}

//=============================================================================

void CZ80::DecodeLDHL_nn_(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD HL,(0x%02X%02X)", *(pAddress + 2), *(pAddress + 1));
}

//=============================================================================

void CZ80::DecodeLDdd_nn_(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,(0x%02X%02X)", Get16BitRegisterString(*(pAddress + 1) >> 4), *(pAddress + 3), *(pAddress + 2));
}

//=============================================================================

void CZ80::DecodeLDIX_nn_(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD IX,(0x%02X%02X)", *(pAddress + 3), *(pAddress + 2));
}

//=============================================================================

void CZ80::DecodeLDIY_nn_(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD IY,(0x%02X%02X)", *(pAddress + 3), *(pAddress + 2));
}

//=============================================================================

void CZ80::DecodeLD_nn_HL(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (0x%02X%02X),HL", *(pAddress + 2), *(pAddress + 1));
}

//=============================================================================

void CZ80::DecodeLD_nn_dd(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (0x%02X%02X),%s", *(pAddress + 3), *(pAddress + 2), Get16BitRegisterString(*(pAddress + 1) >> 4));
}

//=============================================================================

void CZ80::DecodeLD_nn_IX(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (0x%02X%02X),IX", *(pAddress + 3), *(pAddress + 2));
}

//=============================================================================

void CZ80::DecodeLD_nn_IY(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (0x%02X%02X),IY", *(pAddress + 3), *(pAddress + 2));
}

//=============================================================================

void CZ80::DecodeLDSPHL(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD SP,HL");
}

//=============================================================================

void CZ80::DecodeLDSPIX(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD SP,IX");
}

//=============================================================================

void CZ80::DecodeLDSPIY(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD SP,IY");
}

//=============================================================================

void CZ80::DecodePUSHqq(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "PUSH %s", Get16BitRegisterString(*pAddress >> 4));
}

//=============================================================================

void CZ80::DecodePUSHIX(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "PUSH IX");
}

//=============================================================================

void CZ80::DecodePUSHIY(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "PUSH IY");
}

//=============================================================================

void CZ80::DecodePOPqq(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "POP %s", Get16BitRegisterString(*pAddress >> 4));
}

//=============================================================================

void CZ80::DecodePOPIX(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "POP IX");
}

//=============================================================================

void CZ80::DecodePOPIY(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "POP IY");
}

//=============================================================================

























/*
	 Z80 Instruction Set
	 author Neil Franklin, last modification 2008.01.02


Basics:
-------

All numbers and bit patterns in this file are usually given in hex, seldom in
binary, never in decimal or octal.


Sources for this data:
----------------------

1. Full description of the Z80, including an Intel<->Zilog Mnemonic convertion
appendix, contained in the book "Programming the Z80" by Rodney Zaks, 3rd
Edition, 1982. Used that for programming an Columbia Commander 924 (school
computer, first computer I ever programmed) and later an Multitech
Microprofessor MPF 1P (an other schools computer), so it can be assumed to
be correct.

2. Official Siemens (Intel licensee) 8085 (slightly extended 8080, which is
downward compatible Z80 predecessor) instruction shortlist. Used that to
program an TK-80 and an Siemens SME (yet another schools 2 computers). Very
unlikely to contain errors.

Of course errors of my own are to be expected.


Registers:
----------
(in order: data, address, pc, flags, auxillary)

A             8bit  Accumulator
A'            8bit  second copy of A, for EX AF instruction
B C D E H L   8bit  General Purpose Registers
B' C' .. L'   8bit  second copy of B,C,..L, for EXX instruction
BC DE HL     16bit  Register Pairs           (= B,C D,E H,L merged as high,low)
IX IY        16bit  Index Registers
SP           16bit  Stack Pointer            (push pre-decr, pop post-incr)
PC           16bit  Program Counter
F             8bit  Flags Register
F'            8bit  second copy of F, for EX AF instruction
AF           16bit  Register Pair            (= A,F merged as high,low)
IFF1          1bit  Interrupt FlipFlop       (1 enables INT)
IFF2          1bit  storage Interrupt FlipFlop  (copy used only by NMI/RETN)
I             8bit  Interrupt Vector Table Base Register (high address byte)
(low address byte from device reg)
R             7bit  Memory Refresh Register  (address counter for DRAM memory)


Flags Register:
---------------
(in order: from MSB/7 to LSB/0)

S    Sign             (result bit7 set)
Z    Zero             (result all bits cleared)
-    unused
H    Half Carry       (arithmetic result carry bit3->bit4, used only by DAA)
-    unused
P/V  Parity/oVerflow  (logical/shift/rotate: result parity even)
(arithmetic: result carry bit6->bit7)
(block transfer or search instructions: reached end)
(LD A,I and LD A,R instructions: copy of IFF2)
N    Subtract         (operation was additive or subtractive, used only by DAA)
C    Carry            (arithmetic result carry bit7->"bit8")

General policy seems to be:
- 8bit arithmetic (incl DAA): set all SZHVNC (additions N=0, subtractions N=1)
- 16bit arithmetic old (8080) ADD: set HNC (HC from high byte)
	new (Z80) ADC/SBC: set all SZHVNC as in 8bit (HC also from high byte)
										 - 8bit decrement/increment: set SZHVN, leave C unchanged (dec N=1, inc N=0)
	- 16bit decrement/increment: leave all flags, so address computation leaves all
										 - logic stuff: set all SZHPNC (with HNC allways set to constant)
															except new (Z80) BIT: which is very special, screws most flags
																										- shift/rotate old (8080) "A" stuff: set HNC
																															 new (Z80) register stuff: set all SZHVNC, like logic stuff
																																												 - load/store/move/in/out/stack: leave all flags
																																																			except new (Z80) IN r,(C): which does like logic stuff
																																																																 and new (Z80) block load/in/out: which are very special
																																																																																	block compare is even more so
																																																																																									 - jump/call/return: leave all flags


																																																																																													 Memory:
																																																																																													 -------

	Program+Data memory 64k*8bit (16bit address space)
I/O devices 256*8bit or 64k*8bit (8bit or 16bit address space)


	Addressing Modes, in Assembler Syntax:
	--------------------------------------
(in order: reg, immediate, address, reg indirect, reg indexed, pc)

	A               accumulator
	B C D E H L     register
	BC DE HL SP AF  register pair
	IX IY           index register pair
-nameless-      interrupt flag register (only EI/DI)
	I               interrupt register
	R               refresh register
	n               immediate8
	nn              immediate-extended16
(n)             short-address8 (only IN/OUT instr)
	(nn)            extended-address16
	(HL)            register pair indirect
	(BC) (DE)       register pair indirect (only LD A, / LD ,A instr)
	(C)             register pair BC (not just C!) indirect (only IN/OUT instr)
	-nameless-      stack pointer indirect decrement/increment (only PUSH/POP)
	(IX+d) (IY+d)   index register indirect + displacement8 (0..255)
	e               PC + offset8 (-128..+127) (only JR instr)
(IX) (IY)       index register indirect (only JP instr)


	Instruction Formats, in Machine Code Bytes:
	-------------------------------------------
(in order: simple, immediate, register, memory, in/out, jump+condit, prefix)

	oo        opcode8
	oonn      opcode8 immediate8 (only LD 8bit immed)
oonnnn    opcode8 immed-low8 immed-high8 (= little endian) (only LD 16b immed)
	or        opcode5+reg-address3
	orbb      opcode5+reg-address3 immediate8
	or        opcode6+regpair-address2
	or        opcode2+destreg-address3+sourcereg-address3
oonnnn    opcode8 address-low8 address-high8 (= little endian)
	oonn      opcode8 in/out-address8
	ocnnnn    opcode5+condi3 addr-low8 addr-high8 (= little endian) (only JP/CALL)
	oc        opcode5+condition3 (only RET cond instr)
	os        opcode5+address3 (only RST instr)
	ooee      opcode8 offset8 (only JR instr)
ocee      opcode6+condition2 offset8 (only JR instr)
	CBor      prefix-shift-or-bit8 opcode5+reg-address3
	CBbr      prefix-shift-or-bit8 opcode2+bitindex3+reg-address3
	EDoo      prefix-extend8 opcode8
	EDor      prefix-extend8 opcode5+reg-address3
	EDornnnn  prefix-extend8 opcode6+regpair-address2
EDornnnn  prefix-extend8 opc6+regpair-addr2 ad-low8 ad-high8 (= little endian)
	ii**      prefix-indexregswitch7+indexreg-addr1 above...
	ii**dd**  prefix-indexregswitch7+indexreg-addr1 above-first displac8 ab-rest...


	Instruction Bit Patterns and Operations:
	----------------------------------------
(in order: functional grouping: arithmetic, data transfer, jumps, auxillary)

	arithmetic/logic 8bit
	1sooosss
	..ooo...  opcode  operation
	..000...  ADD A,  A = A + source; FlagsSZHVNC,N=0                       "ADD"
	..001...  ADC A,  A = A + source + FlagC
	FlagsSZHVNC,N=0                            "ADd with Carry"
	..010...  SUB     A = A - source; FlagsSZHVNC,N=1                  "SUBtract"
	..011...  SBC A,  A = A - source - FlagC
	FlagsSZHVNC,N=1                       "SuBtract with Carry"
	..100...  AND     A = A bitwise-AND source;
	FlagsSZHPNC,H=1,N=0,C=0                               "AND"
	..101...  XOR     A = A bitwise-excl-OR source
	FlagsSZHPNC,H=0,N=0,C=0                      "eXclusive OR"
	..110...  OR      A = A bitwise-OR source
	FlagsSZHPNC,H=0,N=0,C=0                                "OR"
	..111...  CP      FlagsSZHVC = A - source; FlagN=1                  "ComPare"
	.s...sss  source
	.0...000  B
	.0...001  C
	.0...010  D
	.0...011  E
	.0...100  H
	.0...101  L
	.0...110  (HL)    mem[H,L]
	with prefix 11011101 (HL) -> (IX+d)  mem[IX+mem[PC+]] (displace8)
with prefix 11111101 (HL) -> (IY+d)  mem[IY+mem[PC+]] (displace8)
	.0...111  A
.1...110  n       mem[PC+] (immediate8)

	arithmetic 16bit
	00ss1001  opcode   operation
	........  ADD HL,  HL = HL + source
	FlagsNC,H=bit11-carry,N=0,C=bit15-carry     "ADD"
	with prefix 11011101 HL -> IX
	with prefix 11111101 HL -> IY
	..ss....  source
	..00....  BC       B,C
	..01....  DE       D,E
	..10....  HL       H,L
	with prefix 11011101 HL -> IX
	with prefix 11111101 HL -> IY
	..11....  SP       SP

	11101101 01sso010
	........ ....o...  opcode   operation
	........ ....0...  SBC HL,  HL = HL - source - FlagC    "SuBtract with Carry"
	FlagsSZHVNC,H=bit11-carry,N=1,C=bit15-carry
	........ ....1...  ADC HL,  HL = HL + source + FlagC         "ADd with Carry"
	FlagsSZHVNC,H=bit11-carry,N=0,C=bit15-carry
	........ ..ss....  source
	........ ..00....  BC       B,C
	........ ..01....  DE       D,E
	........ ..10....  HL       H,L
	........ ..11....  SP       SP

	increment/decrement 8bit
	00ddd10o
	.......o  opcode  operation
	.......0  INC     dest = dest + 1; FlagsSZHVN,N=0                 "INCrement"
	.......1  DEC     dest = dest - 1; FlagsSZHVN,N=1                 "DECrement"
	..ddd...  destination
	..000...  B
	..001...  C
	..010...  D
	..011...  E
	..100...  H
	..101...  L
	..110...  (HL)    mem[H,L]
	with prefix 11011101 (HL) -> (IX+d)  mem[IX+mem[PC+]] (displace8)
with prefix 11111101 (HL) -> (IY+d)  mem[IY+mem[PC+]] (displace8)
	..111...  A

	increment/decrement 16bit
	00ddo011
	....o... opcode  operation
	....0... INC     dest = dest + 1 (no flags!)                      "INCrement"
	....1... DEC     dest = dest - 1 (no flags!)                      "DECrement"
	..dd.... destination
	..00.... BC      B,C
	..01.... DE      D,E
	..10.... HL      H,L
	with prefix 11011101 HL -> IX
	with prefix 11111101 HL -> IY
	..11.... SP      SP

	shift/rotate
	000oo111
	...oo...  opcode  operation
	...00...  RLCA    A = A(bit6..0,7); FlagC=A(bit7); FlagsHN,H=0,N=0
	"Rotate Left with branch Carry Accumulator"
	...01...  RRCA    A = A(bit0,7..1); FlagC=A(bit0); FlagsHN,H=0,N=0
	"Rotate Right with branch Carry Accumulator"
	...10...  RLA     FlagC,A = A,FlagC; FlagsHN,H=0,N=0
	"Rotate Left Accumulator"
	...11...  RRA     A,FlagC = FlagC,A; FlagsHN,H=0,N=0
	"Rotate Right Accumulator"

	11001011 00oooddd
	........ ..ooo...  opcode  operation
	........ ..000...  RLC     dest = dest(b6..0,7); FlagC=dest(b7);
	FlagsSZHPN,H=0,N=0    "Rotate Left w branch Carry"
	........ ..001...  RRC     dest = dest(b0,7..1); FlagC=dest(b0);
	FlagsSZHPN,H=0,N=0   "Rotate Right w branch Carry"
	........ ..010...  RL      FlagC,dest = dest,FlagC
	FlagsSZHPN,H=0,N=0                   "Rotate Left"
	........ ..011...  RR      dest,FlagC = FlagC,dest
	FlagsSZHPN,H=0,N=0                  "Rotate Right"
	........ ..100...  SLA     FlagC,dest = dest,0
	FlagsSZHPN,H=0,N=0         "Shift Left Arithmetic"
	........ ..101...  SRA     dest,FlagC = dest(bit7),dest
	FlagsSZHPN,H=0,N=0        "Shift Right Arithmetic"
	........ ..111...  SRL     dest,FlagC = 0,dest
	FlagsSZHPN,H=0,N=0           "Shift Right Logical"
	........ .....ddd  destination
	........ .....000  B
	........ .....001  C
	........ .....010  D
	........ .....011  E
	........ .....100  H
	........ .....101  L
	........ .....110  (HL)    mem[H,L]
	with prefix 11011101 (HL) -> (IX+d)  mem[IX+mem[PC+]] (displace8)
with prefix 11111101 (HL) -> (IY+d)  mem[IY+mem[PC+]] (displace8)
	both cases displace8 as 3rd byte after 11001011 before 00oooddd
	........ .....111  A

	11101101 0110o111
	........ ....o...  opcode  operation
	........ ....0...  RRD  mem[H,L],A(bits0..3) = A(bits0..3),mm[H,L]
	FlagsSZHPN,H=0,N=0             "Rotate Right Decimal"
........ ....1...  RLD  A(bits0..3),mem[H,L] = mem[H,L],A(bits0..3)
	FlagsSZHPN,H=0,N=0              "Rotate Left Decimal"

	bit test and manipulation
	11001011 oobbbddd
	........ oo......  opcode  operation
	........ 01......  BIT     FlagZ = dest bitwise-AND 2^bit
	FlagsSHPN,S=?,Z=1-if-bit=0,H=1,P=?,N=0  "BIt Test"
	........ 10......  RES     dest = dest bitwise-AND NOT 2^bit      "bit RESet"
	........ 11......  SET     dest = dest bitwise-OR 2^bit             "bit SET"
	........ ..bbb...  bit
	........ ..000...  0,
	........ ..001...  1,
	........ ..010...  2,
	........ ..011...  3,
	........ ..100...  4,OnSendException
	........ ..101...  5,
	........ ..110...  6,
	........ ..111...  7,
	........ .....ddd  destination
	........ .....000  B
	........ .....001  C
	........ .....010  D
	........ .....011  E
	........ .....100  H
	........ .....101  L
	........ .....110  (HL)    mem[H,L]
	with prefix 11011101 (HL) -> (IX+d)  mem[IX+mem[PC+]] (displace8)
with prefix 11111101 (HL) -> (IY+d)  mem[IY+mem[PC+]] (displace8)
	both cases displace8 as 3rd byte after 11001011 before 00oooddd
	........ .....111  A

	other specialised arithmetic
	pppppppp oooooooo  opcode  operation
00100111  DAA if FlagN = 0 (after addition/increment)
	if A(bit3..0) > 9 or FlagA = 1 then A = A + 6; FlagC
	if A(bit7..4) > 9 or FlagC = 1 then A = A + 96
if FlagN = 1 (after subraction/decrement/negate)
	if A(bit3..0) > 9 or FlagA = 1 then A = A - 6; FlagC
	if A(bit7..4) > 9 or FlagC = 1 then A = A - 96
	FlagsSZAPC                "Decimal Adjust Accumulator"
	00101111  CPL     A = bitwise-NOT A; FlagsHN,H=1,N=1    "ComPLement"
	11101101 01000100  NEG     A = 0 - A; FlagsSZHVNC,N=1                "NEGate"

	load/store/register 8bit
	00aao010
	....o...  opcode  operation
	....0...  LD ,A   destination = A                                      "LoaD"
	....1...  LD A,   A = source                                           "LoaD"
	..aa....  address, source or destination
	..00....  (BC)    mem[B,C]
	..01....  (DE)    mem[D,E]
..11....  (nn)    mem[mem[PC++]] (address16)

	0sdddsss  opcode  operation
	LD      destination = source                                 "LoaD"
	..ddd...  destination
	..000...  B,
	..001...  C,
	..010...  D,
	..011...  E,
	..100...  H,
	..101...  L,
	..110...  (HL),   mem[H,L]
	with prefix 11011101 (HL) -> (IX+d)  mem[IX+mem[PC+]] (displace8)
with prefix 11111101 (HL) -> (IY+d)  mem[IY+mem[PC+]] (displace8)
	..111...  A,OnSendException
	.1...sss  source
	.1...000  B
	.1...001  C
	.1...010  D
	.1...011  E
	.1...100  H
	.1...101  L
	.1...110  (HL)    mem[H,L]  (not with  LD (HL),  (HALT is there!))
	with prefix 11011101 (HL) -> (IX+d)  mem[IX+mem[PC+]] (displace8)
with prefix 11111101 (HL) -> (IY+d)  mem[IY+mem[PC+]] (displace8)
	.1...111  A
.0...110  n       mem[PC+] (immediate8)

	load/store/register 16bit
	00dd0001  opcode  operation
	........  LD ,nn  destination = mem[PC++] (address16)                  "LoaD"
	..dd....  destination
	..00....  BC      B,C
	..01....  DE      D,E
	..10....  HL      H,L
	with prefix 11011101 HL -> IX
	with prefix 11111101 HL -> IY
	..11....  SP      SP

	0010o010
	....o...  opcode    operation
	....0...  LD (nn),  mem[mem[PC++]] = source (address16)                "LoaD"
	....1...  LD ,(nn)  destination = mem[mem[PC++]] (address16)           "LoaD"
	........  source or destination
	........  HL        H,L
	with prefix 11011101 HL -> IX
	with prefix 11111101 HL -> IY

	11101101 01aao011
	........ ....o...  opcode    operation
	........ ....0...  LD (nn),  mem[mem[PC++]] = source (address16)       "LoaD"
	........ ....1...  LD ,(nn)  destination = mem[mem[PC++]] (address16)  "LoaD"
	........ ..aa....  address, source or destination
	........ ..00....  BC        B,C
	........ ..01....  DE        D,E
........ ..10....  HL        H,L  (duplicate of above groups shorter HL form)
	........ ..11....  SP        SP

	oooooooo  opcode    operation
	11111001  LD SP,HL  SP = HL                                            "LoaD"
	with prefix 11011101 H,L -> IX
	with prefix 11111101 H,L -> IY

	stack push/pop
	11aa0o01
.....o..  opcode  operation (push pre-decr, pop post-incr)
	.....0..  POP     destination = mem[SP++]                              "POP"
	.....1..  PUSH    mem[--SP] = source                                  "PUSH"
	..aa....  address, source or destination
	..00....  BC      B,C
	..01....  DE      D,E
	..10....  HL      H,L
	with prefix 11011101 HL -> IX
	with prefix 11111101 HL -> IY
	..11....  AF      A,F

	exchange 16bit
	oooooooo  opcode      operation
	00001000  EX AF,AF'   AF <=> AF'                        "EXchange AF and AF^"
	11011001  EXX         BC,DE,HL <=> BC',DE',HL'            "EXchange eXtended"
	11100011  EX (SP),HL  mem[SP] <=> HL          "EXchange top of stack with HL"
	with prefix 11011101 HL -> IX
	with prefix 11111101 HL -> IY
	11101011  EX DE,HL    DE <=> HL                          "EXchange DE and HL"

	input and output
	1101o011
	....o...  opcode     operation
	....0...  OUT (n),A  io[n] = A                                       "OUTput"
	....1...  IN A,(n)   A = io[n]                                        "INput"

	11101101 01aaa00o
	........ .......o  opcode     operation
	........ .......0  IN ,(C)    destination = io[BC]
	FlagsSZHPN,H=0,N=0                      "INput"
	........ .......1  OUT (C),   io[BC] = source                        "OUTput"
	........ ..aaa...  address, source or destination
	........ ..000...  B
	........ ..001...  C
	........ ..010...  D
	........ ..011...  E
	........ ..100...  H
	........ ..101...  L
	........ ..111...  A

	block load/compare/in/out
	11101101 100rc0oo
	........ ......oo  opcode     operation
	........ ......00  LD*|LD*R   mem[DE+-1] = mem[HL+-1]; BC = BC-1
	FlagE=1 (FlagP/V special use) if regpair BC=0
	FlagsSZHN,S=?,Z=?,H=0,N=0                "Load"
	........ ......01  CP*|CP*R   FlagZ = A - mem[HL+-1]; BC = BC-1; match stops
	FlagE=1 (FlagP/V special use) if regpair BC=0
	FlagsSHN,S=?,H=?,N=1                  "ComPare"
	........ ......10  IN*|IN*R   B = B-1; mem[HL+-1] = io[BC]
	FlagZ=1 if register B=0
	FlagsSHVN,S=?,H=?,P=?,N=1               "INput"
	........ ......11  OUT*|OT*R  B = B-1; io[BC] = mem[HL+-1]
	FlagZ=1 if register B=0
	FlagsSHVN,S=?,H=?,P=?,N=1              "OUTput"
	........ ....c...  in/de-crement
	........ ....0...  I       DE = DE + 1; HL = HL + 1               "Increment"
	........ ....1...  D       DE = DE - 1; HL = HL - 1               "Decrement"
	........ ...r....  repeat
	........ ...0....  -       do once
	........ ...1....  R       if BC (LD*R|CP*R) or B (IN*R|OT*R) not 0
	then PC = PC-2                          "Repeat"

	other specialised data transfer
	11101101 010oo111
	........ ...oo...  opcode  operation
	........ ...00...  LD I,A  I = A                                       "LoaD"
	........ ...01...  LD R,A  R = A                                       "LoaD"
	........ ...10...  LD A,I  A = I; FlagI (Flagis P/V special use) = IFF
	FlagsSZHN,H=0,N=0                    "LoaD"
	........ ...11...  LD A,R  A = R; FlagI (FlagP/V special use as) = IFF
	FlagsSZHN,H=0,N=0                    "LoaD"

	jumps/subroutines and reset/interrupts
	pppppppp oooooooo  opcode   operation
	00000000  NOP      do nothing                         "No OPeration"
	00011000  JR e     PC = PC+mem[PC+] (offset8)        "Jump Relative"
	11101101 01000101  RETN     PC = mem[SP++]; IFF1 = IFF2
	"RETurn from Non maskable interrupt"
	11101101 01001101  RETI     PC = mem[SP++]; dev IF reset
	"RETurn from Interrupt"
01110110  HALT     NOP until RESET/INT (opc would be LD (HL),(HL)!)
	"HALT processor"
	11000011  JP nn    PC = mem[PC++] (address16)                 "JumP"
	11001001  RET      PC = mem[SP++]           "RETurn from subroutine"
	11001101  CALL nn  mem[--SP] = PC; PC = mem[PC++] (address16)
	"CALL subroutine"
	11101001  JP (HL)  PC = HL                                    "JumP"
	with prefix 11011101 (HL) -> (IX)
with prefix 11111101 (HL) -> (IY)

	oooooooo  opcode   operation
	11aaa111  RST      mem[--SP] = PC                                   "ReSTart"
PC = 0(bit15..6),aaa(bit5..3),0(bit2..0)
	..aaa...  number   restart address
	..000...  0        0000
	..001...  8        0008
	..010...  16       0010
	..011...  24       0018
	..100...  32       0020
	..101...  40       0028
	..110...  48       0030
	..111...  56       0038

	---- pin  RESET      IFF1 = IFF2 = 0; PC = 0000                       "RESET"
	---- pin  NMI        IFF2 = IFF1; IFF1 = 0; mem[--SP] = PC
	PC = 0066                       "Non Maskable Interrupt"
	---- pin  INT        IFF1 = IFF2 = 0                              "INTerrupt"
if IM 0: next instr from extern (usually RST)
	if IM 1: mem[--SP] = PC; PC = 0038
	if IM 2: mem[--SP] = PC; PC = mem[iidd]
	ii from I register, dd from devices INT reg

	branches/conditionals
	oooooooo  opcode   operation
	00010000  DJNZ e   B = B - 1
if B != 0 then PC = PC+mem[PC+] (offset8)
	"Decrement and Jump if Not Zero"

	oooccooo
	ooo..ooo  opcode    operation
001..000  JR cc ,e  if condition then PC = PC+mem[PC+] (offset8)
	"Jump Relative"
	...cc...  cc  condition
	...00...  NZ  Z = 0                                                 "No Zero"
	...01...  Z   Z = 1                                                    "Zero"
	...10...  NC  C = 0                                                "No Carry"
	...11...  C   C = 1                                                   "Carry"

	11cccoo0
	oo...ooo  opcode      operation
	.....00.  RET cc      if condition then PC = mem[SP++]               "RETurn"
	.....01.  JP cc,nn    if condition then PC = mem[PC++] (address16)     "JumP"
	.....10.  CALL cc,nn  if condition then mem[--SP] = PC;
	PC = mem[PC++]               "CALL subroutine"
	..ccc...  cc  condition
	..000...  NZ  Z = 0                                                 "No Zero"
	..001...  Z   Z = 1                                                    "Zero"
	..010...  NC  C = 0                                                "No Carry"
	..011...  C   C = 1                                                   "Carry"
	..100...  PO  P/V = 0                           "Parity Odd" or "no overflow"
	..101...  PE  P/V = 1                             "Parity Even" or "overflow"
	..110...  P   S = 0                                                    "Plus"
	..111...  M   S = 1                                                   "Minus"

	flags
	0011o111
	....o...  opcode  operation
	....0...  SCF     FlagC = 1; FlagsHN,H=0,N=0                "Set Carry Flag"
	....1...  CCF     FlagC = NOT FlagC
	FlagsHN,H=?,N=0                    "Complement Carry Flag"

	1111o011
	....o...  opcode  operation
	....0...  DI      IFF1 = IFF2 = 0                         "Disable Interupt"
	....1...  EI      delayed IFF1 = IFF2 = 1                 "Enable Interrupt"
	allways place exactly before RETI
	for this effect delayed by exactly 1 instruction

	11101101 010mm110
	........ ...mm...  set interrupt mode flipflops              "Interrupt Mode"
	........ ...00...  IM 0, 8080 mode, next instr from extern, usually RST
	........ ...10...  IM 1, Z80 mode 1, mem[--SP] = PC; PC = 0038  (= RST 38)
	........ ...11...  IM 2, Z80 mode 2, mem[--SP] = PC; PC = mem[iidd]
	ii base from reg I, dd vector from device


	Instruction Code List:
	----------------------
(full machine code bytes, in order: opcode number)

	00     NOP          10ee   DJNZ e       20ee   JR NZ,e      30ee   JR NC,e 
	01nnnn LD BC,nn     11nnnn LD DE,nn     21nnnn LD HL,nn     31nnnn LD SP,nn  
	02     LD (BC),A    12     LD (DE),A    22nnnn LD (nn),HL   32nnnn LD (nn),A
	03     INC BC       13     INC DE       23     INC HL       33     INC SP
	04     INC B        14     INC D        24     INC H        34     INC (HL)
05     DEC B        15     DEC D        25     DEC H        35     DEC (HL)
	06nn   LD B,n       16nn   LD D,n       26nn   LD H,n       36nn   LD (HL),n
	07     RLCA         17     RLA          27     DAA          37     SCF
	08     EX AF,AF'    18ee   JR e         28ee   JR Z,e       38ee   JR C,e
	09     ADD HL,BC    19     ADD HL,DE    29     ADD HL,HL    39     ADD HL,SP
0A     LD A,(BC)    1A     LD A,(DE)    2Annnn LD HL,(nn)   3Annnn LD A,(nn)
	0B     DEC BC       1B     DEC DE       2B     DEC HL       3B     DEC SP
	0C     INC C        1C     INC E        2C     INC L        3C     INC A
	0D     DEC C        1D     DEC E        2D     DEC L        3D     DEC A
	0Enn   LD C,n       1Enn   LD E,n       2Enn   LD L,n       3Enn   LD A,n
	0F     RRCA         1F     RRA          2F     CPL          3F     CCF

40     LD B,B       50     LD D,B       60     LD H,B       70     LD (HL)
	41     LD B,C       51     LD D,C       61     LD H,C       71     LD (HL),C
	42     LD B,D       52     LD D,D       62     LD H,D       72     LD (HL),D
	43     LD B,E       53     LD D,E       63     LD H,E       73     LD (HL),E
	44     LD B,H       54     LD D,H       64     LD H,H       74     LD (HL),H
	45     LD B,L       55     LD D,L       65     LD H,L       75     LD (HL),L
	46     LD B,(HL)    56     LD D,(HL)    66     LD H,(HL)    76     HALT
	47     LD B,A       57     LD D,A       67     LD H,A       77     LD (HL),A
	48     LD C,B       58     LD E,B       68     LD L,B       78     LD A,B
	49     LD C,C       59     LD E,C       69     LD L,C       79     LD A,C
	4A     LD C,D       5A     LD E,D       6A     LD L,D       7A     LD A,D
	4B     LD C,E       5B     LD E,E       6B     LD L,E       7B     LD A,E
	4C     LD C,H       5C     LD E,H       6C     LD L,H       7C     LD A,H
	4D     LD C,L       5D     LD E,L       6D     LD L,L       7D     LD A,L
4E     LD C,(HL)    5E     LD E,(HL)    6E     LD L,(HL)    7E     LD A,(HL)
	4F     LD C,A       5F     LD E,A       6F     LD L,A       7F     LD A,A

	40|49|52|5B|64|6D|7F: are all NOPs
	76: would be LD (HL),(HL) (3 cycle NOP) but used for HALT

	80     ADD A,B      90     SUB B        A0     AND B        B0     OR B
	81     ADD A,C      91     SUB C        A1     AND C        B1     OR C
	82     ADD A,D      92     SUB D        A2     AND D        B2     OR D
	83     ADD A,E      93     SUB E        A3     AND E        B3     OR E
	84     ADD A,H      94     SUB H        A4     AND H        B4     OR H
	85     ADD A,L      95     SUB L        A5     AND L        B5     OR L
86     ADD A,(HL)   96     SUB (HL)     A6     AND (HL)     B6     OR (HL)
	87     ADD A,A      97     SUB A        A7     AND A        B7     OR A
	88     ADC A,B      98     SBC A,B      A8     XOR B        B8     CP B
	89     ADC A,C      99     SBC A,C      A9     XOR C        B9     CP C
	8A     ADC A,D      9A     SBC A,D      AA     XOR D        BA     CP D
	8B     ADC A,E      9B     SBC A,E      AB     XOR E        BB     CP E
	8C     ADC A,H      9C     SBC A,H      AC     XOR H        BC     CP H
	8D     ADC A,L      9D     SBC A,L      AD     XOR L        BD     CP L
8E     ADC A,(HL)   9E     SBC A,(HL)   AE     XOR (HL)     BE     CP (HL)
	8F     ADC A,A      9F     SBC A,A      AF     XOR A        BF     CP A

	C0     RET NZ       D0     RET NC       E0     RET PO       F0     RET P
	C1     POP BC       D1     POP DE       E1     POP HL       F1     POP AF
	C2nnnn JP NZ,nn     D2nnnn JP NC,nn     E2nnnn JP PO,nn     F2nnnn JP P,nn
	C3nnnn JP nn        D3nn   OUT (n),A    E3     EX (SP),HL   F3     DI
	C4nnnn CALL NZ,nn   D4nnnn CALL NC,nn   E4nnnn CALL PO,nn   F4nnnn CALL P,nn
	C5     PUSH BC      D5     PUSH DE      E5     PUSH HL      F5     PUSH AF
	C6nn   ADD A,n      D6nn   SUB n        E6nn   AND n        F6nn   OR n
	C7     RST 0        D7     RST 16       E7     RST 32       F7     RST 48
	C8     RET Z        D8     RET C        E8     RET PE       F8     RET M
	C9     RET          D9     EXX          E9     JP (HL)      F9     LD SP,HL
	CAnnnn JP Z,nn      DAnnnn JP C,nn      EAnn   JP PE,nn     FAnnnn JP M,nn
	CB**   prefixCB     DBnn   IN  A,(n)    EB     EX DE,HL     FB     EI
	CCnnnn CALL Z,nn    DCnnnn CAll C,nn    ECnnnn CALL PE,nn   FCnnnn CALL M,nn
	CDnnnn CALL nn      DD**   prefixDD     ED**   prefixED     FD**   prefixFD
	CEnn   ADC A,n      DEnn   SBC n        EEnn   XOR n        FEnn   CP n
	CF     RST 8        DF     RST 24       EF     RST 40       FF     RST 56

	CB**: prefixCB, selects further 256 extended "CB" opcodes, table below
	ED**: prefixED, selects further 256 extended "ED" opcodes, table below
	DD**: prefixDD, converts HL->IX, (HL)->(IX+d), JP (HL)->(IX)
FD**: prefixFD, converts HL->IY, (HL)->(IY+d), JP (HL)->(IY)

	Extended "CB" Instructions:

	CB00   RLC B        CB10   RL B         CB20   SLA B        ----   -
	CB01   RLC C        CB11   RL C         CB21   SLA C        ----   -
	CB02   RLC D        CB12   RL D         CB22   SLA D        ----   -
	CB03   RLC E        CB13   RL E         CB23   SLA E        ----   -
	CB04   RLC H        CB14   RL H         CB24   SLA H        ----   -
	CB05   RLC L        CB15   RL L         CB25   SLA L        ----   -
	CB06   RLC (HL)     CB16   RL (HL)      CB26   SLA (HL)     ----   -
	CB07   RLC A        CB17   RL A         CB27   SLA A        ----   -
	CB08   RRC B        CB18   RR B         CB28   SRA B        CB38   SRL B
	CB09   RRC C        CB19   RR C         CB29   SRA C        CB39   SRL C
	CB0A   RRC D        CB1A   RR D         CB2A   SRA D        CB3A   SRL D
	CB0B   RRC E        CB1B   RR E         CB2B   SRA E        CB3B   SRL E
	CB0C   RRC H        CB1C   RR H         CB2C   SRA H        CB3C   SRL H
	CB0D   RRC L        CB1D   RR L         CB2D   SRA L        CB3D   SRL L
CB0E   RRC (HL)     CB1E   RR (HL)      CB2E   SRA (HL)     CB3E   SRL (HL)
	CB0F   RRC A        CB1F   RR A         CB2F   SRA A        CB3F   SRL A

	CB40   BIT 0,B      CB50   BIT 2,B      CB60   BIT 4,B      CB70   BIT 6,B
	CB41   BIT 0,C      CB51   BIT 2,C      CB61   BIT 4,C      CB71   BIT 6,C
	CB42   BIT 0,D      CB52   BIT 2,D      CB62   BIT 4,D      CB72   BIT 6,D
	CB43   BIT 0,E      CB53   BIT 2,E      CB63   BIT 4,E      CB73   BIT 6,E
	CB44   BIT 0,H      CB54   BIT 2,H      CB64   BIT 4,H      CB74   BIT 6,H
	CB45   BIT 0,L      CB55   BIT 2,L      CB65   BIT 4,L      CB75   BIT 6,L
CB46   BIT 0,(HL)   CB56   BIT 2,(HL)   CB66   BIT 4,(HL)   CB76   BIT 6,(HL)
	CB47   BIT 0,A      CB57   BIT 2,A      CB67   BIT 4,A      CB77   BIT 6,A
	CB48   BIT 1,B      CB58   BIT 3,B      CB68   BIT 5,B      CB78   BIT 7,B
	CB49   BIT 1,C      CB59   BIT 3,C      CB69   BIT 5,C      CB79   BIT 7,C
	CB4A   BIT 1,D      CB5A   BIT 3,D      CB6A   BIT 5,D      CB7A   BIT 7,D
	CB4B   BIT 1,E      CB5B   BIT 3,E      CB6B   BIT 5,E      CB7B   BIT 7,E
	CB4C   BIT 1,H      CB5C   BIT 3,H      CB6C   BIT 5,H      CB7C   BIT 7,H
	CB4D   BIT 1,L      CB5D   BIT 3,L      CB6D   BIT 5,L      CB7D   BIT 7,L
CB4E   BIT 1,(HL)   CB5E   BIT 3,(HL)   CB6E   BIT 5,(HL)   CB7E   BIT 7,(HL)
	CB4F   BIT 1,A      CB5F   BIT 3,A      CB6F   BIT 5,A      CB7F   BIT 7,A

	CB80   RES 0,B      CB90   RES 2,B      CBA0   RES 4,B      CBB0   RES 6,B
	CB81   RES 0,C      CB91   RES 2,C      CBA1   RES 4,C      CBB1   RES 6,C
	CB82   RES 0,D      CB92   RES 2,D      CBA2   RES 4,D      CBB2   RES 6,D
	CB83   RES 0,E      CB93   RES 2,E      CBA3   RES 4,E      CBB3   RES 6,E
	CB84   RES 0,H      CB94   RES 2,H      CBA4   RES 4,H      CBB4   RES 6,H
	CB85   RES 0,L      CB95   RES 2,L      CBA5   RES 4,L      CBB5   RES 6,L
CB86   RES 0,(HL)   CB96   RES 2,(HL)   CBA6   RES 4,(HL)   CBB6   RES 6,(HL)
	CB87   RES 0,A      CB97   RES 2,A      CBA7   RES 4,A      CBB7   RES 6,A
	CB88   RES 1,B      CB98   RES 3,B      CBA8   RES 5,B      CBB8   RES 7,B
	CB89   RES 1,C      CB99   RES 3,C      CBA9   RES 5,C      CBB9   RES 7,C
	CB8A   RES 1,D      CB9A   RES 3,D      CBAA   RES 5,D      CBBA   RES 7,D
	CB8B   RES 1,E      CB9B   RES 3,E      CBAB   RES 5,E      CBBB   RES 7,E
	CB8C   RES 1,H      CB9C   RES 3,H      CBAC   RES 5,H      CBBC   RES 7,H
	CB8D   RES 1,L      CB9D   RES 3,L      CBAD   RES 5,L      CBBD   RES 7,L
CB8E   RES 1,(HL)   CB9E   RES 3,(HL)   CBAE   RES 5,(HL)   CBBE   RES 7,(HL)
	CB8F   RES 1,A      CB9F   RES 3,A      CBAF   RES 5,A      CBBF   RES 7,A

	CBC0   SET 0,B      CBD0   SET 2,B      CBE0   SET 4,B      CBF0   SET 6,B
	CBC1   SET 0,C      CBD1   SET 2,C      CBE1   SET 4,C      CBF1   SET 6,C
	CBC2   SET 0,D      CBD2   SET 2,D      CBE2   SET 4,D      CBF2   SET 6,D
	CBC3   SET 0,E      CBD3   SET 2,E      CBE3   SET 4,E      CBF3   SET 6,E
	CBC4   SET 0,H      CBD4   SET 2,H      CBE4   SET 4,H      CBF4   SET 6,H
	CBC5   SET 0,L      CBD5   SET 2,L      CBE5   SET 4,L      CBF5   SET 6,L
CBC6   SET 0,(HL)   CBD6   SET 2,(HL)   CBE6   SET 4,(HL)   CBF6   SET 6,(HL)
	CBC7   SET 0,A      CBD7   SET 2,A      CBE7   SET 4,A      CBF7   SET 6,A
	CBC8   SET 1,B      CBD8   SET 3,B      CBE8   SET 5,B      CBF8   SET 7,B
	CBC9   SET 1,C      CBD9   SET 3,C      CBE9   SET 5,C      CBF9   SET 7,C
	CBCA   SET 1,D      CBDA   SET 3,D      CBEA   SET 5,D      CBFA   SET 7,D
	CBCB   SET 1,E      CBDB   SET 3,E      CBEB   SET 5,E      CBFB   SET 7,E
	CBCC   SET 1,H      CBDC   SET 3,H      CBEC   SET 5,H      CBFC   SET 7,H
	CBCD   SET 1,L      CBDD   SET 3,L      CBED   SET 5,L      CBFD   SET 7,L
CBCE   SET 1,(HL)   CBDE   SET 3,(HL)   CBEE   SET 5,(HL)   CBFE   SET 7,(HL)
	CBCF   SET 1,A      CBDF   SET 3,A      CBEF   SET 5,A      CBFF   SET 7,A

	Extended "ED" Instructions:

	ED00..ED3F unused

	ED40   IN B,(C)     ED50   IN D,(C)     ED60   IN H,(C)     ----   -
	ED41   OUT (C),B    ED51   OUT (C),D    ED61   OUT (C),H    ----   -
	ED42   SBC HL,BC    ED52   SBC HL,DE    ED62   SBC HL,HL    ED72   SBC HL,SP
	ED43** LD (nn),BC   ED53** LD (nn),DE   ----   -            ED73** LD (nn),SP
	ED44   NEG          ----   -            ----   -            ----   -
	ED45   RETN         ----   -            ----   -            ----   -
	ED46   IM 0         ED56   IM 1         ----   -            ----   -
	ED47   LD I,A       ED57   LD A,I       ED67   RRD          ----   -
ED48   IN C,(C)     ED58   IN E,(C)     ED68   IN L,(C)     ED78   IN A,(C)
	ED49   OUT (C),C    ED59   OUT (C),E    ED69   OUT (C),L    ED79   OUT (C),A
	ED4A   ADC HL,BC    ED5A   ADC HL,DE    ED6A   ADC HL,HL    ED7A   ADC HL,SP
ED4B** LD BC,(nn)   ED5B** LD DE,(nn)   ----   -            ED7B** LD SP,(nn)
	----   -            ----   -            ----   -            ----   -
	ED4D   RETI         ----   -            ----   -            ----   -
	----   -            ED5E   IM 2         ----   -            ----   -
	ED4F   LD R,A       ED5F   LD A,R       ED6F   RLD          ----   -

	EDx3**|EDxB**: actually EDxxnnnn, but no space in table

	ED80..ED9F unused

	EDA0   LDI          EDA8   LDD          EDB0   LDIR         EDB8   LDDR
	EDA1   CPI          EDA9   CPD          EDB1   CPIR         EDB9   CPDR
	EDA2   INI          EDAA   IND          EDB2   INIR         EDBA   INDR
	EDA3   OUTI         EDAB   OUTD         EDB3   OTIR         EDBB   OTDR

	ED[AB][4..7,C..F] unused

	EDC0..EDFF unused


	Instruction Code Table:
	-----------------------
(only opcodes, in order: ver: bit7..6/5..3, hor: bit2..0)

	+    00       01       02       03       04       05       06       07

	00   NOP      LD BC,   LD (BC),AINC BC   INC B    DEC B    LD B,    RLCA
	08   EX AF,AF'ADD HL,BCLD A,(BC)DEC BC   INC C    DEC C    LD C,    RRCA
	10   DJNZ     LD DE,   LD (DE),AINC DE   INC D    DEC D    LD D,    RLA
	18   JR       ADD HL,DELD A,(DE)DEC DE   INC E    DEC E    LD E,    RRA
	20   JR NZ,   LD HL,   LD (),HL INC HL   INC H    DEC H    LD H,    DAA
	28   JR Z,    ADD HL,HLLD HL,() DEC HL   INC L    DEC L    LD L,    CPL
	30   JR NC,   LD SP,   LD (),A  INC SP   INC (HL) DEC (HL) LD (HL), SCF
	38   JR C,    ADD HL,SPLD A,()  DEC SP   INC A    DEC A    LD A,    CCF

	40   LD B,B   LD B,C   LD B,D   LD B,E   LD B,H   LD B,L   LD B,(HL)LD B,A
	48   LD C,B   LD C,C   LD C,D   LD C,E   LD C,H   LD C,L   LD C,(HL)LD C,A
	50   LD D,B   LD D,C   LD D,D   LD D,E   LD D,H   LD D,L   LD D,(HL)LD D,A
	58   LD E,B   LD E,C   LD E,D   LD E,E   LD E,H   LD E,L   LD E,(HL)LD E,A
	60   LD H,B   LD H,C   LD H,D   LD H,E   LD H,H   LD H,L   LD H,(HL)LD H,A
	68   LD L,B   LD L,C   LD L,D   LD L,E   LD L,H   LD L,L   LD L,(HL)LD L,A
	70   LD (HL),BLD (HL),CLD (HL),DLD (HL),ELD (HL),HLD (HL),LHALT     LD (HL),A
	78   LD A,B   LD A,C   LD A,D   LD A,E   LD A,H   LD A,L   LD A,(HL)LD A,A

	80   ADD A,B  ADD A,C  ADD A,D  ADD A,E  ADD A,H  ADD A,L  ADD A,(HLADD A,A
	88   ADC A,B  ADC A,C  ADC A,D  ADC A,E  ADC A,H  ADC A,L  ADC A,(HLADC A,A
	90   SUB B    SUB C    SUB D    SUB E    SUB H    SUB L    SUB (HL) SUB A
	98   SBC A,B  SBC A,C  SBC A,D  SBC A,E  SBC A,H  SBC A,L  SBC A,(HLSBC A,A
	A0   AND B    AND C    AND D    AND E    AND H    AND L    AND (HL) AND A
	A8   XOR B    XOR C    XOR D    XOR E    XOR H    XOR L    XOR (HL) XOR A
	B0   OR B     OR C     OR D     OR E     OR H     OR L     OR (HL)  OR A
	B8   CP B     CP C     CP D     CP E     CP H     CP L     CP (HL)  CP A

	C0   RET NZ   POP BC   JP NZ,   JP       CALL NZ, PUSH BC  ADD A,   RST 0
	C8   RET Z    RET      JP Z,    prefixCB CALL Z,  CALL     ADC A,   RST 8
	D0   RET NC   POP DE   JP NC,   OUT (),A CALL NC, PUSH DE  SUB      RST 16
	D8   RET C    EXX      JP C,    IN A,()  CALL C,  prefixDD SBC A,   RST 24
	E0   RET PO   POP HL   JP PO,   EX (SP),HCALL PO, PUSH HL  AND      RST 32
	E8   RET PE   JP       JP PE,   EX DE,HL CALL PE, prefixED XOR      RST 40
	F0   RET P    POP AF   JP P,    DI       CALL P,  PUSH AF  XOR      RST 48
	F8   RET M    LD SP,HL JP M,    EI       CALL M,  prefixFD CP       RST 56

	Extended "CB" Instructions:

	+    00       01       02       03       04       05       06         07

	00   RLC B    RLC C    RLC D    RLC E    RLC H    RLC L    RLC (HL)   RLC A
	08   RRC B    RRC C    RRC D    RRC E    RRC H    RRC L    RRC (HL)   RRC A
	10   RL B     RL C     RL D     RL E     RL H     RL L     RL (HL)    RL A
	18   RR B     RR C     RR D     RR E     RR H     RR L     RR (HL)    RR A
	20   SLA B    SLA C    SLA D    SLA E    SLA H    SLA L    SLA (HL)   SLA A
	28   SRA B    SRA C    SRA D    SRA E    SRA H    SRA L    SRA (HL)   SRA A
	30   -        -        -        -        -        -        -          -  
	38   SRL B    SRL C    SRL  D   SRL E    SRL H    SRL L    SRL (HL)   SRL A

	40   BIT 0,B  BIT 0,C  BIT 0,D  BIT 0,E  BIT 0,H  BIT 0,L  BIT 0,(HL) BIT 0,A
	48   BIT 1,B  BIT 1,C  BIT 1,D  BIT 1,E  BIT 1,H  BIT 1,L  BIT 1,(HL) BIT 2,A
	50   BIT 2,B  BIT 2,C  BIT 2,D  BIT 2,E  BIT 2,H  BIT 2,L  BIT 2,(HL) BIT 3,A
	58   BIT 3,B  BIT 3,C  BIT 3,D  BIT 3,E  BIT 3,H  BIT 3,L  BIT 3,(HL) BIT 3,A
	60   BIT 4,B  BIT 4,C  BIT 4,D  BIT 4,E  BIT 4,H  BIT 4,L  BIT 4,(HL) BIT 4,A
	68   BIT 5,B  BIT 5,C  BIT 5,D  BIT 5,E  BIT 5,H  BIT 5,L  BIT 5,(HL) BIT 5,A
	70   BIT 6,B  BIT 6,C  BIT 6,D  BIT 6,E  BIT 6,H  BIT 6,L  BIT 6,(HL) BIT 6,A
	78   BIT 7,B  BIT 7,C  BIT 7,D  BIT 7,E  BIT 7,H  BIT 7,L  BIT 7,(HL) BIT 7,A

	80   RES 0,B  RES 0,C  RES 0,D  RES 0,E  RES 0,H  RES 0,L  RES 0,(HL) RES 0,A
	88   RES 1,B  RES 1,C  RES 1,D  RES 1,E  RES 1,H  RES 1,L  RES 1,(HL) RES 1,A
	90   RES 2,B  RES 2,C  RES 2,D  RES 2,E  RES 2,H  RES 2,L  RES 2,(HL) RES 2,A
	98   RES 3,B  RES 3,C  RES 3,D  RES 3,E  RES 3,H  RES 3,L  RES 3,(HL) RES 3,A
	A0   RES 4,B  RES 4,C  RES 4,D  RES 4,E  RES 4,H  RES 4,L  RES 4,(HL) RES 4,A
	A8   RES 5,B  RES 5,C  RES 5,D  RES 5,E  RES 5,H  RES 5,L  RES 5,(HL) RES 5,A
	B0   RES 6,B  RES 6,C  RES 6,D  RES 6,E  RES 6,H  RES 6,L  RES 6,(HL) RES 6,A
	B8   RES 7,B  RES 7,C  RES 7,D  RES 7,E  RES 7,H  RES 7,L  RES 7,(HL) RES 7,A

	C0   SET 0,B  SET 0,C  SET 0,D  SET 0,E  SET 0,H  SET 0,L  SET 0,(HL) SET 0,A
	C8   SET 1,B  SET 1,C  SET 1,D  SET 1,E  SET 1,H  SET 1,L  SET 1,(HL) SET 1,A
	D0   SET 2,B  SET 2,C  SET 2,D  SET 2,E  SET 2,H  SET 2,L  SET 2,(HL) SET 2,A
	D8   SET 3,B  SET 3,C  SET 3,D  SET 3,E  SET 3,H  SET 3,L  SET 3,(HL) SET 3,A
	E0   SET 4,B  SET 4,C  SET 4,D  SET 4,E  SET 4,H  SET 4,L  SET 4,(HL) SET 4,A
	E8   SET 5,B  SET 5,C  SET 5,D  SET 5,E  SET 5,H  SET 5,L  SET 5,(HL) SET 5,A
	F0   SET 6,B  SET 6,C  SET 6,D  SET 6,E  SET 6,H  SET 6,L  SET 6,(HL) SET 6,A
	F8   SET 7,B  SET 7,C  SET 7,D  SET 7,E  SET 7,H  SET 7,L  SET 7,(HL) SET 7,A

	Extended "ED" Instructions (nothing in 00-3F and 80-9F and C0-FF):

	+    00       01        02        03       04       05       06       07

	40   IN B,(C) OUT (C),B SBC HL,BC LD (),BC NEG      RETN     IM 0     LD I,A
	48   IN C,(C) OUT (C),C ADC HL,BC LD BC,() -        -        -        LD R,A
	50   IN D,(C) OUT (C),D SBC HL,DE LD (),DE -        -        IM 1     LD A,I
	58   IN E,(C) OUT (C),E ADC HL,DE LD DE,() -        -        IM 2     LD A,R
	60   IN H,(C) OUT (C),H SBC HL,HL -        -        RETI     -        RRD
	68   IN L,(C) OUT (C),L ADC HL,HL -        -        -        -        RLD
	70   -        -         SBC HL,SP LD (),SP -        -        -        -
	78   IN A,(C) OUT (C),A ADC HL,SP LD SP,() -        -        -        -

	A0   LDI      CPI      INI      OUTI     -        -        -        -
	A8   LDD      CPD      IND      OUTD     -        -        -        -
	B0   LDIR     CPIR     INIR     OTIR     -        -        -        -
	B8   LDDR     CPDR     INDR     OTDR     -        -        -        -
					*/
