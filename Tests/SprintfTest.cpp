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
	TEST_CLASS(SprintfTest)
	{
		TCHAR* dest;

	public:
		TEST_METHOD_INITIALIZE(SprintfTestInitialize)
		{
			dest = NULL;
		}

		TEST_METHOD(SprintfStressTest)
		{
			// Copied from void testsprintfuncs(void)
			Logger::WriteMessage("Doing 130 loops of sarmprintf()s");

			TCHAR *zbuf, *zbuf2;
			size_t zbufsz, zbufsz2;
			unsigned temp;

			for (temp = 1; temp < 130; temp++) {
				zbuf = NULL;
				zbufsz = 0;
				sarmprintf(&zbuf, &zbufsz, NULL, _T("%*s"), temp, "X");
				zbuf2 = NULL;
				zbufsz2 = 0;
				strcpyarmsafe(&zbuf2, &zbufsz2, NULL, zbuf, _T("SprintfStressTest"));
				freesafe(zbuf2, _T("SprintfStressTest"));
				freesafe(zbuf, _T("SprintfStressTest"));
				zbuf = NULL;
				zbufsz = 0;
				sarmprintf(&zbuf, &zbufsz, NULL, _T("%*s"), 131 - temp, _T("X"));
				zbuf2 = NULL;
				zbufsz2 = 0;
				strcpyarmsafe(&zbuf2, &zbufsz2, NULL, zbuf, _T("SprintfStressTest"));
				freesafe(zbuf2, _T("SprintfStressTest"));
				freesafe(zbuf, _T("SprintfStressTest"));
			}
		}

		TEST_METHOD(SprintfBundleTest)
		{
			Logger::WriteMessage("SprintfBundleTest - Doing string fuzzing");
#define NEL 524
			size_t i, h, n;
			TCHAR nelW[NEL];

			for (i = 2; i <= 6; i++)
				for (h = 2; h < NELEM(nelW); h++) {
					TCHAR *zbuf = NULL;
					size_t zbufsz = 0;
					TCHAR szStrW[NEL + 8];

					wmemset(nelW, _T('A'), h - 1);
					nelW[h - 1] = _T('\0');
					wmemset(szStrW, L'#', NELEM(szStrW));
					szStrW[NELEM(szStrW) - 1] = _T('\0');
					int cchResultW = _snwprintf(szStrW, i, nelW);
					int cchActualW = (TCHAR *)wmemchr(szStrW, _T('#'), NELEM(szStrW)) - szStrW;
					TCHAR *szZeroW = (TCHAR *)wmemchr(szStrW, 0, h);
					int cchZeroW = (szZeroW) ? szZeroW - szStrW : -1;
					sarmprintf(&zbuf, &zbufsz, NULL, _T("cchResult:%2d= _snwprintf(szStrW,cch=%zd,nelW  ); // ZeroIdx=%2d cchActual=%d\n"), cchResultW, i, cchZeroW, cchActualW);
					Logger::WriteMessage(zbuf);
					if (h<16)
						for (n = 0; n<h; n++)
							wprintf(szStrW[n]<32 ? _T("0") : _T("%c"), szStrW[n]);
					if (cchActualW > i)
						Assert::Fail(_T(" Bounds Error"));

					wmemset(szStrW, _T('#'), NELEM(szStrW));
					szStrW[NELEM(szStrW) - 1] = _T('\0');
					cchResultW = _snwprintf(szStrW, i, nelW);
					cchActualW = (TCHAR *)wmemchr(szStrW, _T('#'), NELEM(szStrW)) - szStrW;
					szZeroW = (TCHAR *)wmemchr(szStrW, 0, h);
					cchZeroW = (szZeroW) ? szZeroW - szStrW : -1;
					sarmprintf(&zbuf, &zbufsz, NULL, _T("cchResult:%2d=_vsnwprintf(szStrW,cch=%zd,szStrW); // ZeroIdx=%2d cchActual=%d\n"), cchResultW, i, cchZeroW, cchActualW);
					Logger::WriteMessage(zbuf);
					if (h<16)
						for (n = 0; n<h; n++)
							wprintf(szStrW[n]<32 ? _T("0") : _T("%c"), szStrW[n]);
					if (cchActualW>i)
						Assert::Fail(_T(" Bounds Error"));

					if (zbuf)
						freesafe(zbuf, _T("SprintfBundleTest"));
				}
		}

		TEST_METHOD(SprintfLenghtTest)
		{
			Logger::WriteMessage("SprintfLenghtTest");

			TCHAR *zbuf = NULL;
			size_t zbufsz = 0;

			// Test: too long
			zbuf = smprintf(_T("Howdy %d partner"), 57);
			Assert::IsNotNull(zbuf);
			Assert::AreEqual(16, (int)wcslen(zbuf));
			Assert::AreEqual(_T("Howdy 57 partner"), zbuf);
			freesafe(zbuf, _T("SprintfLenghtTest"));

			// Test: too short
			zbuf = smprintf(_T("123456"));
			Assert::IsNotNull(zbuf);
			Assert::AreEqual(6, (int)wcslen(zbuf));
			Assert::AreEqual(_T("123456"), zbuf);
			freesafe(zbuf, _T("SprintfLenghtTest"));

			// Test: too short
			zbuf = smprintf(_T("x2%s"), _T("ex"));
			Assert::IsNotNull(zbuf);
			Assert::AreEqual(4, (int)wcslen(zbuf));
			Assert::AreEqual(_T("x2ex"), zbuf);
			freesafe(zbuf, _T("SprintfLenghtTest"));
			
			// Test: just right
			zbuf = smprintf(_T("1234567"));
			Assert::IsNotNull(zbuf);
			Assert::AreEqual(7, (int)wcslen(zbuf));
			Assert::AreEqual(_T("1234567"), zbuf);
			freesafe(zbuf, _T("SprintfLenghtTest"));

			// Test: smprintfpath
			zbuf = smprintfpath(_T("%s%?\\%s%?\\%s"), _T("A"), _T("B\\"), _T("C"));
			Assert::IsNotNull(zbuf);
			Assert::AreEqual(5, (int)wcslen(zbuf));
			Assert::AreEqual(_T("A\\B\\C"), zbuf);
			freesafe(zbuf, _T("SprintfLenghtTest"));

			TCHAR buf[8];
			size_t rv;
			rv = snprintfX(buf, 8, _T("123456789ABCDEF"));
			Assert::AreEqual(7, (int)wcslen(buf));
			Assert::AreEqual(_T("1234567"), buf);

			rv = snprintfX(buf, 8, _T("123"));
			Assert::AreEqual(3, (int)wcslen(buf));
			Assert::AreEqual(_T("123"), buf);
		}

		// Dynamic clean-up
		TEST_METHOD_CLEANUP(SprintfTestCleanup)
		{
			if (dest)
				freesafe(dest, _T("SprintfTest"));
		}

	};
}