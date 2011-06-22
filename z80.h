#if !defined(__Z80_H__)
#define __Z80_H__

#include "common/platform_types.h"

class CZ80
{
	public:
	protected:
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

			// N.B. The only way to read XF, YF and NF is PUSH AF
		};

		// TODO: Define a union for uint16 and 2x uint8 to describe a register

		struct SRegisters
		{
			uint8 A;
			uint8 F;
			uint8 B;
			uint8 C;
			uint8 D;
			uint8 E;
			uint8 H;
			uint8 L;
			uint16 IX;
			uint16 IY;
			uint16 PC;
			uint16 SP;
			uint8 I;
			uint8 R;
			uint16 AF_;
			uint16 BC_;
			uint16 DE_;
			uint16 HL_;
		} m_register;

		uint32 m_tstate;

	private:

};

#endif // !defined(__Z80_H__)
