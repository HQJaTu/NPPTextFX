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
	TEST_CLASS(ArmReallocTest)
	{
		TCHAR* dest;

	public:
		// Static initialize
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
		}

		// Dynamic initialize
		TEST_METHOD_INITIALIZE(ArmReallocTestInitialize)
		{
			dest = NULL;
		}

		TEST_METHOD(BasicReallocTest)
		{
			Logger::WriteMessage("Testing reallocation");
			int change;
			size_t destsz = 0;

			change = armreallocsafe(&dest, &destsz, 5, ARMSTRATEGY_INCREASE, 0, _T("BasicReallocTest"));
			Assert::IsNotNull(dest);
			Assert::AreEqual(64, (int)destsz);

			if (dest) {
				freesafe(dest, _T("BasicReallocIncreaseTest"));
				dest = NULL;
			}
		}

		TEST_METHOD(BasicReallocIncreaseTest)
		{
			Logger::WriteMessage("Testing reallocation increase");
			int change;
			size_t destsz = 0;

			change = armreallocsafe(&dest, &destsz, 5, ARMSTRATEGY_INCREASE, 0, _T("BasicReallocIncreaseTest"));
			Assert::IsNotNull(dest);
			Assert::AreEqual(64, (int)destsz);

			change = armreallocsafe(&dest, &destsz, 65, ARMSTRATEGY_INCREASE, 0, _T("BasicReallocIncreaseTest"));
			Assert::IsNotNull(dest);
			Assert::AreEqual(128, (int)destsz);

			if (dest) {
				freesafe(dest, _T("BasicReallocIncreaseTest"));
				dest = NULL;
			}
		}

		TEST_METHOD(BasicReallocCleanTest)
		{
			Logger::WriteMessage("Testing reallocation with clean buffer");
			int change;
			size_t destsz = 0;

			change = armreallocsafe(&dest, &destsz, 65, ARMSTRATEGY_INCREASE, 1, _T("BasicReallocCleanTest"));
			Assert::IsNotNull(dest);
			Assert::AreEqual(128, (int)destsz);

			//
			for (unsigned idx = 0; idx < destsz / sizeof(TCHAR); ++idx) {
				TCHAR err[256];
				swprintf(err, 256, _T("Fail! Not cleaned to null at idx: %d"), idx);
				Assert::AreEqual((TCHAR)0, dest[idx], err);
			}

			if (dest) {
				freesafe(dest, _T("BasicReallocIncreaseTest"));
				dest = NULL;
			}
		}

		// Dynamic clean-up
		TEST_METHOD_CLEANUP(ArmReallocTestCleanup)
		{
			if (dest)
				freesafe(dest, _T("BasicReallocIncreaseTest"));
		}

		// Static clean-up
		TEST_CLASS_CLEANUP(ClassCleanup)
		{
		}

	};
}