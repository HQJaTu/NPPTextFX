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
	TEST_CLASS(QuickTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In QuickTest Class Initialize");
			/*
			10 functions:
			"Q:Mark Word or Find Reverse"),pfMarkWordFindReverse
			"Q:Mark Word or Find Forward"),pfMarkWordFindForward
			"Q:+Mark Word or Find Case Sensitive"),pfMarkWordFindCaseSensitive
			"Q:+Mark Word or Find Whole Words"),pfMarkWordFindWholeWord
			"Q:Find matching {([<Brace>])}"),pffindmatchchar,0,FALSE, &skfindmatchchar},//Ctrl-B in Notepad++
			"Q:Mark to matching {([<Brace>])}"),pfmarkmatchchar
			"Q:Delete Marked {([<Brace>])} Pair"),pfdeletebracepair
			"Q:Mark lines to matching {([<Brace>])}"),pfmarkmatchline
			"Q:Find/Replace"),pffindreplace
			"Q:Duplicate Line or Block"),pfDuplicateLineOrBlock
			*/
		}


	};
}