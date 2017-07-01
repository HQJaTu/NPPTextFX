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
	TEST_CLASS(ToolsTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In ToolsTest Class Initialize");

			/*
			14 functions:
			"T:Sort lines case sensitive (at column)"),pfqsortlinesc
			"T:Sort lines case insensitive (at column)"),pfqsortlinesnc
			"T:+Sort ascending"),pfSortAscending,0,TRUE , NULL},
			"T:+Sort outputs only UNIQUE (at column) lines"),pfSortLinesUnique,0,TRUE , NULL},
			"T:Insert Ascii Chart or Character"),pfinsertasciichart
			"T:Insert Ruler"),pfinsertruler
			"T:Insert Line Numbers"),pfinsertlinenumbers
			"T:Delete Line Numbers or First Word"),pfdeletefirstword
			"T:Clean eMail >Quoting"),pfcleanemailquoting
			"T:UUdecode"),pfuudecode
			"T:Base64 Decode"),pfbase64decode
			"T:Word Count"),pfwordcount
			"T:Add up numbers"),pfaddup
			"T:Empty Undo Buffer (be sure to save)"),pfemptyundobuffer
			*/
		}


	};
}