/*

   Cross platform "CVars" functionality.

   This Code is covered under the LGPL.  See COPYING file for the license.

   $Id: CVarParse.cpp 162 2010-02-15 19:11:05Z gsibley $

 */
#include "stdafx.h"
#include "CVar.h"

using namespace CVarUtils;

////////////////////////////////////////////////////////////////////////////////
// remove all spaces from the front and back...
std::string& _RemoveSpaces( std::string &str )
{
    // remove them off the front
    int idx = str.find_first_not_of( ' ' );
    if( idx != 0 ) {
        str = str.substr( idx, str.length() );
    }

    // remove them off the back
    idx = str.find_last_not_of(' ');
    if( idx != -1 ) {
        str = str.substr( 0, idx+1 );
    }

    return str;
}

////////////////////////////////////////////////////////////////////////////////
namespace CVarUtils 
{
    ////////////////////////////////////////////////////////////////////////////////
    bool ProcessCommand( 
            const std::string& sCommand, 
            std::string& sResult,
            bool bExecute                       //< Input:
            )
    {
        TrieNode*node;
        bool bSuccess = true;
        std::string sCmd = sCommand;

        // remove leading and trailing spaces
        int pos = sCmd.find_first_not_of( " ", 0 );
        if( pos >= 0 ) {
            sCmd = sCmd.substr( pos );
        }
        pos = sCmd.find_last_not_of( " " );
        if( pos >= 0 ) {
            sCmd= sCmd.substr( 0, pos+1 );
        }

        // Simply print value if the command is just a variable
        if( ( node = g_pCVarTrie->Find( sCmd ) ) ) {
            //execute function if this is a function cvar
            if( IsConsoleFunc( node ) ) {
                bSuccess &= ExecuteFunction( 
                        sCmd,
                        (CVarUtils::CVar<ConsoleFunc>*)node->m_pNodeData,
                        sResult,
                        bExecute
                        );
            }
            else { //print value associated with this cvar
                sResult = GetValueAsString(node->m_pNodeData).c_str();
            }
        }
        //see if it is an assignment or a function execution (with arguments)
        else{
            int eq_pos; //get the position of the equal sign
            //see if this an assignment
            if( ( eq_pos = sCmd.find( "=" ) ) != -1 ) {
                std::string command, value;
                std::string tmp = sCmd.substr(0, eq_pos ) ;
                command = _RemoveSpaces( tmp );
                value = sCmd.substr( eq_pos+1, sCmd.length() );
                if( !value.empty() ) {
                    value = _RemoveSpaces( value );
                    if( ( node = g_pCVarTrie->Find(command) ) ) {
                        if( bExecute ) {
                            SetValueFromString( node->m_pNodeData, value );
                        }
                    }
                }
                else {
                    if( bExecute ) {
                        sResult = command + ": command not found";
                    }
                    bSuccess = false;
                }
            }
            //check if this is a function
            else if( ( eq_pos = sCmd.find(" ") ) != -1 ) {
                std::string function;
                std::string args;
                function = sCmd.substr( 0, eq_pos );
                //check if this is a valid function name
                if( ( node = g_pCVarTrie->Find( function ) ) && IsConsoleFunc( node ) ) {
                    bSuccess &= ExecuteFunction( 
                            sCmd,
                            ( CVar<ConsoleFunc>*)node->m_pNodeData,
                            sResult,
                            bExecute
                            );
                }
                else {
                    if( bExecute ) {
                        sResult = function + ": function not found";
                    }
                    bSuccess = false;
                }
            }
            else if( !sCmd.empty() ) {
                if( bExecute ) {
                    sResult = sCmd + ": command not found";
                }
                bSuccess = false;
            }
        }
        if( sResult == "" && bSuccess == false ){
            sResult = sCmd + ": command not found";
        }
        return bSuccess;
    }


    ////////////////////////////////////////////////////////////////////////////////
    /// Parses the argument list and calls the function object associated with the
    //  provided variable.
    bool ExecuteFunction( 
            const std::string& sCommand,         //< Input:
            CVarUtils::CVar<ConsoleFunc> *cvar,  //< Input:
            std::string& ,                //< Output:
            bool bExecute                        //< Input:
            )
    {
        std::vector<std::string> argslist;
        std::string args;
        bool bSuccess = true;
        ConsoleFunc func = *(cvar->m_pVarData);
        std::string sCmd = sCommand;

        //extract arguments string
        int pos = sCmd.find( " " );
        if( pos != -1 ) {
            args = sCmd.substr(pos+1, sCmd.length() - pos);
        }
        //parse arguments into a list of strings
        if( args.length() > 0 ) {
            while( ( pos = args.find(" ") ) != -1 ) {
                argslist.push_back(args.substr(0, pos));
                args = args.substr(pos+1, args.length() - pos);
            }
            if( args.length() > 0 ) {
                argslist.push_back(args);
            }
        }

        if( bExecute ) {
            const bool res = (*func)( argslist );
            if( !res ) {
                bSuccess = false;
            }
        }

        return bSuccess;
    }


    ////////////////////////////////////////////////////////////////////////////////
    bool IsConsoleFunc( TrieNode *node )
    {
        if( typeid( ConsoleFunc ).name() == ((CVar<int>*)node->m_pNodeData)->type() ) {
            return true;
        }
        return false; 
    }

}

