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
	TEST_CLASS(VizSettingsTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In VizSettingsTest Class Initialize");

			/*
			12 functions:
			"W:+Viz Text Search Case Sensitive"),pfVizCaseSensitive
			"W:+Viz Text Search Whole Words"),pfVizWholeWords
			"W:+Viz Text Search Regex"),pfVizRegex
			"W:+Viz Copy-Cut Appends to clipboard"),pfVizCutCopyAppend
			"W:+Viz Copy-Cut always converts to CRLF"),pfVizClipboardAlwaysCRLF
			"W:+Viz Copy-Cut replace [NUL] with space"),pfVizClipboardReplaceNulls
			"W:+Viz Copy-Cut also in UTF-8"),pfVizClipboardCopyAlsoUTF8
			"W:+Viz Copy-Cut not in UNICODE"),pfVizClipboardNotUnicode
			"W:+Viz Paste retains position"),pfVizPasteRetainsPosition
			"W:+Viz Paste/Append binary"),pfVizPasteBinary
			"W:+Viz Paste converts EOL to editor"),pfVizPasteToEditorEOL
			"W:+Viz Capture Keyboard Ctrl-C,X,V"),pfCaptureCutCopyPaste
			*/
		}


	};
}