// Ŭnicode please
/*
    
    \file IrrConsoleFunction.h

    Collection of ConsoleFunc's for IrrConsole.

    $Id: IrrConsoleFunction.h 164 2010-03-01 20:19:13Z gsibley $
 */

#pragma once

#include <vector>
#include <cstdlib>
#include "irr_console.h"

////////////////////////////////////////////////////////////////////////////////
/// Print the current CVars version.
bool ConsoleVersion( const std::vector<std::string> & );

////////////////////////////////////////////////////////////////////////////////
///  Help for functions and variables or just general help.
bool ConsoleHelp( const std::vector<std::string> &vArgs );

////////////////////////////////////////////////////////////////////////////////
/// Looks for the lists of substrings provided in vArgs in the CVarTrie.
bool ConsoleFind( const std::vector<std::string> &vArgs );

////////////////////////////////////////////////////////////////////////////////
/// First argument indicates the file name, the following arguments are
//  used to choose a subset of variables to be saved using the provided
//  substrings.
//  Last argument can be used to be verbose when saving.
bool ConsoleSave( const std::vector<std::string> &vArgs );

////////////////////////////////////////////////////////////////////////////////
/// Load CVars from a file.
bool ConsoleLoad( const std::vector<std::string> &vArgs );

////////////////////////////////////////////////////////////////////////////////
/// Exits program from command line.
bool ConsoleExit( const std::vector<std::string> & );

////////////////////////////////////////////////////////////////////////////////
/// Save the console history to a file.
bool ConsoleHistorySave( const std::vector<std::string> &vArgs );

////////////////////////////////////////////////////////////////////////////////
/// Load a previously saved console history from a file.
bool ConsoleHistoryLoad( const std::vector<std::string> &vArgs );

////////////////////////////////////////////////////////////////////////////////
/// clear the console history.
bool ConsoleHistoryClear( const std::vector<std::string> & );

////////////////////////////////////////////////////////////////////////////////
/// Save the current script.
bool ConsoleScriptSave( const std::vector<std::string> &vArgs );

////////////////////////////////////////////////////////////////////////////////
/// Load a script from a file.
bool ConsoleScriptLoad( const std::vector<std::string> &vArgs );

////////////////////////////////////////////////////////////////////////////////
/// Start script recording
bool ConsoleScriptRecordStart( const std::vector<std::string> & );

////////////////////////////////////////////////////////////////////////////////
/// Stop script recording.
bool ConsoleScriptRecordStop( const std::vector<std::string> & );

////////////////////////////////////////////////////////////////////////////////
///  Pause script recording.
bool ConsoleScriptRecordPause( const std::vector<std::string> & );

////////////////////////////////////////////////////////////////////////////////
///  Show the current script.
bool ConsoleScriptShow( const std::vector<std::string> & );

////////////////////////////////////////////////////////////////////////////////
///  Run the current scipt or one from the named file.
bool ConsoleScriptRun( const std::vector<std::string> &vArgs );

////////////////////////////////////////////////////////////////////////////////
///  Save the console settings.
bool ConsoleSettingsSave( const std::vector<std::string> &vArgs );

////////////////////////////////////////////////////////////////////////////////
/// Load console settings.
bool ConsoleSettingsLoad( const std::vector<std::string> &vArgs );

bool ConsoleDriverInfo(const std::vector<std::string> &args);