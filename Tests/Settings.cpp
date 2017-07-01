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
	TEST_CLASS(SettingsTest)
	{

	public:
		TEST_CLASS_INITIALIZE(ClassInitialize)
		{
			Logger::WriteMessage("In SettingsTest Class Initialize");

			/*
			11 functions:
			"S:+Cancel Overwrite Mode moving from current line"),pfBlockOverwrite,0,TRUE , NULL},
			"S:+Autoclose XHTML/XML <Tag>"),pfAutoCloseHTMLtag,0,TRUE , NULL},
			"S:+Autoclose {([Brace"),pfAutoCloseBrace,0,TRUE , NULL},
			"S:+Autoconvert typed leading spaces to tabs"),pfAutoSpace2Tab,0,TRUE , NULL},
			"S:+Autoconvert typed HTML/XML to &&entities;"),pfAutoConvertHTML,0,TRUE , NULL},
			"S:+Disable Subclassing && advanced features"),pfDisableSubclassing
			"S:+Move quick menus out of 'Plugins' menu"),pfSeparateQuickMenus
			"S:+Ctrl-D also dups marked text"),pfCtrlDAlsoDupsBlock
			"S:Visit Notepad++ && ") PLUGIN_NAME_MENU NPPTEXT(" website"),pfhNotepadweb
			"S:Help"),pfhelp
			"S:About ")PLUGIN_NAME_MENU,pfabout
			*/
		}


	};
}