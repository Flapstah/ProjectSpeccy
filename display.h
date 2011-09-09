#if !defined(__DISPLAY_H__)
#define __DISPLAY_H__

#include "common/platform_types.h"

#include "iscreenmemory.h"

class CDisplay
{
public:
	CDisplay(uint32 width, uint32 height, const char* title);
	~CDisplay(void);

	bool Update(IScreenMemory* pScreenMemory);
	void SetDisplayScale(float scale);
	float GetDisplayScale(void);

protected:

	enum eState
	{
		eS_Uninitialised,
		eS_Initialised,
		eS_Window
	};

	enum eTextureID
	{
		eTID_Main = 1
	};

	eState m_state;
	float m_displayScale;
};

#endif // !defined(__DISPLAY_H__)
