// Ŭnicode please 
#include "stdafx.h"
#include "irr_console.h"

#include "cvars/CVar.h"
#include "cvars/TrieNode.h"

#include "console_function.h"
#include "string_util.h"

/// TODO these should move to CVARS
#define CONSOLE_HELP_FILE "helpfile.txt"
#define CONSOLE_HISTORY_FILE ".cvar_history"
#define CONSOLE_SETTINGS_FILE ".console_settings"
#define CONSOLE_SCRIPT_FILE "default.script"
#define CONSOLE_INITIAL_SCRIPT_FILE "initial.script"

using namespace irr;
using namespace irr::core;
using namespace irr::video;
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::ostringstream;

IrrConsole consoleLocal;
IrrConsole *g_console = &consoleLocal;

const irr::video::SColor logColor(255, 255, 255, 64);
const irr::video::SColor commandColor(255, 255, 255, 255);
const irr::video::SColor functionColor(255, 64, 255, 64);
const irr::video::SColor errorColor(255, 255, 128, 64);
const irr::video::SColor helpColor(255,  110, 130, 200);
const irr::video::SColor consoleColor(120, 25, 60, 130);
const irr::video::SColor debugColor(255, 255, 128, 64);
const irr::video::SColor infoColor(255, 255, 128, 64);
const irr::video::SColor warningColor(255, 255, 128, 64);

const std::array<SColor, NUM_LINEPROP> &getLineColorTable()
{
	static bool init = false;
	static std::array<SColor, NUM_LINEPROP> colorList;
	if(init == false) {
		init = true;
		colorList[LINEPROP_LOG] = logColor;
		colorList[LINEPROP_COMMAND] = commandColor;
		colorList[LINEPROP_FUNCTION] = functionColor;
		colorList[LINEPROP_ERROR] = errorColor;
		colorList[LINEPROP_HELP] = helpColor;
		colorList[LINEPROP_DEBUG] = debugColor;
		colorList[LINEPROP_INFO] = infoColor;
		colorList[LINEPROP_WARNING] = warningColor;
	}
	return colorList;
}
const SColor getLineColor(LineProperty p)
{
	return getLineColorTable()[p];
}

////////////////////////////////////////////////////////////////////////////////
/// Return whether first element is greater than the second.
bool StringIndexPairGreater( const std::pair<std::string,int> &e1, const std::pair<std::string,int> &e2 );

////////////////////////////////////////////////////////////////////////////////
/// Utility function.
std::string FindLevel( const std::string &sString, int iMinRecurLevel );

////////////////////////////////////////////////////////////////////////////////
/// remove all spaces from the front and back...
std::string& RemoveSpaces( std::string &str );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Initialise the console. Sets up all the default values.
void IrrConsole::Init(irr::IrrlichtDevice *device, const ConsoleConfig &cfg)
{
	m_pDevice = device;
	m_ConsoleConfig = cfg;
	//load the font
	auto guienv = device->getGUIEnvironment();
	m_pGuiFont = guienv->getFont(cfg.fontName.c_str());
	if(m_pGuiFont == 0) {
		m_pGuiFont = guienv->getBuiltInFont();
	}
	cout << "font loaded!!" << std::endl;

	//calculate the console rectangle
	m_ScreenSize = device->getVideoDriver()->getScreenSize();
	CalculateConsoleRect(m_ScreenSize);

	m_bExecutingHistory = false;
	m_bSavingScript = false;
	m_bConsoleOpen = false;
	m_bIsChanging = false;
	m_nScrollPixels = 0;
	m_nCommandNum = 0;

	// TODO Hard coded for now?
	m_nTextHeight = 14;  
	m_nTextWidth = 15;

	// setup member CVars
	m_Timer.Stamp();
	m_BlinkTimer.Stamp();

	// add basic functions to the console
	CVarUtils::CreateCVar( "console.version", ConsoleVersion, "The current version of IrrConsole" );
	CVarUtils::CreateCVar( "help", ConsoleHelp, "Gives help information about the console or more specifically about a CVar." );
	CVarUtils::CreateCVar( "find", ConsoleFind, "find 'name' will return the list of CVars containing 'name' as a substring." );
	CVarUtils::CreateCVar( "exit", ConsoleExit, "Close the application" );
	CVarUtils::CreateCVar( "quit", ConsoleExit, "Close the application" );
	CVarUtils::CreateCVar( "save", ConsoleSave, "Save the CVars to a file" );
	CVarUtils::CreateCVar( "load", ConsoleLoad, "Load CVars from a file" );

	CVarUtils::CreateCVar( "console.history.load", ConsoleHistoryLoad, "Load console history from a file" );
	CVarUtils::CreateCVar( "console.history.save", ConsoleHistorySave, "Save the console history to a file" );
	CVarUtils::CreateCVar( "console.history.clear", ConsoleHistoryClear, "Clear the current console history" );

	CVarUtils::CreateCVar( "console.settings.load", ConsoleSettingsLoad, "Load console settings from a file" );
	CVarUtils::CreateCVar( "console.settings.save", ConsoleSettingsSave, "Save the console settings to a file" );

	CVarUtils::CreateCVar( "script.record.start", ConsoleScriptRecordStart );
	CVarUtils::CreateCVar( "script.record.stop", ConsoleScriptRecordStop );
	CVarUtils::CreateCVar( "script.record.pause", ConsoleScriptRecordPause );
	CVarUtils::CreateCVar( "script.show", ConsoleScriptShow );
	CVarUtils::CreateCVar( "script.run", ConsoleScriptRun );
	CVarUtils::CreateCVar( "script.save", ConsoleScriptSave );
	CVarUtils::CreateCVar( "script.load", ConsoleScriptLoad );

	//load the default settings file
	SettingsLoad();

	//load the history file
	HistoryLoad();

	//load the initial execute script
	std::ifstream ifs( m_sInitialScriptFileName.c_str() );

	if( ifs.is_open() ) {
		ifs.close();
		ScriptRun(m_sInitialScriptFileName);
	} else {
		//std::cout << "Info: Initial script file, " << m_sInitialScriptFileName << ", not found." << std::endl;
		ifs.clear(std::ios::failbit);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Save settings current console settings to a file
//  @param sFileName the file to save to. if none given the one specified in
//  console.SettingsFileName is use 
//  @return sucess or failure
bool IrrConsole::SettingsSave(std::string sFileName)
{
	if( !m_bExecutingHistory ) {
		if( sFileName == ""){
			if(m_sSettingsFileName != "") {
				sFileName = m_sSettingsFileName;
			}
			else {
				PrintError( "Warning: No default name. Resetting settings filename to: \"%s\".", CONSOLE_SETTINGS_FILE );
				sFileName = m_sHistoryFileName = CONSOLE_SETTINGS_FILE;
			}
		}

		std::ofstream ofs( sFileName.c_str() );

		if( !ofs.is_open() ) {
			PrintError("Error: could not open \"%s\" for saving.", sFileName.c_str());
			return false;
		}

		std::vector<std::string> vSave;
		vSave.push_back(sFileName);
		vSave.push_back("console");
		vSave.push_back("script");
		ConsoleSave( vSave );

		ofs.close();
	}
	return true;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Load console settings from a file.
//  @param sFileName the file to load from. if none given the one specified in
//  console.SettingsFileName is used
//  @return sucess or failure
bool IrrConsole::SettingsLoad( std::string sFileName )
{
	if( sFileName == "" ) {
		if( m_sSettingsFileName != "" ) {
			sFileName = m_sSettingsFileName;
		}
		else {
			PrintError( "Warning: No default name. Resetting settigns filename to: \"%s\".", CONSOLE_SETTINGS_FILE );
			sFileName = m_sSettingsFileName = CONSOLE_SETTINGS_FILE;
		}
	}

	//test if file exists
	std::ifstream ifs( sFileName.c_str() );

	if( ifs.is_open() ) {
		ifs.close();
		std::vector<std::string> v;
		v.push_back(sFileName);
		ConsoleLoad(v);
	}
	else {
		//        std::cout << "Info: Settings file, " << sFileName << ", not found." << std::endl;
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// Return whether first element is greater than the second.
bool StringIndexPairGreater( const std::pair<std::string,int> &e1, const std::pair<std::string,int> &e2 )
{
	return e1.first < e2.first;
}

////////////////////////////////////////////////////////////////////////////////
/// Utility function.
std::string FindLevel( const std::string &sString, int iMinRecurLevel )
{   
	int level = 0;
	int index = sString.length();
	for( unsigned int ii = 0; ii < sString.length(); ii++ ) {
		if( sString.c_str()[ii]=='.' ) {
			level ++;
		}
		if( level == iMinRecurLevel ) {
			index = ii+1;
		}
	}
	return sString.substr( 0, index );
}

////////////////////////////////////////////////////////////////////////////////
/// remove all spaces from the front and back...
std::string& RemoveSpaces( std::string &str )
{
	// remove them off the front
	int idx = str.find_first_not_of( ' ' );
	if( idx > 0 && idx != 0 ) {
		str = str.substr( idx, str.length() );
	}

	// remove them off the back
	idx = str.find_last_not_of(' ');
	if( idx != -1 ) {
		str = str.substr( 0, idx+1 );
	}
	return str;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constructor.
IrrConsole::IrrConsole()
	: // Init our member cvars  (can't init the names in the class decleration) 
	m_fConsoleBlinkRate( CVarUtils::CreateCVar<float>(    "console.BlinkRate", 4.0f ) ), // cursor blinks per sec
	m_fConsoleAnimTime( CVarUtils::CreateCVar<float>(     "console.AnimTime", 0.1f ) ),     // time the console animates
	m_nConsoleMaxHistory( CVarUtils::CreateCVar<int>(     "console.history.MaxHistory", 100 ) ), // max lines ofconsole history
	m_nConsoleLineSpacing( CVarUtils::CreateCVar<int>(    "console.LineSpacing", 5 ) ), // pixels between lines
	m_nConsoleLeftMargin( CVarUtils::CreateCVar<int>(     "console.LeftMargin", 5 ) ),   // left margin in pixels
	m_nConsoleVerticalMargin( CVarUtils::CreateCVar<int>( "console.VertMargin", 8 ) ),
	m_nConsoleMaxLines( CVarUtils::CreateCVar<int>(       "console.MaxLines", 4000 ) ),
	m_fOverlayPercent( CVarUtils::CreateCVar<float>(      "console.OverlayPercent", 0.75f ) ),
	m_sHistoryFileName( CVarUtils::CreateCVar<> (         "console.history.HistoryFileName", std::string( CONSOLE_HISTORY_FILE ) ) ),
	m_sScriptFileName( CVarUtils::CreateCVar<> (          "script.ScriptFileName", std::string( CONSOLE_SCRIPT_FILE ) ) ),
	m_sSettingsFileName( CVarUtils::CreateCVar<> (        "console.settings.SettingsFileName", std::string( CONSOLE_SETTINGS_FILE ) ) ),
	m_sInitialScriptFileName( CVarUtils::CreateCVar<> (   "console.InitialScriptFileName", std::string( CONSOLE_INITIAL_SCRIPT_FILE ) ) ),
	m_pGuiFont(nullptr),
	m_pDevice(nullptr)
{
	m_bSavingScript = false;
	m_bConsoleOpen = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Destructor.
IrrConsole::~IrrConsole()
{
	HistorySave();
	SettingsSave();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
* This will output text to the GL console
*/
void IrrConsole::Printf(const char *msg, ... )
{
	char msgBuf[1024];
	va_list va_alist;

	if (!msg) return;

	va_start( va_alist, msg );
	vsnprintf( msgBuf, 1024, msg, va_alist );
	va_end( va_alist );
	msgBuf[1024 - 1] = '\0';

	EnterLogLine( msgBuf );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
* same as Printf() except it also prints to the terminal
*/
void IrrConsole::Printf_All(const char *msg, ... )
{
	char msgBuf[1024];
	va_list va_alist;

	if (!msg) return;

	va_start( va_alist, msg );
	vsnprintf( msgBuf, 1024, msg, va_alist );
	va_end( va_alist );
	msgBuf[1024 - 1] = '\0';

	EnterLogLine( msgBuf );
	printf( "%s", msgBuf );
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// This will output help formatted text to the GL console.
void IrrConsole::PrintHelp(const char *msg, ... )
{
	char msgBuf[1024];
	va_list va_alist;

	if (!msg) return;

	va_start( va_alist, msg );
	vsnprintf( msgBuf, 1024, msg, va_alist );
	va_end( va_alist );
	msgBuf[1024 - 1] = '\0';

	EnterLogLine( msgBuf, LINEPROP_HELP );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// This will output error formatted text to the GL console.
void IrrConsole::PrintError(const char *msg, ... )
{
	char msgBuf[1024];
	va_list va_alist;

	if (!msg) return;

	va_start( va_alist, msg );
	vsnprintf( msgBuf, 1024, msg, va_alist );
	va_end( va_alist );
	msgBuf[1024 - 1] = '\0';

	EnterLogLine( msgBuf, LINEPROP_ERROR );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Clears all of the console's history.
void IrrConsole::HistoryClear()
{	
	m_sConsoleText.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Save the console history to the named file.
//  If no file is specified the name held in the history.HistoryFileName CVar will be used.
//  @param sFilename save history to this file.
//  @return successor failure
bool IrrConsole::HistorySave( std::string sFileName )
{
	///@TODO check filenames for validity - no spaces or illegal characters.
	if( !m_bExecutingHistory ) {
		if( sFileName == ""){
			if(m_sHistoryFileName != "") {
				sFileName = m_sHistoryFileName;
			}
			else {
				PrintError( "Warning: No default name. Resetting history filename to: \"%s\".", CONSOLE_HISTORY_FILE );
				sFileName = m_sHistoryFileName = CONSOLE_HISTORY_FILE;
			}
		}

		std::ofstream ofs( sFileName.c_str() );

		if( !ofs.is_open() ) {
			PrintError( "Error: could not open \"%s\" for saving.", sFileName.c_str() );
			m_bExecutingHistory = false;
			return false;
		}

		unsigned int nTextSize = m_sConsoleText.size();
		for( int ii = nTextSize-1; ii >= 0 ; --ii ) {
			if( m_sConsoleText[ii].m_nOptions == LINEPROP_COMMAND ) {
				ofs << m_sConsoleText[ii].m_sText << "\n";
			}
		}
		ofs.close();
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Start script recording.
void IrrConsole::ScriptRecordStart()
{	
	m_ScriptText.clear();
	m_bSavingScript = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Stop script recording.
void IrrConsole::ScriptRecordStop()
{	
	if( !m_bSavingScript ) {
		return;
	}
	m_ScriptText.pop_front();
	m_ScriptText.pop_front();
	m_bSavingScript = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pause script recording.
void IrrConsole::ScriptRecordPause()
{
	// unpause
	if( !m_bSavingScript && !m_ScriptText.empty() )
	{
		m_ScriptText.pop_front();
		m_ScriptText.pop_front();
		m_bSavingScript = true;
		return;
	}
	else  // pause
	{
		m_ScriptText.pop_front();
		m_ScriptText.pop_front();
		m_bSavingScript = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Print the script to the console.
void IrrConsole::ScriptShow()
{	
	if( m_bSavingScript ) {
		m_ScriptText.pop_front();
		m_ScriptText.pop_front();
	}
	bool bWasSavingScript = m_bSavingScript;
	m_bSavingScript = false;
	for( int ii = m_ScriptText.size()-1; ii >= 0 ; --ii ) {
		if( m_ScriptText[ii].m_nOptions == LINEPROP_COMMAND ) {
			EnterLogLine( m_ScriptText[ii].m_sText, LINEPROP_COMMAND );
		}
	}
	m_bSavingScript = bWasSavingScript;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  Run the current script or the one specified on disk.
//   @TODO currently overwrites the script in memory. need to allow multiple scripts 
//   to be held and ran for recursion.
bool IrrConsole::ScriptRun( std::string sFileName )
{
	if(!sFileName.empty()){
		bool bsuccess = ScriptLoad(sFileName);
		if(!bsuccess){
			PrintError("Aborting script run");
			return false;
		}
	}

	if( m_bSavingScript ) {
		m_ScriptText.pop_front();
		m_ScriptText.pop_front();
	}
	bool bWasSavingScript = m_bSavingScript;
	m_bSavingScript = false;
	for( int ii = m_ScriptText.size()-1; ii >= 0 ; --ii ) {
		if( m_ScriptText[ii].m_nOptions == LINEPROP_COMMAND ) {
			m_sCurrentCommandBeg = m_ScriptText[ii].m_sText;
			m_sCurrentCommandEnd = "";
			_ProcessCurrentCommand( true );
			m_sCurrentCommandBeg = "";
		}
	}
	m_bSavingScript = bWasSavingScript;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Save the current script to a file.
bool IrrConsole::ScriptSave( std::string sFileName )
{	
	if( !m_bExecutingHistory ) {
		m_bExecutingHistory = true;

		if( m_bSavingScript ) {
			m_ScriptText.pop_front();
			m_ScriptText.pop_front();
		}

		if( sFileName == "" ) {
			sFileName = CVarUtils::GetCVar<std::string>( "script.ScriptFileName" );
		}

		std::ofstream ofs( sFileName.c_str() );

		if( !ofs.is_open() ) {
			PrintError( "Error: could not open \"%s\" for saving.", sFileName.c_str() );
			m_bExecutingHistory = false;
			return false;
		}

		unsigned int nTextSize = m_ScriptText.size();

		for( int ii = nTextSize-1; ii >= 0 ; --ii ) {
			if( m_ScriptText[ii].m_nOptions == LINEPROP_COMMAND ) {
				ofs << m_ScriptText[ii].m_sText << "\n";
			}
		}

		ofs.close();
		m_bExecutingHistory = false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Load history from a file.
//  @param sFileName Load history from this file, else use default.
//  @return sucess or failure.
bool IrrConsole::HistoryLoad( std::string sFileName )
{
	if( sFileName == "" ) {
		if(m_sHistoryFileName != "")
			sFileName = m_sHistoryFileName;
		else {
			PrintError("Warning: No default name. Resetting history filename to: \"%s\".", CONSOLE_HISTORY_FILE);
			sFileName = m_sHistoryFileName = CONSOLE_HISTORY_FILE;
		}
	}

	//test if file exists
	std::ifstream ifs( sFileName.c_str() );

	if( ifs.is_open() ) {
		ifs.close();
		return _LoadExecuteHistory( sFileName, false );
	}
	else {
		//        std::cout << "Info: History file, " << sFileName << ", not found." << std::endl;
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Load a script from a file.
bool IrrConsole::ScriptLoad( std::string sFileName )
{
	if( sFileName == "") {
		if(m_sScriptFileName != "") {
			sFileName = m_sScriptFileName;
		}
		else {
			PrintError("Warning: No default name. Resetting script filename to: \"%s\".", CONSOLE_SCRIPT_FILE);
			sFileName = m_sScriptFileName = CONSOLE_SCRIPT_FILE;
		}
	}

	//test if file exists
	std::ifstream ifs( sFileName.c_str() );

	if( ifs.is_open() ) {
		ifs.close();
		return _LoadExecuteHistory( sFileName, true );
	}
	else {
		//        std::cout << "Info: Script file, " << sFileName << ", not found." << std::endl;
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Load history from a file and execute it if desired.
bool IrrConsole::_LoadExecuteHistory( std::string sFileName, bool bExecute )
{
	if( sFileName == "" ) {
		std::cerr << "_LoadExecuteHistory: No file specified. There is a bug in IrrConsole. Please report it." << std::endl;
		return false;
	}

	if( !m_bExecutingHistory ) {
		m_bExecutingHistory = true;

		std::ifstream ifs( sFileName.c_str() );

		if( !ifs.is_open() ) {
			PrintError("Error: could not open \"%s\" for loading.", sFileName.c_str());
			m_bExecutingHistory = false;
			return false;
		}

		while( ifs.good() ) {
			char linebuf[1024];
			ifs.getline(linebuf, 1024);
			if( ifs.good() ) {
				m_sCurrentCommandBeg = linebuf;
				m_sCurrentCommandEnd = "";
				_ProcessCurrentCommand( bExecute );
				m_sCurrentCommandBeg = "";
			}
		}
		m_bExecutingHistory = false;

		ifs.close();
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool IrrConsole::_IsCursorOn()
{
	float elapsed = static_cast<float>(m_BlinkTimer.Elapsed());
	if(elapsed > (1.0 / m_fConsoleBlinkRate)) {
		m_BlinkTimer.Stamp();
		return true;
	}
	else if( elapsed > 0.50*(1.0 / m_fConsoleBlinkRate) ) {
		return false;
	}
	else{
		return true;
	}
}

////////////////////////////////////////////////////////////////////////////////
void IrrConsole::CursorLeft()
{
	if( m_bConsoleOpen ) {
		if( m_sCurrentCommandBeg.length()>0 ) {
			m_sCurrentCommandEnd 
				= m_sCurrentCommandBeg.substr( m_sCurrentCommandBeg.length()-1, m_sCurrentCommandBeg.length() )
				+ m_sCurrentCommandEnd;
			m_sCurrentCommandBeg 
				= m_sCurrentCommandBeg.substr( 0, m_sCurrentCommandBeg.length()-1 );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void IrrConsole::CursorRight()
{
	if( m_bConsoleOpen ) {
		if( m_sCurrentCommandEnd.length()>0 ) {
			m_sCurrentCommandBeg += m_sCurrentCommandEnd.substr( 0, 1 );
			m_sCurrentCommandEnd = m_sCurrentCommandEnd.substr( 1, m_sCurrentCommandEnd.length() );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void IrrConsole::CursorToBeginningOfLine()
{
	if( m_bConsoleOpen ) {
		m_sCurrentCommandEnd = m_sCurrentCommandBeg+m_sCurrentCommandEnd;
		m_sCurrentCommandBeg = "";
	}
}

////////////////////////////////////////////////////////////////////////////////
void IrrConsole::CursorToEndOfLine()
{
	if( m_bConsoleOpen ) {
		m_sCurrentCommandBeg += m_sCurrentCommandEnd;
		m_sCurrentCommandEnd = "";
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Process key presses.
bool IrrConsole::OnEvent(const irr::SEvent &event)
{
	//! the tilde (~/`) key 
	const wchar_t IC_KEY_TILDE = irr::KEY_OEM_3;	// for US    "`~"

	if(event.EventType == irr::EET_KEY_INPUT_EVENT ) {
		if(event.KeyInput.PressedDown) {
			if(event.KeyInput.Key == irr::KEY_ESCAPE) {
				if(IsOpen()) {
					CloseConsole();
					return true;
				}
			} else if(event.KeyInput.Key == IC_KEY_TILDE) {
				if(!IsOpen()) {
					OpenConsole();
					return true;
				} else if(!event.KeyInput.Control) {
					CloseConsole();
					return true;
				}
			} if(IsOpen()) {
				ProcessKey(event.KeyInput.Char, event.KeyInput.Key,event.KeyInput.Shift, event.KeyInput.Control);
				return true;
			}
		}
	} else if(event.EventType == irr::EET_LOG_TEXT_EVENT) {
		std::array<LineProperty, 5> linePropTable;
		linePropTable[ELL_DEBUG] = LINEPROP_DEBUG;
		linePropTable[ELL_INFORMATION] = LINEPROP_INFO;
		linePropTable[ELL_WARNING] = LINEPROP_WARNING;
		linePropTable[ELL_ERROR] = LINEPROP_ERROR;
		linePropTable[ELL_NONE] = LINEPROP_COMMAND;
		LineProperty lineProperty = linePropTable[event.LogEvent.Level];
		EnterLogLine(event.LogEvent.Text, lineProperty);
		return true;
	} else if(event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
		return IsOpen();
	}
	return false;
}

int IrrConsole::ProcessKey(wchar_t keyChar, irr::EKEY_CODE keyCode, bool bShiftDown, bool bControlDown)
{
	if(false) {
		ostringstream oss;
		oss << "got " << (char)keyChar << "/" << keyCode;
		oss << "(shift=" << bShiftDown << ")";
		oss << "(ctrl=" << bControlDown << ")";
		cout << oss.str() << endl;
	}

	// ctrl과 조합해서 단축키를 사용하는경우
	// 이리 처리해서 나머지 분기를 통과하는쪼깅 유지보수에 유리하다
	if(bControlDown) {
		if(keyCode == KEY_KEY_A) {
			CursorToBeginningOfLine();
		} else if(keyCode == KEY_KEY_E) {
			CursorToEndOfLine();
		}
		return 1;
	}


	assert(bControlDown == false);
	if(keyCode == KEY_LEFT) {
		CursorLeft();
	} else if(keyCode == KEY_RIGHT) {
		CursorRight();
	} else if(keyCode == KEY_PRIOR) {
		//page up
		ScrollUpPage();
	} else if(keyCode == KEY_NEXT) {
		//page down
		ScrollDownPage();
	} else if(keyCode == KEY_UP) {
		if(bShiftDown) {
			ScrollUpLine();
		} else {
			HistoryBack();
		}
	} else if(keyCode == KEY_DOWN) {
		if(bShiftDown) {
			ScrollDownLine();
		} else {
			HistoryForward();
		}
	} else if(keyCode == KEY_HOME) {
		CursorToBeginningOfLine();
	} else if(keyCode == KEY_END) {
		CursorToEndOfLine();
	} else if(keyCode == KEY_TAB) {
		_TabComplete();
	} else if(keyCode == KEY_RETURN) {
		_ProcessCurrentCommand();
		m_sCurrentCommandBeg = "";
		m_sCurrentCommandEnd = "";
		m_nCommandNum = 0; //reset history
		m_nScrollPixels = 0; //reset scrolling
	} else if(keyCode == KEY_BACK) {
		// backspace
		if( m_sCurrentCommandBeg.size() > 0 ) {
			m_sCurrentCommandBeg = m_sCurrentCommandBeg.substr(0, m_sCurrentCommandBeg.size() - 1);
		}
	} else if(keyCode == KEY_DELETE) {
		if( m_sCurrentCommandEnd.size() > 0 ) {
			m_sCurrentCommandEnd = m_sCurrentCommandEnd.substr(1, m_sCurrentCommandEnd.size() );
		}
	} else {
		if(keyChar != 0) {
			m_sCurrentCommandBeg += static_cast<char>(keyChar); // just add the key to the string
			m_nCommandNum = 0; //reset history
			m_nScrollPixels = 0; //reset scrolling
		}
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IrrConsole::ClearCurrentCommand() 
{
	m_sCurrentCommandBeg = ""; 
	m_sCurrentCommandEnd = ""; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Add a line to the history log.
void IrrConsole::EnterLogLine(const std::string &line, const LineProperty prop, bool display)
{
	if( (int)m_sConsoleText.size() >= m_nConsoleMaxHistory ) {
		m_sConsoleText.pop_back();
	}

	m_sConsoleText.push_front(ConsoleLine(line, prop, display));
	if( m_bSavingScript && prop != LINEPROP_ERROR ) {
		m_ScriptText.push_front(ConsoleLine(line, prop, display));
	}
}

////////////////////////////////////////////////////////////////////////////////
/// Retreives command in history, if m_nCommandNum is invalid then
//  m_sCurrentCommand is returned.
std::string IrrConsole::_GetHistory()
{
	std::deque<ConsoleLine>::iterator it;
	int commandCount = 1;

	if( m_nCommandNum <= 0 ) {
		return m_sCurrentCommandBeg+m_sCurrentCommandEnd;
	}

	for( it = m_sConsoleText.begin() ; it != m_sConsoleText.end() ; it++ ) {
		if( it->m_nOptions == LINEPROP_COMMAND ) {
			if( commandCount == m_nCommandNum ) {
				return it->m_sText;
			}
			else {//not the right command
				commandCount++;
			}
		}
	}
	if(  m_nCommandNum > commandCount ) {
		m_nCommandNum = commandCount-1;
	}

	return m_sCurrentCommandBeg+m_sCurrentCommandEnd;
}

////////////////////////////////////////////////////////////////////////////////
void IrrConsole::HistoryBack()
{
	if(m_nCommandNum <= 0 ) {
		m_sOldCommand = m_sCurrentCommandBeg+m_sCurrentCommandEnd;
	}
	m_nCommandNum++;
	std::string temp(m_sCurrentCommandBeg+m_sCurrentCommandEnd);
	m_sCurrentCommandBeg = _GetHistory();
	m_sCurrentCommandEnd = "";
}

////////////////////////////////////////////////////////////////////////////////
void IrrConsole::HistoryForward()
{
	if( m_nCommandNum > 0 ) {
		m_nCommandNum--;
		if( m_nCommandNum == 0 ) { //restore old command line
			m_sCurrentCommandBeg = m_sOldCommand;
			m_sCurrentCommandEnd = "";
		}
		else{
			m_sCurrentCommandBeg = _GetHistory();
			m_sCurrentCommandEnd = "";
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
bool IrrConsole::_IsConsoleFunc( TrieNode *node )
{
	if( typeid( ConsoleFunc ).name() 
		== ((CVarUtils::CVar<int>*)node->m_pNodeData)->type() ) {
			return true;
	}

	return false; 
}

////////////////////////////////////////////////////////////////////////////////
int IrrConsole::_FindRecursionLevel( std::string sCommand )
{
	int level = 0;
	for( unsigned int ii = 0; ii < sCommand.length(); ii++ ) {
		if( sCommand.c_str()[ii]=='.' ) {
			level ++;
		}
	}
	return level;
}


void IrrConsole::PrintAllCVars()
{
	TrieNode* node = g_pCVarTrie->FindSubStr(  RemoveSpaces( m_sCurrentCommandBeg ) );
	if( !node ) {
		return;
	}

	std::cout << "CVars:" << std::endl;

	// Retrieve suggestions (retrieve all leaves by traversing from current node)
	std::vector<TrieNode*> suggest = g_pCVarTrie->CollectAllNodes( node );
	std::sort( suggest.begin(), suggest.end() );
	//output suggestions
	unsigned int nLongestName = 0;
	unsigned int nLongestVal = 0;
	for( unsigned int ii = 0; ii < suggest.size(); ii++ ){
		std::string sName = ( (CVarUtils::CVar<int>*) suggest[ii]->m_pNodeData )->m_sVarName;
		std::string sVal = CVarUtils::GetValueAsString( suggest[ii]->m_pNodeData );
		if( sName.length() > nLongestName ){
			nLongestName = sName.length();
		}
		if( sVal.length() > nLongestVal ){
			nLongestVal = sVal.length();
		}
	}

	if( suggest.size() > 1) {
		for( unsigned int ii = 0; ii < suggest.size(); ii++ ){
			std::string sName = ( (CVarUtils::CVar<int>*) suggest[ii]->m_pNodeData )->m_sVarName;
			std::string sVal = CVarUtils::GetValueAsString( suggest[ii]->m_pNodeData );
			std::string sHelp = CVarUtils::GetHelp( sName ); 
			sName.resize( nLongestName, ' ' );
			sVal.resize( nLongestVal, ' ' );
			printf( "%-s: Default value = %-30s   %-50s\n", sName.c_str(), sVal.c_str(), sHelp.empty() ? "" : sHelp.c_str() );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void IrrConsole::_TabComplete()
{
	TrieNode* node = g_pCVarTrie->FindSubStr(  RemoveSpaces( m_sCurrentCommandBeg ) );
	if( !node ) {
		return;
	}
	else if( node->m_nNodeType == TRIE_LEAF || (node->m_children.size() == 0) ) {
		node = g_pCVarTrie->Find(m_sCurrentCommandBeg);
		if( !_IsConsoleFunc( node ) ) {
			m_sCurrentCommandBeg += " = " + CVarUtils::GetValueAsString(node->m_pNodeData);
		}
	} else {
		// Retrieve suggestions (retrieve all leaves by traversing from current node)
		std::vector<TrieNode*> suggest = g_pCVarTrie->CollectAllNodes(node);
		//output suggestions
		if( suggest.size() > 1) {
			std::vector<std::pair<std::string,int> > suggest_name_index_full;            
			std::vector<std::pair<std::string,int> > suggest_name_index_set;
			// Build list of names with index from suggest
			// Find lowest recursion level
			int iMinRecurLevel = 100000;
			for( unsigned int ii = 0; ii < suggest.size(); ii++ ) {
				std::string sName = ( (CVarUtils::CVar<int>*) suggest[ii]->m_pNodeData )->m_sVarName;
				suggest_name_index_full.push_back( std::pair<std::string,int>( sName, ii ) );
				if( _FindRecursionLevel( sName ) < iMinRecurLevel ) {
					iMinRecurLevel = _FindRecursionLevel( sName );
				}
			}
			// Sort alphabetically (this is useful for later removing
			// duplicate name at a given level and looks nice too...)
			std::sort( suggest_name_index_full.begin(), suggest_name_index_full.end(), StringIndexPairGreater );

			// Remove suggestions at a higher level of recursion
			std::string sCurLevel = "";
			int iCurLevel;
			for( unsigned int ii = 0; ii < suggest_name_index_full.size() ; ii++ ) {
				std::string sCurString = suggest_name_index_full[ii].first;
				iCurLevel = _FindRecursionLevel( sCurString );
				if( sCurLevel.length()==0 ) {
					if( iCurLevel == iMinRecurLevel ) {
						sCurLevel = "";    
						suggest_name_index_set.
							push_back( std::pair<std::string,int>( sCurString,suggest_name_index_full[ii].second ) );
					} else {
						// Add new substring at given level
						sCurLevel = FindLevel( sCurString, iMinRecurLevel );
						suggest_name_index_set.push_back( std::pair<std::string,int>( sCurLevel,suggest_name_index_full[ii].second ) );
					}
				} else {
					if( sCurString.find( sCurLevel ) == std::string::npos ) {
						// Add new substring at given level
						sCurLevel = FindLevel( sCurString, iMinRecurLevel );
						suggest_name_index_set.push_back( std::pair<std::string,int>( sCurLevel,suggest_name_index_full[ii].second ) );
					} 
				}
			}

			// Get all commands and function separately
			// Print out all suggestions to the console
			std::string commands, functions; //collect each type separately

			unsigned int longest = 0;
			for( unsigned int ii = 0; ii < suggest_name_index_set.size(); ii++ ) {
				if( suggest_name_index_set[ii].first.length() > longest ) {
					longest = suggest_name_index_set[ii].first.length();
				}
			}
			longest +=3;

			std::vector<std::string> cmdlines;
			std::vector<std::string> funclines;

			// add command lines
			for( unsigned int ii = 0; ii < suggest_name_index_set.size() ; ii++ ) {
				std::string tmp = suggest_name_index_set[ii].first;
				tmp.resize( longest, ' ' );

				if( (commands+tmp).length() > m_ScreenSize.Width / m_nTextWidth ) {
					cmdlines.push_back( commands );
					commands.clear();
				}
				if( !_IsConsoleFunc( suggest[suggest_name_index_set[ii].second] ) ) {
					commands += tmp;
				}
			}
			if( commands.length() ) cmdlines.push_back( commands );

			// add function lines
			for( unsigned int ii = 0; ii < suggest_name_index_set.size() ; ii++ ) {
				std::string tmp = suggest_name_index_set[ii].first;
				tmp.resize( longest, ' ' );
				if( (functions+tmp).length() > m_ScreenSize.Width / m_nTextWidth ) {
					funclines.push_back( functions );
					functions.clear();
				}
				if( _IsConsoleFunc( suggest[suggest_name_index_set[ii].second] ) ) {
					functions += tmp;
				}
			}
			if( functions.length() ) funclines.push_back( functions );

			// enter the results
			if( cmdlines.size() + funclines.size() > 0 ) {
				EnterLogLine( " ", LINEPROP_LOG );
			}
			for( unsigned int ii = 0; ii < cmdlines.size(); ii++ ) {
				EnterLogLine( cmdlines[ii], LINEPROP_LOG );
			}
			for( unsigned int ii = 0; ii < funclines.size(); ii++ ) {
				EnterLogLine( funclines[ii], LINEPROP_FUNCTION );
			}


			//do partial completion - look for paths with one child down the trie
			int c = m_sCurrentCommandBeg.length();
			while(node->m_children.size() == 1) {
				node = node->m_children.front();
				c++;
			}
			m_sCurrentCommandBeg = suggest_name_index_set[0].first.substr(0, c);
		} else if( suggest.size() == 1 ) {
			// Is this what the use wants? Clear the left bit...
			m_sCurrentCommandEnd = "";
			m_sCurrentCommandBeg = ((CVarUtils::CVar<int>*) suggest[0]->m_pNodeData)->m_sVarName;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// commands are in the following form:
// [Command] = value //sets a value
// or
// [Command] //prints out the command's value
bool IrrConsole::_ProcessCurrentCommand( bool bExecute )
{
	//trie version
	//int error = 0;
	TrieNode*node;
	std::string sRes;
	bool bSuccess = true;

	std::string m_sCurrentCommand = m_sCurrentCommandBeg+m_sCurrentCommandEnd;

	// remove leading and trailing spaces
	int pos = m_sCurrentCommand.find_first_not_of( " ", 0 );
	if( pos >= 0 ) {
		m_sCurrentCommand = m_sCurrentCommand.substr( pos );
	}
	pos = m_sCurrentCommand.find_last_not_of( " " );
	if( pos >= 0 ) {
		m_sCurrentCommand = m_sCurrentCommand.substr( 0, pos+1 );
	}

	// Make sure the command gets added to the history
	if( !m_sCurrentCommand.empty() ) {
		EnterLogLine( m_sCurrentCommand, LINEPROP_COMMAND, false );
	}

	// Simply print value if the command is just a variable
	if( ( node = g_pCVarTrie->Find( m_sCurrentCommand ) ) ) {
		//execute function if this is a function cvar
		if( _IsConsoleFunc( node ) ) {
			bSuccess &= CVarUtils::ExecuteFunction( m_sCurrentCommand, (CVarUtils::CVar<ConsoleFunc>*) node->m_pNodeData, sRes, bExecute );
			EnterLogLine( m_sCurrentCommand, LINEPROP_FUNCTION );
		}
		else { //print value associated with this cvar
			EnterLogLine( ( m_sCurrentCommand + " = " + 
				CVarUtils::GetValueAsString(node->m_pNodeData)).c_str(), LINEPROP_LOG );
		}
	}
	//see if it is an assignment or a function execution (with arguments)
	else {
		int eq_pos; //get the position of the equal sign
		//see if this an assignment
		if( ( eq_pos = m_sCurrentCommand.find( "=" ) ) != -1 ) {
			std::string command, value;
			std::string tmp = m_sCurrentCommand.substr(0, eq_pos ) ;
			command = RemoveSpaces( tmp );
			value = m_sCurrentCommand.substr( eq_pos+1, m_sCurrentCommand.length() );
			if( !value.empty() ) { 
				value = RemoveSpaces( value );
				if( ( node = g_pCVarTrie->Find(command) ) ) {
					if( bExecute ) {
						CVarUtils::SetValueFromString( node->m_pNodeData, value );
					}
					EnterLogLine( ( command + " = " + value ), LINEPROP_LOG );
				}
			}
			else {  
				if( bExecute ) {
					std::string out = "-glconsole: " + command + ": command not found"; 
					EnterLogLine( out, LINEPROP_ERROR );
				}
				bSuccess = false;
			}
		}
		//check if this is a function
		else if( ( eq_pos = m_sCurrentCommand.find(" ") ) != -1 ) {
			std::string function;
			std::string args;
			function = m_sCurrentCommand.substr( 0, eq_pos );
			//check if this is a valid function name
			if( ( node = g_pCVarTrie->Find( function ) ) && _IsConsoleFunc( node ) ) {
				bSuccess &= CVarUtils::ExecuteFunction( m_sCurrentCommand, (CVarUtils::CVar<ConsoleFunc>*)node->m_pNodeData, sRes, bExecute );
				EnterLogLine( m_sCurrentCommand, LINEPROP_FUNCTION );
			}
			else {
				if( bExecute ) {
					std::string out = "-glconsole: " + function + ": function not found"; 
					EnterLogLine( out, LINEPROP_ERROR );
				}
				bSuccess = false;
			}
		}
		else if( !m_sCurrentCommand.empty() ) {
			if( bExecute ) {
				std::string out = "-glconsole: " + m_sCurrentCommand + ": command not found"; 
				EnterLogLine( out, LINEPROP_ERROR );
			}
			bSuccess = false;
		}
		else { // just pressed enter
			EnterLogLine( " ", LINEPROP_LOG );
		}
	}

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Help interpreter function.
//  With no arguments prints out help from FLCONSOLE_HELPFILE
bool IrrConsole::Help(const std::vector<std::string> &vArgs)
{
	if( vArgs.size() != 0 )
	{
		for( size_t i = 0; i < vArgs.size(); i++ ) {
			try {
				PrintHelp( "help for %s", vArgs.at(i).c_str() );
				if(CVarUtils::GetHelp( vArgs.at(i) ).empty())
					PrintHelp( "No help available." );
				else
					PrintHelp( "%s", CVarUtils::GetHelp( vArgs.at(i) ).c_str() );
			}
			catch( CVarUtils::CVarException e ) {
				PrintHelp( "Unknown variable %s.", vArgs.at(i).c_str() );
				return false;
			}
		}
	}
	else {
		//try opening a default helpfile
		//else use the built in help
		std::ifstream sIn( CONSOLE_HELP_FILE  );
		if( sIn.is_open() ) {
			char s[1024];
			while(sIn.good()) {
				sIn.getline(s, 1024);
				PrintHelp(s);
			}
			sIn.close();
		}
		else{
			std::cerr << "WARNING: No custom " << CONSOLE_HELP_FILE << ". Using default IrrConsole help." << std::endl;

			PrintHelp("");
			PrintHelp("----------------- HELP -----------------");
			PrintHelp("Press ~ key to open and close console");
			PrintHelp("Press TAB to see the available commands and functions");
			PrintHelp("Functions are shown in green and variables in yellow");
			PrintHelp("Setting a value: [command] = value");
			PrintHelp("Getting a value: [command]");
			PrintHelp("Functions: [function] [arg1] [arg2] ...");
			PrintHelp("Entering just the function name will give a description.");
			PrintHelp("History: Up and Down arrow keys move through history.");
			PrintHelp("Tab Completion: TAB does tab completion and makes suggestions.");
			PrintHelp("----------------- HELP -----------------");
			PrintHelp("");
		}
	}
	return true;

}


void IrrConsole::RenderConsole(irr::gui::IGUIEnvironment* guienv, irr::video::IVideoDriver *videoDriver, const irr::u32 deltaMillis)
{
	if(IsOpen() == false) {
		return;
	}

	//draw the bg as per configured color
	videoDriver->draw2DRectangle(consoleColor, m_ConsoleRect);

	int consoleHeight = m_ConsoleRect.getHeight();

	int lines = (consoleHeight / m_nTextHeight);
	int scrollLines = (m_nScrollPixels / m_nTextHeight);
	lines += scrollLines;

	//start drawing from bottom of console up...
	int lineLoc = consoleHeight - 1 - m_nConsoleVerticalMargin;

	//draw command line first
	char blink = ' ';
	//figure out if blinking cursor is on or off
	if( _IsCursorOn() ) {
		blink = '_';
	}

	RenderText("> " + m_sCurrentCommandBeg, m_nConsoleLeftMargin, lineLoc + m_nScrollPixels, commandColor);

	int size = m_sCurrentCommandBeg.length();
	std::string em = "";
	for(int i=0;i<size;i++) {
		em = em + " ";
	}
	RenderText("> " + em + blink, m_nConsoleLeftMargin, lineLoc + m_nScrollPixels, commandColor);
	RenderText("> " + em + m_sCurrentCommandEnd, m_nConsoleLeftMargin, lineLoc + m_nScrollPixels, commandColor);

	lineLoc -= m_nTextHeight + m_nConsoleLineSpacing;

	int count = 0;
	for(  int i = 1 ; i < lines; i++ ) {	
		if( count >= m_nConsoleMaxLines) {
			continue;
		}
		if( (int)m_sConsoleText.size() > i - 1 ) {
			//skip this line if it was marked not to be displayed
			if( !(m_sConsoleText.begin() + (i-1))->m_bDisplay) {
				lines++;
				continue;
			}

			std::deque<ConsoleLine>::iterator it = m_sConsoleText.begin() + i - 1;
			std::string fulltext = (*it).m_sText;

			//set the appropriate color
			irr::video::SColor color = getLineColor((*it).m_nOptions);

			//wrap text to multiple lines if necessary
			int chars_per_line = (int)(1.65 * m_ConsoleRect.getWidth() / m_nTextWidth);
			if( chars_per_line == 0 ) {
				// What should we do if the window has width == 0 ?
				return;
			}

			int iterations = (fulltext.length() / chars_per_line) + 1;
			for(int j = iterations -1; j >= 0 ; j-- ) {
				//print one less line now that I have wrapped to another line
				if( j < iterations - 1) {
					lines--;
					lineLoc -= m_nTextHeight + m_nConsoleLineSpacing;
				}
				count++;
				int start = fulltext.substr(j*chars_per_line, chars_per_line).find_first_not_of( ' ' );
				if( start >= 0  ) { 
					RenderText(fulltext.substr(j*chars_per_line+start, chars_per_line), 
						m_nConsoleLeftMargin, lineLoc + m_nScrollPixels, color);
				}
			}
		} else {
			break;
		}

		lineLoc -= (m_nTextHeight + m_nConsoleLineSpacing);
	}
}

void IrrConsole::RenderText(const std::string &text, int x, int y, const irr::video::SColor &color)
{
	int lineHeight = m_nTextHeight + m_nConsoleLineSpacing;
	std::wstring wstr = StringUtil::string2wstring(text);
	rect<s32> pos(x, y - lineHeight/2, x + m_ConsoleRect.getWidth(), y + lineHeight/2);
	m_pGuiFont->draw(wstr.c_str(), pos, color, false, false, &m_ConsoleRect);
}

void IrrConsole::OpenConsole() 
{ 
	m_bConsoleOpen = true; 
	m_bIsChanging = true;
	m_Timer.Stamp(); 
}

void IrrConsole::CloseConsole() 
{ 
	m_bConsoleOpen = false;
	m_bIsChanging = true;
	m_Timer.Stamp(); 
}

void IrrConsole::ToggleConsole()
{
	if( m_bConsoleOpen ) {
		CloseConsole();
	}
	else {
		OpenConsole();
	}
}

void IrrConsole::ScrollDown(int pixels)
{
	if( m_bConsoleOpen ) {
		m_nScrollPixels -= pixels;
		if( m_nScrollPixels < 0 ) {
			m_nScrollPixels = 0;
		}
	}
}

void IrrConsole::ScrollUp(int pixels)
{
	if( m_bConsoleOpen ) {
		m_nScrollPixels += pixels;	
	}
}

void IrrConsole::ScrollUpLine() 
{ 
	ScrollUp( m_nTextHeight + m_nConsoleLineSpacing ); 
}

void IrrConsole::ScrollDownLine()
{ 
	ScrollDown(m_nTextHeight + m_nConsoleLineSpacing); 
}

void IrrConsole::ScrollUpPage() 
{ 
	ScrollUp( (int)((m_ConsoleRect.getHeight()*m_fOverlayPercent) - 2*m_nTextHeight)); 
}

////////////////////////////////////////////////////////////////////////////////
void IrrConsole::ScrollDownPage()
{ 
	ScrollDown( (int)( (m_ConsoleRect.getHeight() *m_fOverlayPercent) - 2*m_nTextHeight ) ); 
}

//! calculate the whole console rect
void IrrConsole::CalculateConsoleRect(const irr::core::dimension2d<s32>& screenSize)
{
	if(m_ConsoleConfig.dimensionRatios.X == 0 || m_ConsoleConfig.dimensionRatios.Y == 0) {
		m_ConsoleRect = rect<s32>(0,0,0,0);
	} else {
		//calculate console dimension
		dimension2d<s32> consoleDim = screenSize;
		consoleDim.Width = (s32)((f32)consoleDim.Width  * m_ConsoleConfig.dimensionRatios.X);
		consoleDim.Height= (s32)((f32)consoleDim.Height * m_ConsoleConfig.dimensionRatios.Y);

		//set vertical alignment
		m_ConsoleRect.UpperLeftCorner.Y = 0;
		m_ConsoleRect.UpperLeftCorner.X = 0;

		//set the lower right corner stuff
		m_ConsoleRect.LowerRightCorner.X = m_ConsoleRect.UpperLeftCorner.X + consoleDim.Width;
		m_ConsoleRect.LowerRightCorner.Y = m_ConsoleRect.UpperLeftCorner.Y + consoleDim.Height;
	}
}

void setUpConsole(irr::IrrlichtDevice *device)
{
	//this is how you alter some of the config params
	ConsoleConfig config;
	config.fontName = "res/console-14.xml";
	config.dimensionRatios.X = 1.0f;
	config.dimensionRatios.Y = 0.8f;

	//now initialize
	g_console->Init(device, config);

	//add function
	CVarUtils::CreateCVar( "driver_info", ConsoleDriverInfo, "Display Irrlicht Driver Info" );	
}