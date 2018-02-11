#include "stdafx.h"
#include "CppUnitTest.h"

#include <windows.h>
#include <wchar.h>
#include <tchar.h>

#include "..\SRC\Scintilla.h"
#include "..\SRC\Scintilla\UniConversion.h"

#define EXTERNC extern "C"
#include "..\Src\NPPTextFX.h"
#include "scintilla_simu.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Tests
{
//#define TEST_BUFFER_CHARS 8192
	char textBuffer[TEST_BUFFER_CHARS];
	unsigned anchorPosition = 0;
	unsigned currentPosition = 0;

	// Static function to emulate Scintilla message API
	LRESULT mockSendScintillaMessage(BOOL which, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		TCHAR tmpString[256];
		char tmpTextBuffer[TEST_BUFFER_CHARS];
		size_t textLen;
		size_t charsToCopy;
		int tmpPos;
		char* tmpChar;
		TextToFind* textToFind;

		// All our calculations are based on fact that anchor position is first, then current position in left-to-right -fashion.
		// User selection can be right-to-left.
		unsigned effectiveEnchorPosition = anchorPosition;
		unsigned effectiveCurrentPosition = currentPosition;
		if (anchorPosition > currentPosition) {
			// Reverse selection
			effectiveEnchorPosition = currentPosition;
			effectiveCurrentPosition = anchorPosition;
		}

		// See: http://www.scintilla.org/ScintillaDoc.html
		switch (Msg) {
		case SCI_START:
			Logger::WriteMessage("SCI_START");
			break;
		case SCI_GETCURRENTPOS:
			return currentPosition;
		case SCI_GETSELECTIONSTART:
		case SCI_GETSELECTIONEND:
			return currentPosition;
		case SCI_GETANCHOR:
			return anchorPosition;
		case SCI_GETEOLMODE:
			return (int)0;	// CRLF
		case SCI_LINEFROMPOSITION:
			return (int)1;
		case SCI_GETLENGTH:
		case SCI_GETTEXTLENGTH:
			return strlen(textBuffer) + 1;	// For NUL terminator, this is the storage length
		case SCI_GETTEXT:
			// Get length of text, or copy the text buffer
			textLen = strlen(textBuffer);
			if (!lParam)
				return textLen;				// Note: No space for NUL terminator, actual text length
			if (!wParam)
				return 0;
			charsToCopy = (textLen < wParam - 1 ? textLen : wParam - 1);
			strncpy_s((char*)lParam, charsToCopy + 1, textBuffer, charsToCopy);
			((char*)lParam)[charsToCopy] = 0;
			return (LRESULT)charsToCopy;	// Note: No space for NUL terminator, actual text length
		case SCI_SETTEXT:
			// Overwrite entire text buffer with new stuff
			if (!lParam)
				return -1;
			strncpy_s(textBuffer, (char*)lParam, TEST_BUFFER_CHARS);
			anchorPosition = 0;
			currentPosition = 0;
			return NULL;
		case SCI_GETSELTEXT:
			// Characters selected. Must return at least 1 for terminating NUL character!
			if (!lParam) {
				if (!effectiveCurrentPosition)
					return 1;				// For NUL terminator, this is the storage length
				return effectiveCurrentPosition - effectiveEnchorPosition + 1;
			}

			// Copy
			strncpy_s((char*)lParam, effectiveCurrentPosition - effectiveEnchorPosition + 1, (char*)(textBuffer + effectiveEnchorPosition), effectiveCurrentPosition - effectiveEnchorPosition);
			((char*)lParam)[effectiveCurrentPosition - effectiveEnchorPosition] = 0;
			return effectiveCurrentPosition - effectiveEnchorPosition + 1;
		case SCI_REPLACESEL:
			// Replace selection with incoming text
			if (!lParam)
				return -1;
			if (effectiveEnchorPosition > 0) {
				// Copy text before selection start
				strncpy_s(tmpTextBuffer, textBuffer, effectiveEnchorPosition);
				tmpTextBuffer[effectiveEnchorPosition] = 0;
			}
			else {
				tmpTextBuffer[0] = 0;
			}
			// Copy incoming text
			textLen = strlen((char*)lParam);
			charsToCopy = TEST_BUFFER_CHARS - effectiveEnchorPosition - textLen;
			strncat(tmpTextBuffer + effectiveEnchorPosition, (char*)lParam, textLen);

			if (effectiveCurrentPosition > 0) {
				// Copy text after selection end
				strncpy_s(tmpTextBuffer + effectiveEnchorPosition + textLen, charsToCopy, (char*)(textBuffer + effectiveCurrentPosition), charsToCopy);
			}
			// Finally replace the text buffer with our temp buffer
			strncpy_s(textBuffer, tmpTextBuffer, TEST_BUFFER_CHARS);
			anchorPosition = anchorPosition;
			currentPosition = effectiveEnchorPosition + textLen; // End of inserted text
			break;
		case SCI_ADDTEXT:
			// Insert text to current position
			if (!lParam)
				return -1;
			if (effectiveCurrentPosition > 0) {
				// Copy text before current position
				strncpy_s(tmpTextBuffer, textBuffer, effectiveCurrentPosition);
				tmpTextBuffer[effectiveCurrentPosition] = 0;
			}
			else {
				tmpTextBuffer[0] = 0;
			}
			// Copy incoming text
			textLen = strlen((char*)lParam);
			if (wParam < textLen)
				textLen = wParam;
			charsToCopy = TEST_BUFFER_CHARS - effectiveCurrentPosition - textLen;
			strncat(tmpTextBuffer + effectiveCurrentPosition, (char*)lParam, textLen);

			if (effectiveCurrentPosition > 0 || textBuffer[effectiveCurrentPosition]) {
				// Copy text after current position end
				strncpy_s(tmpTextBuffer + effectiveCurrentPosition + textLen, charsToCopy, (char*)(textBuffer + effectiveCurrentPosition), charsToCopy);
			}
			// Finally replace the text buffer with our temp buffer
			strncpy_s(textBuffer, tmpTextBuffer, TEST_BUFFER_CHARS);
			anchorPosition = currentPosition = effectiveCurrentPosition + textLen; // End of inserted text
			break;
		case SCI_SELECTIONISRECTANGLE:
			return (BOOL)0;
		case SCI_POSITIONFROMLINE:
			return 1;
		case SCI_SETCURSOR:
		case SCI_BEGINUNDOACTION:
		case SCI_ENDUNDOACTION:
			return NULL;
		case SCI_GETCODEPAGE:
			return 0;	// Default code page
		case SCI_WORDSTARTPOSITION:
			tmpPos = (int)wParam;
			while (tmpPos && textBuffer[tmpPos] != ' ')
				--tmpPos;
			return tmpPos;
		case SCI_WORDENDPOSITION:
			tmpPos = (int)wParam;
			textLen = strlen(textBuffer);
			while (tmpPos < textLen && textBuffer[tmpPos] != ' ')
				++tmpPos;
			return tmpPos;
		case SCI_SETSEL:
			anchorPosition = (unsigned)wParam;
			currentPosition = (unsigned)lParam;
			break;
		case SCI_FINDTEXT:
			// Scintilla.h:
			// #define SCFIND_WHOLEWORD 2
			// #define SCFIND_MATCHCASE 4

			textToFind = (TextToFind*)lParam;
			swprintf(tmpString, 256, _T("Range: %d - %d\n"), textToFind->chrg.cpMin, textToFind->chrg.cpMax);
			Logger::WriteMessage(tmpString);
			{
				// Note: textToFind->lpstrText is wide-character buffer, but because UCS-2, we must treat it as not like one.
				size_t textLength = strlen((const char*)textToFind->lpstrText);
				if (!textToFind->lpstrText || !textLength) {
					Assert::Fail(_T("Need text and need text to search for!"));
				}

				char* ptr = textBuffer;
				while ((ptr = strstr(ptr, (const char*)textToFind->lpstrText))) {
					if ((int)(ptr - textBuffer) >= currentPosition) {
						// Don't find the highlighted text. That's what we're searching
						break;
					}
					tmpChar = ptr++;
				}
			}
			if (!tmpChar)
				return 0;
			return (int)(tmpChar - textBuffer);
		case SCI_GOTOPOS:
			anchorPosition = (unsigned)wParam;
			break;
		default:
			swprintf(tmpString, 256, _T("SendScintillaMessage: Not handling message ID %d"), Msg);
			Logger::WriteMessage(tmpString);
		}

		return 0;
	}

	void resetScintillaSimulator()
	{
		textBuffer[0] = 0;
		anchorPosition = 0;
		currentPosition = 0;
	}

	void setText(const TCHAR* textIn)
	{
		size_t textLength = wcslen(textIn);
		if (!textIn || !textLength) {
			resetScintillaSimulator();
			return;
		}

		// UTF-8 --> UCS-2
		size_t textBufferConvertedLength = UTF8FromUCS2(textIn, textLength, (char*)textBuffer, TEST_BUFFER_CHARS, FALSE);
		if (textBufferConvertedLength >= TEST_BUFFER_CHARS) {
			Assert::Fail(_T("Too long!"));
		}
		textBuffer[textBufferConvertedLength] = '\0';
	}

	TCHAR* convertTextbuffer(const char* input)
	{
		static TCHAR wideTextBuffer[TEST_BUFFER_CHARS];
		const char* bufferToConvert = input;
		if (input == NULL)
			bufferToConvert = textBuffer;

		unsigned sln = UCS2FromUTF8(bufferToConvert, strlen(bufferToConvert), wideTextBuffer, TEST_BUFFER_CHARS, FALSE, NULL);
		wideTextBuffer[sln] = '\0';

		return wideTextBuffer;
	}


	TEST_CLASS(ScintillaSimulator)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("ScintillaSimulator: Set Scintilla Message Sender as: mockSendScintillaMessage()");
			::gScintillaMessageSender = &mockSendScintillaMessage;
			textBuffer[0] = 0;	// Null terminate our test string
		}

		TEST_METHOD(ScintillaTest)
		{
			unsigned textLength;
			resetScintillaSimulator();

			// Empty buffer test:
			Assert::AreEqual(0, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));

			// Fill buffer:
			const TCHAR testText1_wide[] = _T("This is a test text.");
			const char testText1_narrow[] = "This is a test text.";
			mockSendScintillaMessage(0, SCI_SETTEXT, 0, (LPARAM)testText1_narrow);
			Assert::AreEqual(testText1_wide, convertTextbuffer());

			// Text in a buffer length test:
			textLength = mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL);
			Assert::AreEqual(20, (int)textLength);

			// Copy all text out of buffer:
			char tmpTextBuffer[TEST_BUFFER_CHARS];
			Assert::AreEqual(20, (int)mockSendScintillaMessage(0, SCI_GETTEXT, textLength + 1, (LPARAM)tmpTextBuffer));
			Assert::AreEqual(testText1_wide, convertTextbuffer(tmpTextBuffer));
			// Copy partial text out of buffer:
			Assert::AreEqual(10, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 10 + 1, (LPARAM)tmpTextBuffer));
			Assert::AreEqual(_T("This is a "), convertTextbuffer(tmpTextBuffer));

			// Text selection test:
			resetScintillaSimulator();
			const TCHAR testText2_wide[] = _T("123456789012345678901234567890");
			const char testText2_narrow[] = "123456789012345678901234567890";
			mockSendScintillaMessage(0, SCI_SETTEXT, 0, (LPARAM)testText2_narrow);
			Assert::AreEqual(testText2_wide, convertTextbuffer());

			// No selection, char for NUL
			Assert::AreEqual(1, (int)mockSendScintillaMessage(0, SCI_GETSELTEXT, 0, NULL));

			// Selection of 10 chars from start:
			currentPosition = 10;
			Assert::AreEqual(11, (int)mockSendScintillaMessage(0, SCI_GETSELTEXT, 0, NULL));
			mockSendScintillaMessage(0, SCI_GETSELTEXT, 0, (LPARAM)tmpTextBuffer);
			Assert::AreEqual(_T("1234567890"), convertTextbuffer(tmpTextBuffer));

			// Selection of 10 chars from start reversed:
			anchorPosition = 10;
			currentPosition = 0;
			Assert::AreEqual(11, (int)mockSendScintillaMessage(0, SCI_GETSELTEXT, 0, NULL));
			mockSendScintillaMessage(0, SCI_GETSELTEXT, 0, (LPARAM)tmpTextBuffer);
			Assert::AreEqual(_T("1234567890"), convertTextbuffer(tmpTextBuffer));

			// Selection of 10 chars, skipping the first character:
			anchorPosition = 1;
			currentPosition = 11;
			Assert::AreEqual(11, (int)mockSendScintillaMessage(0, SCI_GETSELTEXT, 0, NULL));
			mockSendScintillaMessage(0, SCI_GETSELTEXT, 0, (LPARAM)tmpTextBuffer);
			Assert::AreEqual(_T("2345678901"), convertTextbuffer(tmpTextBuffer));

			// Replace test, delete first 3 characters
			mockSendScintillaMessage(0, SCI_SETTEXT, 0, (LPARAM)testText2_narrow);
			Assert::AreEqual(testText2_wide, convertTextbuffer());
			anchorPosition = 0;
			currentPosition = 3;
			mockSendScintillaMessage(0, SCI_REPLACESEL, 0, (LPARAM)"");
			Assert::AreEqual(_T("456789012345678901234567890"), convertTextbuffer());
			Assert::AreEqual(27, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));

			// Replace test, leave 3 first characters as is, then delete next 3 characters
			mockSendScintillaMessage(0, SCI_SETTEXT, 0, (LPARAM)testText2_narrow);
			Assert::AreEqual(testText2_wide, convertTextbuffer());
			anchorPosition = 3;
			currentPosition = 6;
			mockSendScintillaMessage(0, SCI_REPLACESEL, 0, (LPARAM)_T(""));
			Assert::AreEqual(_T("123789012345678901234567890"), convertTextbuffer());
			Assert::AreEqual(27, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));

			// Replace test, replace first 3 characters with ABCDEF
			mockSendScintillaMessage(0, SCI_SETTEXT, 0, (LPARAM)testText2_narrow);
			Assert::AreEqual(testText2_wide, convertTextbuffer());
			anchorPosition = 0;
			currentPosition = 3;
			mockSendScintillaMessage(0, SCI_REPLACESEL, 0, (LPARAM)"ABCDEF");
			Assert::AreEqual(_T("ABCDEF456789012345678901234567890"), convertTextbuffer());
			Assert::AreEqual(33, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));

			// Replace test, leave 3 first characters as is, then replace next 3 characters with ABCDEF
			mockSendScintillaMessage(0, SCI_SETTEXT, 0, (LPARAM)testText2_narrow);
			Assert::AreEqual(testText2_wide, convertTextbuffer());
			anchorPosition = 3;
			currentPosition = 6;
			mockSendScintillaMessage(0, SCI_REPLACESEL, 0, (LPARAM)"ABCDEF");
			Assert::AreEqual(_T("123ABCDEF789012345678901234567890"), convertTextbuffer());
			Assert::AreEqual(33, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));

			// Add test, add text to empty buffer
			mockSendScintillaMessage(0, SCI_SETTEXT, 0, (LPARAM)"");
			Assert::AreEqual(0, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));

			// Fill the empty buffer, copy all of it:
			mockSendScintillaMessage(0, SCI_ADDTEXT, 20, (LPARAM)testText1_narrow);
			Assert::AreEqual(20, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));
			Assert::AreEqual(testText1_wide, convertTextbuffer());

			// Clear, then fill buffer, copy only 10 characters of it:
			mockSendScintillaMessage(0, SCI_SETTEXT, 0, (LPARAM)"");
			Assert::AreEqual(0, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));
			mockSendScintillaMessage(0, SCI_ADDTEXT, 10, (LPARAM)testText1_narrow);
			Assert::AreEqual(10, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));
			Assert::AreEqual(_T("This is a "), convertTextbuffer());

			// Clear buffer with "123", then fill buffer only 10 characters of it:
			mockSendScintillaMessage(0, SCI_SETTEXT, 0, (LPARAM)"123");
			Assert::AreEqual(3, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));
			currentPosition = 3;
			mockSendScintillaMessage(0, SCI_ADDTEXT, 10, (LPARAM)testText1_narrow);
			Assert::AreEqual(13, (int)mockSendScintillaMessage(0, SCI_GETTEXT, 0, NULL));
			Assert::AreEqual(_T("123This is a "), convertTextbuffer());
		}

#define CONVERTED_BUFFER_CHARS 256
		TEST_METHOD(UCS2ConversionWithNothingToConvertTest)
		{
			const TCHAR testTextNoChangesUnicode[] = _T("This is a test text.");
			const char testTextNoChanges[] = "This is a test text.";
			char converted[CONVERTED_BUFFER_CHARS];
			TCHAR convertedUnicode[CONVERTED_BUFFER_CHARS];

			// Convert our Unicode into UCS-2
			unsigned sln = UTF8FromUCS2(testTextNoChangesUnicode, wcslen(testTextNoChangesUnicode), converted, CONVERTED_BUFFER_CHARS, FALSE);
			converted[sln] = '\0';

			unsigned strLenOriginalText;
			unsigned strLenConvertedText;
			strLenOriginalText = strlen(testTextNoChanges);
			strLenConvertedText = strlen(converted);
			Assert::AreEqual(strLenConvertedText, strLenOriginalText);
			for (int idx = 0; idx < strLenOriginalText; ++idx) {
				char assumed = testTextNoChanges[idx];
				char actual = converted[idx];
				Assert::AreEqual((int)assumed, (int)actual);
			}

			// Convert it back
			sln = UCS2FromUTF8(converted, sln, convertedUnicode, CONVERTED_BUFFER_CHARS, FALSE, NULL);
			convertedUnicode[sln] = '\0';

			Assert::AreEqual(testTextNoChangesUnicode, convertedUnicode);
		}

		TEST_METHOD(UCS2ConversionWithCharactersToConvertTest)
		{
			// Unicode hex:
			// 54 00 68 00 69 00 73 00 20 00 69 00 73 00 20 00 a3 00 20 00 61 00 20 00 f6 00 e4 00 e5 00 d6 00 c4 00 c5 00 20 00 74 00 65 00 73 00 74 00 20 00 ef 53 e3 53 ef 53 50 4e 20 00 6f 00 72 00 20 00 42 0e 04 0e 04 0e 32 0e 42 0e 04 0e 25 0e 32 0e 20 00 74 00 65 00 78 00 74 00 2e 00 00
			// UCS-2 hex:
			// 54 68 69 73 20 69 73 20 c2 a3 20 61 20 c3 b6 c3 a4 c3 a5 c3 96 c3 84 c3 85 20 74 65 73 74 20 e5 8f af e5 8f a3 e5 8f af e4 b9 90 20 6f 72 20 e0 b9 82 e0 b8 84 e0 b8 84 e0 b8 b2 e0 b9 82 e0 b8 84 e0 b8 a5 e0 b8 b2 20 74 65 78 74 2e 00
			const TCHAR testTextUnicode[] = _T("This is £ a öäåÖÄÅ test 可口可乐 or โคคาโคลา text.");
			const BYTE testTextUCS2Bytes[] = { 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0xc2, 0xa3, 0x20, 0x61, 0x20, 0xc3,
				0xb6, 0xc3, 0xa4, 0xc3, 0xa5, 0xc3, 0x96, 0xc3, 0x84, 0xc3, 0x85, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0xe5, 0x8f,
				0xaf, 0xe5, 0x8f, 0xa3, 0xe5, 0x8f, 0xaf, 0xe4, 0xb9, 0x90, 0x20, 0x6f, 0x72, 0x20, 0xe0, 0xb9, 0x82, 0xe0, 0xb8,
				0x84, 0xe0, 0xb8, 0x84, 0xe0, 0xb8, 0xb2, 0xe0, 0xb9, 0x82, 0xe0, 0xb8, 0x84, 0xe0, 0xb8, 0xa5, 0xe0, 0xb8, 0xb2,
				0x20, 0x74, 0x65, 0x78, 0x74, 0x2e, 0x00 };
			char converted[CONVERTED_BUFFER_CHARS];
			TCHAR convertedUnicode[CONVERTED_BUFFER_CHARS];

			// Convert our Unicode into UCS-2
			unsigned numberOfBytesInConvertedText = UTF8FromUCS2(testTextUnicode, wcslen(testTextUnicode), converted, CONVERTED_BUFFER_CHARS, FALSE);
			converted[numberOfBytesInConvertedText] = '\0';

			unsigned numberOfBytesExpected;
			numberOfBytesExpected = sizeof(testTextUCS2Bytes);
			Assert::AreEqual(numberOfBytesInConvertedText + 1, numberOfBytesExpected);			// Also compare the terminating NUL at the end
			for (int idx = 0; idx < numberOfBytesExpected + 1; ++idx) {
				BYTE assumed = testTextUCS2Bytes[idx];
				BYTE actual = converted[idx];
				Assert::AreEqual((int)assumed, (int)actual);
			}

			// Convert it back
			numberOfBytesExpected = UCS2FromUTF8(converted, numberOfBytesInConvertedText, convertedUnicode, CONVERTED_BUFFER_CHARS, FALSE, NULL);
			convertedUnicode[numberOfBytesExpected] = '\0';

			Assert::AreEqual(testTextUnicode, convertedUnicode);
		}
		/*
		TEST_METHOD(CodePageTest)
		{
			resetScintillaSimulator();
			scintillaCodePage = 0;

			const char testTextNoChanges[] = "This is a test text.";
			const char testTextWithQuotesIn[] = "This' is a test text.";
			const char testTextWithQuotesOut[] = "This\" is a test text.";
			currentPosition = 7;	// Do only first 7 characters

			// No-op test:
			strcpy_s((char*)textBuffer, TEST_BUFFER_CHARS, testTextNoChanges);
			::pfconvert1q2q();
			unsigned numberOfBytesExpected;
			unsigned numberOfBytesInConvertedText;
			numberOfBytesExpected = strlen(testTextNoChanges);
			numberOfBytesInConvertedText = strlen((char*)textBuffer);
			Assert::AreEqual(numberOfBytesInConvertedText, numberOfBytesExpected);
			for (int idx = 0; idx < numberOfBytesExpected; ++idx) {
				char assumed = testTextNoChanges[idx];
				char actual = ((char*)textBuffer)[idx];
				Assert::AreEqual((int)assumed, (int)actual);
			}

			// Change test:
			strcpy_s((char*)textBuffer, TEST_BUFFER_CHARS, testTextWithQuotesIn);
			::pfconvert1q2q();
			numberOfBytesExpected = strlen(testTextWithQuotesOut);
			numberOfBytesInConvertedText = strlen((char*)textBuffer);
			Assert::AreEqual(numberOfBytesInConvertedText, numberOfBytesExpected);
			for (int idx = 0; idx < numberOfBytesExpected; ++idx) {
				char assumed = testTextWithQuotesOut[idx];
				char actual = ((char*)textBuffer)[idx];
				Assert::AreEqual((int)assumed, (int)actual);
			}

		}
		*/

	};

}