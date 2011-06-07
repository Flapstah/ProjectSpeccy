#if !defined(__PLATFORM_TYPES_H__)
#define __PLATFORM_TYPES_H__

//==============================================================================

typedef signed char				int8;
typedef signed short			int16;
typedef signed int				int32;
typedef signed __int64		int64;

typedef unsigned char			uint8;
typedef unsigned short		uint16;
typedef unsigned int			uint32;
typedef unsigned __int64	uint64;

template <size_t _size>
union mchar_t
{
	wchar_t	m_UTF16[_size];
	char		m_UTF8[_size * sizeof(wchar_t)];
};

//==============================================================================

#endif // End [!defined(__PLATFORM_TYPES_H__)]
// [EOF]