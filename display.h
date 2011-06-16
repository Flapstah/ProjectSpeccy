#include "common/platform_types.h"

#define WIDTH (256)
#define HEIGHT (192)
#define ATTR_SIZE (768)

class CDisplay
{
public:
	CDisplay(uint32 width, uint32 height, const char* title);
	~CDisplay(void);

	bool Update(void);

	bool OpenSCR(const char* fileName);

protected:
	uint32 PixelByteIndex(uint8 x, uint8 y);
	uint32 AttributeByteIndex(uint8 x, uint8 y);
	void UpdateScreen(const uint8* pScreenMemory);

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

	uint32 m_videoMemory[WIDTH * HEIGHT];
	eState m_state;
	float m_displayScale;
//	uint32 m_textureID;
};
