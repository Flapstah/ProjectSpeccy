#include "z80.h"

bool CZ80::Decode(void*& pAddress)
{
	return true;
}

void CZ80::ImplementLDrr(void)
{
	// M Cycles 1
	// T States 4
	uint8* pDestination = DecodeRegister(*m_register.PC >> 3);
	uint8* pSource = DecodeRegister(*m_register.PC++);
	*pDestination = *pSource;
}

void CZ80::ImplementLDrHL(void)
{
	// M Cycles 2
	// T States 7 (4, 3)
	uint8* pDestination = DecodeRegister(*m_register.PC >> 3);
	uint8* pSource = DecodeRegister(*m_register.PC++);
	*pDestination = *pSource;
}

void CZ80::ImplementLDrn(void)
{
	// M Cycles 2
	// T States 7 (4, 3)
	uint8* pDestination = DecodeRegister(*m_register.PC++ >> 3);
	*pDestination = *m_register.PC++;
}

void CZ80::DecodeLD(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,%s", DecodeRegister(*pAddress >> 3), DecodeRegister(*(pAddress + 1)));
}

void CZ80::DecodeLDn(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,%d", DecodeRegister(*pAddress >> 3), *(pAddress + 1));
}

void CZ80::DecodeLDrIXd(void)
{
	// M Cycles 5
	// T States 19 (4, 4, 3, 5, 3)
	uint8* pDestination = DecodeRegister(*++m_register.PC++ >> 3);
	uint8* pSource = static_cast<uint8*>(m_register.IX + *m_register.PC++);
	*pDestination = *pSource;
}

void CZ80::DecodeLDrIXd(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,(IX+%d)", DecodeRegister(*(pAddress + 1) >> 3), *(pAddress + 2));
}

void CZ80::DecodeLDrIYd(void)
{
	// M Cycles 5
	// T States 19 (4, 4, 3, 5, 3)
	uint8* pDestination = DecodeRegister(*++m_register.PC++ >> 3);
	uint8* pSource = static_cast<uint8*>(m_register.IY + *m_register.PC++);
	*pDestination = *pSource;
}

void CZ80::DecodeLDrIYd(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD %s,(IY+%d)", DecodeRegister(*(pAddress + 1) >> 3), *(pAddress + 2));
}

void CZ80::ImplementLDHLr(void)
{
	// M Cycles 2
	// T States 7 (4, 3)
	uint8* pDestination = (static_cast<uint16>(m_register.H) << 0x100) | m_register.L;
	uint8* pSource = DecodeRegister(*m_register.PC++);
	*pDestination = *pSource;
}

void CZ80::ImplementLDHLr(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (HL),%s", DecodeRegister(*pAddress));
}

void CZ80::ImplementLDIXdr(void)
{
	// M Cycles 5
	// T States 19 (4, 4, 3, 5, 3)
	uint8* pSource = DecodeRegister(*++m_register.PC++ >> 3);
	uint8* pDestination = static_cast<uint8*>(m_register.IX + *m_register.PC++);
	*pDestination = *pSource;
}

void CZ80::DecodeLDIXdr(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (IX+%d),%s", *(pAddress + 2), DecodeRegister(*(pAddress + 1) >> 3));
}

void CZ80::ImplementLDIYdr(void)
{
	// M Cycles 5
	// T States 19 (4, 4, 3, 5, 3)
	uint8* pSource = DecodeRegister(*++m_register.PC++ >> 3);
	uint8* pDestination = static_cast<uint8*>(m_register.IY + *m_register.PC++);
	*pDestination = *pSource;
}

void CZ80::DecodeLDIYdr(const uint8* pAddress, char* pMnemonic)
{
	sprintf(pMnemonic, "LD (IY+%d),%s", *(pAddress + 2), DecodeRegister(*(pAddress + 1) >> 3));
}












 
void CZ80::ImplementINC(void)
{
	// TODO: Flags
	uint8* pDestination = DecodeRegister(*m_register.PC >> 3);
	++*pDestination;
}

void CZ80::DecodeINC(void* pAddress, char* pMnemonic)
{
	*pMnemonic++ = 'I';
	*pMnemonic++ = 'N';
	*pMnemonic++ = 'C';
	*pMnemonic++ = ' ';
	DecodeRegister(*pAddress >> 3);
}

void CZ80::ImplementDEC(void)
{
	// TODO: Flags
	uint8* pDestination = DecodeRegister(*m_register.PC >> 3);
	--*pDestination;
}

void CZ80::DecodeDEC(void* pAddress, char* pMnemonic)
{
	*pMnemonic++ = 'D';
	*pMnemonic++ = 'E';
	*pMnemonic++ = 'C';
	*pMnemonic++ = ' ';
	DecodeRegister(*pAddress >> 3);
}

void CZ80::ImplementADD(void)
{
	// TODO: Flags
	uint8* pSource = DecodeAddress(*m_register.PC);
	m_register.A += *pSource;
}

void CZ80::DecodeADD(void* pAddress, char* pMnemonic)
{
	*pMnemonic++ = 'A';
	*pMnemonic++ = 'D';
	*pMnemonic++ = 'D';
	*pMnemonic++ = ' ';
	*pMnemonic++ = 'A';
	*pMnemonic++ = ',';
	DecodeRegister(*pAddress);
}

void CZ80::ImplementADC(void)
{
	// TODO: Flags
	uint8* pSource = DecodeAddress(*m_register.PC);
	m_register.A += *pSource;
}

void CZ80::DecodeADC(void* pAddress, char* pMnemonic)
{
	*pMnemonic++ = 'A';
	*pMnemonic++ = 'D';
	*pMnemonic++ = 'C';
	*pMnemonic++ = ' ';
	*pMnemonic++ = 'A';
	*pMnemonic++ = ',';
	DecodeRegister(*pAddress);
}

void CZ80::ImplementSUB(void)
{
	// TODO: Flags
	uint8* pSource = DecodeAddress(*m_register.PC);
	m_register.A -= *pSource;
}

void CZ80::DecodeSUB(void* pAddress, char* pMnemonic)
{
	*pMnemonic++ = 'S';
	*pMnemonic++ = 'U';
	*pMnemonic++ = 'B';
	*pMnemonic++ = ' ';
	*pMnemonic++ = 'A';
	*pMnemonic++ = ',';
	DecodeRegister(*pAddress);
}

void CZ80::ImplementSBC(void)
{
	// TODO: Flags
	uint8* pSource = DecodeAddress(*m_register.PC);
	m_register.A -= *pSource;
}

void CZ80::DecodeSBC(void* pAddress, char* pMnemonic)
{
	*pMnemonic++ = 'S';
	*pMnemonic++ = 'B';
	*pMnemonic++ = 'C';
	*pMnemonic++ = ' ';
	*pMnemonic++ = 'A';
	*pMnemonic++ = ',';
	DecodeRegister(*pAddress);
}

void CZ80::ImplementAND(void)
{
	// TODO: Flags
	uint8* pSource = DecodeAddress(*m_register.PC);
	m_register.A &= *pSource;
}

void CZ80::DecodeAND(void)
{
	*pMnemonic++ = 'A';
	*pMnemonic++ = 'N';
	*pMnemonic++ = 'D';
	*pMnemonic++ = ' ';
	DecodeRegister(*pAddress);
}

void CZ80::ImplementXOR(void)
{
	// TODO: Flags
	uint8* pSource = DecodeAddress(*m_register.PC);
	m_register.A ^= *pSource;
}

void CZ80::DecodeXOR(void)
{
	*pMnemonic++ = 'X';
	*pMnemonic++ = 'O';
	*pMnemonic++ = 'R';
	*pMnemonic++ = ' ';
	DecodeRegister(*pAddress);
}

void CZ80::ImplementOR(void)
{
	// TODO: Flags
	uint8* pSource = DecodeAddress(*m_register.PC);
	m_register.A |= *pSource;
}

void CZ80::DecodeOR(void)
{
	*pMnemonic++ = 'O';
	*pMnemonic++ = 'R';
	*pMnemonic++ = ' ';
	DecodeRegister(*pAddress);
}

void CZ80::ImplementCP(void)
{
	// TODO: Flags
//	uint8* pSource = DecodeAddress(*m_register.PC);
//	m_register.A |= *pSource;
}

void CZ80::DecodeCP(void)
{
	*pMnemonic++ = 'C';
	*pMnemonic++ = 'P';
	*pMnemonic++ = ' ';
	DecodeRegister(*pAddress);
}

void CZ80::ImplementHALT(void)
{
}

void CZ80::DecodeHALT(void* pAddress, char* pMnemonic)
{
	*pMnemonic++ = 'H';
	*pMnemonic++ = 'A';
	*pMnemonic++ = 'L';
	*pMnemonic++ = 'T';
}

uint8* DecodeRegister(uint8 threeBits)
{
	uint8* pAddress = NULL;

	switch (threeBits)
	{
		case 0: pAddress = &m_register.B; break;
		case 1: pAddress = &m_register.C; break;
		case 2: pAddress = &m_register.D; break;
		case 3: pAddress = &m_register.E; break;
		case 4: pAddress = &m_register.H; break;
		case 5: pAddress = &m_register.L; break;
		case 6: pAddress = (static_cast<uint16>(m_register.H) << 0x100) | m_register.L; break;
		case 7: pAddress = &m_register.A; break;
		default: break;
	}

	return pAddress;
}

const char* DecodeRegister(uint8 threeBits, char* pMnemonic)
{
	switch (threeBits)
	{
		case 0: return "B";			break;
		case 1: return "C";			break;
		case 2: return "D";			break;
		case 3: return "E";			break;
		case 4: return "H";			break;
		case 5: return "L";			break;
		case 6: return "(HL)";	break;
		case 7: return "A";			break;
		default: break;
	}
}

bool CZ80::Execute(void)
{

	return true;
}


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
