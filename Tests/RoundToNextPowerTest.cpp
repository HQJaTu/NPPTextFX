#include "stdafx.h"
#include "CppUnitTest.h"

#include <windows.h>
#include <wchar.h>
#include <tchar.h>

#define EXTERNC extern "C"
#include "..\Src\NPPTextFX.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// See: Using Microsoft.VisualStudio.TestTools.CppUnitTestFramework
// https://msdn.microsoft.com/en-us/library/hh694604.aspx

TEST_MODULE_INITIALIZE(ModuleInitialize)
{
	Logger::WriteMessage("In RoundToNextPowerTest Module Initialize. There can be only 1 module initializer, and this is it.");
}

namespace Tests
{		
	TEST_CLASS(RoundToNextPowerTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In RoundToNextPowerTest Class Initialize");
		}

		TEST_METHOD(BasicRoundings)
		{
			Logger::WriteMessage("Testing basic ones");
			size_t result;

			result = ::roundtonextpower(0);
			Assert::AreEqual(1, (int)result);

			result = ::roundtonextpower(1);
			Assert::AreEqual(1, (int)result);

			result = ::roundtonextpower(2);
			Assert::AreEqual(2, (int)result);

			result = ::roundtonextpower(3);
			Assert::AreEqual(4, (int)result);

			result = ::roundtonextpower(4);
			Assert::AreEqual(4, (int)result);

			result = ::roundtonextpower(5);
			Assert::AreEqual(8, (int)result);

			result = ::roundtonextpower(8);
			Assert::AreEqual(8, (int)result);

			result = ::roundtonextpower(9);
			Assert::AreEqual(16, (int)result);
		}

		TEST_METHOD(RoundingLimits)
		{
			Logger::WriteMessage("Testing rounding limits");
			size_t result;

			// 128:
			result = ::roundtonextpower(129);
			Assert::AreEqual(256, (int)result);

			result = ::roundtonextpower(128);
			Assert::AreEqual(128, (int)result);

			result = ::roundtonextpower(127);
			Assert::AreEqual(128, (int)result);

			// 1024:
			result = ::roundtonextpower(1025);
			Assert::AreEqual(2048, (int)result);

			result = ::roundtonextpower(1024);
			Assert::AreEqual(1024, (int)result);

			result = ::roundtonextpower(1023);
			Assert::AreEqual(1024, (int)result);

			// 32768:
			result = ::roundtonextpower(32769);
			Assert::AreEqual(65536, (int)result);

			result = ::roundtonextpower(32768);
			Assert::AreEqual(32768, (int)result);

			result = ::roundtonextpower(32767);
			Assert::AreEqual(32768, (int)result);
		}

	};
}