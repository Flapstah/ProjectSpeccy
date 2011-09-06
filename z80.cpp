#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TEMP
#include <GL/glfw.h>
// TEMP

#include "z80.h"

//=============================================================================
// TODO:
// Work out why delete and cursor keys don't work
// Possible BCD error?
// Separate out the block repeat instructions to do a single operation and put
// the PC back until BC == 0
//=============================================================================

// Helper macros to determine 8 and 16 bit registers from opcodes
#define REGISTER_8BIT(_3bits_) m_RegisterMemory[m_8BitRegisterOffset[_3bits_ & 0x07]]
#define REGISTER_16BIT(_2bits_) *(reinterpret_cast<uint16*>(&m_RegisterMemory[m_16BitRegisterOffset[_2bits_ & 0x03]]))
#define REGISTER_16BIT_LO(_2bits_) m_RegisterMemory[m_16BitRegisterOffset[_2bits_ & 0x03] + LO]
#define REGISTER_16BIT_HI(_2bits_) m_RegisterMemory[m_16BitRegisterOffset[_2bits_ & 0x03] + HI]

//=============================================================================

//=============================================================================
//	All information contained herein is based on Zilog's "Z80 Family CPU User
//	Manual".
//
//	Minor timing corrections have been made for instruction execution time where
//	each T State is assumed to be 0.25 microseconds, based on a 4MHz clock.
//=============================================================================

uint16 g_addressBreakpoint = 0x1024; // ED_ENTER
uint16 g_dataBreakpoint = 0; //0x5C3A; // ERR_NR
uint16 g_stackContentsBreakpoint = 0xDC62;
uint8 g_stackContentsBreakpointNumItems = 64;

#define LEE_COMPATIBLE

//=============================================================================

CZ80::CZ80(uint8* pMemory, float clockSpeedMHz)
	// Map registers to register memory
	: m_AF(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_AF])))
	, m_BC(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_BC])))
	, m_DE(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_DE])))
	, m_HL(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_HL])))
	, m_IX(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_IX])))
	, m_IXh(m_RegisterMemory[eR_IXh])
	, m_IXl(m_RegisterMemory[eR_IXl])
	, m_IY(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_IY])))
	, m_IYh(m_RegisterMemory[eR_IYh])
	, m_IYl(m_RegisterMemory[eR_IYl])
	, m_SP(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_SP])))
	, m_PC(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_PC])))
	, m_PCh(m_RegisterMemory[eR_PCh])
	, m_PCl(m_RegisterMemory[eR_PCl])
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
	, m_State(*(reinterpret_cast<SProcessorState*>(&m_RegisterMemory[eR_State])))
	, m_address(*(reinterpret_cast<uint16*>(&m_RegisterMemory[eR_Address])))
	, m_addresshi(m_RegisterMemory[eR_Addressh])
	, m_addresslo(m_RegisterMemory[eR_Addressl])
	, m_pMemory(pMemory)
	, m_clockSpeedMHz(clockSpeedMHz)
	, m_reciprocalClockSpeedMHz(1.0f / clockSpeedMHz)
#if defined(NDEBUG)
	, m_enableDebug(false)
#else
	, m_enableDebug(true)
#endif // defined(NDEBUG)
	, m_enableUnattendedDebug(false)
	, m_enableOutputStatus(false)
	, m_enableBreakpoints(false)
	, m_enableProgramFlowBreakpoints(false)
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

	Reset();
}

//=============================================================================

void CZ80::Reset(void)
{
	memset(m_RegisterMemory, 0, sizeof(m_RegisterMemory));
}

//=============================================================================

float CZ80::Update(float milliseconds)
{
	float microseconds_available = milliseconds * 1000.0f;
	float tstates_available = microseconds_available / m_reciprocalClockSpeedMHz;

	if (GetEnableDebug() || GetEnableUnattendedDebug())
	{
		// TODO: Interrupts need servicing here, I think
		tstates_available -= SingleStep();
	}
	else
	{
		if (m_State.m_IFF1)
		{
			m_State.m_IFF2 = m_State.m_IFF1;
			m_State.m_IFF1 = 0;

			switch (m_State.m_InterruptMode)
			{
				case 0:
					break;
				case 1:
					if (m_pMemory[m_PC] == 0x76)
					{
						++m_PC;
					}

					WriteMemory(--m_SP, m_PCh);
					WriteMemory(--m_SP, m_PCl);
					m_PC = 0x0038;
					tstates_available -= 13; // RST (11) + 2
					break;
				case 2:
					break;
				default:
					m_enableDebug = true;
					HitBreakpoint("illegal interrupt mode");
					break;
			}
		}

		while (tstates_available >= 4.0f)
		{
			tstates_available -= SingleStep();
	
			if (GetEnableDebug())
				break;
		}
	}

	microseconds_available = tstates_available * m_reciprocalClockSpeedMHz;
	return microseconds_available / 1000.0f;
}

//=============================================================================

float CZ80::SingleStep(void)
{
	if (GetEnableDebug() || GetEnableUnattendedDebug())
	{
		if (GetEnableOutputStatus())
		{
			OutputStatus();
		}

		OutputInstruction(m_PC);
	}

	if (GetEnableBreakpoints() && (m_PC == g_addressBreakpoint))
	{
		HitBreakpoint("address");
	}

	uint16 prevPC = m_PC;
	uint16 prevSP = m_SP;
	float microseconds_elapsed = static_cast<float>(Step()) * m_reciprocalClockSpeedMHz;

	if ((prevPC < 0x386E) && (m_PC >= 0x386E))
	{
		fprintf(stderr, "[Z80] PC just jumped from %04X to %04X, an invalid ROM location\n", prevPC, m_PC);
		OutputInstruction(prevPC);
		HitBreakpoint("invalid ROM location");
		exit(EXIT_FAILURE);
	}

	if ((m_SP >= 0x5C00) && (m_SP <= 0x5CB5))
	{
		fprintf(stderr, "[Z80] SP just jumped into the system variables area (changed from %04X to %04X), at location %04X\n", prevSP, m_SP, prevPC);
		OutputInstruction(prevPC);
		HitBreakpoint("SP corrupt");
	}

//	static uint16 origSP = 0;
//	if ((origSP == 0) && (m_SP != 0))
//	{
//		origSP = m_SP;
//	}

//	m_address = g_stackContentsBreakpoint;
//	if (GetEnableBreakpoints() && (origSP != 0))
//	{
//		uint16 offset;
//		g_stackContentsBreakpointNumItems = (origSP - m_SP) >> 1;
//		fprintf(stderr, "[Z80] Checking %d items from SP address %04X\n", g_stackContentsBreakpointNumItems, m_SP);
//		if (g_stackContentsBreakpointNumItems > 20)
//		{
//			OutputInstruction(prevPC);
//			HitBreakpoint("too many stack items");
//		}
//
//		for (uint16 index = 0; index < g_stackContentsBreakpointNumItems; ++index)
//		{
//			offset = index << 1;
//			if ((m_pMemory[m_SP + offset] == m_addresslo) && (m_pMemory[m_SP + offset + 1] == m_addresshi))
//			{
//				fprintf(stderr, "[Z80] %04X is now on the stack (offset %d) at address %04X\n", m_address, offset, prevPC);
//				OutputInstruction(prevPC);
//				HitBreakpoint("stack contents");
//			}
//		}
//	}

	return microseconds_elapsed;
}

//=============================================================================

void CZ80::LoadSNA(uint8* regs)
{
	uint8 index = 0;
	m_I = regs[index++];

	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_HLalt = m_address;
	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_DEalt = m_address;
	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_BCalt = m_address;
	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_AFalt = m_address;

	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_HL = m_address;
	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_DE = m_address;
	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_BC = m_address;
	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_IX = m_address;
	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_IY = m_address;

	m_State.m_IFF2 = (regs[index] >> 1) & 0x01;
	m_State.m_IFF1 = regs[index++] & 0x01;

	m_R = regs[index++];

	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_AF = m_address;
	m_addresslo = regs[index++];
	m_addresshi = regs[index++];
	m_SP = m_address;

	m_State.m_InterruptMode = regs[index++];

	ImplementRETN();
}

//=============================================================================

bool CZ80::GetEnableDebug(void) const
{
 	return m_enableDebug;
}

//=============================================================================

void CZ80::SetEnableDebug(bool set)
{
 	m_enableDebug = set;
	fprintf(stderr, "[Z80] Turning %s debug mode\n", (m_enableDebug) ? "on" : "off");
	if (m_enableDebug)
	{
		OutputInstruction(m_PC);
		if (GetEnableOutputStatus())
		{
			OutputStatus();
		}
	}
	else
	{
		m_enableUnattendedDebug = false;
	}
}

//=============================================================================

bool CZ80::GetEnableUnattendedDebug(void) const
{
 	return m_enableUnattendedDebug;
}

//=============================================================================

void CZ80::SetEnableUnattendedDebug(bool set)
{
 	m_enableUnattendedDebug = set;
	fprintf(stderr, "[Z80] Turning %s unattended debug mode\n", (m_enableUnattendedDebug) ? "on" : "off");
}

//=============================================================================

bool CZ80::GetEnableOutputStatus(void) const
{
 	return m_enableOutputStatus;
}

//=============================================================================

void CZ80::SetEnableOutputStatus(bool set)
{
 	m_enableOutputStatus = set;
	fprintf(stderr, "[Z80] Turning output status %s\n", (m_enableOutputStatus) ? "on" : "off");
	if (m_enableOutputStatus)
	{
		OutputStatus();
	}
}

//=============================================================================

bool CZ80::GetEnableBreakpoints(void) const
{
	return m_enableBreakpoints;
}

//=============================================================================

void CZ80::SetEnableBreakpoints(bool set)
{
 	m_enableBreakpoints = set;
	fprintf(stderr, "[Z80] Enable breakpoints %s\n", (m_enableBreakpoints) ? "on" : "off");
	if (!m_enableBreakpoints && GetEnableProgramFlowBreakpoints())
	{
		SetEnableProgramFlowBreakpoints(false);
	}
}

//=============================================================================

bool CZ80::GetEnableProgramFlowBreakpoints(void) const
{
	return m_enableProgramFlowBreakpoints;
}

//=============================================================================

void CZ80::SetEnableProgramFlowBreakpoints(bool set)
{
 	m_enableProgramFlowBreakpoints = set;
	fprintf(stderr, "[Z80] Enable program flow breakpoints %s\n", (m_enableProgramFlowBreakpoints) ? "on" : "off");
}

//=============================================================================

void CZ80::OutputStatus(void)
{
	fprintf(stdout, "--------\n");
#if defined(LEE_COMPATIBLE)
	fprintf(stdout, "FLAGS = S  Z  -  H  -  P  N  C\n");
	fprintf(stdout, "        %i  %i  %i  %i  %i  %i  %i  %i\n", (m_F & eF_S) >> 7, (m_F & eF_Z) >> 6, (m_F & eF_Y) >> 5, (m_F & eF_H) >> 4, (m_F & eF_X) >> 3, (m_F & eF_PV) >> 2, (m_F & eF_N) >> 1, (m_F & eF_C));
	fprintf(stdout, "AF= %04X\n", m_AF);
	fprintf(stdout, "BC= %04X\n", m_BC);
	fprintf(stdout, "DE= %04X\n", m_DE);
	fprintf(stdout, "HL= %04X\n", m_HL);
	fprintf(stdout, "IX= %04X\n", m_IX);
	fprintf(stdout, "IY= %04X\n", m_IY);
	fprintf(stdout, "SP= %04X\n", m_SP);
#else
	fprintf(stdout, "[Z80]        Registers:\n");
	fprintf(stdout, "[Z80]        AF'=%04X  AF=%04X  A=%02X  F=%02X  S:%i Z:%i Y:%i H:%i X:%i PV:%i N:%i C:%i \n", m_AFalt, m_AF, m_A, m_F, (m_F & eF_S) >> 7, (m_F & eF_Z) >> 6, (m_F & eF_Y) >> 5, (m_F & eF_H) >> 4, (m_F & eF_X) >> 3, (m_F & eF_PV) >> 2, (m_F & eF_N) >> 1, (m_F & eF_C));
	fprintf(stdout, "[Z80]        BC'=%04X  BC=%04X  B=%02X  C=%02X  (%02X %02X %02X %02X %02X %02X %02X %02X)\n", m_BCalt, m_BC, m_B, m_C, m_pMemory[m_BC], m_pMemory[m_BC + 1], m_pMemory[m_BC + 2], m_pMemory[m_BC + 3], m_pMemory[m_BC + 4], m_pMemory[m_BC + 5], m_pMemory[m_BC + 6], m_pMemory[m_BC + 7]);
	fprintf(stdout, "[Z80]        DE'=%04X  DE=%04X  D=%02X  E=%02X  (%02X %02X %02X %02X %02X %02X %02X %02X)\n", m_DEalt, m_DE, m_D, m_E, m_pMemory[m_DE], m_pMemory[m_DE + 1], m_pMemory[m_DE + 2], m_pMemory[m_DE + 3], m_pMemory[m_DE + 4], m_pMemory[m_DE + 5], m_pMemory[m_DE + 6], m_pMemory[m_DE + 7]);
	fprintf(stdout, "[Z80]        HL'=%04X  HL=%04X  H=%02X  L=%02X  (%02X %02X %02X %02X %02X %02X %02X %02X)\n", m_HLalt, m_HL, m_H, m_L, m_pMemory[m_HL], m_pMemory[m_HL + 1], m_pMemory[m_HL + 2], m_pMemory[m_HL + 3], m_pMemory[m_HL + 4], m_pMemory[m_HL + 5], m_pMemory[m_HL + 6], m_pMemory[m_HL + 7]);
	fprintf(stdout, "[Z80]        IX=%04X  (%02X %02X %02X %02X %02X %02X %02X *%02X* %02X %02X %02X %02X %02X %02X %02X)\n", m_IX, m_pMemory[m_IX - 7], m_pMemory[m_IX - 6], m_pMemory[m_IX - 5], m_pMemory[m_IX - 4], m_pMemory[m_IX - 3], m_pMemory[m_IX - 2], m_pMemory[m_IX - 1], m_pMemory[m_IX], m_pMemory[m_IX + 1], m_pMemory[m_IX + 2], m_pMemory[m_IX + 3], m_pMemory[m_IX + 4], m_pMemory[m_IX + 5], m_pMemory[m_IX + 6], m_pMemory[m_IX + 7]);
	fprintf(stdout, "[Z80]        IY=%04X  (%02X %02X %02X %02X %02X %02X %02X *%02X* %02X %02X %02X %02X %02X %02X %02X)\n", m_IY, m_pMemory[m_IY - 7], m_pMemory[m_IY - 6], m_pMemory[m_IY - 5], m_pMemory[m_IY - 4], m_pMemory[m_IY - 3], m_pMemory[m_IY - 2], m_pMemory[m_IY - 1], m_pMemory[m_IY], m_pMemory[m_IY + 1], m_pMemory[m_IY + 2], m_pMemory[m_IY + 3], m_pMemory[m_IY + 4], m_pMemory[m_IY + 5], m_pMemory[m_IY + 6], m_pMemory[m_IY + 7]);
	fprintf(stdout, "[Z80]        I=%02X  R=%02X  IM=%i  IFF1=%i  IFF2=%i\n", m_I, m_R, m_State.m_InterruptMode, m_State.m_IFF1, m_State.m_IFF2);
	fprintf(stdout, "[Z80]        SP=%04X (%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X)\n", m_SP, m_pMemory[m_SP], m_pMemory[m_SP + 1], m_pMemory[m_SP + 2], m_pMemory[m_SP + 3], m_pMemory[m_SP + 4], m_pMemory[m_SP + 5], m_pMemory[m_SP + 6], m_pMemory[m_SP + 7], m_pMemory[m_SP + 8], m_pMemory[m_SP + 9], m_pMemory[m_SP + 10], m_pMemory[m_SP + 11], m_pMemory[m_SP + 12], m_pMemory[m_SP + 13], m_pMemory[m_SP + 14], m_pMemory[m_SP + 15]);
	fprintf(stdout, "[Z80]        PC=%04X (%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X)\n", m_PC, m_pMemory[m_PC], m_pMemory[m_PC + 1], m_pMemory[m_PC + 2], m_pMemory[m_PC + 3], m_pMemory[m_PC + 4], m_pMemory[m_PC + 5], m_pMemory[m_PC + 6], m_pMemory[m_PC + 7], m_pMemory[m_PC + 8], m_pMemory[m_PC + 9], m_pMemory[m_PC + 10], m_pMemory[m_PC + 11], m_pMemory[m_PC + 12], m_pMemory[m_PC + 13], m_pMemory[m_PC + 14], m_pMemory[m_PC + 15]);
#endif
	fprintf(stdout, "--------\n");
}

//=============================================================================

void CZ80::OutputInstruction(uint16 address)
{
	uint16 decodeAddress = address;
	char buffer[64];
	Decode(decodeAddress, buffer);
#if defined(LEE_COMPATIBLE)
	fprintf(stdout, "%04X :", address);
	switch (decodeAddress - address)
	{
		case 1:
			fprintf(stdout, "%02X                      %s\n", m_pMemory[address], buffer);
			break;

		case 2:
			fprintf(stdout, "%02X %02X                   %s\n", m_pMemory[address], m_pMemory[address + 1], buffer);
			break;

		case 3:
			fprintf(stdout, "%02X %02X %02X                %s\n", m_pMemory[address], m_pMemory[address + 1], m_pMemory[address + 2], buffer);
			break;

		case 4:
			fprintf(stdout, "%02X %02X %02X %02X             %s\n", m_pMemory[address], m_pMemory[address + 1], m_pMemory[address + 2], m_pMemory[address + 3], buffer);
			break;
	}
#else
	fprintf(stdout, "[Z80] %04X : ", address);
	switch (decodeAddress - address)
	{
		case 1:
			fprintf(stdout, "%02X          : %s\n", m_pMemory[address], buffer);
			break;

		case 2:
			fprintf(stdout, "%02X %02X       : %s\n", m_pMemory[address], m_pMemory[address + 1], buffer);
			break;

		case 3:
			fprintf(stdout, "%02X %02X %02X    : %s\n", m_pMemory[address], m_pMemory[address + 1], m_pMemory[address + 2], buffer);
			break;

		case 4:
			fprintf(stdout, "%02X %02X %02X %02X : %s\n", m_pMemory[address], m_pMemory[address + 1], m_pMemory[address + 2], m_pMemory[address + 3], buffer);
			break;
	}
#endif
}

//=============================================================================

void CZ80::HitBreakpoint(const char* type)
{
	if (GetEnableBreakpoints())
	{
		fprintf(stderr, "[Z80] Hit %s breakpoint at address %04X\n", type, m_PC);
		if (GetEnableUnattendedDebug())
		{
			fprintf(stdout, "[Z80] Hit %s breakpoint at address %04X\n", type, m_PC);
		}
		OutputInstruction(m_PC);
		if (GetEnableProgramFlowBreakpoints())
		{
			m_enableUnattendedDebug = false;
		}
		m_enableDebug = true;
	}
}

//=============================================================================

void CZ80::HandleIllegalOpcode(void)
{
	OutputStatus();
	exit(EXIT_FAILURE);
}

//=============================================================================

uint32 CZ80::Step(void)
{
		switch (m_pMemory[m_PC])
		{
			case 0x00:
				return ImplementNOP();
				break;

			case 0x10:
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementDJNZe();
				break;

			case 0x20:
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementJRNZe();
				break;

			case 0x30:
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementJRNCe();
				break;

			case 0x01: // LD BC,nn
			case 0x11: // LD DE,nn
			case 0x21: // LD HL,nn
			case 0x31: // LD SP,nn
				return ImplementLDddnn();
				break;

			case 0x02:
				return ImplementLD_BC_A();
				break;

			case 0x12:
				return ImplementLD_DE_A();
				break;

			case 0x22:
				return ImplementLD_nn_HL();
				break;

			case 0x32:
				return ImplementLD_nn_A();
				break;

			case 0x03: // INC BC
			case 0x13: // INC DE
			case 0x23: // INC HL
			case 0x33: // INC SP
				return ImplementINCdd();
				break;

			case 0x04: // INC B
			case 0x14: // INC D
			case 0x24: // INC H
			case 0x0C: // INC C
			case 0x1C: // INC E
			case 0x2C: // INC L
			case 0x3C: // INC A
				return ImplementINCr();
				break;

			case 0x34:
				return ImplementINC_HL_();
				break;

			case 0x05: // DEC B
			case 0x15: // DEC D
			case 0x25: // DEC H
			case 0x0D: // DEC C
			case 0x1D: // DEC E
			case 0x2D: // DEC L
			case 0x3D: // DEC A
				return ImplementDECr();
				break;

			case 0x35:
				return ImplementDEC_HL_();
				break;

			case 0x06: // LD B,n
			case 0x16: // LD D,n
			case 0x26: // LD H,n
			case 0x0E: // LD C,n
			case 0x1E: // LD E,n
			case 0x2E: // LD L,n
			case 0x3E: // LD A,n
				return ImplementLDrn();
				break;

			case 0x36:
				return ImplementLD_HL_n();
				break;

			case 0x07:
				return ImplementRLCA();
				break;

			case 0x17:
				return ImplementRLA();
				break;

			case 0x27:
				return ImplementDAA();
				break;

			case 0x37:
				return ImplementSCF();
				break;

			case 0x08:
				return ImplementEXAFAF();
				break;

			case 0x18:
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementJRe();
				break;

			case 0x28:
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementJRZe();
				break;

			case 0x38:
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementJRCe();
				break;

			case 0x09: // ADD HL,BC
			case 0x19: // ADD HL,DE
			case 0x29: // ADD HL,HL
			case 0x39: // ADD HL,SP
				return ImplementADDHLdd();
				break;

			case 0x0A:
				return ImplementLDA_BC_();
				break;

			case 0x1A:
				return ImplementLDA_DE_();
				break;

			case 0x2A:
				return ImplementLDHL_nn_();
				break;

			case 0x3A:
				return ImplementLDA_nn_();
				break;

			case 0x0B: // DEC BC
			case 0x1B: // DEC DE
			case 0x2B: // DEC HL
			case 0x3B: // DEC SP
				return ImplementDECdd();
				break;

			case 0x0F:
				return ImplementRRCA();
				break;

			case 0x1F:
				return ImplementRRA();
				break;

			case 0x2F:
				return ImplementCPL();
				break;

			case 0x3F:
				return ImplementCCF();
				break;

			case 0x40: // LD B,B
			case 0x41: // LD B,C
			case 0x42: // LD B,D
			case 0x43: // LD B,E
			case 0x44: // LD B,H
			case 0x45: // LD B,L
			case 0x47: // LD B,A
			case 0x50: // LD D,B
			case 0x51: // LD D,C
			case 0x52: // LD D,D
			case 0x53: // LD D,E
			case 0x54: // LD D,H
			case 0x55: // LD D,L
			case 0x57: // LD D,A
			case 0x60: // LD H,B
			case 0x61: // LD H,C
			case 0x62: // LD H,D
			case 0x63: // LD H,E
			case 0x64: // LD H,H
			case 0x65: // LD H,L
			case 0x67: // LD H,A
			case 0x48: // LD C,B
			case 0x49: // LD C,C
			case 0x4A: // LD C,D
			case 0x4B: // LD C,E
			case 0x4C: // LD C,H
			case 0x4D: // LD C,L
			case 0x4F: // LD C,A
			case 0x58: // LD E,B
			case 0x59: // LD E,C
			case 0x5A: // LD E,D
			case 0x5B: // LD E,E
			case 0x5C: // LD E,H
			case 0x5D: // LD E,L
			case 0x5F: // LD E,A
			case 0x68: // LD L,B
			case 0x69: // LD L,C
			case 0x6A: // LD L,D
			case 0x6B: // LD L,E
			case 0x6C: // LD L,H
			case 0x6D: // LD L,L
			case 0x6F: // LD L,A
			case 0x78: // LD A,B
			case 0x79: // LD A,C
			case 0x7A: // LD A,D
			case 0x7B: // LD A,E
			case 0x7C: // LD A,H
			case 0x7D: // LD A,L
			case 0x7F: // LD A,A
				return ImplementLDrr();
				break;

			case 0x46: // LD B,(HL)
			case 0x56: // LD D,(HL)
			case 0x66: // LD H,(HL)
			case 0x4E: // LD C,(HL)
			case 0x5E: // LD E,(HL)
			case 0x6E: // LD L,(HL)
			case 0x7E: // LD A,(HL)
				return ImplementLDr_HL_();
				break;

			case 0x70: // LD (HL),B
			case 0x71: // LD (HL),C
			case 0x72: // LD (HL),D
			case 0x73: // LD (HL),E
			case 0x74: // LD (HL),H
			case 0x75: // LD (HL),L
			case 0x77: // LD (HL),A
				return ImplementLD_HL_r();
				break;

			case 0x76:
				return ImplementHALT();
				break;

			case 0x80: // ADD A,B
			case 0x81: // ADD A,C
			case 0x82: // ADD A,D
			case 0x83: // ADD A,E
			case 0x84: // ADD A,H
			case 0x85: // ADD A,L
			case 0x87: // ADD A,A
				return ImplementADDAr();
				break;

			case 0x86:
				return ImplementADDA_HL_();
				break;

			case 0x90: // SUB A,B
			case 0x91: // SUB A,C
			case 0x92: // SUB A,D
			case 0x93: // SUB A,E
			case 0x94: // SUB A,H
			case 0x95: // SUB A,L
			case 0x97: // SUB A,A
				return ImplementSUBr();
				break;

			case 0x96:
				return ImplementSUB_HL_();
				break;

			case 0xA0: // AND A,B
			case 0xA1: // AND A,C
			case 0xA2: // AND A,D
			case 0xA3: // AND A,E
			case 0xA4: // AND A,H
			case 0xA5: // AND A,L
			case 0xA7: // AND A,A
				return ImplementANDr();
				break;

			case 0xA6:
				return ImplementAND_HL_();
				break;

			case 0xB0: // OR A,B
			case 0xB1: // OR A,C
			case 0xB2: // OR A,D
			case 0xB3: // OR A,E
			case 0xB4: // OR A,H
			case 0xB5: // OR A,L
			case 0xB7: // OR A,A
				return ImplementORr();
				break;

			case 0xB6:
				return ImplementOR_HL_();
				break;

			case 0x88: // ADC A,B
			case 0x89: // ADC A,C
			case 0x8A: // ADC A,D
			case 0x8B: // ADC A,E
			case 0x8C: // ADC A,H
			case 0x8D: // ADC A,L
			case 0x8F: // ADC A,A
				return ImplementADCAr();
				break;

			case 0x8E:
				return ImplementADCA_HL_();
				break;

			case 0x98: // SBC A,B
			case 0x99: // SBC A,C
			case 0x9A: // SBC A,D
			case 0x9B: // SBC A,E
			case 0x9C: // SBC A,H
			case 0x9D: // SBC A,L
			case 0x9F: // SBC A,A
				return ImplementSBCAr();
				break;

			case 0x9E:
				return ImplementSBCA_HL_();
				break;

			case 0xA8: // XOR A,B
			case 0xA9: // XOR A,C
			case 0xAA: // XOR A,D
			case 0xAB: // XOR A,E
			case 0xAC: // XOR A,H
			case 0xAD: // XOR A,L
			case 0xAF: // XOR A,A
				return ImplementXORr();
				break;

			case 0xAE:
				return ImplementXOR_HL_();
				break;

			case 0xB8: // CP A,B
			case 0xB9: // CP A,C
			case 0xBA: // CP A,D
			case 0xBB: // CP A,E
			case 0xBC: // CP A,H
			case 0xBD: // CP A,L
			case 0xBF: // CP A,A
				return ImplementCPr();
				break;

			case 0xBE:
				return ImplementCP_HL_();
				break;

			case 0xC0: // RET NZ
			case 0xD0: // RET NC
			case 0xE0: // RET PO
			case 0xF0: // RET P
			case 0xC8: // RET Z
			case 0xD8: // RET C
			case 0xE8: // RET PE
			case 0xF8: // RET M
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementRETcc();
				break;

			case 0xC1: // POP BC
			case 0xD1: // POP DE
			case 0xE1: // POP HL
				return ImplementPOPqq();
				break;

			case 0xF1: // POP AF
				return ImplementPOPAF();
				break;

			case 0xC2: // JP NZ,nn
			case 0xD2: // JP NC,nn
			case 0xE2: // JP PO,nn
			case 0xF2: // JP P,nn
			case 0xCA: // JP Z,nn
			case 0xDA: // JP C,nn
			case 0xEA: // JP PE,nn
			case 0xFA: // JP M,nn
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementJPccnn();
				break;

			case 0xC3:
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementJPnn();
				break;

			case 0xD3:
				return ImplementOUT_n_A();
				break;

			case 0xE3:
				return ImplementEX_SP_HL();
				break;

			case 0xF3:
				return ImplementDI();
				break;

			case 0xC4: // CALL NZ,nn
			case 0xD4: // CALL NC,nn
			case 0xE4: // CALL PO,nn
			case 0xF4: // CALL P,nn
			case 0xCC: // CALL Z,nn
			case 0xDC: // CALL C,nn
			case 0xEC: // CALL PE,nn
			case 0xFC: // CALL M,nn
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementCALLccnn();
				break;

			case 0xC5: // PUSH BC
			case 0xD5: // PUSH DE
			case 0xE5: // PUSH HL
				return ImplementPUSHqq();
				break;

			case 0xF5: // PUSH AF
				return ImplementPUSHAF();
				break;

			case 0xC6:
				return ImplementADDAn();
				break;

			case 0xD6:
				return ImplementSUBn();
				break;

			case 0xE6:
				return ImplementANDn();
				break;

			case 0xF6:
				return ImplementORn();
				break;

			case 0xC7: // RST 0
			case 0xD7: // RST 16
			case 0xE7: // RST 32
			case 0xF7: // RST 48
			case 0xCF: // RST 8
			case 0xDF: // RST 24
			case 0xEF: // RST 40
			case 0xFF: // RST 56
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementRSTp();
				break;

			case 0xC9:
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementRET();
				break;

			case 0xD9:
				return ImplementEXX();
				break;

			case 0xE9:
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementJP_HL_();
				break;

			case 0xF9:
				return ImplementLDSPHL();
				break;

			case 0xCB: // Prefix CB
				switch (m_pMemory[m_PC + 1])
				{
					case 0x00: // RLC B
					case 0x01: // RLC C
					case 0x02: // RLC D
					case 0x03: // RLC E
					case 0x04: // RLC H
					case 0x05: // RLC L
					case 0x07: // RLC A
						return ImplementRLCr();
						break;

					case 0x06:
						return ImplementRLC_HL_();
						break;

					case 0x08: // RRC B
					case 0x09: // RRC C
					case 0x0A: // RRC D
					case 0x0B: // RRC E
					case 0x0C: // RRC H
					case 0x0D: // RRC L
					case 0x0F: // RRC A
						return ImplementRRCr();
						break;

					case 0x0E:
						return ImplementRRC_HL_();
						break;

					case 0x10: // RL B
					case 0x11: // RL C
					case 0x12: // RL D
					case 0x13: // RL E
					case 0x14: // RL H
					case 0x15: // RL L
					case 0x17: // RL A
						return ImplementRLr();
						break;

					case 0x16:
						return ImplementRL_HL_();
						break;

					case 0x18: // RR B
					case 0x19: // RR C
					case 0x1A: // RR D
					case 0x1B: // RR E
					case 0x1C: // RR H
					case 0x1D: // RR L
					case 0x1F: // RR A
						return ImplementRRr();
						break;

					case 0x1E:
						return ImplementRR_HL_();
						break;

					case 0x20: // SLA B
					case 0x21: // SLA C
					case 0x22: // SLA D
					case 0x23: // SLA E
					case 0x24: // SLA H
					case 0x25: // SLA L
					case 0x27: // SLA A
						return ImplementSLAr();
						break;

					case 0x26:
						return ImplementSLA_HL_();
						break;

					case 0x28: // SRA B
					case 0x29: // SRA C
					case 0x2A: // SRA D
					case 0x2B: // SRA E
					case 0x2C: // SRA H
					case 0x2D: // SRA L
					case 0x2F: // SRA A
						return ImplementSRAr();
						break;

					case 0x2E:
						return ImplementSRA_HL_();
						break;

					case 0x38: // SRL B
					case 0x39: // SRL C
					case 0x3A: // SRL D
					case 0x3B: // SRL E
					case 0x3C: // SRL H
					case 0x3D: // SRL L
					case 0x3F: // SRL A
						return ImplementSRLr();
						break;

					case 0x3E:
						return ImplementSRL_HL_();
						break;

					case 0x40: // BIT 0,B
					case 0x41: // BIT 0,C
					case 0x42: // BIT 0,D
					case 0x43: // BIT 0,E
					case 0x44: // BIT 0,H
					case 0x45: // BIT 0,L
					case 0x47: // BIT 0,A
					case 0x48: // BIT 1,B
					case 0x49: // BIT 1,C
					case 0x4A: // BIT 1,D
					case 0x4B: // BIT 1,E
					case 0x4C: // BIT 1,H
					case 0x4D: // BIT 1,L
					case 0x4F: // BIT 1,A
					case 0x50: // BIT 2,B
					case 0x51: // BIT 2,C
					case 0x52: // BIT 2,D
					case 0x53: // BIT 2,E
					case 0x54: // BIT 2,H
					case 0x55: // BIT 2,L
					case 0x57: // BIT 2,A
					case 0x58: // BIT 3,B
					case 0x59: // BIT 3,C
					case 0x5A: // BIT 3,D
					case 0x5B: // BIT 3,E
					case 0x5C: // BIT 3,H
					case 0x5D: // BIT 3,L
					case 0x5F: // BIT 3,A
					case 0x60: // BIT 4,B
					case 0x61: // BIT 4,C
					case 0x62: // BIT 4,D
					case 0x63: // BIT 4,E
					case 0x64: // BIT 4,H
					case 0x65: // BIT 4,L
					case 0x67: // BIT 4,A
					case 0x68: // BIT 5,B
					case 0x69: // BIT 5,C
					case 0x6A: // BIT 5,D
					case 0x6B: // BIT 5,E
					case 0x6C: // BIT 5,H
					case 0x6D: // BIT 5,L
					case 0x6F: // BIT 5,A
					case 0x70: // BIT 6,B
					case 0x71: // BIT 6,C
					case 0x72: // BIT 6,D
					case 0x73: // BIT 6,E
					case 0x74: // BIT 6,H
					case 0x75: // BIT 6,L
					case 0x77: // BIT 6,A
					case 0x78: // BIT 7,B
					case 0x79: // BIT 7,C
					case 0x7A: // BIT 7,D
					case 0x7B: // BIT 7,E
					case 0x7C: // BIT 7,H
					case 0x7D: // BIT 7,L
					case 0x7F: // BIT 7,A
						return ImplementBITbr();
						break;

					case 0x46: // BIT 0,(HL)
					case 0x4E: // BIT 1,(HL)
					case 0x56: // BIT 2,(HL)
					case 0x5E: // BIT 3,(HL)
					case 0x66: // BIT 4,(HL)
					case 0x6E: // BIT 5,(HL)
					case 0x76: // BIT 6,(HL)
					case 0x7E: // BIT 7,(HL)
						return ImplementBITb_HL_();
						break;

					case 0x80: // RES 0,B
					case 0x81: // RES 0,C
					case 0x82: // RES 0,D
					case 0x83: // RES 0,E
					case 0x84: // RES 0,H
					case 0x85: // RES 0,L
					case 0x87: // RES 0,A
					case 0x88: // RES 1,B
					case 0x89: // RES 1,C
					case 0x8A: // RES 1,D
					case 0x8B: // RES 1,E
					case 0x8C: // RES 1,H
					case 0x8D: // RES 1,L
					case 0x8F: // RES 1,A
					case 0x90: // RES 2,B
					case 0x91: // RES 2,C
					case 0x92: // RES 2,D
					case 0x93: // RES 2,E
					case 0x94: // RES 2,H
					case 0x95: // RES 2,L
					case 0x97: // RES 2,A
					case 0x98: // RES 3,B
					case 0x99: // RES 3,C
					case 0x9A: // RES 3,D
					case 0x9B: // RES 3,E
					case 0x9C: // RES 3,H
					case 0x9D: // RES 3,L
					case 0x9F: // RES 3,A
					case 0xA0: // RES 4,B
					case 0xA1: // RES 4,C
					case 0xA2: // RES 4,D
					case 0xA3: // RES 4,E
					case 0xA4: // RES 4,H
					case 0xA5: // RES 4,L
					case 0xA7: // RES 4,A
					case 0xA8: // RES 5,B
					case 0xA9: // RES 5,C
					case 0xAA: // RES 5,D
					case 0xAB: // RES 5,E
					case 0xAC: // RES 5,H
					case 0xAD: // RES 5,L
					case 0xAF: // RES 5,A
					case 0xB0: // RES 6,B
					case 0xB1: // RES 6,C
					case 0xB2: // RES 6,D
					case 0xB3: // RES 6,E
					case 0xB4: // RES 6,H
					case 0xB5: // RES 6,L
					case 0xB7: // RES 6,A
					case 0xB8: // RES 7,B
					case 0xB9: // RES 7,C
					case 0xBA: // RES 7,D
					case 0xBB: // RES 7,E
					case 0xBC: // RES 7,H
					case 0xBD: // RES 7,L
					case 0xBF: // RES 7,A
						return ImplementRESbr();
						break;

					case 0x86: // RES 0,(HL)
					case 0x8E: // RES 1,(HL)
					case 0x96: // RES 2,(HL)
					case 0x9E: // RES 3,(HL)
					case 0xA6: // RES 4,(HL)
					case 0xAE: // RES 5,(HL)
					case 0xB6: // RES 6,(HL)
					case 0xBE: // RES 7,(HL)
						return ImplementRESb_HL_();
						break;

					case 0xC0: // SET 0,B
					case 0xC1: // SET 0,C
					case 0xC2: // SET 0,D
					case 0xC3: // SET 0,E
					case 0xC4: // SET 0,H
					case 0xC5: // SET 0,L
					case 0xC7: // SET 0,A
					case 0xC8: // SET 1,B
					case 0xC9: // SET 1,C
					case 0xCA: // SET 1,D
					case 0xCB: // SET 1,E
					case 0xCC: // SET 1,H
					case 0xCD: // SET 1,L
					case 0xCF: // SET 1,A
					case 0xD0: // SET 2,B
					case 0xD1: // SET 2,C
					case 0xD2: // SET 2,D
					case 0xD3: // SET 2,E
					case 0xD4: // SET 2,H
					case 0xD5: // SET 2,L
					case 0xD7: // SET 2,A
					case 0xD8: // SET 3,B
					case 0xD9: // SET 3,C
					case 0xDA: // SET 3,D
					case 0xDB: // SET 3,E
					case 0xDC: // SET 3,H
					case 0xDD: // SET 3,L
					case 0xDF: // SET 3,A
					case 0xE0: // SET 4,B
					case 0xE1: // SET 4,C
					case 0xE2: // SET 4,D
					case 0xE3: // SET 4,E
					case 0xE4: // SET 4,H
					case 0xE5: // SET 4,L
					case 0xE7: // SET 4,A
					case 0xE8: // SET 5,B
					case 0xE9: // SET 5,C
					case 0xEA: // SET 5,D
					case 0xEB: // SET 5,E
					case 0xEC: // SET 5,H
					case 0xED: // SET 5,L
					case 0xEF: // SET 5,A
					case 0xF0: // SET 6,B
					case 0xF1: // SET 6,C
					case 0xF2: // SET 6,D
					case 0xF3: // SET 6,E
					case 0xF4: // SET 6,H
					case 0xF5: // SET 6,L
					case 0xF7: // SET 6,A
					case 0xF8: // SET 7,B
					case 0xF9: // SET 7,C
					case 0xFA: // SET 7,D
					case 0xFB: // SET 7,E
					case 0xFC: // SET 7,H
					case 0xFD: // SET 7,L
					case 0xFF: // SET 7,A
						return ImplementSETbr();
						break;

					case 0xC6: // SET 0,(HL)
					case 0xCE: // SET 1,(HL)
					case 0xD6: // SET 2,(HL)
					case 0xDE: // SET 3,(HL)
					case 0xE6: // SET 4,(HL)
					case 0xEE: // SET 5,(HL)
					case 0xF6: // SET 6,(HL)
					case 0xFE: // SET 7,(HL)
						return ImplementSETb_HL_();
						break;

				default:
					fprintf(stderr, "[Z80] Unhandled opcode CB %02X at address %04X\n", m_pMemory[m_PC + 1], m_PC);
					HandleIllegalOpcode();
					return 0;
					break;
				};
				break;

			case 0xDB:
				return ImplementINA_n_();
				break;

			case 0xEB:
				return ImplementEXDEHL();
				break;

			case 0xFB:
				return ImplementEI();
				break;

			case 0xCD:
				if (GetEnableProgramFlowBreakpoints())
				{
					HitBreakpoint("program flow");
				}
				return ImplementCALLnn();
				break;

			case 0xDD: // Prefix DD
				switch (m_pMemory[m_PC + 1])
				{
					case 0x46: // LD B,(IX+d)
					case 0x56: // LD D,(IX+d)
					case 0x66: // LD H,(IX+d)
					case 0x4E: // LD C,(IX+d)
					case 0x5E: // LD E,(IX+d)
					case 0x6E: // LD L,(IX+d)
					case 0x7E: // LD A,(IX+d)
						return ImplementLDr_IXd_();
						break;

					case 0x70: // LD (IX+d),B
					case 0x71: // LD (IX+d),C
					case 0x72: // LD (IX+d),D
					case 0x73: // LD (IX+d),E
					case 0x74: // LD (IX+d),H
					case 0x75: // LD (IX+d),L
					case 0x77: // LD (IX+d),A
						return ImplementLD_IXd_r();
						break;

					case 0x09: // ADD IX,BC
					case 0x19: // ADD IX,DE
					case 0x29: // ADD IX,HL
					case 0x39: // ADD IX,SP
						return ImplementADDIXdd();
						break;

					case 0x23:
						return ImplementINCIX();
						break;

					case 0x2B:
						return ImplementDECIX();
						break;

					case 0x34:
						return ImplementINC_IXd_();
						break;

					case 0x35:
						return ImplementDEC_IXd_();
						break;

					case 0x36:
						return ImplementLD_IXd_n();
						break;

					case 0xE9:
						if (GetEnableProgramFlowBreakpoints())
						{
							HitBreakpoint("program flow");
						}
						return ImplementJP_IX_();
						break;

					case 0x21:
						return ImplementLDIXnn();
						break;

					case 0x2A:
						return ImplementLDIX_nn_();
						break;

					case 0xF9:
						return ImplementLDSPIX();
						break;

					case 0xE5:
						return ImplementPUSHIX();
						break;

					case 0xE1:
						return ImplementPOPIX();
						break;

					case 0xE3:
						return ImplementEX_SP_IX();
						break;

					case 0x86:
						return ImplementADDA_IXd_();
						break;

					case 0x96:
						return ImplementSUB_IXd_();

					case 0xA6:
						return ImplementAND_IXd_();
						break;

					case 0xB6:
						return ImplementOR_IXd_();
						break;

					case 0x8E:
						return ImplementADCA_IXd_();
						break;

					case 0x9E:
						return ImplementSBCA_IXd_();
						break;

					case 0xAE:
						return ImplementXOR_IXd_();
						break;

					case 0xBE:
						return ImplementCP_IXd_();
						break;

					case 0xCB: // Prefix CB
						switch (m_pMemory[m_PC + 3])
						{
							case 0x06:
								return ImplementRLC_IXd_();
								break;

							case 0x0E:
								return ImplementRRC_IXd_();
								break;

							case 0x16:
								return ImplementRL_IXd_();
								break;

							case 0x1E:
								return ImplementRR_IXd_();
								break;

							case 0x26:
								return ImplementSLA_IXd_();
								break;

							case 0x2E:
								return ImplementSRA_IXd_();
								break;

							case 0x3E:
								return ImplementSRL_IXd_();
								break;

							case 0x46: // BIT 0,(IX+d)
							case 0x4E: // BIT 1,(IX+d)
							case 0x56: // BIT 2,(IX+d)
							case 0x5E: // BIT 3,(IX+d)
							case 0x66: // BIT 4,(IX+d)
							case 0x6E: // BIT 5,(IX+d)
							case 0x76: // BIT 6,(IX+d)
							case 0x7E: // BIT 7,(IX+d)
								return ImplementBITb_IXd_();
								break;

							case 0x86: // RES 0,(IX+d)
							case 0x8E: // RES 1,(IX+d)
							case 0x96: // RES 2,(IX+d)
							case 0x9E: // RES 3,(IX+d)
							case 0xA6: // RES 4,(IX+d)
							case 0xAE: // RES 5,(IX+d)
							case 0xB6: // RES 6,(IX+d)
							case 0xBE: // RES 7,(IX+d)
								return ImplementRESb_IXd_();
								break;

							case 0xC6: // SET 0,(IX+d)
							case 0xCE: // SET 1,(IX+d)
							case 0xD6: // SET 2,(IX+d)
							case 0xDE: // SET 3,(IX+d)
							case 0xE6: // SET 4,(IX+d)
							case 0xEE: // SET 5,(IX+d)
							case 0xF6: // SET 6,(IX+d)
							case 0xFE: // SET 7,(IX+d)
								return ImplementSETb_IXd_();
								break;

							default:
								fprintf(stderr, "[Z80] Unhandled opcode DD CB %02X %02X at address %04X\n", m_pMemory[m_PC + 2], m_pMemory[m_PC + 3], m_PC);
								HandleIllegalOpcode();
								return 0;
								break;
						};
						break;

				default:
					fprintf(stderr, "[Z80] Unhandled opcode DD %02X at address %04X\n", m_pMemory[m_PC + 1], m_PC);
					HandleIllegalOpcode();
					return 0;
					break;
				};
				break;

			case 0xED: // Prefix ED
				switch (m_pMemory[m_PC + 1])
				{
					case 0x40: // IN B,(C)
					case 0x50: // IN D,(C)
					case 0x60: // IN H,(C)
					case 0x48: // IN C,(C)
					case 0x58: // IN E,(C)
					case 0x68: // IN L,(C)
					case 0x78: // IN A,(C)
						return ImplementINr_C_();
						break;

					case 0x41: // OUT (C),B
					case 0x51: // OUT (C),D
					case 0x61: // OUT (C),H
					case 0x49: // OUT (C),C
					case 0x59: // OUT (C),E
					case 0x69: // OUT (C),L
					case 0x79: // OUT (C),A
						return ImplementOUT_C_r();
						break;

					case 0x42: // SBC HL,BC
					case 0x52: // SBC HL,DE
					case 0x62: // SBC HL,HL
					case 0x72: // SBC HL,SP
						return ImplementSBCHLdd();
						break;

					case 0x43: // LD (nn),BC
					case 0x53: // LD (nn),DE
					case 0x73: // LD (nn),SP
						return ImplementLD_nn_dd();
						break;

					case 0x44:
						return ImplementNEG();
						break;

					case 0x45:
						if (GetEnableProgramFlowBreakpoints())
						{
							HitBreakpoint("program flow");
						}
						return ImplementRETN();
						break;

					case 0x46:
						return ImplementIM0();
						break;

					case 0x56:
						return ImplementIM1();
						break;

					case 0x47:
						return ImplementLDIA();
						break;

					case 0x57:
						return ImplementLDAI();
						break;

					case 0x67:
						return ImplementRRD();
						break;

					case 0x4A: // ADC HL,BC
					case 0x5A: // ADC HL,DE
					case 0x6A: // ADC HL,HL
					case 0x7A: // ADC HL,SP
						return ImplementADCHLdd();
						break;

					case 0x4B: // LD BC,(nn)
					case 0x5B: // LD DE,(nn)
					case 0x7B: // LD SP,(nn)
						return ImplementLDdd_nn_();
						break;

					case 0x4D:
						if (GetEnableProgramFlowBreakpoints())
						{
							HitBreakpoint("program flow");
						}
						return ImplementRETI();
						break;

					case 0x5E:
						return ImplementIM2();
						break;

					case 0x4F:
						return ImplementLDRA();
						break;

					case 0x5F:
						return ImplementLDAR();
						break;

					case 0x6F:
						return ImplementRLD();
						break;

					case 0xA0:
						return ImplementLDI();
						break;

					case 0xA8:
						return ImplementLDD();
						break;

					case 0xB0:
						return ImplementLDIR();
						break;

					case 0xB8:
						return ImplementLDDR();
						break;

					case 0xA1:
						return ImplementCPI();
						break;

					case 0xA9:
						return ImplementCPD();
						break;

					case 0xB1:
						return ImplementCPIR();
						break;

					case 0xB9:
						return ImplementCPDR();
						break;

					case 0xA2:
						return ImplementINI();
						break;

					case 0xAA:
						return ImplementIND();
						break;

					case 0xB2:
						return ImplementINIR();
						break;

					case 0xBA:
						return ImplementINDR();
						break;

					case 0xA3:
						return ImplementOUTI();
						break;

					case 0xAB:
						return ImplementOUTD();
						break;

					case 0xB3:
						return ImplementOTIR();
						break;

					case 0xBB:
						return ImplementOTDR();
						break;

				default:
					fprintf(stderr, "[Z80] Unhandled opcode ED %02X at address %04X\n", m_pMemory[m_PC + 1], m_PC);
					HandleIllegalOpcode();
					return 0;
					break;
				};
				break;

			case 0xFD: // Prefix FD
				switch (m_pMemory[m_PC + 1])
				{
					case 0x46: // LD B,(IY+d)
					case 0x56: // LD D,(IY+d)
					case 0x66: // LD H,(IY+d)
					case 0x4E: // LD C,(IY+d)
					case 0x5E: // LD E,(IY+d)
					case 0x6E: // LD L,(IY+d)
					case 0x7E: // LD A,(IY+d)
						return ImplementLDr_IYd_();
						break;

					case 0x70: // LD (IY+d),B
					case 0x71: // LD (IY+d),C
					case 0x72: // LD (IY+d),D
					case 0x73: // LD (IY+d),E
					case 0x74: // LD (IY+d),H
					case 0x75: // LD (IY+d),L
					case 0x77: // LD (IY+d),A
						return ImplementLD_IYd_r();
						break;

					case 0x09: // ADD IY,BC
					case 0x19: // ADD IY,DE
					case 0x29: // ADD IY,HL
					case 0x39: // ADD IY,SP
						return ImplementADDIYdd();
						break;

					case 0x23:
						return ImplementINCIY();
						break;

					case 0x2B:
						return ImplementDECIY();
						break;

					case 0x34:
						return ImplementINC_IYd_();
						break;

					case 0x35:
						return ImplementDEC_IYd_();
						break;

					case 0x36:
						return ImplementLD_IYd_n();
						break;

					case 0xE9:
						if (GetEnableProgramFlowBreakpoints())
						{
							HitBreakpoint("program flow");
						}
						return ImplementJP_IY_();
						break;

					case 0x21:
						return ImplementLDIYnn();
						break;

					case 0x2A:
						return ImplementLDIY_nn_();
						break;

					case 0xF9:
						return ImplementLDSPIY();
						break;

					case 0xE5:
						return ImplementPUSHIY();
						break;

					case 0xE1:
						return ImplementPOPIY();
						break;

					case 0xE3:
						return ImplementEX_SP_IY();
						break;

					case 0x86:
						return ImplementADDA_IYd_();
						break;

					case 0x96:
						return ImplementSUB_IYd_();

					case 0xA6:
						return ImplementAND_IYd_();
						break;

					case 0xB6:
						return ImplementOR_IYd_();
						break;

					case 0x8E:
						return ImplementADCA_IYd_();
						break;

					case 0x9E:
						return ImplementSBCA_IYd_();
						break;

					case 0xAE:
						return ImplementXOR_IYd_();
						break;

					case 0xBE:
						return ImplementCP_IYd_();
						break;

					case 0xCB: // Prefix CB
						switch (m_pMemory[m_PC + 3])
						{
							case 0x06:
								return ImplementRLC_IYd_();
								break;

							case 0x0E:
								return ImplementRRC_IYd_();
								break;

							case 0x16:
								return ImplementRL_IYd_();
								break;

							case 0x1E:
								return ImplementRR_IYd_();
								break;

							case 0x26:
								return ImplementSLA_IYd_();
								break;

							case 0x2E:
								return ImplementSRA_IYd_();
								break;

							case 0x3E:
								return ImplementSRL_IYd_();
								break;

							case 0x46: // BIT 0,(IY+d)
							case 0x4E: // BIT 1,(IY+d)
							case 0x56: // BIT 2,(IY+d)
							case 0x5E: // BIT 3,(IY+d)
							case 0x66: // BIT 4,(IY+d)
							case 0x6E: // BIT 5,(IY+d)
							case 0x76: // BIT 6,(IY+d)
							case 0x7E: // BIT 7,(IY+d)
								return ImplementBITb_IYd_();
								break;

							case 0x86: // RES 0,(IY+d)
							case 0x8E: // RES 1,(IY+d)
							case 0x96: // RES 2,(IY+d)
							case 0x9E: // RES 3,(IY+d)
							case 0xA6: // RES 4,(IY+d)
							case 0xAE: // RES 5,(IY+d)
							case 0xB6: // RES 6,(IY+d)
							case 0xBE: // RES 7,(IY+d)
								return ImplementRESb_IYd_();
								break;

							case 0xC6: // SET 0,(IY+d)
							case 0xCE: // SET 1,(IY+d)
							case 0xD6: // SET 2,(IY+d)
							case 0xDE: // SET 3,(IY+d)
							case 0xE6: // SET 4,(IY+d)
							case 0xEE: // SET 5,(IY+d)
							case 0xF6: // SET 6,(IY+d)
							case 0xFE: // SET 7,(IY+d)
								return ImplementSETb_IYd_();
								break;

							default:
								fprintf(stderr, "[Z80] Unhandled opcode FD CB %02X %02X at address %04X\n", m_pMemory[m_PC + 2], m_pMemory[m_PC + 3], m_PC);
								HandleIllegalOpcode();
								return 0;
								break;
						};
						break;

				default:
					fprintf(stderr, "[Z80] Unhandled opcode FD %02X at address %04X\n", m_pMemory[m_PC + 1], m_PC);
					HandleIllegalOpcode();
					return 0;
					break;
				};
				break;

			case 0xCE:
				return ImplementADCAn();
				break;

			case 0xDE:
				return ImplementSBCAn();
				break;

			case 0xEE:
				return ImplementXORn();
				break;

			case 0xFE:
				return ImplementCPn();
				break;

			default:
				fprintf(stderr, "[Z80] Unhandled opcode %02X at address %04X\n", m_pMemory[m_PC], m_PC);
				HandleIllegalOpcode();
				return 0;
				break;
		}
}

//=============================================================================

void CZ80::Decode(uint16& address, char* pMnemonic)
{
		switch (m_pMemory[address])
		{
			case 0x00:
				DecodeNOP(address, pMnemonic);
				break;

			case 0x10:
				DecodeDJNZe(address, pMnemonic);
				break;

			case 0x20:
				DecodeJRNZe(address, pMnemonic);
				break;

			case 0x30:
				DecodeJRNCe(address, pMnemonic);
				break;

			case 0x01: // LD BC,nn
			case 0x11: // LD DE,nn
			case 0x21: // LD HL,nn
			case 0x31: // LD SP,nn
				DecodeLDddnn(address, pMnemonic);
				break;

			case 0x02:
				DecodeLD_BC_A(address, pMnemonic);
				break;

			case 0x12:
				DecodeLD_DE_A(address, pMnemonic);
				break;

			case 0x22:
				DecodeLD_nn_HL(address, pMnemonic);
				break;

			case 0x32:
				DecodeLD_nn_A(address, pMnemonic);
				break;

			case 0x03: // INC BC
			case 0x13: // INC DE
			case 0x23: // INC HL
			case 0x33: // INC SP
				DecodeINCdd(address, pMnemonic);
				break;

			case 0x04: // INC B
			case 0x14: // INC D
			case 0x24: // INC H
			case 0x34: // INC (HL)
			case 0x0C: // INC C
			case 0x1C: // INC E
			case 0x2C: // INC L
			case 0x3C: // INC A
				DecodeINCr(address, pMnemonic);
				break;

			case 0x05: // DEC B
			case 0x15: // DEC D
			case 0x25: // DEC H
			case 0x35: // DEC (HL)
			case 0x0D: // DEC C
			case 0x1D: // DEC E
			case 0x2D: // DEC L
			case 0x3D: // DEC A
				DecodeDECr(address, pMnemonic);
				break;

			case 0x06: // LD B,n
			case 0x16: // LD D,n
			case 0x26: // LD H,n
			case 0x0E: // LD C,n
			case 0x1E: // LD E,n
			case 0x2E: // LD L,n
			case 0x3E: // LD A,n
				DecodeLDrn(address, pMnemonic);
				break;

			case 0x36:
				DecodeLD_HL_n(address, pMnemonic);
				break;

			case 0x07:
				DecodeRLCA(address, pMnemonic);
				break;

			case 0x17:
				DecodeRLA(address, pMnemonic);
				break;

			case 0x27:
				DecodeDAA(address, pMnemonic);
				break;

			case 0x37:
				DecodeSCF(address, pMnemonic);
				break;

			case 0x08:
				DecodeEXAFAF(address, pMnemonic);
				break;

			case 0x18:
				DecodeJRe(address, pMnemonic);
				break;

			case 0x28:
				DecodeJRZe(address, pMnemonic);
				break;

			case 0x38:
				DecodeJRCe(address, pMnemonic);
				break;

			case 0x09: // ADD HL,BC
			case 0x19: // ADD HL,DE
			case 0x29: // ADD HL,HL
			case 0x39: // ADD HL,SP
				DecodeADDHLdd(address, pMnemonic);
				break;

			case 0x0A:
				DecodeLDA_BC_(address, pMnemonic);
				break;

			case 0x1A:
				DecodeLDA_DE_(address, pMnemonic);
				break;

			case 0x2A:
				DecodeLDHL_nn_(address, pMnemonic);
				break;

			case 0x3A:
				DecodeLDA_nn_(address, pMnemonic);
				break;

			case 0x0B: // DEC BC
			case 0x1B: // DEC DE
			case 0x2B: // DEC HL
			case 0x3B: // DEC SP
				DecodeDECdd(address, pMnemonic);
				break;

			case 0x0F:
				DecodeRRCA(address, pMnemonic);
				break;

			case 0x1F:
				DecodeRRA(address, pMnemonic);
				break;

			case 0x2F:
				DecodeCPL(address, pMnemonic);
				break;

			case 0x3F:
				DecodeCCF(address, pMnemonic);
				break;

			case 0x40: // LD B,B
			case 0x41: // LD B,C
			case 0x42: // LD B,D
			case 0x43: // LD B,E
			case 0x44: // LD B,H
			case 0x45: // LD B,L
			case 0x46: // LD B,(HL)
			case 0x47: // LD B,A
			case 0x50: // LD D,B
			case 0x51: // LD D,C
			case 0x52: // LD D,D
			case 0x53: // LD D,E
			case 0x54: // LD D,H
			case 0x55: // LD D,L
			case 0x56: // LD D,(HL)
			case 0x57: // LD D,A
			case 0x60: // LD H,B
			case 0x61: // LD H,C
			case 0x62: // LD H,D
			case 0x63: // LD H,E
			case 0x64: // LD H,H
			case 0x65: // LD H,L
			case 0x66: // LD H,(HL)
			case 0x67: // LD H,A
			case 0x48: // LD C,B
			case 0x49: // LD C,C
			case 0x4A: // LD C,D
			case 0x4B: // LD C,E
			case 0x4C: // LD C,H
			case 0x4D: // LD C,L
			case 0x4E: // LD C,(HL)
			case 0x4F: // LD C,A
			case 0x58: // LD E,B
			case 0x59: // LD E,C
			case 0x5A: // LD E,D
			case 0x5B: // LD E,E
			case 0x5C: // LD E,H
			case 0x5D: // LD E,L
			case 0x5E: // LD E,(HL)
			case 0x5F: // LD E,A
			case 0x68: // LD L,B
			case 0x69: // LD L,C
			case 0x6A: // LD L,D
			case 0x6B: // LD L,E
			case 0x6C: // LD L,H
			case 0x6D: // LD L,L
			case 0x6E: // LD L,(HL)
			case 0x6F: // LD L,A
			case 0x70: // LD (HL),B
			case 0x71: // LD (HL),C
			case 0x72: // LD (HL),D
			case 0x73: // LD (HL),E
			case 0x74: // LD (HL),H
			case 0x75: // LD (HL),L
			case 0x77: // LD (HL),A
			case 0x78: // LD A,B
			case 0x79: // LD A,C
			case 0x7A: // LD A,D
			case 0x7B: // LD A,E
			case 0x7C: // LD A,H
			case 0x7D: // LD A,L
			case 0x7F: // LD A,A
			case 0x7E: // LD A,(HL)
				DecodeLDrr(address, pMnemonic);
				break;

			case 0x76:
				DecodeHALT(address, pMnemonic);
				break;

			case 0x80: // ADD A,B
			case 0x81: // ADD A,C
			case 0x82: // ADD A,D
			case 0x83: // ADD A,E
			case 0x84: // ADD A,H
			case 0x85: // ADD A,L
			case 0x86: // ADD A,(HL)
			case 0x87: // ADD A,A
				DecodeADDAr(address, pMnemonic);
				break;

			case 0x90: // SUB B
			case 0x91: // SUB C
			case 0x92: // SUB D
			case 0x93: // SUB E
			case 0x94: // SUB H
			case 0x95: // SUB L
			case 0x96: // SUB (HL)
			case 0x97: // SUB A
				DecodeSUBr(address, pMnemonic);
				break;

			case 0xA0: // AND B
			case 0xA1: // AND C
			case 0xA2: // AND D
			case 0xA3: // AND E
			case 0xA4: // AND H
			case 0xA5: // AND L
			case 0xA6: // AND (HL)
			case 0xA7: // AND A
				DecodeANDr(address, pMnemonic);
				break;

			case 0xB0: // OR B
			case 0xB1: // OR C
			case 0xB2: // OR D
			case 0xB3: // OR E
			case 0xB4: // OR H
			case 0xB5: // OR L
			case 0xB6: // OR (HL)
			case 0xB7: // OR A
				DecodeORr(address, pMnemonic);
				break;

			case 0x88: // ADC A,B
			case 0x89: // ADC A,C
			case 0x8A: // ADC A,D
			case 0x8B: // ADC A,E
			case 0x8C: // ADC A,H
			case 0x8D: // ADC A,L
			case 0x8E: // ADC A,(HL)
			case 0x8F: // ADC A,A
				DecodeADCAr(address, pMnemonic);
				break;

			case 0x98: // SBC A,B
			case 0x99: // SBC A,C
			case 0x9A: // SBC A,D
			case 0x9B: // SBC A,E
			case 0x9C: // SBC A,H
			case 0x9D: // SBC A,L
			case 0x9E: // SBC A,(HL)
			case 0x9F: // SBC A,A
				DecodeSBCAr(address, pMnemonic);
				break;

			case 0xA8: // XOR B
			case 0xA9: // XOR C
			case 0xAA: // XOR D
			case 0xAB: // XOR E
			case 0xAC: // XOR H
			case 0xAD: // XOR L
			case 0xAE: // XOR (HL)
			case 0xAF: // XOR A
				DecodeXORr(address, pMnemonic);
				break;

			case 0xB8: // CP B
			case 0xB9: // CP C
			case 0xBA: // CP D
			case 0xBB: // CP E
			case 0xBC: // CP H
			case 0xBD: // CP L
			case 0xBE: // CP (HL)
			case 0xBF: // CP A
				DecodeCPr(address, pMnemonic);
				break;

			case 0xC0: // RET NZ
			case 0xD0: // RET NC
			case 0xE0: // RET PO
			case 0xF0: // RET P
			case 0xC8: // RET Z
			case 0xD8: // RET C
			case 0xE8: // RET PE
			case 0xF8: // RET M
				DecodeRETcc(address, pMnemonic);
				break;

			case 0xC1: // POP BC
			case 0xD1: // POP DE
			case 0xE1: // POP HL
				DecodePOPqq(address, pMnemonic);
				break;

			case 0xF1: // POP AF
				DecodePOPAF(address, pMnemonic);
				break;

			case 0xC2: // JP NZ,nn
			case 0xD2: // JP NC,nn
			case 0xE2: // JP PO,nn
			case 0xF2: // JP P,nn
			case 0xCA: // JP Z,nn
			case 0xDA: // JP C,nn
			case 0xEA: // JP PE,nn
			case 0xFA: // JP M,nn
				DecodeJPccnn(address, pMnemonic);
				break;

			case 0xC3:
				DecodeJPnn(address, pMnemonic);
				break;

			case 0xD3:
				DecodeOUT_n_A(address, pMnemonic);
				break;

			case 0xE3:
				DecodeEX_SP_HL(address, pMnemonic);
				break;

			case 0xF3:
				DecodeDI(address, pMnemonic);
				break;

			case 0xC4: // CALL NZ,nn
			case 0xD4: // CALL NC,nn
			case 0xE4: // CALL PO,nn
			case 0xF4: // CALL P,nn
			case 0xCC: // CALL Z,nn
			case 0xDC: // CALL C,nn
			case 0xEC: // CALL PE,nn
			case 0xFC: // CALL M,nn
				DecodeCALLccnn(address, pMnemonic);
				break;

			case 0xC5: // PUSH BC
			case 0xD5: // PUSH DE
			case 0xE5: // PUSH HL
				DecodePUSHqq(address, pMnemonic);
				break;

			case 0xF5: // PUSH AF
				DecodePUSHAF(address, pMnemonic);
				break;

			case 0xC6:
				DecodeADDAn(address, pMnemonic);
				break;

			case 0xD6:
				DecodeSUBn(address, pMnemonic);
				break;

			case 0xE6:
				DecodeANDn(address, pMnemonic);
				break;

			case 0xF6:
				DecodeORn(address, pMnemonic);
				break;

			case 0xC7: // RST 0
			case 0xD7: // RST 16
			case 0xE7: // RST 32
			case 0xF7: // RST 48
			case 0xCF: // RST 8
			case 0xDF: // RST 24
			case 0xEF: // RST 40
			case 0xFF: // RST 56
				DecodeRSTp(address, pMnemonic);
				break;

			case 0xC9:
				DecodeRET(address, pMnemonic);
				break;

			case 0xD9:
				DecodeEXX(address, pMnemonic);
				break;

			case 0xE9:
				DecodeJP_HL_(address, pMnemonic);
				break;

			case 0xF9:
				DecodeLDSPHL(address, pMnemonic);
				break;

			case 0xCB: // Prefix CB
				switch (m_pMemory[address + 1])
				{
					case 0x00: // RLC B
					case 0x01: // RLC C
					case 0x02: // RLC D
					case 0x03: // RLC E
					case 0x04: // RLC H
					case 0x05: // RLC L
					case 0x06: // RLC (HL)
					case 0x07: // RLC A
						DecodeRLCr(address, pMnemonic);
						break;

					case 0x08: // RRC B
					case 0x09: // RRC C
					case 0x0A: // RRC D
					case 0x0B: // RRC E
					case 0x0C: // RRC H
					case 0x0D: // RRC L
					case 0x0E: // RRC (HL)
					case 0x0F: // RRC A
						DecodeRRCr(address, pMnemonic);
						break;

					case 0x10: // RL B
					case 0x11: // RL C
					case 0x12: // RL D
					case 0x13: // RL E
					case 0x14: // RL H
					case 0x15: // RL L
					case 0x16: // RL (HL)
					case 0x17: // RL A
						DecodeRLr(address, pMnemonic);
						break;

					case 0x18: // RR B
					case 0x19: // RR C
					case 0x1A: // RR D
					case 0x1B: // RR E
					case 0x1C: // RR H
					case 0x1D: // RR L
					case 0x1E: // RR (HL)
					case 0x1F: // RR A
						DecodeRRr(address, pMnemonic);
						break;

					case 0x20: // SLA B
					case 0x21: // SLA C
					case 0x22: // SLA D
					case 0x23: // SLA E
					case 0x24: // SLA H
					case 0x25: // SLA L
					case 0x26: // SLA (HL)
					case 0x27: // SLA A
						DecodeSLAr(address, pMnemonic);
						break;

					case 0x28: // SRA B
					case 0x29: // SRA C
					case 0x2A: // SRA D
					case 0x2B: // SRA E
					case 0x2C: // SRA H
					case 0x2D: // SRA L
					case 0x2E: // SRA (HL)
					case 0x2F: // SRA A
						DecodeSRAr(address, pMnemonic);
						break;

					case 0x38: // SRL B
					case 0x39: // SRL C
					case 0x3A: // SRL D
					case 0x3B: // SRL E
					case 0x3C: // SRL H
					case 0x3D: // SRL L
					case 0x3E: // SRL (HL)
					case 0x3F: // SRL A
						DecodeSRLr(address, pMnemonic);
						break;

					case 0x40: // BIT 0,B
					case 0x41: // BIT 0,C
					case 0x42: // BIT 0,D
					case 0x43: // BIT 0,E
					case 0x44: // BIT 0,H
					case 0x45: // BIT 0,L
					case 0x47: // BIT 0,A
					case 0x46: // BIT 0,(HL)
					case 0x48: // BIT 1,B
					case 0x49: // BIT 1,C
					case 0x4A: // BIT 1,D
					case 0x4B: // BIT 1,E
					case 0x4C: // BIT 1,H
					case 0x4D: // BIT 1,L
					case 0x4E: // BIT 1,(HL)
					case 0x4F: // BIT 1,A
					case 0x50: // BIT 2,B
					case 0x51: // BIT 2,C
					case 0x52: // BIT 2,D
					case 0x53: // BIT 2,E
					case 0x54: // BIT 2,H
					case 0x55: // BIT 2,L
					case 0x56: // BIT 2,(HL)
					case 0x57: // BIT 2,A
					case 0x58: // BIT 3,B
					case 0x59: // BIT 3,C
					case 0x5A: // BIT 3,D
					case 0x5B: // BIT 3,E
					case 0x5C: // BIT 3,H
					case 0x5D: // BIT 3,L
					case 0x5E: // BIT 3,(HL)
					case 0x5F: // BIT 3,A
					case 0x60: // BIT 4,B
					case 0x61: // BIT 4,C
					case 0x62: // BIT 4,D
					case 0x63: // BIT 4,E
					case 0x64: // BIT 4,H
					case 0x65: // BIT 4,L
					case 0x66: // BIT 4,(HL)
					case 0x67: // BIT 4,A
					case 0x68: // BIT 5,B
					case 0x69: // BIT 5,C
					case 0x6A: // BIT 5,D
					case 0x6B: // BIT 5,E
					case 0x6C: // BIT 5,H
					case 0x6D: // BIT 5,L
					case 0x6E: // BIT 5,(HL)
					case 0x6F: // BIT 5,A
					case 0x70: // BIT 6,B
					case 0x71: // BIT 6,C
					case 0x72: // BIT 6,D
					case 0x73: // BIT 6,E
					case 0x74: // BIT 6,H
					case 0x75: // BIT 6,L
					case 0x76: // BIT 6,(HL)
					case 0x77: // BIT 6,A
					case 0x78: // BIT 7,B
					case 0x79: // BIT 7,C
					case 0x7A: // BIT 7,D
					case 0x7B: // BIT 7,E
					case 0x7C: // BIT 7,H
					case 0x7D: // BIT 7,L
					case 0x7E: // BIT 7,(HL)
					case 0x7F: // BIT 7,A
						DecodeBITbr(address, pMnemonic);
						break;

					case 0x80: // RES 0,B
					case 0x81: // RES 0,C
					case 0x82: // RES 0,D
					case 0x83: // RES 0,E
					case 0x84: // RES 0,H
					case 0x85: // RES 0,L
					case 0x86: // RES 0,(HL)
					case 0x87: // RES 0,A
					case 0x88: // RES 1,B
					case 0x89: // RES 1,C
					case 0x8A: // RES 1,D
					case 0x8B: // RES 1,E
					case 0x8C: // RES 1,H
					case 0x8D: // RES 1,L
					case 0x8E: // RES 1,(HL)
					case 0x8F: // RES 1,A
					case 0x90: // RES 2,B
					case 0x91: // RES 2,C
					case 0x92: // RES 2,D
					case 0x93: // RES 2,E
					case 0x94: // RES 2,H
					case 0x95: // RES 2,L
					case 0x96: // RES 2,(HL)
					case 0x97: // RES 2,A
					case 0x98: // RES 3,B
					case 0x99: // RES 3,C
					case 0x9A: // RES 3,D
					case 0x9B: // RES 3,E
					case 0x9C: // RES 3,H
					case 0x9D: // RES 3,L
					case 0x9E: // RES 3,(HL)
					case 0x9F: // RES 3,A
					case 0xA0: // RES 4,B
					case 0xA1: // RES 4,C
					case 0xA2: // RES 4,D
					case 0xA3: // RES 4,E
					case 0xA4: // RES 4,H
					case 0xA5: // RES 4,L
					case 0xA6: // RES 4,(HL)
					case 0xA7: // RES 4,A
					case 0xA8: // RES 5,B
					case 0xA9: // RES 5,C
					case 0xAA: // RES 5,D
					case 0xAB: // RES 5,E
					case 0xAC: // RES 5,H
					case 0xAD: // RES 5,L
					case 0xAE: // RES 5,(HL)
					case 0xAF: // RES 5,A
					case 0xB0: // RES 6,B
					case 0xB1: // RES 6,C
					case 0xB2: // RES 6,D
					case 0xB3: // RES 6,E
					case 0xB4: // RES 6,H
					case 0xB5: // RES 6,L
					case 0xB6: // RES 6,(HL)
					case 0xB7: // RES 6,A
					case 0xB8: // RES 7,B
					case 0xB9: // RES 7,C
					case 0xBA: // RES 7,D
					case 0xBB: // RES 7,E
					case 0xBC: // RES 7,H
					case 0xBD: // RES 7,L
					case 0xBE: // RES 7,(HL)
					case 0xBF: // RES 7,A
						DecodeRESbr(address, pMnemonic);
						break;

					case 0xC0: // SET 0,B
					case 0xC1: // SET 0,C
					case 0xC2: // SET 0,D
					case 0xC3: // SET 0,E
					case 0xC4: // SET 0,H
					case 0xC5: // SET 0,L
					case 0xC6: // SET 0,(HL)
					case 0xC7: // SET 0,A
					case 0xC8: // SET 1,B
					case 0xC9: // SET 1,C
					case 0xCA: // SET 1,D
					case 0xCB: // SET 1,E
					case 0xCC: // SET 1,H
					case 0xCD: // SET 1,L
					case 0xCE: // SET 1,(HL)
					case 0xCF: // SET 1,A
					case 0xD0: // SET 2,B
					case 0xD1: // SET 2,C
					case 0xD2: // SET 2,D
					case 0xD3: // SET 2,E
					case 0xD4: // SET 2,H
					case 0xD5: // SET 2,L
					case 0xD6: // SET 2,(HL)
					case 0xD7: // SET 2,A
					case 0xD8: // SET 3,B
					case 0xD9: // SET 3,C
					case 0xDA: // SET 3,D
					case 0xDB: // SET 3,E
					case 0xDC: // SET 3,H
					case 0xDD: // SET 3,L
					case 0xDE: // SET 3,(HL)
					case 0xDF: // SET 3,A
					case 0xE0: // SET 4,B
					case 0xE1: // SET 4,C
					case 0xE2: // SET 4,D
					case 0xE3: // SET 4,E
					case 0xE4: // SET 4,H
					case 0xE6: // SET 4,(HL)
					case 0xE5: // SET 4,L
					case 0xE7: // SET 4,A
					case 0xE8: // SET 5,B
					case 0xE9: // SET 5,C
					case 0xEA: // SET 5,D
					case 0xEB: // SET 5,E
					case 0xEC: // SET 5,H
					case 0xED: // SET 5,L
					case 0xEE: // SET 5,(HL)
					case 0xEF: // SET 5,A
					case 0xF0: // SET 6,B
					case 0xF1: // SET 6,C
					case 0xF2: // SET 6,D
					case 0xF3: // SET 6,E
					case 0xF4: // SET 6,H
					case 0xF5: // SET 6,L
					case 0xF7: // SET 6,A
					case 0xF6: // SET 6,(HL)
					case 0xF8: // SET 7,B
					case 0xF9: // SET 7,C
					case 0xFA: // SET 7,D
					case 0xFB: // SET 7,E
					case 0xFC: // SET 7,H
					case 0xFD: // SET 7,L
					case 0xFE: // SET 7,(HL)
					case 0xFF: // SET 7,A
						DecodeSETbr(address, pMnemonic);
						break;

				default:
					fprintf(stderr, "[Z80] Error decoding unhandled opcode CB %02X at address %04X\n", m_pMemory[address + 1], address);
					HandleIllegalOpcode();
					return;
					break;
				};
				break;

			case 0xDB:
				DecodeINA_n_(address, pMnemonic);
				break;

			case 0xEB:
				DecodeEXDEHL(address, pMnemonic);
				break;

			case 0xFB:
				DecodeEI(address, pMnemonic);
				break;

			case 0xCD:
				DecodeCALLnn(address, pMnemonic);
				break;

			case 0xDD: // Prefix DD
				switch (m_pMemory[address + 1])
				{
					case 0x46: // LD B,(IX+d)
					case 0x56: // LD D,(IX+d)
					case 0x66: // LD H,(IX+d)
					case 0x4E: // LD C,(IX+d)
					case 0x5E: // LD E,(IX+d)
					case 0x6E: // LD L,(IX+d)
					case 0x7E: // LD A,(IX+d)
						DecodeLDr_IXd_(address, pMnemonic);
						break;

					case 0x70: // LD (IX+d),B
					case 0x71: // LD (IX+d),C
					case 0x72: // LD (IX+d),D
					case 0x73: // LD (IX+d),E
					case 0x74: // LD (IX+d),H
					case 0x75: // LD (IX+d),L
					case 0x77: // LD (IX+d),A
						DecodeLD_IXd_r(address, pMnemonic);
						break;

					case 0x09: // ADD IX,BC
					case 0x19: // ADD IX,DE
					case 0x29: // ADD IX,HL
					case 0x39: // ADD IX,SP
						DecodeADDIXdd(address, pMnemonic);
						break;

					case 0x23:
						DecodeINCIX(address, pMnemonic);
						break;

					case 0x2B:
						DecodeDECIX(address, pMnemonic);
						break;

					case 0x34:
						DecodeINC_IXd_(address, pMnemonic);
						break;

					case 0x35:
						DecodeDEC_IXd_(address, pMnemonic);
						break;

					case 0x36:
						DecodeLD_IXd_n(address, pMnemonic);
						break;

					case 0xE9:
						DecodeJP_IX_(address, pMnemonic);
						break;

					case 0x21:
						DecodeLDIXnn(address, pMnemonic);
						break;

					case 0x2A:
						DecodeLDIX_nn_(address, pMnemonic);
						break;

					case 0xF9:
						DecodeLDSPIX(address, pMnemonic);
						break;

					case 0xE5:
						DecodePUSHIX(address, pMnemonic);
						break;

					case 0xE1:
						DecodePOPIX(address, pMnemonic);
						break;

					case 0xE3:
						DecodeEX_SP_IX(address, pMnemonic);
						break;

					case 0x86:
						DecodeADDA_IXd_(address, pMnemonic);
						break;

					case 0x96:
						DecodeSUB_IXd_(address, pMnemonic);

					case 0xA6:
						DecodeAND_IXd_(address, pMnemonic);
						break;

					case 0xB6:
						DecodeOR_IXd_(address, pMnemonic);
						break;

					case 0x8E:
						DecodeADCA_IXd_(address, pMnemonic);
						break;

					case 0x9E:
						DecodeSBCA_IXd_(address, pMnemonic);
						break;

					case 0xAE:
						DecodeXOR_IXd_(address, pMnemonic);
						break;

					case 0xBE:
						DecodeCP_IXd_(address, pMnemonic);
						break;

					case 0xCB: // Prefix CB
						switch (m_pMemory[address + 3])
						{
							case 0x06:
								DecodeRLC_IXd_(address, pMnemonic);
								break;

							case 0x0E:
								DecodeRRC_IXd_(address, pMnemonic);
								break;

							case 0x16:
								DecodeRL_IXd_(address, pMnemonic);
								break;

							case 0x1E:
								DecodeRR_IXd_(address, pMnemonic);
								break;

							case 0x26:
								DecodeSLA_IXd_(address, pMnemonic);
								break;

							case 0x2E:
								DecodeSRA_IXd_(address, pMnemonic);
								break;

							case 0x3E:
								DecodeSRL_IXd_(address, pMnemonic);
								break;

							case 0x46: // BIT 0,(IX+d)
							case 0x4E: // BIT 1,(IX+d)
							case 0x56: // BIT 2,(IX+d)
							case 0x5E: // BIT 3,(IX+d)
							case 0x66: // BIT 4,(IX+d)
							case 0x6E: // BIT 5,(IX+d)
							case 0x76: // BIT 6,(IX+d)
							case 0x7E: // BIT 7,(IX+d)
								DecodeBITb_IXd_(address, pMnemonic);
								break;

							case 0x86: // RES 0,(IX+d)
							case 0x8E: // RES 1,(IX+d)
							case 0x96: // RES 2,(IX+d)
							case 0x9E: // RES 3,(IX+d)
							case 0xA6: // RES 4,(IX+d)
							case 0xAE: // RES 5,(IX+d)
							case 0xB6: // RES 6,(IX+d)
							case 0xBE: // RES 7,(IX+d)
								DecodeRESb_IXd_(address, pMnemonic);
								break;

							case 0xC6: // SET 0,(IX+d)
							case 0xCE: // SET 1,(IX+d)
							case 0xD6: // SET 2,(IX+d)
							case 0xDE: // SET 3,(IX+d)
							case 0xE6: // SET 4,(IX+d)
							case 0xEE: // SET 5,(IX+d)
							case 0xF6: // SET 6,(IX+d)
							case 0xFE: // SET 7,(IX+d)
								DecodeSETb_IXd_(address, pMnemonic);
								break;

							default:
								fprintf(stderr, "[Z80] Error decoding unhandled opcode DD CB %02X %02X at address %04X\n", m_pMemory[address + 2], m_pMemory[address + 3], address);
								HandleIllegalOpcode();
								return;
								break;
						};
						break;

				default:
					fprintf(stderr, "[Z80] Error decoding unhandled opcode DD %02X at address %04X\n", m_pMemory[address + 1], address);
					HandleIllegalOpcode();
					return;
					break;
				};
				break;

			case 0xED: // Prefix ED
				switch (m_pMemory[address + 1])
				{
					case 0x40: // IN B,(C)
					case 0x50: // IN D,(C)
					case 0x60: // IN H,(C)
					case 0x48: // IN C,(C)
					case 0x58: // IN E,(C)
					case 0x68: // IN L,(C)
					case 0x78: // IN A,(C)
						DecodeINr_C_(address, pMnemonic);
						break;

					case 0x41: // OUT (C),B
					case 0x51: // OUT (C),D
					case 0x61: // OUT (C),H
					case 0x49: // OUT (C),C
					case 0x59: // OUT (C),E
					case 0x69: // OUT (C),L
					case 0x79: // OUT (C),A
						DecodeOUT_C_r(address, pMnemonic);
						break;

					case 0x42: // SBC HL,BC
					case 0x52: // SBC HL,DE
					case 0x62: // SBC HL,HL
					case 0x72: // SBC HL,SP
						DecodeSBCHLdd(address, pMnemonic);
						break;

					case 0x43: // LD (nn),BC
					case 0x53: // LD (nn),DE
					case 0x73: // LD (nn),SP
						DecodeLD_nn_dd(address, pMnemonic);
						break;

					case 0x44:
						DecodeNEG(address, pMnemonic);
						break;

					case 0x45:
						DecodeRETN(address, pMnemonic);
						break;

					case 0x46:
						DecodeIM0(address, pMnemonic);
						break;

					case 0x56:
						DecodeIM1(address, pMnemonic);
						break;

					case 0x47:
						DecodeLDIA(address, pMnemonic);
						break;

					case 0x57:
						DecodeLDAI(address, pMnemonic);
						break;

					case 0x67:
						DecodeRRD(address, pMnemonic);
						break;

					case 0x4A: // ADC HL,BC
					case 0x5A: // ADC HL,DE
					case 0x6A: // ADC HL,HL
					case 0x7A: // ADC HL,SP
						DecodeADCHLdd(address, pMnemonic);
						break;

					case 0x4B: // LD BC,(nn)
					case 0x5B: // LD DE,(nn)
					case 0x7B: // LD SP,(nn)
						DecodeLDdd_nn_(address, pMnemonic);
						break;

					case 0x4D:
						DecodeRETI(address, pMnemonic);
						break;

					case 0x5E:
						DecodeIM2(address, pMnemonic);
						break;

					case 0x4F:
						DecodeLDRA(address, pMnemonic);
						break;

					case 0x5F:
						DecodeLDAR(address, pMnemonic);
						break;

					case 0x6F:
						(address, pMnemonic);
						break;

					case 0xA0:
						DecodeLDI(address, pMnemonic);
						break;

					case 0xA8:
						DecodeLDD(address, pMnemonic);
						break;

					case 0xB0:
						DecodeLDIR(address, pMnemonic);
						break;

					case 0xB8:
						DecodeLDDR(address, pMnemonic);
						break;

					case 0xA1:
						DecodeCPI(address, pMnemonic);
						break;

					case 0xA9:
						DecodeCPD(address, pMnemonic);
						break;

					case 0xB1:
						DecodeCPIR(address, pMnemonic);
						break;

					case 0xB9:
						DecodeCPDR(address, pMnemonic);
						break;

					case 0xA2:
						DecodeINI(address, pMnemonic);
						break;

					case 0xAA:
						DecodeIND(address, pMnemonic);
						break;

					case 0xB2:
						DecodeINIR(address, pMnemonic);
						break;

					case 0xBA:
						DecodeINDR(address, pMnemonic);
						break;

					case 0xA3:
						DecodeOUTI(address, pMnemonic);
						break;

					case 0xAB:
						DecodeOUTD(address, pMnemonic);
						break;

					case 0xB3:
						DecodeOTIR(address, pMnemonic);
						break;

					case 0xBB:
						DecodeOTDR(address, pMnemonic);
						break;

				default:
					fprintf(stderr, "[Z80] Error decoding unhandled opcode ED %02X at address %04X\n", m_pMemory[address + 1], address);
					HandleIllegalOpcode();
					return;
					break;
				};
				break;

			case 0xFD: // Prefix FD
				switch (m_pMemory[address + 1])
				{
					case 0x46: // LD B,(IY+d)
					case 0x56: // LD D,(IY+d)
					case 0x66: // LD H,(IY+d)
					case 0x4E: // LD C,(IY+d)
					case 0x5E: // LD E,(IY+d)
					case 0x6E: // LD L,(IY+d)
					case 0x7E: // LD A,(IY+d)
						DecodeLDr_IYd_(address, pMnemonic);
						break;

					case 0x70: // LD (IY+d),B
					case 0x71: // LD (IY+d),C
					case 0x72: // LD (IY+d),D
					case 0x73: // LD (IY+d),E
					case 0x74: // LD (IY+d),H
					case 0x75: // LD (IY+d),L
					case 0x77: // LD (IY+d),A
						DecodeLD_IYd_r(address, pMnemonic);
						break;

					case 0x09: // ADD IY,BC
					case 0x19: // ADD IY,DE
					case 0x29: // ADD IY,HL
					case 0x39: // ADD IY,SP
						DecodeADDIYdd(address, pMnemonic);
						break;

					case 0x23:
						DecodeINCIY(address, pMnemonic);
						break;

					case 0x2B:
						DecodeDECIY(address, pMnemonic);
						break;

					case 0x34:
						DecodeINC_IYd_(address, pMnemonic);
						break;

					case 0x35:
						DecodeDEC_IYd_(address, pMnemonic);
						break;

					case 0x36:
						DecodeLD_IYd_n(address, pMnemonic);
						break;

					case 0xE9:
						DecodeJP_IY_(address, pMnemonic);
						break;

					case 0x21:
						DecodeLDIYnn(address, pMnemonic);
						break;

					case 0x2A:
						DecodeLDIY_nn_(address, pMnemonic);
						break;

					case 0xF9:
						DecodeLDSPIY(address, pMnemonic);
						break;

					case 0xE5:
						DecodePUSHIY(address, pMnemonic);
						break;

					case 0xE1:
						DecodePOPIY(address, pMnemonic);
						break;

					case 0xE3:
						DecodeEX_SP_IY(address, pMnemonic);
						break;

					case 0x86:
						DecodeADDA_IYd_(address, pMnemonic);
						break;

					case 0x96:
						DecodeSUB_IYd_(address, pMnemonic);
						break;

					case 0xA6:
						DecodeAND_IYd_(address, pMnemonic);
						break;

					case 0xB6:
						DecodeOR_IYd_(address, pMnemonic);
						break;

					case 0x8E:
						DecodeADCA_IYd_(address, pMnemonic);
						break;

					case 0x9E:
						DecodeSBCA_IYd_(address, pMnemonic);
						break;

					case 0xAE:
						DecodeXOR_IYd_(address, pMnemonic);
						break;

					case 0xBE:
						DecodeCP_IYd_(address, pMnemonic);
						break;

					case 0xCB: // Prefix CB
						switch (m_pMemory[address + 3])
						{
							case 0x06:
								DecodeRLC_IYd_(address, pMnemonic);
								break;

							case 0x0E:
								DecodeRRC_IYd_(address, pMnemonic);
								break;

							case 0x16:
								DecodeRL_IYd_(address, pMnemonic);
								break;

							case 0x1E:
								DecodeRR_IYd_(address, pMnemonic);
								break;

							case 0x26:
								DecodeSLA_IYd_(address, pMnemonic);
								break;

							case 0x2E:
								DecodeSRA_IYd_(address, pMnemonic);
								break;

							case 0x3E:
								DecodeSRL_IYd_(address, pMnemonic);
								break;

							case 0x46: // BIT 0,(IY+d)
							case 0x4E: // BIT 1,(IY+d)
							case 0x56: // BIT 2,(IY+d)
							case 0x5E: // BIT 3,(IY+d)
							case 0x66: // BIT 4,(IY+d)
							case 0x6E: // BIT 5,(IY+d)
							case 0x76: // BIT 6,(IY+d)
							case 0x7E: // BIT 7,(IY+d)
								DecodeBITb_IYd_(address, pMnemonic);
								break;

							case 0x86: // RES 0,(IY+d)
							case 0x8E: // RES 1,(IY+d)
							case 0x96: // RES 2,(IY+d)
							case 0x9E: // RES 3,(IY+d)
							case 0xA6: // RES 4,(IY+d)
							case 0xAE: // RES 5,(IY+d)
							case 0xB6: // RES 6,(IY+d)
							case 0xBE: // RES 7,(IY+d)
								DecodeRESb_IYd_(address, pMnemonic);
								break;

							case 0xC6: // SET 0,(IY+d)
							case 0xCE: // SET 1,(IY+d)
							case 0xD6: // SET 2,(IY+d)
							case 0xDE: // SET 3,(IY+d)
							case 0xE6: // SET 4,(IY+d)
							case 0xEE: // SET 5,(IY+d)
							case 0xF6: // SET 6,(IY+d)
							case 0xFE: // SET 7,(IY+d)
								DecodeSETb_IYd_(address, pMnemonic);
								break;

							default:
								fprintf(stderr, "[Z80] Error decoding unhandled opcode FD CB %02X %02X at address %04X\n", m_pMemory[address + 2], m_pMemory[address + 3], address);
								HandleIllegalOpcode();
								return;
								break;
						};
						break;

				default:
					fprintf(stderr, "[Z80] Error decoding unhandled opcode FD %02X at address %04X\n", m_pMemory[address + 1], address);
					HandleIllegalOpcode();
					return;
					break;
				};
				break;

			case 0xCE:
				DecodeADCAn(address, pMnemonic);
				break;

			case 0xDE:
				DecodeSBCAn(address, pMnemonic);
				break;

			case 0xEE:
				DecodeXORn(address, pMnemonic);
				break;

			case 0xFE:
				DecodeCPn(address, pMnemonic);
				break;

			default:
				fprintf(stderr, "[Z80] Error decoding unhandled opcode %02X at address %04X\n", m_pMemory[address], address);
				HandleIllegalOpcode();
				return;
				break;
		}
}

//=============================================================================

uint8 CZ80::HandleArithmeticAddFlags(uint16 source1, uint16 source2)
{
	uint16 result = source1 + source2;
	uint16 half = (source1 & 0x0F) + (source2 & 0x0F);
	uint16 overflow = ((source1 & source2 & ~result) | (~source1 & ~source2 & result)) >> 5;

	m_F = (result & (eF_S | eF_Y | eF_X)) | (((result & 0xFF) == 0) ? eF_Z : 0) | (half & eF_H) | (overflow & eF_PV) | ((result >> 8) & eF_C);

	return (result & 0xFF);
}

//=============================================================================

uint8 CZ80::HandleArithmeticSubtractFlags(uint16 source1, uint16 source2)
{
	uint16 result = source1 - source2;
	uint16 half = (source1 & 0x0F) - (source2 & 0x0F);
	uint16 overflow = ((source1 & ~source2 & ~result) | (~source1 & source2 & result)) >> 5;

	m_F = (result & (eF_S | eF_Y | eF_X)) | (((result & 0xFF) == 0) ? eF_Z : 0) | (half & eF_H) | (overflow & eF_PV) | eF_N | ((result >> 8) & eF_C);

	return (result & 0xFF);
}

//=============================================================================

void CZ80::HandleLogicalFlags(uint8 source)
{
	uint8 parity = source;
	parity ^= parity >> 4;
	parity &= 0xF;
	parity = ((0x6996 >> parity) << 2);
	m_F = (source & (eF_S | eF_Y | eF_X)) | ((source == 0) ? eF_Z : 0) | (~parity & eF_PV);
}

//=============================================================================

uint16 CZ80::Handle16BitArithmeticAddFlags(uint32 source1, uint32 source2)
{
	uint32 result = source1 + source2;
	uint32 half = (source1 & 0x0FFF) + (source2 & 0x0FFF) >> 8;
	uint32 overflow = ((source1 & source2 & ~result) | (~source1 & ~source2 & result)) >> 13;

	m_F = ((result >> 8) & (eF_S | eF_X | eF_Y)) | (((result & 0xFFFF) == 0) ? eF_Z : 0) | (half & eF_H) | (overflow & eF_PV) | ((result >> 16) & eF_C);

	return (result & 0xFFFF);
}

//=============================================================================

uint16 CZ80::Handle16BitArithmeticSubtractFlags(uint32 source1, uint32 source2)
{
	uint32 result = source1 - source2;
	uint32 half = (source1 & 0x0FFF) - (source2 & 0x0FFF) >> 8;
	uint32 overflow = ((source1 & ~source2 & ~result) | (~source1 & source2 & result)) >> 13;

	m_F = ((result >> 8) & (eF_S | eF_X | eF_Y)) | (((result & 0xFFFF) == 0) ? eF_Z : 0) | (half & eF_H) | (overflow & eF_PV) | eF_N | ((result >> 16) & eF_C);

	return (result & 0xFFFF);
}

//=============================================================================

bool CZ80::WriteMemory(uint16 address, uint8 byte)
{
	bool success = true;

	if (address >= 0x4000)
	{
		if (GetEnableBreakpoints() && (address == g_dataBreakpoint))
		{
			fprintf(stderr, "[Z80] writing %02X to %04X\n", byte, address);
			HitBreakpoint("data");
		}

		m_pMemory[address] = byte;
	}
	else
	{
		HitBreakpoint("write to ROM");
		success = false;
	}

	return success;
}

//=============================================================================

bool CZ80::ReadMemory(uint16 address, uint8& byte)
{
	byte = m_pMemory[address];

	if (GetEnableBreakpoints() && (address == g_dataBreakpoint))
	{
		fprintf(stderr, "[Z80] reading %02X from %04X\n", byte, address);
		HitBreakpoint("data");
	}

	return true;
}

//=============================================================================

bool CZ80::WritePort(uint16 address, uint8 byte)
{
	return true;
}

//=============================================================================

bool CZ80::ReadPort(uint16 address, uint8& byte)
{
	uint8 mask = 0;
	switch (address)
	{
		case 0xFEFE: // SHIFT, Z, X, C, V
			byte = 0x1F;
			if (glfwGetKey(GLFW_KEY_LSHIFT) == GLFW_PRESS) mask |= 0x01;
			if (glfwGetKey('Z') == GLFW_PRESS) mask |= 0x02;
			if (glfwGetKey('X') == GLFW_PRESS) mask |= 0x04;
			if (glfwGetKey('C') == GLFW_PRESS) mask |= 0x08;
			if (glfwGetKey('V') == GLFW_PRESS) mask |= 0x10;
			byte &= ~mask;
			break;
		case 0xFDFE: // A, S, D, F, G
			byte = 0x1F;
			if (glfwGetKey('A') == GLFW_PRESS) mask |= 0x01;
			if (glfwGetKey('S') == GLFW_PRESS) mask |= 0x02;
			if (glfwGetKey('D') == GLFW_PRESS) mask |= 0x04;
			if (glfwGetKey('F') == GLFW_PRESS) mask |= 0x08;
			if (glfwGetKey('G') == GLFW_PRESS) mask |= 0x10;
			byte &= ~mask;
			break;
		case 0xFBFE: // Q, W, E, R, T
			byte = 0x1F;
			if (glfwGetKey('Q') == GLFW_PRESS) mask |= 0x01;
			if (glfwGetKey('W') == GLFW_PRESS) mask |= 0x02;
			if (glfwGetKey('E') == GLFW_PRESS) mask |= 0x04;
			if (glfwGetKey('R') == GLFW_PRESS) mask |= 0x08;
			if (glfwGetKey('T') == GLFW_PRESS) mask |= 0x10;
			byte &= ~mask;
			break;
		case 0xF7FE: // 1, 2, 3, 4, 5
			byte = 0x1F;
			if (glfwGetKey('1') == GLFW_PRESS) mask |= 0x01;
			if (glfwGetKey('2') == GLFW_PRESS) mask |= 0x02;
			if (glfwGetKey('3') == GLFW_PRESS) mask |= 0x04;
			if (glfwGetKey('4') == GLFW_PRESS) mask |= 0x08;
			if (glfwGetKey('5') == GLFW_PRESS) mask |= 0x10;
			byte &= ~mask;
			break;
		case 0xEFFE: // 0, 9, 8, 7, 6
			byte = 0x1F;
			if (glfwGetKey('0') == GLFW_PRESS) mask |= 0x01;
			if (glfwGetKey('9') == GLFW_PRESS) mask |= 0x02;
			if (glfwGetKey('8') == GLFW_PRESS) mask |= 0x04;
			if (glfwGetKey('7') == GLFW_PRESS) mask |= 0x08;
			if (glfwGetKey('6') == GLFW_PRESS) mask |= 0x10;
			byte &= ~mask;
			break;
		case 0xDFFE: // P, O, I, U, Y
			byte = 0x1F;
			if (glfwGetKey('P') == GLFW_PRESS) mask |= 0x01;
			if (glfwGetKey('O') == GLFW_PRESS) mask |= 0x02;
			if (glfwGetKey('I') == GLFW_PRESS) mask |= 0x04;
			if (glfwGetKey('U') == GLFW_PRESS) mask |= 0x08;
			if (glfwGetKey('Y') == GLFW_PRESS) mask |= 0x10;
			byte &= ~mask;
			break;
		case 0xBFFE: // ENTER, L, K, J, H
			byte = 0x1F;
			if (glfwGetKey(GLFW_KEY_ENTER) == GLFW_PRESS) mask |= 0x01;
			if (glfwGetKey('L') == GLFW_PRESS) mask |= 0x02;
			if (glfwGetKey('K') == GLFW_PRESS) mask |= 0x04;
			if (glfwGetKey('J') == GLFW_PRESS) mask |= 0x08;
			if (glfwGetKey('H') == GLFW_PRESS) mask |= 0x10;
			byte &= ~mask;
			break;
		case 0x7FFE: // SPACE, SYM SHIFT, M, N, B
			byte = 0x1F;
			if (glfwGetKey(GLFW_KEY_SPACE) == GLFW_PRESS) mask |= 0x01;
			if (glfwGetKey(GLFW_KEY_RSHIFT) == GLFW_PRESS) mask |= 0x02;
			if (glfwGetKey('M') == GLFW_PRESS) mask |= 0x04;
			if (glfwGetKey('N') == GLFW_PRESS) mask |= 0x08;
			if (glfwGetKey('B') == GLFW_PRESS) mask |= 0x10;
			byte &= ~mask;
			break;

		default:
			fprintf(stderr, "ReadPort for address %04X\n", address);
			byte = 0;
			break;
	}


	return true;
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

const char* CZ80::GetConditionString(uint8 threeBits)
{
	switch (threeBits & 0x07)
	{
		case 0: return "NZ";		break; // Not Zero
		case 1: return "Z";			break; // Zero
		case 2: return "NC";		break; // Not Carry
		case 3: return "C";			break; // Carry
		case 4: return "PO";		break; // Parity Odd
		case 5: return "PE";		break; // Parity Even
		case 6: return "P";			break; // Positive (Not Sign)
		case 7: return "M";			break; // Minus (Sign)
	}
}

bool CZ80::IsConditionTrue(uint8 threeBits)
{
	switch (threeBits & 0x07)
	{
		case 0: return (m_F & eF_Z) == 0;		break; // Not Zero
		case 1: return (m_F & eF_Z) != 0;		break; // Zero
		case 2: return (m_F & eF_C) == 0;		break; // Not Carry
		case 3: return (m_F & eF_C) != 0;		break; // Carry
		case 4: return (m_F & eF_PV) == 0;	break; // Parity Odd
		case 5: return (m_F & eF_PV) != 0;	break; // Parity Even
		case 6: return (m_F & eF_S) == 0;		break; // Positive (Not Sign)
		case 7: return (m_F & eF_S) != 0;		break; // Minus (Sign)
	}
}

//=============================================================================

//-----------------------------------------------------------------------------
//	8-Bit Load Group
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementLDrr(void)
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
	uint8 opcode;
	ReadMemory(m_PC, opcode);
	REGISTER_8BIT(opcode >> 3) = REGISTER_8BIT(opcode);
	++m_PC;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementLDrn(void)
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
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	ReadMemory(m_PC++, REGISTER_8BIT(opcode >> 3));
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementLDr_HL_(void)
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
	uint8 opcode, data;
	ReadMemory(m_PC++, opcode);
	ReadMemory(m_HL, data);
	REGISTER_8BIT(opcode >> 3) = data;
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementLDr_IXd_(void)
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
	uint8 opcode, data;
 	int8 displacement;
	ReadMemory(++m_PC, opcode);
	ReadMemory(++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	ReadMemory(m_IX + displacement, data);
	++m_PC;
	REGISTER_8BIT(opcode >> 3) = data;
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementLDr_IYd_(void)
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
	uint8 opcode, data;
 	int8 displacement;
	ReadMemory(++m_PC, opcode);
	ReadMemory(++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	ReadMemory(m_IY + displacement, data);
	++m_PC;
	REGISTER_8BIT(opcode >> 3) = data;
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementLD_HL_r(void)
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
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	WriteMemory(m_HL, REGISTER_8BIT(opcode));
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementLD_IXd_r(void)
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
	uint8 opcode;
 	int8 displacement;
	ReadMemory(++m_PC, opcode);
	ReadMemory(++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	WriteMemory(m_IX + displacement, REGISTER_8BIT(opcode));
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementLD_IYd_r(void)
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
	uint8 opcode;
 	int8 displacement;
	ReadMemory(++m_PC, opcode);
	ReadMemory(++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	WriteMemory(m_IY + displacement, REGISTER_8BIT(opcode));
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementLD_HL_n(void)
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
	uint8 operand;
	ReadMemory(++m_PC, operand);
	++m_PC;
	WriteMemory(m_HL, operand);
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementLD_IXd_n(void)
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
	uint8 operand;
 	int8 displacement;
	ReadMemory(++++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	ReadMemory(++m_PC, operand);
	++m_PC;
	WriteMemory(m_IX + displacement, operand);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementLD_IYd_n(void)
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
	uint8 operand;
 	int8 displacement;
	ReadMemory(++++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	ReadMemory(++m_PC, operand);
	++m_PC;
	WriteMemory(m_IY + displacement, operand);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementLDA_BC_(void)
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
	uint8 byte;
	ReadMemory(m_BC, byte);
	++m_PC;
	m_A = byte;
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementLDA_DE_(void)
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
	uint8 byte;
	ReadMemory(m_DE, byte);
	++m_PC;
	m_A = byte;
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementLDA_nn_(void)
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
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	uint8 byte;
	ReadMemory(m_address, byte);
	m_A = byte;
	return 13;
}

//=============================================================================

uint32 CZ80::ImplementLD_BC_A(void)
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
	WriteMemory(m_BC, m_A);
	++m_PC; 
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementLD_DE_A(void)
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
	WriteMemory(m_DE, m_A);
	++m_PC; 
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementLD_nn_A(void)
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
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	WriteMemory(m_address, m_A);
	return 13;
}

//=============================================================================

uint32 CZ80::ImplementLDAI(void)
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
	m_F &= eF_C;
	m_F |= (m_A & eF_S) | (eF_Z & (m_A == 0)) | (m_State.m_IFF2) ? eF_PV : 0 | (m_A & (eF_X | eF_Y));
	++++m_PC;
	return 9;
}

//=============================================================================

uint32 CZ80::ImplementLDAR(void)
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
	m_F &= eF_C;
	m_F |= (m_A & eF_S) | (eF_Z & (m_A == 0)) | (m_State.m_IFF2) ? eF_PV : 0 | (m_A & (eF_X | eF_Y));
	++++m_PC;
	return 9;
}

//=============================================================================

uint32 CZ80::ImplementLDIA(void)
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
	return 9;
}

//=============================================================================

uint32 CZ80::ImplementLDRA(void)
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
	return 9;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	16-Bit Load Group
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementLDddnn(void)
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
	uint8 opcode;
	ReadMemory(m_PC, opcode);
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	REGISTER_16BIT(opcode >> 4) = m_address;
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementLDIXnn(void)
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
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	m_IX = m_address;
	return 14;
}

//=============================================================================

uint32 CZ80::ImplementLDIYnn(void)
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
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	m_IY = m_address;
	return 14;
}

//=============================================================================

uint32 CZ80::ImplementLDHL_nn_(void)
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
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	ReadMemory(m_address++, m_L);
	ReadMemory(m_address, m_H);
	return 16;
}

//=============================================================================

uint32 CZ80::ImplementLDdd_nn_(void)
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
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	ReadMemory(m_address++, REGISTER_16BIT_LO(opcode >> 4));
	ReadMemory(m_address, REGISTER_16BIT_HI(opcode >> 4));
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementLDIX_nn_(void)
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
	++++m_PC;
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	ReadMemory(m_address++, m_IXl);
	ReadMemory(m_address, m_IXh);
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementLDIY_nn_(void)
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
	++++m_PC;
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	ReadMemory(m_address++, m_IYl);
	ReadMemory(m_address, m_IYh);
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementLD_nn_HL(void)
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
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	WriteMemory(m_address++, m_L);
	WriteMemory(m_address, m_H);
	return 16;
}

//=============================================================================

uint32 CZ80::ImplementLD_nn_dd(void)
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
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	WriteMemory(m_address++, REGISTER_16BIT_LO(opcode >> 4));
	WriteMemory(m_address, REGISTER_16BIT_HI(opcode >> 4));
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementLD_nn_IX(void)
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
	++++m_PC;
	ReadMemory(m_PC++, m_addresslo);
	ReadMemory(m_PC++, m_addresshi);
	WriteMemory(m_address++, m_IXl);
	WriteMemory(m_address, m_IXh);
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementLD_nn_IY(void)
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
	++++m_PC;
	ReadMemory(m_PC++, m_addresslo);
	ReadMemory(m_PC++, m_addresshi);
	WriteMemory(m_address++, m_IYl);
	WriteMemory(m_address, m_IYh);
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementLDSPHL(void)
{
	//
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
	return 6;
}

//=============================================================================

uint32 CZ80::ImplementLDSPIX(void)
{
	//
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
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementLDSPIY(void)
{
	//
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
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementPUSHqq(void)
{
	//
	// Operation:	(SP-2) <- qql, (SP-1) <- qqh
	// Op Code:		PUSH
	// Operands:	qq
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|d|d|0|1|0|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								HL					10
	//								AF					11 (see below)
	//
	//							M Cycles		T States					MHz E.T.
	//								3						11 (5,3,3)				2.75
	//
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	WriteMemory(--m_SP, REGISTER_16BIT_HI(opcode >> 4));
	WriteMemory(--m_SP, REGISTER_16BIT_LO(opcode >> 4));
	return 11;
}

//=============================================================================

uint32 CZ80::ImplementPUSHAF(void)
{
	//
	// Operation:	(SP-2) <- F, (SP-1) <- A
	// Op Code:		PUSH
	// Operands:	qq
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|0|1|0|1| F5
	//						+-+-+-+-+-+-+-+-+
	//
	//						SP is mapped to 11
	//						in all other opcodes
	//						so special case for
	//						AF is needed here.
	//
	//							M Cycles		T States					MHz E.T.
	//								3						11 (5,3,3)				2.75
	//
	IncrementR(1);
	++m_PC;
	WriteMemory(--m_SP, m_A);
	WriteMemory(--m_SP, m_F);
	return 11;
}

//=============================================================================

uint32 CZ80::ImplementPUSHIX(void)
{
	//
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
	++++m_PC;
	WriteMemory(--m_SP, m_IXh);
	WriteMemory(--m_SP, m_IXl);
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementPUSHIY(void)
{
	//
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
	++++m_PC;
	WriteMemory(--m_SP, m_IYh);
	WriteMemory(--m_SP, m_IYl);
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementPOPqq(void)
{
	//
	// Operation:	qqh <- (SP+1), qql <- (SP)
	// Op Code:		POP
	// Operands:	qq
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|d|d|0|0|0|1|
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
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	ReadMemory(m_SP++, REGISTER_16BIT_LO(opcode >> 4));
	ReadMemory(m_SP++, REGISTER_16BIT_HI(opcode >> 4));
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementPOPAF(void)
{
	//
	// Operation:	A <- (SP+1), F <- (SP)
	// Op Code:		POP
	// Operands:	qq
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|0|0|0|1| F1
	//						+-+-+-+-+-+-+-+-+
	//
	//						SP is mapped to 11
	//						in all other opcodes
	//						so special case for
	//						AF is needed here.
	//
	//							M Cycles		T States					MHz E.T.
	//								3						10 (4,3,3)				2.75
	//
	IncrementR(1);
	++m_PC;
	ReadMemory(m_SP++, m_F);
	ReadMemory(m_SP++, m_A);
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementPOPIX(void)
{
	//
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
	++++m_PC;
	ReadMemory(m_SP++, m_IXl);
	ReadMemory(m_SP++, m_IXh);
	return 14;
}

//=============================================================================

uint32 CZ80::ImplementPOPIY(void)
{
	//
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
	++++m_PC;
	ReadMemory(m_SP++, m_IYl);
	ReadMemory(m_SP++, m_IYh);
	return 14;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Exchange, Block Transfer and Search Group
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementEXDEHL(void)
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
	++m_PC;
	m_DE ^= m_HL;
	m_HL ^= m_DE;
	m_DE ^= m_HL;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementEXAFAF(void)
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
	++m_PC;
	m_AF ^= m_AFalt;
	m_AFalt ^= m_AF;
	m_AF ^= m_AFalt;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementEXX(void)
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
	++m_PC;
	m_BC ^= m_BCalt;
	m_BCalt ^= m_BC;
	m_BC ^= m_BCalt;

	m_DE ^= m_DEalt;
	m_DEalt ^= m_DE;
	m_DE ^= m_DEalt;

	m_HL ^= m_HLalt;
	m_HLalt ^= m_HL;
	m_HL ^= m_HLalt;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementEX_SP_HL(void)
{
	//
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
	++m_PC;
	ReadMemory(m_SP, m_addresslo);
	ReadMemory(m_SP + 1, m_addresshi);
	WriteMemory(m_SP + 1, m_H);
	WriteMemory(m_SP, m_L);
	m_HL = m_address;
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementEX_SP_IX(void)
{
	//
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
	++++m_PC;
	ReadMemory(m_SP, m_addresslo);
	ReadMemory(m_SP + 1, m_addresshi);
	WriteMemory(m_SP++, m_IXl);
	WriteMemory(m_SP++, m_IXh);
	m_IX = m_address;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementEX_SP_IY(void)
{
	//
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
	++++m_PC;
	ReadMemory(m_SP, m_addresslo);
	ReadMemory(m_SP + 1, m_addresshi);
	WriteMemory(m_SP++, m_IYl);
	WriteMemory(m_SP++, m_IYh);
	m_IY = m_address;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementLDI(void)
{
	//
	// Operation:	(DE) <- (HL), DE <- DE+1, HL <- HL+1, BC <- BC-1
	// Op Code:		LDI
	// Operands:	(DE), DE, (HL), HL, BC
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|0|0|0|0| A0
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,4,3,5)			4.00
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL++, byte);
	WriteMemory(m_DE++, byte);
	--m_BC;
	// From The Undocumented Z80:
	byte += m_A;
	m_F &= (eF_S | eF_Z | eF_C);
	m_F |= (byte & eF_X) | ((byte << 4) & eF_Y);
	m_F |= (m_BC != 0) ? eF_PV : 0;
	return 16;
}

//=============================================================================

uint32 CZ80::ImplementLDIR(void)
{
	//
	// Operation:	(DE) <- (HL), DE <- DE+1, HL <- HL+1, BC <- BC-1
	// Op Code:		LDIR
	// Operands:	(DE), DE, (HL), HL, BC
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|0|0|0|0| B0
	//						+-+-+-+-+-+-+-+-+
	//
	//						For BC != 0
	//							M Cycles		T States					MHz E.T.
	//								5						21 (4,4,3,5,5)		5.25
	//
	//						For BC == 0
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,4,3,5)			4.00
	//
	uint32 tstates = ImplementLDI();
	if (m_BC != 0)
	{
		tstates += 5;
		----m_PC;
	}
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementLDD(void)
{
	//
	// Operation:	(DE) <- (HL), DE <- DE-1, HL <- HL-1, BC <- BC-1
	// Op Code:		LDD
	// Operands:	(DE), DE, (HL), HL, BC
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|1|0|0|0| A8
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,4,3,5)			4.00
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL--, byte);
	WriteMemory(m_DE--, byte);
	--m_BC;
	// From The Undocumented Z80:
	byte += m_A;
	m_F &= (eF_S | eF_Z | eF_C);
	m_F |= (byte & eF_X) | ((byte << 4) & eF_Y);
	m_F |= (m_BC != 0) ? eF_PV : 0;
	return 16;
}

//=============================================================================

uint32 CZ80::ImplementLDDR(void)
{
	//
	// Operation:	(DE) <- (HL), DE <- DE-1, HL <- HL-1, BC <- BC-1
	// Op Code:		LDDR
	// Operands:	(DE), DE, (HL), HL, BC
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|1|0|0|0| B8
	//						+-+-+-+-+-+-+-+-+
	//
	//						For BC != 0
	//							M Cycles		T States					MHz E.T.
	//								5						21 (4,4,3,5,5)		5.25
	//
	//						For BC == 0
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,4,3,5)			4.00
	//
	uint32 tstates = ImplementLDD();
	if (m_BC != 0)
	{
		tstates += 5;
		----m_PC;
	}
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementCPI(void)
{
	//
	// Operation:	A - (HL), HL <- HL+1, BC <- BC-1
	// Op Code:		CPI
	// Operands:	A, (HL), HL, BC
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|0|0|0|1| A1
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,4,3,5)			4.00
	//
	IncrementR(2);
	++++m_PC;
	uint8 _HL_;
	ReadMemory(m_HL++, _HL_);
	uint8 origA = m_A;
	uint8 result = origA - _HL_;
	uint8 half_result = (origA & 0x0F) - (_HL_ & 0x0F);
	--m_BC;
	// From The Undocumented Z80:
	uint8 origF = m_F;
	HandleArithmeticSubtractFlags(m_A, _HL_);
	uint8 byte = m_A - _HL_ - ((m_F & eF_H) >> 4);
	m_F &= eF_C;
	m_F |= (byte & eF_X) | ((byte << 4) & eF_Y) | (origF & eF_C);
	return 16;
}

//=============================================================================

uint32 CZ80::ImplementCPIR(void)
{
	//
	// Operation:	A - (HL), HL <- HL+1, BC <- BC-1
	// Op Code:		CPIR
	// Operands:	A, (HL), HL, BC
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|0|0|0|1| B1
	//						+-+-+-+-+-+-+-+-+
	//
	//						For BC != 0
	//							M Cycles		T States					MHz E.T.
	//								5						21 (4,4,3,5,5)		5.25
	//
	//						For BC == 0
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,4,3,5)			4.00
	//
	uint32 tstates = ImplementCPI();
	if (m_BC != 0)
	{
		tstates += 5;
		----m_PC;
	}
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementCPD(void)
{
	//
	// Operation:	A - (HL), HL <- HL-1, BC <- BC-1
	// Op Code:		CPD
	// Operands:	A, (HL), HL, BC
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|1|0|0|1| A9
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,4,3,5)			4.00
	//
	IncrementR(2);
	++++m_PC;
	uint8 _HL_;
	ReadMemory(m_HL--, _HL_);
	uint8 origA = m_A;
	uint8 result = origA - _HL_;
	uint8 half_result = (origA & 0x0F) - (_HL_ & 0x0F);
	--m_BC;
	// From The Undocumented Z80:
	uint8 origF = m_F;
	HandleArithmeticSubtractFlags(m_A, _HL_);
	uint8 byte = m_A - _HL_ - ((m_F & eF_H) >> 4);
	m_F &= eF_C;
	m_F |= (byte & eF_X) | ((byte << 4) & eF_Y) | (origF & eF_C);
	return 16;
}

//=============================================================================

uint32 CZ80::ImplementCPDR(void)
{
	//
	// Operation:	A - (HL), HL <- HL-1, BC <- BC-1
	// Op Code:		CPDR
	// Operands:	A, (HL), HL, BC
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|1|0|0|1| B9
	//						+-+-+-+-+-+-+-+-+
	//
	//						For BC != 0
	//							M Cycles		T States					MHz E.T.
	//								5						21 (4,4,3,5,5)		5.25
	//
	//						For BC == 0
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,4,3,5)			4.00
	//
	uint32 tstates = ImplementCPD();
	if (m_BC != 0)
	{
		tstates += 5;
		----m_PC;
	}
	return tstates;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	8-Bit Arithmetic Group
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementADDAr(void)
{
	//
	// Operation:	A <- A+r
	// Op Code:		ADD
	// Operands:	A, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|0|0|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
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
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8 source = REGISTER_8BIT(opcode);
	m_A = HandleArithmeticAddFlags(m_A, source);
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementADDAn(void)
{
	//
	// Operation:	A <- A+n
	// Op Code:		ADD
	// Operands:	A, n
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|0|1|1|0| C6
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	uint8 source;
	ReadMemory(++m_PC, source);
	++m_PC;
	m_A = HandleArithmeticAddFlags(m_A, source);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementADDA_HL_(void)
{
	//
	// Operation:	A <- A+(HL)
	// Op Code:		ADD
	// Operands:	A, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|0|0|1|1|0| 86
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	++m_PC;
	uint8 source;
	ReadMemory(m_HL, source);
	m_A = HandleArithmeticAddFlags(m_A, source);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementADDA_IXd_(void)
{
	//
	// Operation:	A <- A+(IX+d)
	// Op Code:		ADD
	// Operands:	A, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|0|0|1|1|0| 86
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	uint8 source, displacement;
	ReadMemory(m_PC++, displacement);
	ReadMemory(m_IX + displacement, source);
	m_A = HandleArithmeticAddFlags(m_A, source);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementADDA_IYd_(void)
{
	//
	// Operation:	A <- A+(IY+d)
	// Op Code:		ADD
	// Operands:	A, (IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|0|0|1|1|0| 86
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	uint8 source, displacement;
	ReadMemory(m_PC++, displacement);
	ReadMemory(m_IY + displacement, source);
	m_A = HandleArithmeticAddFlags(m_A, source);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementADCAr(void)
{
	//
	// Operation:	A <- A+r+C
	// Op Code:		ADC
	// Operands:	A, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|0|1|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
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
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8 source = REGISTER_8BIT(opcode) + (m_F & eF_C);
	m_A = HandleArithmeticAddFlags(m_A, source);
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementADCAn(void)
{
	//
	// Operation:	A <- A+n+C
	// Op Code:		ADC
	// Operands:	A, n
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|1|1|0| CE
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	uint8 source;
	ReadMemory(++m_PC, source);
	++m_PC;
	source += (m_F & eF_C);
	m_A = HandleArithmeticAddFlags(m_A, source);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementADCA_HL_(void)
{
	//
	// Operation:	A <- A+(HL)+C
	// Op Code:		ADC
	// Operands:	A, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|0|1|1|1|0| 8E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	++m_PC;
	uint8 source;
	ReadMemory(m_HL, source);
	source += (m_F & eF_C);
	m_A = HandleArithmeticAddFlags(m_A, source);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementADCA_IXd_(void)
{
	//
	// Operation:	A <- A+(IX+d)+C
	// Op Code:		ADC
	// Operands:	A, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|0|1|1|1|0| 8E
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	uint8 source, displacement;
	ReadMemory(m_PC++, displacement);
	ReadMemory(m_IX + displacement, source);
	source += (m_F & eF_C);
	m_A = HandleArithmeticAddFlags(m_A, source);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementADCA_IYd_(void)
{
	//
	// Operation:	A <- A+(IY+d)+C
	// Op Code:		ADC
	// Operands:	A, (IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|0|1|1|1|0| 8E
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	uint8 source, displacement;
	ReadMemory(m_PC++, displacement);
	ReadMemory(m_IY + displacement, source);
	source += (m_F & eF_C);
	m_A = HandleArithmeticAddFlags(m_A, source);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementSUBr(void)
{
	//
	// Operation:	A <- A-r
	// Op Code:		SUB
	// Operands:	A, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|1|0|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
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
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8 source = REGISTER_8BIT(opcode);
	m_A = HandleArithmeticSubtractFlags(m_A, source);
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementSUBn(void)
{
	//
	// Operation:	A <- A-n
	// Op Code:		SUB
	// Operands:	A, n
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|0|1|1|0| D6
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	uint8 source;
	ReadMemory(++m_PC, source);
	++m_PC;
	m_A = HandleArithmeticSubtractFlags(m_A, source);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementSUB_HL_(void)
{
	//
	// Operation:	A <- A-(HL)
	// Op Code:		SUB
	// Operands:	A, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|1|0|1|1|0| 96
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	++m_PC;
	uint8 source;
	ReadMemory(m_HL, source);
	m_A = HandleArithmeticSubtractFlags(m_A, source);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementSUB_IXd_(void)
{
	//
	// Operation:	A <- A-(IX+d)
	// Op Code:		SUB
	// Operands:	A, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|1|0|1|1|0| 96
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	uint8 source, displacement;
	ReadMemory(m_PC++, displacement);
	ReadMemory(m_IX + displacement, source);
	m_A = HandleArithmeticSubtractFlags(m_A, source);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementSUB_IYd_(void)
{
	//
	// Operation:	A <- A-(IY+d)
	// Op Code:		SUB
	// Operands:	A, (IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|1|0|1|1|0| 96
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	uint8 source, displacement;
	ReadMemory(m_PC++, displacement);
	ReadMemory(m_IY + displacement, source);
	m_A = HandleArithmeticSubtractFlags(m_A, source);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementSBCAr(void)
{
	//
	// Operation:	A <- A-r-C
	// Op Code:		SBC
	// Operands:	A, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|0|1|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
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
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8 source = REGISTER_8BIT(opcode) + (m_F & eF_C);
	m_A = HandleArithmeticSubtractFlags(m_A, source);
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementSBCAn(void)
{
	//
	// Operation:	A <- A-n-C
	// Op Code:		SBC
	// Operands:	A, n
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|1|0| DE
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	uint8 source;
	ReadMemory(++m_PC, source);
	++m_PC;
	source += (m_F & eF_C);
	m_A = HandleArithmeticSubtractFlags(m_A, source);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementSBCA_HL_(void)
{
	//
	// Operation:	A <- A-(HL)-C
	// Op Code:		SBC
	// Operands:	A, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|1|1|1|1|0| 9E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	uint8 source;
	ReadMemory(m_HL, source);
	++m_PC;
	source += (m_F & eF_C);
	m_A = HandleArithmeticSubtractFlags(m_A, source);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementSBCA_IXd_(void)
{
	//
	// Operation:	A <- A-(IX+d)-C
	// Op Code:		SBC
	// Operands:	A, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|1|1|1|1|0| 9E
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 source;
	ReadMemory(m_IX + displacement, source);
	source += (m_F & eF_C);
	m_A = HandleArithmeticSubtractFlags(m_A, source);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementSBCA_IYd_(void)
{
	//
	// Operation:	A <- A-(IY+d)-C
	// Op Code:		SBC
	// Operands:	A, (IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|1|1|1|1|0| 9E
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 source;
	ReadMemory(m_IY + displacement, source);
	source += (m_F & eF_C);
	m_A = HandleArithmeticSubtractFlags(m_A, source);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementANDr(void)
{
	//
	// Operation:	A <- A&r
	// Op Code:		AND
	// Operands:	A, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|0|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
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
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	m_A &= REGISTER_8BIT(opcode);
	HandleLogicalFlags(m_A);
	m_F |= eF_H;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementANDn(void)
{
	//
	// Operation:	A <- A&n
	// Op Code:		AND
	// Operands:	A, n
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|0|1|1|0| E6
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	uint8 byte;
	ReadMemory(++m_PC, byte);
	++m_PC;
	m_A &= byte;
	HandleLogicalFlags(m_A);
	m_F |= eF_H;
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementAND_HL_(void)
{
	//
	// Operation:	A <- A&(HL)
	// Op Code:		AND
	// Operands:	A, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|0|1|1|0| A6
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	m_A &= byte;
	HandleLogicalFlags(m_A);
	m_F |= eF_H;
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementAND_IXd_(void)
{
	//
	// Operation:	A <- A&(IX+d)
	// Op Code:		AND
	// Operands:	A, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|0|1|1|0| A6
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	m_A &= byte;
	HandleLogicalFlags(m_A);
	m_F |= eF_H;
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementAND_IYd_(void)
{
	//
	// Operation:	A <- A&(IY+d)
	// Op Code:		AND
	// Operands:	A, (IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|0|1|1|0| A6
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	m_A &= byte;
	HandleLogicalFlags(m_A);
	m_F |= eF_H;
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementORr(void)
{
	//
	// Operation:	A <- A|r
	// Op Code:		OR
	// Operands:	A, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|0|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
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
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	m_A |= REGISTER_8BIT(opcode);
	HandleLogicalFlags(m_A);
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementORn(void)
{
	//
	// Operation:	A <- A|n
	// Op Code:		OR
	// Operands:	A, n
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|0|1|1|0| F6
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	uint8 byte;
	ReadMemory(++m_PC, byte);
	++m_PC;
	m_A |= byte;
	HandleLogicalFlags(m_A);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementOR_HL_(void)
{
	//
	// Operation:	A <- A|(HL)
	// Op Code:		OR
	// Operands:	A, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|0|1|1|0| B6
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	m_A |= byte;
	HandleLogicalFlags(m_A);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementOR_IXd_(void)
{
	//
	// Operation:	A <- A|(IX+d)
	// Op Code:		OR
	// Operands:	A, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|0|1|1|0| B6
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	m_A |= byte;
	HandleLogicalFlags(m_A);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementOR_IYd_(void)
{
	//
	// Operation:	A <- A|(IY+d)
	// Op Code:		OR
	// Operands:	A, (IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|0|1|1|0| B6
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	m_A |= byte;
	HandleLogicalFlags(m_A);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementXORr(void)
{
	//
	// Operation:	A <- A^r
	// Op Code:		XOR
	// Operands:	A, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|1|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
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
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	m_A ^= REGISTER_8BIT(opcode);
	HandleLogicalFlags(m_A);
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementXORn(void)
{
	//
	// Operation:	A <- A^n
	// Op Code:		XOR
	// Operands:	A, n
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|1|0| EE
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	uint8 byte;
	ReadMemory(++m_PC, byte);
	++m_PC;
	m_A ^= byte;
	HandleLogicalFlags(m_A);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementXOR_HL_(void)
{
	//
	// Operation:	A <- A^(HL)
	// Op Code:		XOR
	// Operands:	A, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|1|1|1|0| AE
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	m_A ^= byte;
	HandleLogicalFlags(m_A);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementXOR_IXd_(void)
{
	//
	// Operation:	A <- A^(IX+d)
	// Op Code:		XOR
	// Operands:	A, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|1|1|1|0| AE
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	m_A ^= byte;
	HandleLogicalFlags(m_A);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementXOR_IYd_(void)
{
	//
	// Operation:	A <- A^(IY+d)
	// Op Code:		XOR
	// Operands:	A, (IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|1|1|1|0| AE
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	m_A ^= byte;
	HandleLogicalFlags(m_A);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementCPr(void)
{
	//
	// Operation:	A - r
	// Op Code:		CP
	// Operands:	A, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|1|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
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
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8 source = REGISTER_8BIT(opcode);
	HandleArithmeticSubtractFlags(m_A, source);
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementCPn(void)
{
	//
	// Operation:	A - n
	// Op Code:		CP
	// Operands:	A, n
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|0|1|1|0| D6
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	uint8 source;
	ReadMemory(++m_PC, source);
	++m_PC;
	HandleArithmeticSubtractFlags(m_A, source);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementCP_HL_(void)
{
	//
	// Operation:	A - (HL)
	// Op Code:		CP
	// Operands:	A, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|1|0|1|1|0| 96
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7	(4,3)						1.75
	//
	IncrementR(1);
	++m_PC;
	uint8 source;
	ReadMemory(m_HL, source);
	HandleArithmeticSubtractFlags(m_A, source);
	return 7;
}

//=============================================================================

uint32 CZ80::ImplementCP_IXd_(void)
{
	//
	// Operation:	A - (IX+d)
	// Op Code:		CP
	// Operands:	A, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|1|0|1|1|0| 96
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 source;
	ReadMemory(m_IX + displacement, source);
	HandleArithmeticSubtractFlags(m_A, source);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementCP_IYd_(void)
{
	//
	// Operation:	A <- A-(IY+d)
	// Op Code:		CP
	// Operands:	A, (IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|0|1|0|1|1|0| 96
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						19 (4,4,3,5,3)		4.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 source;
	ReadMemory(m_IY + displacement, source);
	HandleArithmeticSubtractFlags(m_A, source);
	return 19;
}

//=============================================================================

uint32 CZ80::ImplementINCr(void)
{
	//
	// Operation:	r <- r+1
	// Op Code:		INC
	// Operands:	r
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|r|r|r|1|0|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
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
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8& reg = REGISTER_8BIT(opcode >> 3);
	uint8 origF = m_F;
	reg = HandleArithmeticAddFlags(reg, 1);
	m_F &= ~eF_C;
	m_F |= (origF & eF_C);
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementINC_HL_(void)
{
	//
	// Operation:	(HL) <- (HL)+1
	// Op Code:		INC
	// Operands:	(HL)
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|1|0|0| 34
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						11 (4,4,3)				2.75
	//
	IncrementR(1);
	++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 origF = m_F;
	byte = HandleArithmeticAddFlags(byte, 1);
	m_F &= ~eF_C;
	m_F |= (origF & eF_C);
	WriteMemory(m_HL, byte);
	return 11;
}

//=============================================================================

uint32 CZ80::ImplementINC_IXd_(void)
{
	//
	// Operation:	(IX+d) <- (IX+d)+1
	// Op Code:		INC
	// Operands:	(IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|1|0|0| 34
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	uint8 origF = m_F;
	byte = HandleArithmeticAddFlags(byte, 1);
	m_F &= ~eF_C;
	m_F |= (origF & eF_C);
	WriteMemory(m_IX + displacement, byte);
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementINC_IYd_(void)
{
	//
	// Operation:	(IY+d) <- (IY+d)+1
	// Op Code:		INC
	// Operands:	(IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|1|0|0| 34
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	uint8 origF = m_F;
	byte = HandleArithmeticAddFlags(byte, 1);
	m_F &= ~eF_C;
	m_F |= (origF & eF_C);
	WriteMemory(m_IY + displacement, byte);
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementDECr(void)
{
	//
	// Operation:	r <- r-1
	// Op Code:		DEC
	// Operands:	r
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|r|r|r|1|0|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
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
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	int8& reg = *reinterpret_cast<int8*>(&REGISTER_8BIT(opcode >> 3));
	uint8 origF = m_F;
	reg = HandleArithmeticSubtractFlags(reg, 1);
	m_F &= ~eF_C;
	m_F |= (origF & eF_C);
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementDEC_HL_(void)
{
	//
	// Operation:	(HL) <- (HL)-1
	// Op Code:		DEC
	// Operands:	(HL)
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|1|0|1| 35
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						11 (4,4,3)				2.75
	//
	IncrementR(1);
	++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 origF = m_F;
	byte = HandleArithmeticSubtractFlags(byte, 1);
	m_F &= ~eF_C;
	m_F |= (origF & eF_C);
	WriteMemory(m_HL, byte);
	return 11;
}

//=============================================================================

uint32 CZ80::ImplementDEC_IXd_(void)
{
	//
	// Operation:	(IX+d) <- (IX+d)-1
	// Op Code:		DEC
	// Operands:	(IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|1|0|1| 35
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	uint8 origF = m_F;
	byte = HandleArithmeticSubtractFlags(byte, 1);
	m_F &= ~eF_C;
	m_F |= (origF & eF_C);
	WriteMemory(m_IX + displacement, byte);
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementDEC_IYd_(void)
{
	//
	// Operation:	(IY+d) <- (IY+d)-1
	// Op Code:		DEC
	// Operands:	(IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|1|0|1| 35
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	uint8 origF = m_F;
	byte = HandleArithmeticSubtractFlags(byte, 1);
	m_F &= ~eF_C;
	m_F |= (origF & eF_C);
	WriteMemory(m_IY + displacement, byte);
	return 23;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	General-Purpose Arithmetic and CPU Control Groups
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementDAA(void)
{
	//
	// Operation:	A
	// Op Code:		DAA
	// Operands:	A
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|1|1|1| 27
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	++m_PC;
	uint8 carry = (m_F & eF_C) | ((m_A > 0x99) ? eF_C : 0);

	if (m_F & eF_N)
	{
		if ((m_F & eF_H) || ((m_A & 0x0F) > 9))
		{
			m_A -= 0x06;
		}
		if ((m_F & eF_C) || ((m_A & 0xF0) > 0x90))
		{
			m_A -= 0x60;
		}
	}
	else
	{
		if ((m_F & eF_H) || ((m_A & 0x0F) > 9))
		{
			m_A += 0x06;
		}
		if ((m_F & eF_C) || ((m_A & 0xF0) > 0x90))
		{
			m_A += 0x60;
		}
	}

	uint8 origF = m_F;
	HandleLogicalFlags(m_A);
	m_F |= (origF & eF_N) | carry;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementCPL(void)
{
	//
	// Operation:	A <- ~A
	// Op Code:		CPL
	// Operands:	A
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|1|1|1|1| 27
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	++m_PC;
	m_A = ~m_A;
	m_F &= (eF_S | eF_Z | eF_PV | eF_C);
	m_F |= (eF_H | eF_N) | (m_A & (eF_X | eF_Y));
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementNEG(void)
{
	//
	// Operation:	A <- 0-A
	// Op Code:		NEG
	// Operands:	A
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|0|0|0|1|0|0| 44
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00
	//
	IncrementR(2);
	++++m_PC;
	uint8 origA = m_A;
	m_A = HandleArithmeticSubtractFlags(0, m_A);
	m_F &= ~(eF_PV | eF_C);
	m_F |= ((origA == 0x80) ? eF_PV : 0) | ((origA) ? eF_C : 0);
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementCCF(void)
{
	//
	// Operation:	CF <- ~CF
	// Op Code:		CCF
	// Operands:	CF
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|1|1|1|1| 3F
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	++m_PC;
	uint8 origF = m_F;
	m_F = (origF & (eF_S | eF_Z | eF_PV)) | (((origF & eF_C) << 4) & eF_H) | (~origF & eF_C);
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementSCF(void)
{
	//
	// Operation:	CF <- 1
	// Op Code:		SCF
	// Operands:	CF
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|1|1|1| 37
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	++m_PC;
	m_F &= (eF_S | eF_Z | eF_PV);
	m_F |= eF_C;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementNOP(void)
{
	//
	// Operation:	---
	// Op Code:		NOP
	// Operands:	---
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|0|0|0|0| 00
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	++m_PC;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementHALT(void)
{
	//
	// Operation:	---
	// Op Code:		HALT
	// Operands:	---
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|1|1|0|1|1|0| 76
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	// Intentionally don't increment PC
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementDI(void)
{
	//
	// Operation:	IFF <- 0
	// Op Code:		DI
	// Operands:	IFF
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|0|0|1|1| F3
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	++m_PC;
	m_State.m_IFF1 = 0;
	m_State.m_IFF2 = 0;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementEI(void)
{
	//
	// Operation:	IFF <- 1
	// Op Code:		DI
	// Operands:	IFF
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|0|1|1| FB
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	++m_PC;
	m_State.m_IFF1 = 1;
	m_State.m_IFF2 = 1;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementIM0(void)
{
	//
	// Operation:	---
	// Op Code:		IM
	// Operands:	0
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|0|0|0|1|1|0| 46
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00
	//
	IncrementR(2);
	++++m_PC;
	m_State.m_InterruptMode = 0;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementIM1(void)
{
	//
	// Operation:	---
	// Op Code:		IM
	// Operands:	1
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|0|1|0|1|1|0| 56
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00
	//
	IncrementR(2);
	++++m_PC;
	m_State.m_InterruptMode = 1;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementIM2(void)
{
	//
	// Operation:	---
	// Op Code:		IM
	// Operands:	2
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|0|1|1|1|1|0| 5E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00
	//
	IncrementR(2);
	++++m_PC;
	m_State.m_InterruptMode = 2;
	return 8;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	16-Bit Arithmetic Group
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementADDHLdd(void)
{
	//
	// Operation:	HL <- HL+dd
	// Op Code:		ADD
	// Operands:	HL, dd
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|d|d|0|1|0|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								HL					10
	//								SP					11
	//
	//							M Cycles		T States					MHz E.T.
	//								3						11 (4,4,3)				2.75
	//
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint16 source = REGISTER_16BIT(opcode >> 4);
	uint8 origF = m_F;
	m_HL = Handle16BitArithmeticAddFlags(m_HL, source);
	m_F &= ~(eF_S | eF_Z | eF_PV);
	m_F |= (origF & (eF_S | eF_Z | eF_PV));
	return 11;
}

//=============================================================================

uint32 CZ80::ImplementADCHLdd(void)
{
	//
	// Operation:	HL <- HL+dd+C
	// Op Code:		ADC
	// Operands:	HL, dd
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|d|d|1|0|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								HL					10
	//								AF					11
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,4,4,3)			3.75
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	uint16 source = REGISTER_16BIT(opcode >> 4) + (m_F & eF_C);
	m_HL = Handle16BitArithmeticAddFlags(m_HL, source);
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementSBCHLdd(void)
{
	//
	// Operation:	HL <- HL-dd-C
	// Op Code:		SBC
	// Operands:	HL, dd
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|d|d|0|0|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								HL					10
	//								AF					11
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,4,4,3)			3.75
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	uint16 source = REGISTER_16BIT(opcode >> 4) + (m_F & eF_C);
	m_HL = Handle16BitArithmeticSubtractFlags(m_HL, source);
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementADDIXdd(void)
{
	//
	// Operation:	IX <- IX+dd
	// Op Code:		ADD
	// Operands:	IX, dd
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|d|d|1|0|0|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								IX					10
	//								AF					11
	//
	//							M Cycles		T States					MHz E.T.
	//								3						15 (4,4,4,3)			3.75
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	opcode >>= 4;
	uint16 source = (opcode == 2) ? m_IX : REGISTER_16BIT(opcode);
	uint8 origF = m_F;
	m_IX = Handle16BitArithmeticAddFlags(m_IX, source);
	m_F &= ~(eF_S | eF_Z | eF_PV);
	m_F |= (origF & (eF_S | eF_Z | eF_PV));
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementADDIYdd(void)
{
	//
	// Operation:	IY <- IY+dd
	// Op Code:		ADD
	// Operands:	IY, dd
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|d|d|1|0|0|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								IY					10
	//								AF					11
	//
	//							M Cycles		T States					MHz E.T.
	//								3						15 (4,4,4,3)			3.75
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	opcode >>= 4;
	uint16 source = (opcode == 2) ? m_IY : REGISTER_16BIT(opcode);
	uint8 origF = m_F;
	m_IY = Handle16BitArithmeticAddFlags(m_IY, source);
	m_F &= ~(eF_S | eF_Z | eF_PV);
	m_F |= (origF & (eF_S | eF_Z | eF_PV));
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementINCdd(void)
{
	//
	// Operation:	dd <- dd+1
	// Op Code:		INC
	// Operands:	dd
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|d|d|0|0|1|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where dd is any of:
	//								BC					00
	//								DE					01
	//								HL					10
	//								SP					11
	//
	//							M Cycles		T States					MHz E.T.
	//								1						6									1.50
	//
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	++REGISTER_16BIT(opcode >> 4);
	return 6;
}

//=============================================================================

uint32 CZ80::ImplementINCIX(void)
{
	//
	// Operation:	IX <- IX+1
	// Op Code:		INC
	// Operands:	IX
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|0|1|1| 23
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						10 (4,6)					2.50
	//
	IncrementR(2);
	++++m_PC;
	++m_IX;
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementINCIY(void)
{
	//
	// Operation:	IY <- IY+1
	// Op Code:		INC
	// Operands:	IY
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|0|1|1| 23
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						10 (4,6)					2.50
	//
	IncrementR(2);
	++++m_PC;
	++m_IY;
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementDECdd(void)
{
	//
	// Operation:	dd <- dd-1
	// Op Code:		DEC
	// Operands:	dd
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|d|d|1|0|1|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						6									1.50
	//
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	--REGISTER_16BIT(opcode >> 4);
	return 6;
}

//=============================================================================

uint32 CZ80::ImplementDECIX(void)
{
	//
	// Operation:	IX <- IX-1
	// Op Code:		DEC
	// Operands:	IX
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|1|0|1|1| 2B
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						10 (4,6)					2.50
	//
	IncrementR(2);
	++++m_PC;
	--m_IX;
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementDECIY(void)
{
	//
	// Operation:	IY <- IY-1
	// Op Code:		DEC
	// Operands:	IY
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|1|0|1|1| 2B
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						10 (4,6)					2.50
	//
	IncrementR(2);
	++++m_PC;
	--m_IY;
	return 10;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Rotate and Shift Group
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementRLCA(void)
{
	//
	// Operation:	C <- 7<-0 A
	//								 +--+
	// Op Code:		RLCA
	// Operands:	A
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|0|1|1|1| 07
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00	
	//
	IncrementR(1);
	++m_PC;
	uint8 carry = (m_A & eF_S) >> 7;
	m_A = (m_A << 1) | carry;
	m_F &= (eF_S | eF_Z | eF_PV);
	m_F |= (m_A & (eF_X | eF_Y)) | carry;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementRLA(void)
{
	//
	// Operation:	C <- 7<-0 A
	//						+-------+
	// Op Code:		RLA
	// Operands:	A
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|0|1|1|1| 07
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00	
	//
	IncrementR(1);
	++m_PC;
	uint8 carry = (m_A & eF_S) >> 7;
	m_A = (m_A << 1) | (m_F & eF_C);
	m_F &= (eF_S | eF_Z | eF_PV);
	m_F |= (m_A & (eF_X | eF_Y)) | carry;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementRRCA(void)
{
	//
	// Operation:	A 7->0 -> C
	//							+--+
	// Op Code:		RRCA
	// Operands:	A
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|1|1|1|1| 0F
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00	
	//
	IncrementR(1);
	++m_PC;
	uint8 carry = (m_A & eF_C);
	m_A = (m_A >> 1) | (carry << 7);
	m_F &= (eF_S | eF_Z | eF_PV);
	m_F |= (m_A & (eF_X | eF_Y)) | carry;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementRRA(void)
{
	//
	// Operation:	A 7->0 -> C
	//							+-------+
	// Op Code:		RRCA
	// Operands:	A
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|1|1|1|1| 1F
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00	
	//
	IncrementR(1);
	++m_PC;
	uint8 carry = (m_A & eF_C);
	m_A = (m_A >> 1) | ((m_F & eF_C) << 7);
	m_F &= (eF_S | eF_Z | eF_PV);
	m_F |= (m_A & (eF_X | eF_Y)) | carry;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementRLCr(void)
{
	//
	// Operation:	C <- 7<-0 r
	//								 +--+
	// Op Code:		RLC
	// Operands:	r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|0|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00	
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	uint8& reg = REGISTER_8BIT(opcode);
	uint8 carry = (reg & eF_S) >> 7;
	reg = (reg << 1) | carry;
	HandleLogicalFlags(reg);
	m_F |= carry;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementRLC_HL_(void)
{
	//
	// Operation:	C <- 7<-0 (HL)
	//								 +--+
	// Op Code:		RLC
	// Operands:	(HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|0|1|1|0| 06
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,4,4,3)			3.75	
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 carry = (byte & eF_S) >> 7;
	byte = (byte << 1) | carry;
	WriteMemory(m_HL, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementRLC_IXd_(void)
{
	//
	// Operation:	C <- 7<-0 (IX+d)
	//								 +--+
	// Op Code:		RLC
	// Operands:	(IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|0|1|1|0| 06
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75	
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	uint8 carry = (byte & eF_S) >> 7;
	byte = (byte << 1) | carry;
	WriteMemory(m_IX + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementRLC_IYd_(void)
{
	//
	// Operation:	C <- 7<-0 (IY+d)
	//								 +--+
	// Op Code:		RLC
	// Operands:	(IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|0|1|1|0| 06
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75	
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	uint8 carry = (byte & eF_S) >> 7;
	byte = (byte << 1) | carry;
	WriteMemory(m_IY + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementRLr(void)
{
	//
	// Operation:	C <- 7<-0 r
	//						+-------+
	// Op Code:		RL
	// Operands:	r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|0|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00	
	//
	IncrementR(2);
	++m_PC;
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8& reg = REGISTER_8BIT(opcode);
	uint8 carry = (reg & eF_S) >> 7;
	reg = (reg << 1) | (m_F & eF_C);
	HandleLogicalFlags(reg);
	m_F |= carry;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementRL_HL_(void)
{
	//
	// Operation:	C <- 7<-0 (HL)
	//						+-------+
	// Op Code:		RL
	// Operands:	(HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|0|1|1|0| 16
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,4,4,3)			3.75	
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 carry = (byte & eF_S) >> 7;
	byte = (byte << 1) | (m_F & eF_C);
	WriteMemory(m_HL, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementRL_IXd_(void)
{
	//
	// Operation:	C <- 7<-0 (IX+d)
	//						+-------+
	// Op Code:		RL
	// Operands:	(IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|0|1|1|0| 16
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	uint8 carry = (byte & eF_S) >> 7;
	byte = (byte << 1) | (m_F & eF_C);
	WriteMemory(m_IX + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementRL_IYd_(void)
{
	//
	// Operation:	C <- 7<-0 (IY+d)
	//						+-------+
	// Op Code:		RL
	// Operands:	(IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|0|1|1|0| 16
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	uint8 carry = (byte & eF_S) >> 7;
	byte = (byte << 1) | (m_F & eF_C);
	WriteMemory(m_IY + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementRRCr(void)
{
	//
	// Operation:	r 7->0 -> C
	//						  +--+
	// Op Code:		RRC
	// Operands:	r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|1|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00	
	//
	IncrementR(2);
	++m_PC;
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8& reg = REGISTER_8BIT(opcode);
	uint8 carry = (reg & eF_C);
	reg = (reg >> 1) | (carry << 7);
	HandleLogicalFlags(reg);
	m_F |= carry;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementRRC_HL_(void)
{
	//
	// Operation:	(HL) 7->0 -> C
	//								 +--+
	// Op Code:		RRC
	// Operands:	(HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|1|1|1|0| 0E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,4,4,3)			3.75	
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1) | (carry << 7);
	WriteMemory(m_HL, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementRRC_IXd_(void)
{
	//
	// Operation:	(IX+d) 7->0 -> C
	//									 +--+
	// Op Code:		RRC
	// Operands:	(IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|1|1|1|0| 0E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75	
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1) | (carry << 7);
	WriteMemory(m_IX + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementRRC_IYd_(void)
{
	//
	// Operation:	(IY+d) 7->0 -> C
	//									 +--+
	// Op Code:		RRC
	// Operands:	(IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|0|1|1|1|0| 0E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1) | (carry << 7);
	WriteMemory(m_IY + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementRRr(void)
{
	//
	// Operation:	r 7->0 -> C
	//						  +-------+
	// Op Code:		RR
	// Operands:	r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|1|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00	
	//
	IncrementR(2);
	++m_PC;
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8& reg = REGISTER_8BIT(opcode);
	uint8 carry = (reg & eF_C);
	reg = (reg >> 1) | ((m_F & eF_C) << 7);
	uint8 parity = reg;
	HandleLogicalFlags(reg);
	m_F |= carry;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementRR_HL_(void)
{
	//
	// Operation:	(HL) 7->0 -> C
	//								 +-------+
	// Op Code:		RR
	// Operands:	(HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|1|1|1|0| 1E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,4,4,3)			3.75	
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1) | ((m_F & eF_C) << 7);
	WriteMemory(m_HL, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementRR_IXd_(void)
{
	//
	// Operation:	(IX+d) 7->0 -> C
	//									 +-------+
	// Op Code:		RR
	// Operands:	(IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|1|1|1|0| 1E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75	
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1) | ((m_F & eF_C) << 7);
	WriteMemory(m_IX + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementRR_IYd_(void)
{
	//
	// Operation:	(IY+d) 7->0 -> C
	//									 +-------+
	// Op Code:		RR
	// Operands:	(IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|1|1|1|0| 1E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75	
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1) | ((m_F & eF_C) << 7);
	WriteMemory(m_IY + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementSLAr(void)
{
	//
	// Operation:	C <- 7<-0 r
	// Op Code:		SLA
	// Operands:	r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00	
	//
	IncrementR(2);
	++m_PC;
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8& reg = REGISTER_8BIT(opcode);
	uint8 carry = (reg & eF_S) >> 7;
	reg = (reg << 1);
	HandleLogicalFlags(reg);
	m_F |= carry;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementSLA_HL_(void)
{
	//
	// Operation:	C <- 7<-0 (HL)
	// Op Code:		SLA
	// Operands:	(HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|1|1|0| 26
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,4,4,3)			3.75	
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 carry = (byte & eF_S) >> 7;
	byte = (byte << 1);
	WriteMemory(m_HL, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementSLA_IXd_(void)
{
	//
	// Operation:	C <- 7<-0 (IX+d)
	// Op Code:		SLA
	// Operands:	(IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|1|1|0| 26
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	uint8 carry = (byte & eF_S) >> 7;
	byte = (byte << 1);
	WriteMemory(m_IX + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementSLA_IYd_(void)
{
	//
	// Operation:	C <- 7<-0 (IY+d)
	// Op Code:		SLA
	// Operands:	(IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|1|1|0| 26
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	uint8 carry = (byte & eF_S) >> 7;
	byte = (byte << 1);
	WriteMemory(m_IY + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementSRAr(void)
{
	//
	// Operation:	r 7->0 -> C
	//							++
	// Op Code:		SRA
	// Operands:	r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|1|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00	
	//
	IncrementR(2);
	++m_PC;
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8& reg = REGISTER_8BIT(opcode);
	uint8 sign = (reg & eF_S);
	uint8 carry = (reg & eF_C);
	reg = (reg >> 1) | sign;
	HandleLogicalFlags(reg);
	m_F |= carry;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementSRA_HL_(void)
{
	//
	// Operation:	(HL) 7->0 -> C
	//								 ++
	// Op Code:		SRA
	// Operands:	(HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|1|1|1|0| 2E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,4,4,3)			3.75	
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 sign = (byte & eF_S);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1) | sign;
	WriteMemory(m_HL, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementSRA_IXd_(void)
{
	//
	// Operation:	(IX+d) 7->0 -> C
	//									 ++
	// Op Code:		SRA
	// Operands:	(IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|1|1|1|0| 2E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	uint8 sign = (byte & eF_S);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1) | sign;
	WriteMemory(m_IX + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementSRA_IYd_(void)
{
	//
	// Operation:	(IY+d) 7->0 -> C
	//									 ++
	// Op Code:		SRA
	// Operands:	(IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|1|1|1|0| 2E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	uint8 sign = (byte & eF_S);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1) | sign;
	WriteMemory(m_IY + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementSRLr(void)
{
	//
	// Operation:	0 -> r 7->0 -> C
	// Op Code:		SRL
	// Operands:	r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|1|r|r|r|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8	(4,4)						2.00	
	//
	IncrementR(2);
	++m_PC;
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8& reg = REGISTER_8BIT(opcode);
	uint8 carry = (reg & eF_C);
	reg = (reg >> 1);
	HandleLogicalFlags(reg);
	m_F |= carry;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementSRL_HL_(void)
{
	//
	// Operation:	0 -> (HL) 7->0 -> C
	// Op Code:		SRL
	// Operands:	(HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|1|1|1|0| 3E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						15 (4,4,4,3)			3.75	
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1);
	WriteMemory(m_HL, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 15;
}

//=============================================================================

uint32 CZ80::ImplementSRL_IXd_(void)
{
	//
	// Operation:	0 -> (IX+d) 7->0 -> C
	//									 ++
	// Op Code:		SRL
	// Operands:	(IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|1|1|1|0| 3E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1);
	WriteMemory(m_IX + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementSRL_IYd_(void)
{
	//
	// Operation:	0 -> (IY+d) 7->0 -> C
	// Op Code:		SRL
	// Operands:	(IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|1|1|1|0| 3E
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								6						23 (4,4,3,5,4,3)	5.75
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	uint8 carry = (byte & eF_C);
	byte = (byte >> 1);
	WriteMemory(m_IY + displacement, byte);
	HandleLogicalFlags(byte);
	m_F |= carry;
	return 23;
}

//=============================================================================

uint32 CZ80::ImplementRLD(void)
{
	//
	//										+----->-----+
	// Operation:	A 7-4 3-0 (HL) 7-4 3-0
	//										+---<--+ +<-+
	// Op Code:		RLD
	// Operands:	A, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|1|0|1|1|1|1| 6F
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						18 (4,4,3,4,3)		3.75
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 origA = m_A;
	m_A = (m_A & 0xF0) | ((byte & 0xF0) >> 4);
	byte = (byte << 4) | (origA & 0x0F);
	WriteMemory(m_HL, byte);
	uint8 origF = m_F;
	HandleLogicalFlags(m_A);
	m_F |= (origF & eF_C);
	return 18;
}

//=============================================================================

uint32 CZ80::ImplementRRD(void)
{
	//
	//										+--->--+ +->+
	// Operation:	A 7-4 3-0 (HL) 7-4 3-0
	//										+-----<-----+
	// Op Code:		RRD
	// Operands:	A, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|1|0|0|1|1|1| 67
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						18 (4,4,3,4,3)		3.75
	//
	IncrementR(2);
	++++m_PC;
	uint8 byte;
	ReadMemory(m_HL, byte);
	uint8 origA = m_A;
	m_A = (m_A & 0xF0) | (byte & 0x0F);
	byte = (byte >> 4) | (origA & 0x0F);
	WriteMemory(m_HL, byte);
	uint8 origF = m_F;
	HandleLogicalFlags(m_A);
	m_F |= (origF & eF_C);
	return 18;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Bit Set, Reset and Test Group
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementBITbr(void)
{
	//
	// Operation:	Z <- rb
	// Op Code:		BIT
	// Operands:	b, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|b|b|b|r|r|r|
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
	//								2						8 (4,4)						2.00
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	uint8 origF = m_F;
	HandleLogicalFlags(REGISTER_8BIT(opcode) & mask);
	m_F |= (eF_H | (origF & eF_C));
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementBITb_HL_(void)
{
	//
	// Operation:	Z <- (HL)b
	// Op Code:		BIT
	// Operands:	b, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|b|b|b|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						12 (4,4,4)				3.00
	//
	IncrementR(2);
	uint8 opcode, byte;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	ReadMemory(m_HL, byte);
	uint8 origF = m_F;
	HandleLogicalFlags(byte & mask);
	m_F |= (eF_H | (origF & eF_C));
	return 12;
}

//=============================================================================

uint32 CZ80::ImplementBITb_IXd_(void)
{
	//
	// Operation:	Z <- (IX+d)b
	// Op Code:		BIT
	// Operands:	b, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|b|b|b|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						20 (4,4,3,5,4)		5.00
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 opcode, byte;
	ReadMemory(m_PC++, opcode);
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	ReadMemory(m_IX + displacement, byte);
	uint8 origF = m_F;
	HandleLogicalFlags(byte & mask);
	m_F |= (eF_H | (origF & eF_C));
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementBITb_IYd_(void)
{
	//
	// Operation:	Z <- (IX+d)b
	// Op Code:		BIT
	// Operands:	b, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|b|b|b|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						20 (4,4,3,5,4)		5.00
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 opcode, byte;
	ReadMemory(m_PC++, opcode);
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	ReadMemory(m_IY + displacement, byte);
	uint8 origF = m_F;
	HandleLogicalFlags(byte & mask);
	m_F |= (eF_H | (origF & eF_C));
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementSETbr(void)
{
	//
	// Operation:	rb <- 1
	// Op Code:		SET
	// Operands:	b, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|b|b|b|r|r|r|
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
	//								2						8 (4,4)						2.00
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	uint8& reg = REGISTER_8BIT(opcode);
	reg |= mask;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementSETb_HL_(void)
{
	//
	// Operation:	(HL) <- 1
	// Op Code:		SET
	// Operands:	b, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|b|b|b|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						12 (4,4,4)				3.00
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	uint8 byte;
	ReadMemory(m_HL, byte);
	byte |= mask;
	WriteMemory(m_HL, byte);
	return 12;
}

//=============================================================================

uint32 CZ80::ImplementSETb_IXd_(void)
{
	//
	// Operation:	(IX+d)b <- 1
	// Op Code:		BIT
	// Operands:	b, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|b|b|b|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						20 (4,4,3,5,4)		5.00
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	byte |= mask;
	WriteMemory(m_IX + displacement, byte);
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementSETb_IYd_(void)
{
	//
	// Operation:	(IY+d)b <- 1
	// Op Code:		BIT
	// Operands:	b, (IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|b|b|b|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						20 (4,4,3,5,4)		5.00
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	byte |= mask;
	WriteMemory(m_IY + displacement, byte);
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementRESbr(void)
{
	//
	// Operation:	rb <- 1
	// Op Code:		RES
	// Operands:	b, r
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|b|b|b|r|r|r|
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
	//								2						8 (4,4)						2.00
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	uint8& reg = REGISTER_8BIT(opcode);
	reg &= ~mask;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementRESb_HL_(void)
{
	//
	// Operation:	(HL) <- 1
	// Op Code:		RES
	// Operands:	b, (HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|b|b|b|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						12 (4,4,4)				3.00
	//
	IncrementR(2);
	uint8 opcode, byte;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	ReadMemory(m_HL, byte);
	byte &= ~mask;
	WriteMemory(m_HL, byte);
	return 12;
}

//=============================================================================

uint32 CZ80::ImplementRESb_IXd_(void)
{
	//
	// Operation:	(IX+d)b <- 1
	// Op Code:		BIT
	// Operands:	b, (IX+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|b|b|b|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						20 (4,4,3,5,4)		5.00
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	uint8 byte;
	ReadMemory(m_IX + displacement, byte);
	byte &= ~mask;
	WriteMemory(m_IX + displacement, byte);
	return 20;
}

//=============================================================================

uint32 CZ80::ImplementRESb_IYd_(void)
{
	//
	// Operation:	(IY+d)b <- 1
	// Op Code:		BIT
	// Operands:	b, (IY+d)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|1|1| CB
	//						+-+-+-+-+-+-+-+-+
	//						|d|d|d|d|d|d|d|d|
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|b|b|b|1|1|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						20 (4,4,3,5,4)		5.00
	//
	IncrementR(2);
	++++m_PC;
	int8 displacement;
	ReadMemory(m_PC++, *(reinterpret_cast<uint8*>(&displacement)));
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	uint8 mask = 1 << ((opcode & 0x38) >> 3);
	uint8 byte;
	ReadMemory(m_IY + displacement, byte);
	byte &= ~mask;
	WriteMemory(m_IY + displacement, byte);
	return 20;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Jump Group
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementJPnn(void)
{
	//
	// Operation:	PC <- nn
	// Op Code:		JP
	// Operands:	nn
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|0|0|1|1| C3
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						10 (4,3,3)				2.50
	//
	IncrementR(1);
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	m_PC = m_address;
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementJPccnn(void)
{
	//
	// Operation:	If cc true, PC <- nn
	// Op Code:		JP
	// Operands:	cc, nn
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|c|c|c|0|1|0|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where ccc is any of:
	//								NZ					000
	//								Z						001
	//								NC					010
	//								C						011
	//								PO					100
	//								PE					101
	//								P						101
	//								M						111
	//
	//							M Cycles		T States					MHz E.T.
	//								3						10 (4,3,3)				2.50
	//
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	ReadMemory(m_PC++, m_addresslo);
	ReadMemory(m_PC++, m_addresshi);
	if (IsConditionTrue((opcode & 0x38) >> 3))
	{
		m_PC = m_address;
	}
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementJRe(void)
{
	//
	// Operation:	PC <- PC + e
	// Op Code:		JR
	// Operands:	e
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|1|0|0|0| 18
	//						+-+-+-+-+-+-+-+-+
	//						|e|e|e|e|e|e|e|e| e-2
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						12 (4,3,5)				3.00
	//
	IncrementR(1);
	int8 displacement;
	ReadMemory(++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	m_PC += displacement + 1;
	return 12;
}

//=============================================================================

uint32 CZ80::ImplementJRCe(void)
{
	//
	// Operation: if C == 0, continue
	//						if C == 1, PC <- PC + e
	// Op Code:		JR
	// Operands:	C, e
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|1|0|0|0| 38
	//						+-+-+-+-+-+-+-+-+
	//						|e|e|e|e|e|e|e|e| e-2
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75		C == 0
	//								3						12 (4,3,5)				3.00		C == 1
	//
	IncrementR(1);
	uint32 tstates = 0;
	int8 displacement;
	ReadMemory(++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	if (m_F & eF_C)
	{
		m_PC += displacement;
		tstates += 12;
	}
	else
	{
		tstates += 7;
	}
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementJRNCe(void)
{
	//
	// Operation: if C == 1, continue
	//						if C == 0, PC <- PC + e
	// Op Code:		JR
	// Operands:	C, e
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|1|0|0|0|0| 30
	//						+-+-+-+-+-+-+-+-+
	//						|e|e|e|e|e|e|e|e| e-2
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75		C == 1
	//								3						12 (4,3,5)				3.00		C == 0
	//
	IncrementR(1);
	uint32 tstates = 0;
	int8 displacement;
	ReadMemory(++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	if (m_F & eF_C)
	{
		tstates += 7;
	}
	else
	{
		m_PC += displacement;
		tstates += 12;
	}
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementJRZe(void)
{
	//
	// Operation: if Z == 0, continue
	//						if Z == 1, PC <- PC + e
	// Op Code:		JR
	// Operands:	Z, e
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|1|0|0|0| 28
	//						+-+-+-+-+-+-+-+-+
	//						|e|e|e|e|e|e|e|e| e-2
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75		Z == 0
	//								3						12 (4,3,5)				3.00		Z == 1
	//
	IncrementR(1);
	uint32 tstates = 0;
	int8 displacement;
	ReadMemory(++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	if (m_F & eF_Z)
	{
		m_PC += displacement;
		tstates += 12;
	}
	else
	{
		tstates += 7;
	}
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementJRNZe(void)
{
	//
	// Operation: if Z == 1, continue
	//						if Z == 0, PC <- PC + e
	// Op Code:		JR
	// Operands:	Z, e
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|1|0|0|0|0|0| 20
	//						+-+-+-+-+-+-+-+-+
	//						|e|e|e|e|e|e|e|e| e-2
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						7 (4,3)						1.75		Z == 1
	//								3						12 (4,3,5)				3.00		Z == 0
	//
	IncrementR(1);
	uint32 tstates = 0;
	int8 displacement;
	ReadMemory(++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	if (m_F & eF_Z)
	{
		tstates += 7;
	}
	else
	{
		m_PC += displacement;
		tstates += 12;
	}
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementJP_HL_(void)
{
	//
	// Operation: PC <- HL
	// Op Code:		JP
	// Operands:	(HL)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|0|0|1| E9
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								1						4									1.00
	//
	IncrementR(1);
	m_PC = m_HL;
	return 4;
}

//=============================================================================

uint32 CZ80::ImplementJP_IX_(void)
{
	// Operation: PC <- IX
	// Op Code:		JP
	// Operands:	(IX)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|1|0|1| DD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|0|0|1| E9
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8 (4,4)						1.00
	//
	IncrementR(2);
	m_PC = m_IX;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementJP_IY_(void)
{
	// Operation: PC <- IY
	// Op Code:		JP
	// Operands:	(IY)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|1|1|1|0|1| FD
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|0|0|1| E9
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8 (4,4)						1.00
	//
	IncrementR(2);
	m_PC = m_IY;
	return 8;
}

//=============================================================================

uint32 CZ80::ImplementDJNZe(void)
{
	//
	// Operation: if (B - 1) == 0, continue
	//						if (B - 1) != 0, PC <- PC + e
	// Op Code:		DJNZ
	// Operands:	B, e
	//						+-+-+-+-+-+-+-+-+
	//						|0|0|0|1|0|0|0|0| 20
	//						+-+-+-+-+-+-+-+-+
	//						|e|e|e|e|e|e|e|e| e-2
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								2						8 (5,3)						2.00		B != 0
	//								3						13 (5,3,5)				3.25		B == 0
	//
	IncrementR(1);
	uint32 tstates = 0;
	int8 displacement;
	ReadMemory(++m_PC, *(reinterpret_cast<uint8*>(&displacement)));
	++m_PC;
	if (--m_B == 0)
	{
		tstates += 8;
	}
	else
	{
		m_PC += displacement;
		tstates += 13;
	}
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Call and Return Group
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementCALLnn(void)
{
	//
	// Operation: (SP - 1) <- PCH, (SP - 2) <- PCL, PC <- nn
	// Op Code:		CALL
	// Operands:	nn
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|1|0|1| CD
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						17 (4,3,4,3,3)		4.25
	//
	IncrementR(1);
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	WriteMemory(--m_SP, m_PCh);
	WriteMemory(--m_SP, m_PCl);
	m_PC = m_address;
	return 17;
}

//=============================================================================

uint32 CZ80::ImplementCALLccnn(void)
{
	//
	// Operation: If cc true, (SP - 1) <- PCH, (SP - 2) <- PCL, PC <- nn
	// Op Code:		CALL
	// Operands:	nn
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|c|c|c|1|0|0|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where ccc is any of:
	//								NZ					000
	//								Z						001
	//								NC					010
	//								C						011
	//								PO					100
	//								PE					101
	//								P						101
	//								M						111
	//
	//							M Cycles		T States					MHz E.T.
	//								5						17 (4,3,4,3,3)		4.25	cc is true
	//								3						10 (4,3,3)				2.50	cc is false
	//
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC, opcode);
	ReadMemory(++m_PC, m_addresslo);
	ReadMemory(++m_PC, m_addresshi);
	++m_PC;
	uint32 tstates = 0;
	if (IsConditionTrue((opcode & 0x38) >> 3))
	{
		WriteMemory(--m_SP, m_PCh);
		WriteMemory(--m_SP, m_PCl);
		m_PC = m_address;
		tstates += 17;
	}
	else
	{
		tstates += 10;
	}
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementRET(void)
{
	//
	// Operation: PCl <- (SP), PCh <- (SP + 1)
	// Op Code:		RET
	// Operands:	--
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|0|1|0|0|1| C9
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						10 (4,3,3)				2.50
	//
	IncrementR(1);
	ReadMemory(m_SP++, m_PCl);
	ReadMemory(m_SP++, m_PCh);
	return 10;
}

//=============================================================================

uint32 CZ80::ImplementRETcc(void)
{
	//
	// Operation: If cc true, PCl <- (SP), PCh <- (SP + 1)
	// Op Code:		RET
	// Operands:	cc
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|c|c|c|0|0|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where ccc is any of:
	//								NZ					000
	//								Z						001
	//								NC					010
	//								C						011
	//								PO					100
	//								PE					101
	//								P						101
	//								M						111
	//
	//							M Cycles		T States					MHz E.T.
	//								3						11 (5,3,3)				2.75	cc is true
	//								1						5									1.25	cc is false
	//
	IncrementR(1);
	uint32 tstates = 0;
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	if (IsConditionTrue((opcode & 0x38) >> 3))
	{
		ReadMemory(m_SP++, m_PCl);
		ReadMemory(m_SP++, m_PCh);
		tstates += 11;
	}
	else
	{
		tstates += 5;
	}

	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementRETI(void)
{
	//
	// Operation:	Return from Interrupt
	// Op Code:		RETI
	// Operands:	--
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|0|0|1|1|0|1| 4D
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						14 (4,4,3,3)			3.50
	//
	IncrementR(2);
	ReadMemory(m_SP++, m_PCl);
	ReadMemory(m_SP++, m_PCh);
	return 14;
}

//=============================================================================

uint32 CZ80::ImplementRETN(void)
{
	//
	// Operation:	Return from Interrupt
	// Op Code:		RETN
	// Operands:	--
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|0|0|0|1|0|1| 45
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						14 (4,4,3,3)			3.50
	//
	IncrementR(2);
	m_State.m_IFF1 = m_State.m_IFF2;
	ReadMemory(m_SP++, m_PCl);
	ReadMemory(m_SP++, m_PCh);
	return 14;
}

//=============================================================================

uint32 CZ80::ImplementRSTp(void)
{
	//
	// Operation:	(SP - 1) <- PCH, (SP - 2) <- PCL, PCH <- 0, PCL <- p
	// Op Code:		RST
	// Operands:	p
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|t|t|t|1|1|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//								p						 t
	//							 00H					000
	//							 08H					001
	//							 10H					010
	//							 18H					011
	//							 20H					100
	//							 28H					101
	//							 30H					110
	//							 38H					111
	//
	//							M Cycles		T States					MHz E.T.
	//								3						11 (5,3,3)				2.75
	//
	IncrementR(1);
	uint8 opcode;
	ReadMemory(m_PC++, opcode);
	WriteMemory(--m_SP, m_PCh);
	WriteMemory(--m_SP, m_PCl);
	m_PCh = 0;
	m_PCl = 8 * ((opcode & 0x38) >> 3);
	return 11;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Input and Output Group
//-----------------------------------------------------------------------------

//=============================================================================

uint32 CZ80::ImplementINA_n_(void)
{
	//
	// Operation:	A <- (n)
	// Op Code:		IN
	// Operands:	A, (n)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|1|0|1|1| DB
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						11 (4,3,4)				2.75
	//
	IncrementR(2);
	m_addresshi = m_A;
	ReadMemory(++m_PC, m_addresslo);
	++m_PC;
	ReadPort(m_address, m_A);
	return 11;
}

//=============================================================================

uint32 CZ80::ImplementINr_C_(void)
{
	//
	// Operation:	r <- (C)
	// Op Code:		IN
	// Operands:	r, (C)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|0|1|1| EB
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|r|r|r|0|0|0|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where rrr is any of:
	//								B						000
	//								C						001
	//								D						010
	//								E						011
	//								H						100
	//								L						101
	//						Undefined				110
	//								A						111
	//
	//							M Cycles		T States					MHz E.T.
	//								3						12 (4,4,4)				3.00
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	m_addresshi = m_B;
	m_addresslo = m_C;
	ReadPort(m_address, REGISTER_8BIT(opcode >> 3));
	return 12;
}

//=============================================================================

uint32 CZ80::ImplementINI(void)
{
	//
	// Operation:	(HL) <- (C), B <- B - 1, HL <- HL + 1
	// Op Code:		INI
	// Operands:	--
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|0|0|1|0| A2
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,5,3,4)			4.00
	//
	IncrementR(2);
	++++m_PC;
	m_addresshi = m_B;
	m_addresslo = m_C;
	uint8 data;
	ReadPort(m_address, data);
	WriteMemory(m_HL++, data);
	--m_B;
	return 16;
}

//=============================================================================

uint32 CZ80::ImplementINIR(void)
{
	//
	// Operation:	(HL) <- (C), B <- B - 1, HL <- HL + 1
	// Op Code:		INI
	// Operands:	--
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|0|0|1|0| A2
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						21 (4,5,3,4,5)		5.25		B != 0
	//								4						16 (4,5,3,4)			4.00		B == 0
	//
	uint32 tstates = 0;
	uint8 data;
	++++m_PC;
	do
	{
		IncrementR(2);
		m_addresshi = m_B;
		m_addresslo = m_C;
		ReadPort(m_address, data);
		WriteMemory(m_HL++, data);
		tstates += 16;
	} while ((--m_B != 0) && ((tstates += 5), true));
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementIND(void)
{
	//
	// Operation:	(HL) <- (C), B <- B - 1, HL <- HL - 1
	// Op Code:		IND
	// Operands:	A, (n)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|1|0|1|0| AA
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,5,3,4)			4.00		B == 0
	//
	IncrementR(2);
	++++m_PC;
	m_addresshi = m_B;
	m_addresslo = m_C;
	uint8 data;
	ReadPort(m_address, data);
	WriteMemory(m_HL--, data);
	--m_B;
	return 16;
}

//=============================================================================

uint32 CZ80::ImplementINDR(void)
{
	//
	// Operation:	(HL) <- (C), B <- B - 1, HL <- HL - 1
	// Op Code:		INI
	// Operands:	A, (n)
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|0|0|1|0| BA
	//						+-+-+-+-+-+-+-+-+x
	//
	//							M Cycles		T States					MHz E.T.
	//								5						21 (4,5,3,4,5)		5.25		B != 0
	//								4						16 (4,5,3,4)			4.00		B == 0
	//
	uint32 tstates = 0;
	uint8 data;
	++++m_PC;
	do
	{
		IncrementR(2);
		m_addresshi = m_B;
		m_addresslo = m_C;
		ReadPort(m_address, data);
		WriteMemory(m_HL--, data);
		tstates += 16;
	} while ((--m_B != 0) && ((tstates += 5), true));
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementOUT_n_A(void)
{
	//
	// Operation:	(n) <- A
	// Op Code:		OUT
	// Operands:	(n), A
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|0|1|0|0|1|1| D3
	//						+-+-+-+-+-+-+-+-+
	//						|n|n|n|n|n|n|n|n|
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								3						11 (4,3,4)				2.75
	//
	m_addresshi = m_A;
	ReadMemory(++m_PC, m_addresslo);
	++m_PC;
	WritePort(m_address, m_A);
	return 11;
}

//=============================================================================

uint32 CZ80::ImplementOUT_C_r(void)
{
	//
	// Operation:	(C) <- r
	// Op Code:		OUT
	// Operands:	(C), r
	//						+-+-+-+-+-+-+-+-+
	//						|1x|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|0|1|r|r|r|0|0|1|
	//						+-+-+-+-+-+-+-+-+
	//
	//						where rrr is any of:
	//								B						000
	//								C						001
	//								D						010
	//								E						011
	//								H						100
	//								L						101
	//						Undefined				110
	//								A						111
	//
	//							M Cycles		T States					MHz E.T.
	//								3						12 (4,4,4)				3.00
	//
	IncrementR(2);
	uint8 opcode;
	ReadMemory(++m_PC, opcode);
	++m_PC;
	m_addresshi = m_B;
	m_addresslo = m_C;
	WritePort(m_address, REGISTER_8BIT(opcode >> 3));
	return 12;
}

//=============================================================================

uint32 CZ80::ImplementOUTI(void)
{
	//
	// Operation:	(C) <- (HL), B <- B - 1, HL <- HL + 1
	// Op Code:		OUTI
	// Operands:	--
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|0|0|1|1| A3
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,5,3,4)			4.00
	//
	IncrementR(2);
	++++m_PC;
	m_addresshi = m_B;
	m_addresslo = m_C;
	uint8 data;
	ReadMemory(m_HL++, data);
	WritePort(m_address, data);
	--m_B;
	return 16;
}

//=============================================================================

uint32 CZ80::ImplementOTIR(void)
{
	//
	// Operation:	(C) <- (HL), B <- B - 1, HL <- HL + 1
	// Op Code:		OTIR
	// Operands:	--
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|0|0|1|1| B3
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						21 (4,5,3,4,5)		5.25		B != 0
	//								4						16 (4,5,3,4)			4.00		B == 0
	//
	uint32 tstates = 0;
	uint8 data;
	++++m_PC;
	do
	{
		IncrementR(2);
		m_addresshi = m_B;
		m_addresslo = m_C;
		ReadMemory(m_HL++, data);
		WritePort(m_address, data);
		tstates += 16;
	} while ((--m_B != 0) && ((tstates += 5), true));
	return tstates;
}

//=============================================================================

uint32 CZ80::ImplementOUTD(void)
{
	//
	// Operation:	(C) <- (HL), B <- B - 1, HL <- HL - 1
	// Op Code:		OUTD
	// Operands:	--
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|0|1|0|1|1| AB
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								4						16 (4,5,3,4)			4.00
	//
	IncrementR(2);
	++++m_PC;
	m_addresshi = m_B;
	m_addresslo = m_C;
	uint8 data;
	ReadMemory(m_HL--, data);
	WritePort(m_address, data);
	--m_B;
	return 16;
}

//=============================================================================

uint32 CZ80::ImplementOTDR(void)
{
	//
	// Operation:	(C) <- (HL), B <- B - 1, HL <- HL - 1
	// Op Code:		OTIR
	// Operands:	--
	//						+-+-+-+-+-+-+-+-+
	//						|1|1|1|0|1|1|0|1| ED
	//						+-+-+-+-+-+-+-+-+
	//						|1|0|1|1|1|0|1|1| BB
	//						+-+-+-+-+-+-+-+-+
	//
	//							M Cycles		T States					MHz E.T.
	//								5						21 (4,5,3,4,5)		5.25		B != 0
	//								4						16 (4,5,3,4)			4.00		B == 0
	//
	uint32 tstates = 0;
	uint8 data;
	++++m_PC;
	do
	{
		IncrementR(2);
		m_addresshi = m_B;
		m_addresslo = m_C;
		ReadMemory(m_HL--, data);
		WritePort(m_address, data);
		tstates += 16;
	} while ((--m_B != 0) && ((tstates += 5), true));
	return tstates;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	8-Bit Load Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeLDrr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,%s", Get8BitRegisterString(m_pMemory[address] >> 3), Get8BitRegisterString(m_pMemory[address]));
	++address;
}

//=============================================================================

void CZ80::DecodeLDrn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,%02X", Get8BitRegisterString(m_pMemory[address] >> 3), m_pMemory[address + 1]);
	address += 2;
}

//=============================================================================

void CZ80::DecodeLDr_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,(IX+%02X)", Get8BitRegisterString(m_pMemory[address + 1] >> 3), m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeLDr_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,(IY+%02X)", Get8BitRegisterString(m_pMemory[address + 1] >> 3), m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeLD_HL_r(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (HL),%s", Get8BitRegisterString(m_pMemory[address++]));
}

//=============================================================================

void CZ80::DecodeLD_IXd_r(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (IX+%02X),%s", m_pMemory[address + 2], Get8BitRegisterString(m_pMemory[address + 1]));
	address += 3;
}

//=============================================================================

void CZ80::DecodeLD_IYd_r(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (IY+%02X),%s", m_pMemory[address + 2], Get8BitRegisterString(m_pMemory[address + 1]));
	address += 3;
}

//=============================================================================

void CZ80::DecodeLD_HL_n(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (HL),%02X", m_pMemory[address + 1]);
	address += 2;
}

//=============================================================================

void CZ80::DecodeLD_IXd_n(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (IX+%02X),%02X", m_pMemory[address + 2], m_pMemory[address + 3]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeLD_IYd_n(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (IY+%02X),%02X", m_pMemory[address + 2], m_pMemory[address + 3]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeLDA_BC_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD A,(BC)");
	++address;
}

//=============================================================================

void CZ80::DecodeLDA_DE_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD A,(DE)");
	++address;
}

//=============================================================================

void CZ80::DecodeLDA_nn_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD A,(%02X%02X)", m_pMemory[address + 2], m_pMemory[address + 1]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeLD_BC_A(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (BC),A");
	++address;
}

//=============================================================================

void CZ80::DecodeLD_DE_A(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (DE),A");
	++address;
}

//=============================================================================

void CZ80::DecodeLD_nn_A(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (%02X%02X),A", m_pMemory[address + 2], m_pMemory[address + 1]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeLDAI(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD A,I");
	++++address;
}

//=============================================================================

void CZ80::DecodeLDAR(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD A,R");
	++++address;
}

//=============================================================================

void CZ80::DecodeLDIA(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD I,A");
	++++address;
}

//=============================================================================

void CZ80::DecodeLDRA(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD R,A");
	++++address;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	16-Bit Load Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeLDddnn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,%02X%02X", Get16BitRegisterString(m_pMemory[address] >> 4), m_pMemory[address + 2], m_pMemory[address + 1]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeLDIXnn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD IX,%02X%02X", m_pMemory[address + 3], m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeLDIYnn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD IY,%02X%02X", m_pMemory[address + 3], m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeLDHL_nn_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD HL,(%02X%02X)", m_pMemory[address + 2], m_pMemory[address + 1]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeLDdd_nn_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,(%02X%02X)", Get16BitRegisterString(m_pMemory[address + 1] >> 4), m_pMemory[address + 3], m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeLDIX_nn_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD IX,(%02X%02X)", m_pMemory[address + 3], m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeLDIY_nn_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD IY,(%02X%02X)", m_pMemory[address + 3], m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeLD_nn_HL(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (%02X%02X),HL", m_pMemory[address + 2], m_pMemory[address + 1]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeLD_nn_dd(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (%02X%02X),%s", m_pMemory[address + 3], m_pMemory[address + 2], Get16BitRegisterString(m_pMemory[address + 1] >> 4));
	address += 4;
}

//=============================================================================

void CZ80::DecodeLD_nn_IX(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (%02X%02X),IX", m_pMemory[address + 3], m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeLD_nn_IY(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (%02X%02X),IY", m_pMemory[address + 3], m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeLDSPHL(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD SP,HL");
	++address;
}

//=============================================================================

void CZ80::DecodeLDSPIX(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD SP,IX");
	++address;
}

//=============================================================================

void CZ80::DecodeLDSPIY(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LD SP,IY");
	++address;
}

//=============================================================================

void CZ80::DecodePUSHqq(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "PUSH %s", Get16BitRegisterString(m_pMemory[address] >> 4));
	++address;
}

//=============================================================================

void CZ80::DecodePUSHAF(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "PUSH AF");
	++address;
}

//=============================================================================

void CZ80::DecodePUSHIX(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "PUSH IX");
	address += 2;
}

//=============================================================================

void CZ80::DecodePUSHIY(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "PUSH IY");
	address += 2;
}

//=============================================================================

void CZ80::DecodePOPqq(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "POP %s", Get16BitRegisterString(m_pMemory[address] >> 4));
	++address;
}

//=============================================================================

void CZ80::DecodePOPAF(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "POP AF");
	++address;
}

//=============================================================================

void CZ80::DecodePOPIX(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "POP IX");
	address += 2;
}

//=============================================================================

void CZ80::DecodePOPIY(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "POP IY");
	address += 2;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Exchange, Block Transfer and Search Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeEXDEHL(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "EX DE,HL");
	++address;
}

//=============================================================================

void CZ80::DecodeEXAFAF(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "EX AF,AF'");
	++address;
}

//=============================================================================

void CZ80::DecodeEXX(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "EXX");
	++address;
}

//=============================================================================

void CZ80::DecodeEX_SP_HL(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "EX (SP),HL");
	++address;
}

//=============================================================================

void CZ80::DecodeEX_SP_IX(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "EX (SP),IX");
	address += 2;
}

//=============================================================================

void CZ80::DecodeEX_SP_IY(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "EX (SP),IY");
	address += 2;
}

//=============================================================================

void CZ80::DecodeLDI(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LDI");
	address += 2;
}

//=============================================================================

void CZ80::DecodeLDIR(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LDIR");
	address += 2;
}

//=============================================================================

void CZ80::DecodeLDD(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LDD");
	address += 2;
}

//=============================================================================

void CZ80::DecodeLDDR(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "LDDR");
	address += 2;
}

//=============================================================================

void CZ80::DecodeCPI(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "CPI");
	address += 2;
}

//=============================================================================

void CZ80::DecodeCPIR(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "CPIR");
	address += 2;
}

//=============================================================================

void CZ80::DecodeCPD(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "CPD");
	address += 2;
}

//=============================================================================

void CZ80::DecodeCPDR(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "CPDR");
	address += 2;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	8-Bit Arithmetic Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeADDAr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "ADD A,%s", Get8BitRegisterString(m_pMemory[address++]));
}

//=============================================================================

void CZ80::DecodeADDAn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "ADD A,%02X", m_pMemory[++address]);
	++address;
}

//=============================================================================

void CZ80::DecodeADDA_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "ADD A,(IX+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeADDA_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "ADD A,(IY+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeADCAr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "ADC A,%s", Get8BitRegisterString(m_pMemory[address++]));
}

//=============================================================================

void CZ80::DecodeADCAn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "ADC A,%02X", m_pMemory[++address]);
	++address;
}

//=============================================================================

void CZ80::DecodeADCA_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "ADC A,(IX+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeADCA_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "ADC A,(IY+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeSUBr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SUB %s", Get8BitRegisterString(m_pMemory[address++]));
}

//=============================================================================

void CZ80::DecodeSUBn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SUB %02X", m_pMemory[++address]);
	++address;
}

//=============================================================================

void CZ80::DecodeSUB_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SUB (IX+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeSUB_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SUB (IY+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeSBCAr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SBC A,%s", Get8BitRegisterString(m_pMemory[address++]));
}

//=============================================================================

void CZ80::DecodeSBCAn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SBC A,%02X", m_pMemory[++address]);
	++address;
}

//=============================================================================

void CZ80::DecodeSBCA_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SBC A,(IX+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeSBCA_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SBC A,(IY+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeANDr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "AND %s", Get8BitRegisterString(m_pMemory[address++]));
}

//=============================================================================

void CZ80::DecodeANDn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "AND %02X", m_pMemory[++address]);
	++address;
}

//=============================================================================

void CZ80::DecodeAND_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "AND (IX+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeAND_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "AND (IY+%02X)", m_pMemory[address += 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeORr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "OR %s", Get8BitRegisterString(m_pMemory[address++]));
}

//=============================================================================

void CZ80::DecodeORn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "OR %02X", m_pMemory[++address]);
	++address;
}

//=============================================================================

void CZ80::DecodeOR_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "OR (IX+%02X)", m_pMemory[address += 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeOR_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "OR (IY+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeXORr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "XOR %s", Get8BitRegisterString(m_pMemory[address++]));
}

//=============================================================================

void CZ80::DecodeXORn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "XOR %02X", m_pMemory[++address]);
	++address;
}

//=============================================================================

void CZ80::DecodeXOR_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "XOR (IX+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeXOR_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "XOR (IY+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeCPr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "CP %s", Get8BitRegisterString(m_pMemory[address++]));
}

//=============================================================================

void CZ80::DecodeCPn(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "CP %02X", m_pMemory[++address]);
	++address;
}

//=============================================================================

void CZ80::DecodeCP_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "CP (IX+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeCP_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "CP (IY+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeINCr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "INC %s", Get8BitRegisterString(m_pMemory[address++] >> 3));
}

//=============================================================================

void CZ80::DecodeINC_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "INC (IX+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeINC_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "INC (IY+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeDECr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "DEC %s", Get8BitRegisterString(m_pMemory[address++] >> 3));
}

//=============================================================================

void CZ80::DecodeDEC_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "DEC (IX+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

void CZ80::DecodeDEC_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "DEC (IY+%02X)", m_pMemory[address + 2]);
	address += 3;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	General-Purpose Arithmetic and CPU Control Groups
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeDAA(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "DAA");
	++address;
}

//=============================================================================

void CZ80::DecodeCPL(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "CPL");
	++address;
}

//=============================================================================

void CZ80::DecodeNEG(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "NEG");
	address += 2;
}

//=============================================================================

void CZ80::DecodeCCF(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "CCF");
	++address;
}

//=============================================================================

void CZ80::DecodeSCF(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SCF");
	++address;
}

//=============================================================================

void CZ80::DecodeNOP(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "NOP");
	++address;
}

//=============================================================================

void CZ80::DecodeHALT(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "HALT");
	++address;
}

//=============================================================================

void CZ80::DecodeDI(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "DI");
	++address;
}

//=============================================================================

void CZ80::DecodeEI(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "EI");
	++address;
}

//=============================================================================

void CZ80::DecodeIM0(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "IM 0");
	address += 2;
}

//=============================================================================

void CZ80::DecodeIM1(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "IM 1");
	address += 2;
}

//=============================================================================

void CZ80::DecodeIM2(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "IM 2");
	address += 2;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	16-Bit Arithmetic Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeADDHLdd(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "ADD HL,%s", Get16BitRegisterString(m_pMemory[address++] >> 4));
}

//=============================================================================

void CZ80::DecodeADCHLdd(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "ADC HL,%s", Get16BitRegisterString(m_pMemory[++address] >> 4));
	++address;
}

//=============================================================================

void CZ80::DecodeSBCHLdd(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SBC HL,%s", Get16BitRegisterString(m_pMemory[++address] >> 4));
	++address;
}

//=============================================================================

void CZ80::DecodeADDIXdd(uint16& address, char* pMnemonic)
{
	uint8 opcode = m_pMemory[++address] >> 4;
	sprintf(pMnemonic, "ADD IX,%s", (opcode == 2) ? "IX" : Get16BitRegisterString(m_pMemory[address] >> 4));
	++address;
}

//=============================================================================

void CZ80::DecodeADDIYdd(uint16& address, char* pMnemonic)
{
	uint8 opcode = m_pMemory[++address] >> 4;
	sprintf(pMnemonic, "ADD IY,%s", (opcode == 2) ? "IY" : Get16BitRegisterString(m_pMemory[address] >> 4));
	++address;
}

//=============================================================================

void CZ80::DecodeINCdd(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "INC %s", Get16BitRegisterString(m_pMemory[address++] >> 4));
}

//=============================================================================

void CZ80::DecodeINCIX(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "INC IX");
	address += 2;
}

//=============================================================================

void CZ80::DecodeINCIY(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "INC IY");
	address += 2;
}

//=============================================================================

void CZ80::DecodeDECdd(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "DEC %s", Get16BitRegisterString(m_pMemory[address++] >> 4));
}

//=============================================================================

void CZ80::DecodeDECIX(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "DEC IX");
	address += 2;
}

//=============================================================================

void CZ80::DecodeDECIY(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "DEC IY");
	address += 2;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Rotate and Shift Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeRLCA(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RLCA");
	address += 1;
}

//=============================================================================

void CZ80::DecodeRLA(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RLA");
	address += 1;
}

//=============================================================================

void CZ80::DecodeRRCA(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RRCA");
	address += 1;
}

//=============================================================================

void CZ80::DecodeRRA(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RRA");
	address += 1;
}

//=============================================================================

void CZ80::DecodeRLCr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RLC %s", Get8BitRegisterString(m_pMemory[++address]));
	++address;
}

//=============================================================================

void CZ80::DecodeRLC_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RLC (IX+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeRLC_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RLC (IY+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeRLr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RL %s", Get8BitRegisterString(m_pMemory[++address]));
	++address;
}

//=============================================================================

void CZ80::DecodeRL_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RL (IX+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeRL_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RL (IY+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeRRCr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RRC %s", Get8BitRegisterString(m_pMemory[++address]));
	++address;
}

//=============================================================================

void CZ80::DecodeRRC_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RRC (IX+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeRRC_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RRC (IY+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeRRr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RR %s", Get8BitRegisterString(m_pMemory[++address]));
	++address;
}

//=============================================================================

void CZ80::DecodeRR_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RR (IX+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeRR_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RR (IY+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeSLAr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SLA %s", Get8BitRegisterString(m_pMemory[++address]));
	++address;
}

//=============================================================================

void CZ80::DecodeSLA_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SLA (IX+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeSLA_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SLA (IY+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeSRAr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SRA %s", Get8BitRegisterString(m_pMemory[++address]));
	++address;
}

//=============================================================================

void CZ80::DecodeSRA_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SRA (IX+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeSRA_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SRA (IY+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeSRLr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SRL %s", Get8BitRegisterString(m_pMemory[++address]));
	++address;
}

//=============================================================================

void CZ80::DecodeSRL_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SRL (IX+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeSRL_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SRL (IY+%02X)", m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeRLD(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RLD");
	address += 2;
}

//=============================================================================

void CZ80::DecodeRRD(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RRD");
	address += 2;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Bit Set, Reset and Test Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeBITbr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "BIT %d,%s", (m_pMemory[address + 1] >> 3) & 0x07, Get8BitRegisterString(m_pMemory[address + 1]));
	address += 2;
}

//=============================================================================

void CZ80::DecodeBITb_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "BIT %d,(IX+%02X)", (m_pMemory[address + 3] >> 3) & 0x07, m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeBITb_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "BIT %d,(IY+%02X)", (m_pMemory[address + 3] >> 3) & 0x07, m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeSETbr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SET %d,%s", (m_pMemory[address + 1] >> 3) & 0x07, Get8BitRegisterString(m_pMemory[address + 1]));
	address += 2;
}

//=============================================================================

void CZ80::DecodeSETb_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SET %d,(IX+%02X)", (m_pMemory[address + 3] >> 3) & 0x07, m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeSETb_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "SET %d,(IY+%02X)", (m_pMemory[address + 3] >> 3) & 0x07, m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeRESbr(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RES %d,%s", (m_pMemory[address + 1] >> 3) & 0x07, Get8BitRegisterString(m_pMemory[address + 1]));
	address += 2;
}

//=============================================================================

void CZ80::DecodeRESb_IXd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RES %d,(IX+%02X)", (m_pMemory[address + 3] >> 3) & 0x07, m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

void CZ80::DecodeRESb_IYd_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RES %d,(IY+%02X)", (m_pMemory[address + 3] >> 3) & 0x07, m_pMemory[address + 2]);
	address += 4;
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Jump Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeJPnn(uint16& address, char* pMnemonic)
{
	uint16 addr = m_pMemory[address + 1] + (static_cast<int16>(m_pMemory[address + 2]) << 8);
	address += 3;
	sprintf(pMnemonic, "JP %04X", addr);
}

//=============================================================================

void CZ80::DecodeJPccnn(uint16& address, char* pMnemonic)
{
	uint8 cc = (m_pMemory[address++] & 0x38) >> 3;
	uint16 addr = m_pMemory[address] + (static_cast<int16>(m_pMemory[address + 1]) << 8);
	address += 2;
	sprintf(pMnemonic, "JP %s,%04X", GetConditionString(cc), addr);
}

//=============================================================================

void CZ80::DecodeJRe(uint16& address, char* pMnemonic)
{
	address += 2;
#if defined(LEE_COMPATIBLE)
	sprintf(pMnemonic, "JR %02X", m_pMemory[address - 1]);
#else
	sprintf(pMnemonic, "JR %04X", address + static_cast<int8>(m_pMemory[address - 1]));
#endif
}

//=============================================================================

void CZ80::DecodeJRCe(uint16& address, char* pMnemonic)
{
	address += 2;
#if defined(LEE_COMPATIBLE)
	sprintf(pMnemonic, "JR C,%02X", m_pMemory[address - 1]);
#else
	sprintf(pMnemonic, "JR C,%04X", address + static_cast<int8>(m_pMemory[address - 1]));
#endif
}

//=============================================================================

void CZ80::DecodeJRNCe(uint16& address, char* pMnemonic)
{
	address += 2;
#if defined(LEE_COMPATIBLE)
	sprintf(pMnemonic, "JR NC,%02X", m_pMemory[address - 1]);
#else
	sprintf(pMnemonic, "JR NC,%04X", address + static_cast<int8>(m_pMemory[address - 1]));
#endif
}

//=============================================================================

void CZ80::DecodeJRZe(uint16& address, char* pMnemonic)
{
	address += 2;
#if defined(LEE_COMPATIBLE)
	sprintf(pMnemonic, "JR Z,%02X", m_pMemory[address - 1]);
#else
	sprintf(pMnemonic, "JR Z,%04X", address + static_cast<int8>(m_pMemory[address - 1]));
#endif
}

//=============================================================================

void CZ80::DecodeJRNZe(uint16& address, char* pMnemonic)
{
	address += 2;
#if defined(LEE_COMPATIBLE)
	sprintf(pMnemonic, "JR NZ,%02X", m_pMemory[address - 1]);
#else
	sprintf(pMnemonic, "JR NZ,%04X", address + static_cast<int8>(m_pMemory[address - 1]));
#endif
}

//=============================================================================

void CZ80::DecodeJP_HL_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "JP (HL)");
	address += 1;
}

//=============================================================================

void CZ80::DecodeJP_IX_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "JP (IX)");
	address += 2;
}

//=============================================================================

void CZ80::DecodeJP_IY_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "JP (IY)");
	address += 2;
}

//=============================================================================

void CZ80::DecodeDJNZe(uint16& address, char* pMnemonic)
{
	address += 2;
#if defined(LEE_COMPATIBLE)
	sprintf(pMnemonic, "DJNZ %02X", m_pMemory[address - 1]);
#else
	sprintf(pMnemonic, "DJNZ %04X", address + static_cast<int8>(m_pMemory[address - 1]));
#endif
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Call and Return Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeCALLnn(uint16& address, char* pMnemonic)
{
	uint16 addr = m_pMemory[address + 1] + (static_cast<int16>(m_pMemory[address + 2]) << 8);
	sprintf(pMnemonic, "CALL %04X", addr);
	address += 3;
}

//=============================================================================

void CZ80::DecodeCALLccnn(uint16& address, char* pMnemonic)
{
	uint8 cc = (m_pMemory[address] & 0x38) >> 3;
	uint16 addr = m_pMemory[address + 1] + (static_cast<int16>(m_pMemory[address + 2]) << 8);
	sprintf(pMnemonic, "CALL %s,%04X", GetConditionString(cc), addr);
	address += 3;
}

//=============================================================================

void CZ80::DecodeRET(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RET");
	++address;
}

//=============================================================================

void CZ80::DecodeRETcc(uint16& address, char* pMnemonic)
{
	uint8 cc = (m_pMemory[address++] & 0x38) >> 3;
	sprintf(pMnemonic, "RET %s", GetConditionString(cc));
}

//=============================================================================

void CZ80::DecodeRETI(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RETI");
	address += 2;
}

//=============================================================================

void CZ80::DecodeRETN(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "RETN");
	address += 2;
}

//=============================================================================

void CZ80::DecodeRSTp(uint16& address, char* pMnemonic)
{
	uint8 location = ((m_pMemory[address++] & 0x38) >> 3) * 8;
	sprintf(pMnemonic, "RST $%02X", location);
}

//=============================================================================

//-----------------------------------------------------------------------------
//	Input and Output Group
//-----------------------------------------------------------------------------

//=============================================================================

void CZ80::DecodeINA_n_(uint16& address, char* pMnemonic)
{
	uint8 n = m_pMemory[address + 1];
	sprintf(pMnemonic, "IN A,(%02X)", n);
	address += 2;
}

//=============================================================================

void CZ80::DecodeINr_C_(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "IN %s,(C)", Get8BitRegisterString(m_pMemory[address + 1] >> 3));
	address += 2;
}

//=============================================================================

void CZ80::DecodeINI(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "INI");
	address += 2;
}

//=============================================================================

void CZ80::DecodeINIR(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "INIR");
	address += 2;
}

//=============================================================================

void CZ80::DecodeIND(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "IND");
	address += 2;
}

//=============================================================================

void CZ80::DecodeINDR(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "INDR");
	address += 2;
}

//=============================================================================

void CZ80::DecodeOUT_n_A(uint16& address, char* pMnemonic)
{
	uint8 n = m_pMemory[address + 1];
	sprintf(pMnemonic, "OUT (%02X),A", n);
	address += 2;
}

//=============================================================================

void CZ80::DecodeOUT_C_r(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "OUT (C),%s", Get8BitRegisterString(m_pMemory[address + 1] >> 3));
	address += 2;
}

//=============================================================================

void CZ80::DecodeOUTI(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "OUTI");
	address += 2;
}

//=============================================================================

void CZ80::DecodeOTIR(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "OTIR");
	address += 2;
}

//=============================================================================

void CZ80::DecodeOUTD(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "OUTD");
	address += 2;
}

//=============================================================================

void CZ80::DecodeOTDR(uint16& address, char* pMnemonic)
{
	sprintf(pMnemonic, "OTDR");
	address += 2;
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
