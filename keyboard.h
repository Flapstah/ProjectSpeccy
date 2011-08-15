#if !defined(__KEYBOARD_H__)
#define __KEYBOARD_H__

class CKeyboard
{
public:
	static	bool				Initialise(void);
	static	bool				Uninitialise(void);

	static	void				Update(int key, int action);
	static	bool				IsKeyPressed(int key);
	static	bool				IsKeyHeld(int key);

protected:
	static	bool				s_keyState[512];
	static	bool				s_keyPrevState[512];
};

#endif // !defined(__KEYBOARD_H__)

