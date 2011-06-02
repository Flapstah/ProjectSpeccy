#if !defined(__PLATFORM_TYPES_H__)
#define __PLATFORM_TYPES_H__

//==============================================================================

#include <stdint.h>

//==============================================================================

typedef int8_t	  int8;
typedef int16_t	  int16;
typedef int32_t	  int32;
typedef int64_t	  int64;

typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef uint64_t  uint64;

template <size_t _size>
union mchar_t
{
	wchar_t	m_UTF16[_size];
	char		m_UTF8[_size * sizeof(wchar_t)];
};

//==============================================================================

#endif // End [!defined(__PLATFORM_TYPES_H__)]
// [EOF]
