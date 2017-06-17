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
	TEST_CLASS(StrnCpyArmTest)
	{
		TCHAR* dest;

	public:
		TEST_METHOD_INITIALIZE(StrnCpyArmTestInitialize)
		{
			dest = NULL;
		}

		TEST_METHOD(StringCopyTest)
		{
			Logger::WriteMessage("Testing reallocation");
			int change;
			size_t destsz = 0;
			size_t stringLen = 0;
			const TCHAR testString[] = _T("This is the test string to be copied. It contains UTF-8 characters: £ $ öäå ÖÄÅ! Also US-ASCII.");

			change = strncpyarmsafe(&dest, &destsz, &stringLen, testString, wcslen(testString), _T("NPPGetSpecialFolderLocationarm"));
			Assert::IsNotNull(dest);
			Assert::AreEqual(256, (int)destsz);
			Assert::AreEqual((int)wcslen(testString), (int)stringLen);
			Assert::AreEqual(testString, dest);

			if (dest) {
				freesafe(dest, _T("StrnCpyArmTest"));
				dest = NULL;
			}
		}


		// Dynamic clean-up
		TEST_METHOD_CLEANUP(StrnCpyArmTestCleanup)
		{
			if (dest)
				freesafe(dest, _T("StrnCpyArmTest"));
		}

	};
}