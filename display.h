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
	uint32 m_videoMemory[WIDTH * HEIGHT];
//	uint32 m_textureID;
};
