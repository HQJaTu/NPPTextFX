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
	TEST_CLASS(EditTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In EditTest Class Initialize");
			/*
			23 functions:
			"E:Fill Down Insert"),pffilldownins
			"E:Fill Down Overwrite"),pffilldownover
			"E:Insert (Clipboard) through lines"),pfinsertclipboardcolumn
			"E:Reindent C++ code"),pfreindentcode
			"E:Leading space to tabs or tabs to spaces"),pfspace2tabs
			"E:Leading space to tabs or tabs to spaces width=8"),pfspace2tabs8
			"E:Trim Trailing Spaces"),pftrimtrailingspace // Notepad++ 3.2 is slow and only trims the entire file
			"E:Indent text sticky left margin"),pfindentlines
			"E:Indent && surround { text lines }"),pfindentlinessurround
			"E:Delete Blank Lines"),pfdeleteblanklines
			"E:Delete Surplus Blank Lines"),pfdeleteblanklines2
			"E:Strip unquoted text (VB) separate by (Clipboard<=20)"),pffindqtstringvb
			"E:Strip unquoted text (C) separate by (Clipboard<=20)"),pffindqtstringc
			"E:Kill unquoted (VB) whitespace"),pfkillwhitenonqtvb
			"E:Kill unquoted (C) whitespace"),pfkillwhitenonqtc
			"E:Split lines at (clipboard character) or , (VB)"),pfsplitlinesatchvb
			"E:Split lines at (clipboard character) or , (C)"),pfsplitlinesatchc
			"E:Line up multiple lines by (,)"),pflineupcomma
			"E:Line up multiple lines by (=)"),pflineupequals
			"E:Line up multiple lines by (Clipboard Character)"),pflineupclipboard
			"E:Unwrap Text"),pfunwraptext
			"E:ReWrap Text to (Clipboard or 72) width"),pfrewraptext
			"E:Pad rectangular selection with spaces"),pfextendblockspaces
			*/
		}


	};
}