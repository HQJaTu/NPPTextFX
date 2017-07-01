#include "stdafx.h"
#include "CppUnitTest.h"

#include <windows.h>
#include <wchar.h>
#include <tchar.h>

#define EXTERNC extern "C"
#include "..\Src\NPPTextFX.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Tests
{
	TEST_CLASS(InsertTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In InsertTest Class Initialize");

			/*
			5 functions:
			"I:Current Full Path"),pfinsertCurrentFullPath
			"I:Current File Name"),pfinsertCurrentFileName
			"I:Current Directory"),pfinsertCurrentDirectory
			"I:Date && Time - short format"),pfinsertShortDateTime
			"I:Date && Time - long format"),pfinsertLongDateTime
			*/
		}


	};
}