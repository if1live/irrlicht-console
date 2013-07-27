// Ŭnicode please
#include "stdafx.h"

#include "console_function.h"
#include "string_util.h"

#include <vector>
#include <cstdlib>
#include "irr_console.h"

using namespace std;
using namespace irr;

////////////////////////////////////////////////////////////////////////////////
/// Print the current CVars version.
bool ConsoleVersion( const std::vector<std::string>& )
{
	printf( "CVars v%d.%d.%d", 
		CVARS_MAJOR_REV, 
		CVARS_MINOR_REV, 
		CVARS_PATCH_REV );
	return true;
}

////////////////////////////////////////////////////////////////////////////////
///  Help for functions and variables or just general help.
bool ConsoleHelp( const std::vector<std::string> &vArgs )
{
	return g_console->Help( vArgs );
}

////////////////////////////////////////////////////////////////////////////////
/// Looks for the lists of substrings provided in vArgs in the CVarTrie.
bool ConsoleFind( const std::vector<std::string> &vArgs )
{
	if(vArgs.size() > 0 ) {
		for( size_t i=0; i<vArgs.size(); i++ ) {
			std::vector<std::string> vCVarNames = g_pCVarTrie->FindListSubStr( vArgs.at(i) );

			for( size_t j=0; j<vCVarNames.size(); j++ ) { 
				printf( "%s", vCVarNames[j].c_str() );       
			}
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// First argument indicates the file name, the following arguments are
//  used to choose a subset of variables to be saved using the provided
//  substrings.
//  Last argument can be used to be verbose when saving.
bool ConsoleSave( const std::vector<std::string> &vArgs )
{
	std::string sFile = "cvars.xml";
	std::vector< std::string > vAcceptedSubstrings;

	if( vArgs.size() > 0 ) {
		sFile = vArgs.at( 0 );
		for( size_t i=1; i<vArgs.size(); i++ ) {
			vAcceptedSubstrings.push_back( vArgs.at( i ) );
		}
	}

	printf( "Saving cvars to \"%s\".", sFile.c_str() );
	if( !CVarUtils::Save( sFile, vAcceptedSubstrings ) ) {
		printf( "Error saving file.\n" );
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// Load CVars from a file.
bool ConsoleLoad( const std::vector<std::string> &vArgs )
{
	std::string sFile = "cvars.xml";
	std::vector< std::string > vAcceptedSubstrings;

	if( vArgs.size() > 0 ) {
		sFile = vArgs.at( 0 ); 
		for( size_t i=1; i<vArgs.size(); i++ ) {
			vAcceptedSubstrings.push_back( vArgs.at(i) );
		}
	}
	printf( "Loading file \"%s\".", sFile.c_str() );
	if( !CVarUtils::Load( sFile, vAcceptedSubstrings ) ) {
		printf( "Error: Could not load \"%s\".", sFile.c_str() );
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// Exits program from command line.
bool ConsoleExit( const std::vector<std::string> &vArgs )
{
	g_console->getDevice()->closeDevice();
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// Save the console history to a file.
bool ConsoleHistorySave( const std::vector<std::string> &vArgs )
{
	if( vArgs.size() != 0 ) {
		return g_console->HistorySave( vArgs.at( 0 ) );
	}
	else {
		return g_console->HistorySave();
	}
}

////////////////////////////////////////////////////////////////////////////////
/// Load a previously saved console history from a file.
bool ConsoleHistoryLoad( const std::vector<std::string> &vArgs )
{
	if( vArgs.size() != 0 ) {
		return g_console->HistoryLoad( vArgs.at( 0 ) );
	}
	else {
		return g_console->HistoryLoad();
	}
}

////////////////////////////////////////////////////////////////////////////////
/// clear the console history.
bool ConsoleHistoryClear( const std::vector<std::string> &vArgs )
{
	g_console->HistoryClear();
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// Save the current script.
bool ConsoleScriptSave( const std::vector<std::string> &vArgs )
{
	if( vArgs.size() != 0 ) {
		return g_console->ScriptSave( vArgs.at( 0 ) );
	}
	else {
		return g_console->ScriptSave();
	}

}

////////////////////////////////////////////////////////////////////////////////
/// Load a script from a file.
bool ConsoleScriptLoad( const std::vector<std::string> &vArgs )
{
	if( vArgs.size() != 0 ) {
		return g_console->ScriptLoad( vArgs.at( 0 ) );
	}
	else {
		return g_console->ScriptLoad();
	}
}

////////////////////////////////////////////////////////////////////////////////
/// Start script recording
bool ConsoleScriptRecordStart( const std::vector<std::string> &vArgs )
{
	g_console->ScriptRecordStart();
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// Stop script recording.
bool ConsoleScriptRecordStop( const std::vector<std::string> &vArgs )
{
	g_console->ScriptRecordStop();
	return true;
}

////////////////////////////////////////////////////////////////////////////////
///  Pause script recording.
bool ConsoleScriptRecordPause( const std::vector<std::string> &vArgs )
{
	g_console->ScriptRecordPause();
	return true;
}

////////////////////////////////////////////////////////////////////////////////
///  Show the current script.
bool ConsoleScriptShow( const std::vector<std::string> &vArgs )
{
	g_console->ScriptShow();
	return true;
}

////////////////////////////////////////////////////////////////////////////////
///  Run the current scipt or one from the named file.
bool ConsoleScriptRun( const std::vector<std::string> &vArgs )
{
	if( vArgs.size() != 0 ) {
		return g_console->ScriptRun( vArgs.at( 0 ) );
	}
	else {
		return g_console->ScriptRun();
	}
}

////////////////////////////////////////////////////////////////////////////////
///  Save the console settings.
bool ConsoleSettingsSave( const std::vector<std::string> &vArgs )
{
	if( vArgs.size() != 0 ) {
		return g_console->SettingsSave( vArgs.at( 0 ) );
	}
	else {
		return g_console->SettingsSave();
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
/// Load console settings.
bool ConsoleSettingsLoad( const std::vector<std::string> &vArgs )
{
	if( vArgs.size() != 0 ) {
		return g_console->SettingsLoad( vArgs.at( 0 ) );
	}
	else {
		return g_console->SettingsLoad();
	}
	return false;
}

bool ConsoleDriverInfo(const std::vector<std::string> &args)
{
	IrrlichtDevice *device = g_console->getDevice();
	if(device) {
		irr::video::IVideoDriver* driver = device->getVideoDriver();
		std::string msg = " Irrlicht Version : ";
		msg += device->getVersion();
		g_console->EnterLogLine(msg.c_str());

		msg = " OS Version : ";
		auto osInfo = device->getOSOperator()->getOperatingSystemVersion();
		msg += osInfo.c_str();
		g_console->EnterLogLine(msg.c_str());

		msg = " Display Driver : ";
		msg += StringUtil::wstring2string(device->getVideoDriver()->getName());
		g_console->EnterLogLine(msg.c_str());

		return true;
	} else {
		g_console->EnterLogLine("No valid irrlicht device detected!!");
		return false;
	}
}