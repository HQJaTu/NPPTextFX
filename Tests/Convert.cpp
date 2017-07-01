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
	TEST_CLASS(ConvertTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In ConvertTest Class Initialize");

			/*
			27 functions:
			"C:Encode URI Component"),pfencodeURIcomponent
			"C:Encode HTML (&&<>")"),pfencodeHTML
			"C:Strip HTML tags table tabs"),pfstripHTMLtags
			"C:Strip HTML tags table nontabs"),pfstripHTMLnotabs
			"C:Submit to W3C HTML Validator"),pfsubmitHTML
			"C:Submit to W3C CSS Validator"),pfsubmitCSS
			"C:Convert text to code command("text=\\"value\\"");"),pfprepostpendlines
			"C:Convert Decimal Number to Binary"),pfdecimal2binary
			"C:Convert Decimal Number to Octal"),pfdecimal2octal
			"C:Convert Decimal Number to Hex"),pfdecimal2hex
			"C:Convert Hex Number to Decimal"),pfhex2decimal
			"C:Convert Octal Number to Decimal"),pfoctal2decimal
			"C:Convert Binary Number to Decimal"),pfbinary2decimal
			"C:Convert C-style Number to Decimal"),pfcnum2decimal
			"C:Convert text to Hex-16"),pftohex16
			"C:Convert text to Hex-32"),pftohex32
			"C:Convert text to Hex-64"),pftohex64
			"C:Convert text to Hex-128"),pftohex128
			"C:Convert hex byte runs into LE-WORDS"),pfhexbyterunstolittlendian2
			"C:Convert hex byte runs into LE-DWORDS"),pfhexbyterunstolittlendian4
			"C:Convert LE-words to hex byte runs"),pflittlendiantohexbyteruns
			"C:Convert Hex to text"),pffromhex
			"C:ROT13 Text"),pfrot13
			"C:Convert EBCDIC to ASCII"),pfEBCDIC2ascii
			"C:Convert ASCII to EBCDIC"),pfascii2EBCDIC
			"C:Convert KOI8_R to CP1251"),pfKOI8_Rtocp1251
			"C:Convert CP1251 to KOI8_R"),pfcp1251toKOI8_R
			*/
		}


	};
}