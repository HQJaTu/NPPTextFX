#pragma once
#include "stdafx.h"
#include "CppUnitTest.h"

#include <windows.h>
#include <tchar.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define EXTERNC extern "C"

namespace Tests
{
#define TEST_BUFFER_CHARS 8192
	EXTERNC char textBuffer[TEST_BUFFER_CHARS];
	EXTERNC unsigned anchorPosition;
	EXTERNC unsigned currentPosition;

	LRESULT mockSendScintillaMessage(BOOL which, UINT Msg, WPARAM wParam, LPARAM lParam);
	void setText(const TCHAR* textIn);
	TCHAR* convertTextbuffer(const char* input = NULL);
	void resetScintillaSimulator();
}

