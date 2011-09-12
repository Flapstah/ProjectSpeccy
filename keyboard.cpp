#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glfw.h>

#include "keyboard.h"

//=============================================================================

bool CKeyboard::s_keyState[512];
bool CKeyboard::s_keyPrevState[512];

//=============================================================================

void CKeyboard::Initialise(void)
{
	memset(s_keyState, 0, sizeof(s_keyState) * sizeof(bool));
	memset(s_keyPrevState, 0, sizeof(s_keyPrevState) * sizeof(bool));

	glfwSetKeyCallback(Update);
}

//=============================================================================

void CKeyboard::Uninitialise(void)
{
}

//=============================================================================

void CKeyboard::Update(int key, int action)
{
	s_keyState[key] = (action == GLFW_PRESS) ? true : false;
//	if (key >= GLFW_KEY_SPECIAL)
//	{
//		fprintf(stderr, "key %d, %s\n", key, (action == GLFW_PRESS) ? "pressed" : "released");
//	}
//	else
//	{
//		fprintf(stderr, "key '%c', %s\n", key, (action == GLFW_PRESS) ? "pressed" : "released");
//	}
}

//=============================================================================

bool CKeyboard::IsKeyPressed(int key)
{
	bool pressed = (s_keyState[key] && !s_keyPrevState[key]);
	s_keyPrevState[key] = s_keyState[key];
	return pressed;
}

//=============================================================================

bool CKeyboard::IsKeyHeld(int key)
{
	bool held = (s_keyState[key] && s_keyPrevState[key]);
	s_keyPrevState[key] = s_keyState[key];
	return held;
}

//=============================================================================

