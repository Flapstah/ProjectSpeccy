#if !defined(__IMEMORY_H__)
#define __IMEMORY_H__

struct IMemory
{
	virtual void WriteMemory(uint16 address, uint8 byte) = 0;
	virtual uint8 ReadMemory(uint16 address) const = 0;
	virtual void WritePort(uint16 address, uint8 byte) = 0;
	virtual uint8 ReadPort(uint16 address) const = 0;
};

#endif // !defined(__IMEMORY_H__)
