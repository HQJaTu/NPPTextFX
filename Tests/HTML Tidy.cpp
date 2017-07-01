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
	TEST_CLASS(HTMLTidyTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In HTMLTidyTest Class Initialize");
			/*
			6 functions:
			"D:About Tidy"),pfabouttidy
			"D:Visit HTML Tidy SourceForge website"),pfhtmltidyweb
			"D:Download libTidy.DLL from SourceForge"),pfgethtmltidyweb
			"D:Reload libTidy.DLL"),pfreloadtidydll
			"D:Refresh Menu from ") NPPTIDY_INI_MENU,pfmpxtidyrebuild
			"D:Tidy (most recent ") NPPTIDY_CFG_MENU NPPTEXT(")"),pfhtmltidy
			*/
		}


	};
}