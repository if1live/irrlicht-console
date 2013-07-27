// Ŭnicode please
/**
Text console interface to CVars.

This Code is covered under the LGPL.  See COPYING file for the license.

$Id: IrrConsole.h 175 2010-03-28 18:47:29Z gsibley $
*/

#pragma once

#include "cvars/Cvar.h"
#include "cvars/Timestamp.h"

////////////////////////////////////////////////////////////////////////////////
/// The type of line entered. Used to determine how each line is treated.
enum LineProperty {
	LINEPROP_LOG,         // text coming from a text being logged to the console
	LINEPROP_COMMAND,     // command entered at the console
	LINEPROP_FUNCTION,    // a function
	LINEPROP_ERROR,       // an error
	LINEPROP_HELP,         //help text
	LINEPROP_DEBUG,
	LINEPROP_INFO,
	LINEPROP_WARNING,
	NUM_LINEPROP,
};

bool onConsoleEvent(const irr::SEvent &event);
void setUpConsole(irr::IrrlichtDevice *device);

struct ConsoleConfig {
public:
	typedef irr::core::stringw WideString;
	typedef irr::core::stringc String;

public:
	//! constructor
	ConsoleConfig() { setDefaults(); }
	//! set the defaults on the console config
	void setDefaults()
	{
		dimensionRatios.X = 1.0f;
		dimensionRatios.Y = 0.8f;
		fontName = "data/font/console.bmp";
	}

	//! this contains the Width and Height ratios to the main view port (0 - 1)
	irr::core::vector2df dimensionRatios;

	//! this is the font name
	String fontName;
};


////////////////////////////////////////////////////////////////////////////////
/// A line of text contained in the console can be either inputted commands or
//  log text from the application.
class ConsoleLine {
public:
	ConsoleLine(const std::string &t, LineProperty p = LINEPROP_LOG, bool display = true)
		: m_sText(t), m_nOptions(p), m_bDisplay(display) {}
	std::string m_sText;        //< Actual text.
	LineProperty m_nOptions;    //< See LineProperty enum.
	bool m_bDisplay;            //< Display on the console screen?
};

////////////////////////////////////////////////////////////////////////////////
///  The IrrConsole class, of which there will only ever be one instance.
class IrrConsole : public irr::IEventReceiver {
public:
	IrrConsole();
	~IrrConsole();

	//render 관련
public:
	void RenderConsole(irr::gui::IGUIEnvironment* guienv, irr::video::IVideoDriver *videoDriver, const irr::u32 deltaMillis);
	void RenderText(const std::string &text, int x, int y, const irr::video::SColor &color);
	
	//commands to the console...
	void ToggleConsole();
	bool IsOpen() const { return m_bConsoleOpen; }
	bool IsChanging() const { return m_bIsChanging; }
	void OpenConsole();
	void CloseConsole();

	//! calculate the whole console rect
	void CalculateConsoleRect(const irr::core::dimension2d<irr::s32>& screenSize);
	//! get the console config reference
	ConsoleConfig& getConfig() { return m_ConsoleConfig; }
	const ConsoleConfig& getConfig() const { return m_ConsoleConfig; }

	irr::IrrlichtDevice *getDevice() { return m_pDevice; }

public:
	// call this after OpenGL is up 
	void Init(irr::IrrlichtDevice *device, const ConsoleConfig &cfg);

	//Prints to console using familiar printf style
	void Printf(const char *msg, ...);
	void Printf_All(const char *msg, ...);

	void PrintHelp(const char *msg, ... );
	void PrintError(const char *msg, ... );

	void PrintAllCVars();

	// Help.
	bool Help(const std::vector<std::string> &vArgs);

	bool HistorySave( std::string sFileName = "" );
	bool HistoryLoad( std::string sFileName = "" );

	bool SettingsSave(std::string sFileName = "");
	bool SettingsLoad(std::string sFileName = "");

	bool ScriptRun( std::string sFileName = "" );
	bool ScriptSave( std::string sFileName = "" );
	bool ScriptLoad( std::string sFileName = "" );

	// Script interface.
	void ScriptRecordStart();
	void ScriptRecordStop();
	void ScriptRecordPause();
	void ScriptShow();


	/// clears all of the console's history.
	void HistoryClear();

	/// Add a character to the command line.
	virtual bool OnEvent(const irr::SEvent &evt);
	int ProcessKey(wchar_t keyChar, irr::EKEY_CODE keyCode, bool bShiftDown, bool bControlDown);

	/// enter a full line of text to the log text.
	void EnterLogLine( const std::string &line, LineProperty prop = LINEPROP_LOG, bool display = true );

	irr::core::rect<irr::s32> GetBackgroundRect() const;

private:
	//scrolling text up and down in the console
	void ScrollUp(int pixels);
	void ScrollDown(int pixels);
	void ScrollUpLine();
	void ScrollDownLine();
	void ScrollUpPage();
	void ScrollDownPage();

	void CursorLeft();
	void CursorRight();
	void CursorToBeginningOfLine();
	void CursorToEndOfLine();

	/// Clear the current command.
	void ClearCurrentCommand();

	/// display previous command in history on command line.
	void HistoryBack();

	/// go forward in the history.
	void HistoryForward();

	/// Height of the console in pixels (even if it is currently animating).
	void          _TabComplete();
	bool          _ProcessCurrentCommand( bool bExecute = true );
	bool          _ExecuteFunction( CVarUtils::CVar<ConsoleFunc> * cvar, bool bExecute );
	bool          _IsCursorOn();
	bool          _IsConsoleFunc( TrieNode *node );
	int           _FindRecursionLevel( std::string sCommand );
	bool          _LoadExecuteHistory( std::string sFileName = "", bool bExecute=false );
	std::string   _GetHistory();

private:
	// member cvars accessible from console
	float& m_fConsoleBlinkRate;
	float& m_fConsoleAnimTime;
	int&   m_nConsoleMaxHistory;
	int&   m_nConsoleLineSpacing;
	int&   m_nConsoleLeftMargin;
	int&   m_nConsoleVerticalMargin;
	int&   m_nConsoleMaxLines;
	float& m_fOverlayPercent;
	std::string& m_sHistoryFileName;
	std::string& m_sScriptFileName;
	std::string& m_sSettingsFileName;
	std::string& m_sInitialScriptFileName;

	bool          m_bExecutingHistory; //Are we executing a script or not.
	bool          m_bSavingScript; // Are we saving a script.
	bool          m_bConsoleOpen; // whether the console is drawing or not.
	bool          m_bIsChanging;
	TimeStamp     m_Timer;
	TimeStamp     m_BlinkTimer;

	int           m_nWidth;
	int           m_nHeight;
	int           m_nViewportX;
	int           m_nViewportY;
	int           m_nViewportWidth;
	int           m_nViewportHeight;
	int           m_nTextHeight;
	int           m_nTextWidth;
	int           m_nScrollPixels;  //the number of pixels the text has been scrolled "up"
	
	int           m_nCommandNum;

	//history variables
	std::string   m_sOldCommand;
	char          m_cLastChar;
	unsigned int  m_nSuggestionNum;

	//std::string m_sMaxPrefix; // max prefix
	std::string             m_sCurrentCommandBeg; //< command being typed, up to cursor
	std::string             m_sCurrentCommandEnd; //< command being typed, after cursor
	std::deque<ConsoleLine> m_sConsoleText;       //< all lines of console text
	std::deque<ConsoleLine> m_ScriptText;         //< lines of console text marked as script
	
	//! console config data
	ConsoleConfig m_ConsoleConfig;

	//! the console rectangle
	irr::core::rect<irr::s32> m_ConsoleRect;
	irr::core::dimension2d<irr::s32> m_ScreenSize;

	//! the font of the console
	irr::gui::IGUIFont* m_pGuiFont;

	irr::IrrlichtDevice *m_pDevice;
};


extern IrrConsole *g_console;