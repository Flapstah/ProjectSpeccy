#include "common/platform_types.h"

#define WIDTH (256)
#define HEIGHT (192)

class CDisplay
{
public:
	CDisplay(uint32 width, uint32 height, const char* title);
	~CDisplay(void);

	bool Update(void);

protected:
	enum eState
	{
		eS_Uninitialised,
		eS_Initialised,
		eS_Window
	};

	eState m_state;
	uint32 m_videoMemory[WIDTH * HEIGHT];
	uint32 m_textureID;
};
