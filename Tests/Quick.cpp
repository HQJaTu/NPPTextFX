#include "stdafx.h"
#include "CppUnitTest.h"

#include <windows.h>
#include <wchar.h>
#include <tchar.h>

#define EXTERNC extern "C"
#include "..\Src\NPPTextFX.h"
#include "scintilla_simu.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Tests
{
	TEST_CLASS(QuickTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In QuickTest Class Initialize");
			Logger::WriteMessage("ClassInitialize: Set Scintilla Message Sender as: mockSendScintillaMessage()");
			::gScintillaMessageSender = &mockSendScintillaMessage;
			resetScintillaSimulator();

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

		TEST_METHOD(MarkWordOrFindReverseTest)
		{
			resetScintillaSimulator();
			//                                    012345678901234567
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			currentPosition = 11;
			anchorPosition = currentPosition;

			// Select word "test":
			// Forward or reverse has no meaning in this test. A word get selected.
			setText(testTextNoChanges);
			::pfMarkWordFindReverse();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());
			Assert::AreEqual(9, (int)currentPosition);
			Assert::AreEqual(14, (int)anchorPosition);

			// Do a reverse search for "te".
			// 1) Select "te" from word, "text"
			// 2) Reverse find must select "te" from word, "test"
			currentPosition = 15;
			anchorPosition = currentPosition + 2;
			::pfMarkWordFindReverse();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());
			Assert::AreEqual(10, (int)currentPosition);
			Assert::AreEqual(12, (int)anchorPosition);
		}

		TEST_METHOD(MarkWordOrFindForwardTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			currentPosition = 11;
			anchorPosition = currentPosition;

			// Select word "test":
			// Forward or reverse has no meaning in this test. A word get selected.
			setText(testTextNoChanges);
			::pfMarkWordFindForward();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());
			Assert::AreEqual(14, (int)currentPosition);
			Assert::AreEqual(9, (int)anchorPosition);

			// Select word "This":
			currentPosition = 0;
			anchorPosition = currentPosition;
			::pfMarkWordFindForward();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());
			Assert::AreEqual(4, (int)currentPosition);
			Assert::AreEqual(0, (int)anchorPosition);
		}

		TEST_METHOD(MarkWordOrFindReverseCaseTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a teSt text.");
			currentPosition = 11;
			anchorPosition = currentPosition;

			// Select word "test":
			setText(testTextNoChanges);
			::pfMarkWordFindReverse();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());
			Assert::AreEqual(9, (int)currentPosition);
			Assert::AreEqual(14, (int)anchorPosition);

			// Select word "test":
			::pfMarkWordFindCaseSensitive();
			::pfMarkWordFindReverse();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());
			Assert::AreEqual(9, (int)currentPosition);
			Assert::AreEqual(14, (int)anchorPosition);

			// Select word "This":
			currentPosition = 0;
			anchorPosition = currentPosition;
			::pfMarkWordFindReverse();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());
			Assert::AreEqual(0, (int)currentPosition);
			Assert::AreEqual(4, (int)anchorPosition);
		}

	};
}