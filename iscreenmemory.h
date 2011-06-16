#if !defined (__ISCREENMEMORY_H__)
#define __ISCREENMEMORY_H__

#include "common/platform_types.h"

struct IScreenMemory
{
	virtual					~IScreenMemory(void) {};

	virtual	const void*	GetScreenMemory(void) const = 0;
	virtual	uint32			GetScreenWidth(void) const = 0;
	virtual	uint32			GetScreenHeight(void) const = 0;
};

#endif // !defined (__ISCREENMEMORY_H__)
