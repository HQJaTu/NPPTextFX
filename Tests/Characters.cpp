#include "stdafx.h"
#include "CppUnitTest.h"

#include <windows.h>
#include <wchar.h>
#include <tchar.h>

#include "..\SRC\Scintilla.h"

#define EXTERNC extern "C"
#include "..\Src\NPPTextFX.h"
#include "scintilla_simu.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Tests
{
	TEST_CLASS(CharactersTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("ClassInitialize: Set Scintilla Message Sender as: mockSendScintillaMessage()");
			::gScintillaMessageSender = &mockSendScintillaMessage;
			resetScintillaSimulator();

			/*
			23 functions:
			"Convert quotes to ""),pfconvert1q2q
			"Convert quotes to '"),pfconvert2q1q
			"Swap quotes (" <-> ')"),pfconvertqtswap
			"Drop quotes " && '"),pfconvertqtdrop
			"Escape " to \\""),pfconvertescapesq
			"Escape ' to \\'"),pfconvertescape1qs1q
			"Escape ' to \\""),pfconvertescape1qsq
			"Escape both "&&' to \\"&&\\'"),pfconvertescapeboth
			"unEscape \\" to ""),pfconvertunescapesq
			"unEscape \\' to '"),pfconvertunescapes1q1q
			"unEscape \\" to '"),pfconvertunescapesq1q
			"unEscape both \\"&&\\' to "&&'"),pfconvertunescapeboth
			"Escape " to """),pfconvertescape2q22q
			"Escape ' to """),pfconvertescape1q22q
			"unEscape "" to ""),pfconvertunescape22q2q
			"unEscape "" to '"),pfconvertunescape22q1q
			"UPPER CASE"),pfconvertuppercase
			"lower case"),pfconvertlowercase
			"Proper Case"),pfconvertpropercase
			"Sentence case."),pfconvertsentencecase
			"iNVERT cASE"),pfconvertinvertcase
			"Zap all characters to space"),pfzapspace
			"Zap all non printable characters to #"),pfzapnonprint
			*/
		}

		TEST_METHOD(ConvertDoubleQuoteToSingeQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This' is a test text.");
			const TCHAR testTextWithQuotesOut[] = _T("This\" is a test text.");
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvert1q2q();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvert1q2q();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ConvertSingleQuoteToDoubleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\" is a test text.");
			const TCHAR testTextWithQuotesOut[] = _T("This' is a test text.");
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvert2q1q();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvert2q1q();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(SwapDoubleQuoteSingleQuotePairTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is a test text.");
			const TCHAR testTextWithQuotesOut[] = _T("This'\" is a test text.");
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertqtswap();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertqtswap();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(RemoveDoubleAndSingleQuotesTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is a test text.");
			const TCHAR testTextWithQuotesOut[] = _T("This is a test text.");
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertqtdrop();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertqtdrop();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(AddSlashToDoubleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is a test text.");		// Note: This"' is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\\\"' is a test text.");	// Note: This\"' is a test text.
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertescapesq();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertescapesq();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(RemoveSlashFromDoubleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\\\"\\\' is a test text.");	// Note: This\"\' is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\"\\' is a test text.");		// Note: This"\' is a test text.
			currentPosition = 8;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertunescapesq();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertunescapesq();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(AddSlashToSingleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is a test text.");		// Note: This"' is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\"\\' is a test text.");	// Note: This\"' is a test text.
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertescape1qs1q();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertescape1qs1q();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(RemoveSlashFromSingleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\\\"\\' is a test text.");		// Note: This\"\' is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\\\"' is a test text.");		// Note: This\"' is a test text.
			currentPosition = 8;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertunescapes1q1q();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertunescapes1q1q();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ConvertSingleQuoteToEscapedDoubleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is a test text.");		// Note: This"' is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\"\\\" is a test text.");	// Note: This"\" is a test text.
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertescape1qsq();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertescape1qsq();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ConvertEscapedDoubleQuoteToSingleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"\\\" is a test text.");	// Note: This"\" is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\"' is a test text.");	// Note: This"' is a test text.
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertunescapesq1q();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertunescapesq1q();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(EscapeBothSingleAndDoubleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is a test text.");		// Note: This"' is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\\\"\\' is a test text.");	// Note: This\"\' is a test text.
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertescapeboth();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertescapeboth();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(RemoveEscapesBothSingleAndDoubleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\\\"\\' is' a test text.");	// Note: This\"\' is' a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\"' is' a test text.");		// Note: This"' is' a test text.
			currentPosition = 13;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertunescapeboth();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertunescapeboth();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(DuplicateDoubleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is a test text.");		// Note: This"' is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\"\"' is a test text.");	// Note: This""' is a test text.
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertescape2q22q();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertescape2q22q();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ConvertDuplicateDoubleQuoteIntoOneDoubleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"\"' is a test text.");	// Note: This""' is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\"' is a test text.");	// Note: This"' is a test text.
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertunescape22q2q();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertunescape22q2q();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ConvertDuplicateDoubleQuoteIntoSingleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"\"' is a test text.");	// Note: This""' is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This'' is a test text.");		// Note: This'' is a test text.
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertunescape22q1q();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertunescape22q1q();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ConvertSingleQuoteToTwoDoubleQuoteTest)
		{
			resetScintillaSimulator();
			const TCHAR testTextNoChanges[] = _T("This is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is a test text.");		// Note: This"' is a test text.
			const TCHAR testTextWithQuotesOut[] = _T("This\"\"\" is a test text.");	// Note: This""" is a test text.
			currentPosition = 7;

			// No-op test:
			setText(testTextNoChanges);
			::pfconvertescape1q22q();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			setText(testTextWithQuotesIn);
			::pfconvertescape1q22q();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ConvertUppercaseTest)
		{
			const TCHAR testTextNoChanges[] = _T("THIS IS A TEST TEXT.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is a test text.");
			const TCHAR testTextWithQuotesOut[] = _T("THIS\"' Is a test text.");

			// No-op test:
			resetScintillaSimulator();
			currentPosition = 8;
			setText(testTextNoChanges);
			::pfconvertuppercase();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			resetScintillaSimulator();
			currentPosition = 8;
			setText(testTextWithQuotesIn);
			::pfconvertuppercase();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ConvertLowercaseTest)
		{
			const TCHAR testTextNoChanges[] = _T("this is a test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is A test text.");
			const TCHAR testTextWithQuotesOut[] = _T("this\"' is a test text.");

			// No-op test:
			resetScintillaSimulator();
			currentPosition = 11;
			setText(testTextNoChanges);
			::pfconvertlowercase();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			resetScintillaSimulator();
			currentPosition = 11;
			setText(testTextWithQuotesIn);
			::pfconvertlowercase();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ConvertPropercaseTest)
		{
			const TCHAR testTextNoChanges[] = _T("This Is A Test text.");
			const TCHAR testTextWithQuotesIn[] = _T("This\"' is A test text.");
			const TCHAR testTextWithQuotesOut[] = _T("This\"' Is A test text.");

			// No-op test:
			resetScintillaSimulator();
			currentPosition = 11;
			setText(testTextNoChanges);
			::pfconvertpropercase();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			resetScintillaSimulator();
			currentPosition = 11;
			setText(testTextWithQuotesIn);
			::pfconvertpropercase();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ConvertSentencecaseTest)
		{
			const TCHAR testTextNoChanges[] = _T("This! Is? A. Test text.");
			const TCHAR testTextWithQuotesIn[] = _T("this\"? is'. a! test text.");
			const TCHAR testTextWithQuotesOut[] = _T("This\"? Is'. A! Test text.");

			// No-op test:
			resetScintillaSimulator();
			currentPosition = 20;
			setText(testTextNoChanges);
			::pfconvertsentencecase();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			resetScintillaSimulator();
			currentPosition = 20;
			setText(testTextWithQuotesIn);
			::pfconvertsentencecase();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(InvertTextCaseTest)
		{
			const TCHAR testTextNoChanges[] = _T("12- ? . ** & ''5_ () ^ $9");
			const TCHAR testTextWithQuotesIn[] = _T("This\"? Is'. A! Test text.");
			const TCHAR testTextWithQuotesOut[] = _T("tHIS\"? iS'. a! tEST text.");

			// No-op test 1:
			resetScintillaSimulator();
			currentPosition = 20;
			setText(testTextNoChanges);
			::pfconvertinvertcase();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			resetScintillaSimulator();
			currentPosition = 20;
			setText(testTextWithQuotesIn);
			::pfconvertinvertcase();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ReplaceWithSpacesTest)
		{
			const TCHAR testTextNoChanges[] = _T("               ext.");
			const TCHAR testTextWithQuotesIn[] = _T("this\"? is'. a! test text.");
			const TCHAR testTextWithQuotesOut[] = _T("               test text.");

			// No-op test:
			resetScintillaSimulator();
			currentPosition = 15;
			setText(testTextNoChanges);
			::pfzapspace();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			resetScintillaSimulator();
			currentPosition = 15;
			setText(testTextWithQuotesIn);
			::pfzapspace();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

		TEST_METHOD(ReplaceNonPrintableWithHashTest)
		{
			const TCHAR testTextNoChanges[] = _T("               ext.");
			const TCHAR testTextWithQuotesIn[] = _T("\vthis\"?\a is'. a! test text.");
			const TCHAR testTextWithQuotesOut[] = _T("#this\"?# is'. a! test text.");

			// No-op test:
			resetScintillaSimulator();
			currentPosition = 15;
			setText(testTextNoChanges);
			::pfzapnonprint();
			Assert::AreEqual(testTextNoChanges, convertTextbuffer());

			// Change test:
			resetScintillaSimulator();
			currentPosition = 15;
			setText(testTextWithQuotesIn);
			::pfzapnonprint();
			Assert::AreEqual(testTextWithQuotesOut, convertTextbuffer());
		}

	};

}