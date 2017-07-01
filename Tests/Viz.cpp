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
	TEST_CLASS(VizTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In VizTest Class Initialize");

			/*
			26 functions:
			"V:Show Between-Selected or All-Reset Lines"),pfvizshowselectedalllines
			"V:Hide Between-Selected or All-Reset Lines"),pfvizhideselectedalllines
			"V:Invert Visibility Between-Selected or All Lines"),pfvizinvertselectedalllines
			"V:Hide Lines with (Clipboard) text"),pfvizhidecliplines
			"V:Hide Lines without (Clipboard) text"),pfvizhideclipclines
			"V:Show Lines with (Clipboard) text"),pfvizshowcliplines
			"V:Show Lines without (Clipboard) text"),pfvizshowclipclines
			"V:Show More Lines around my position..."),pfvizshowmorelines
			"V:Hide/Show sequence all steps"),pfvizsequenceall
			"V:Hide/Show sequence singlestep start"),pfvizsequencestart
			"V:Hide/Show sequence singlestep next"),pfvizsequencenext
			"V:Hide/Show sequence singlestep rest"),pfvizsequencerest
			"V:Select as Hide/Show sequence"),pfvizselectassequence
			"V:Insert Show/Hide Sequence"),pfvizinsertsequence
			"V:Copy Visible Selection"),pfvizcopyvisible
			"V:Cut Visible Selection"),pfvizcutvisible
			"V:Delete Visible Selection"),pfvizdeletevisible
			"V:Copy Invisible Selection"),pfvizcopyinvisible
			"V:Cut Invisible Selection"),pfvizcutinvisible
			"V:Delete Invisible Selection"),pfvizdeleteinvisible
			"V:Copy Entire Selection (no append)"),pfcopyallnoappend
			"V:Cut Entire Selection (no append)"),pfcutallnoappend
			"V:Copy && Append Entire Selection"),pfcopyallappend
			"V:Cut && Append Entire Selection"),pfcutallappend
			"V:Paste as UTF-8/ANSI"),pfVizPasteUTF8
			"V:Paste"),pfVizPasteUNICODE
			*/
		}


	};
}