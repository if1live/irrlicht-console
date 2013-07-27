/*

    Cross platform "CVars" functionality.

    This Code is covered under the LGPL.  See COPYING file for the license.

    $Id: Trie.h 162 2010-02-15 19:11:05Z gsibley $

 */


#ifndef _TRIE_H__
#define _TRIE_H__

#include <string>
#include <vector>

#include "CVar.h"

class TrieNode;

class Trie
{
 public:
    Trie();
    ~Trie();
    void Init();
    // add string to tree and store data at leaf
    void         Insert( std::string s, void *data );
    // finds s in the tree and returns the node (may not be a leaf)
    // returns null otherwise
    TrieNode*    FindSubStr( const std::string& s );
    std::vector<std::string> FindListSubStr( const std::string& s );
    TrieNode*    Find( const std::string& s );
    void*        FindData( const std::string& s );

    bool         Exists( const std::string& s );

    TrieNode*    GetRoot();
    void         SetAcceptedSubstrings( std::vector< std::string > vAcceptedSubstrings );
    bool         IsNameAcceptable( const std::string& sVarName );
    bool         IsVerbose();
    void         SetVerbose( bool bVerbose );

    // does an in order traversal starting at node and printing all leaves
    // to a list
    std::vector<std::string> CollectAllNames( TrieNode* node );
    // traverse from the supplied node and return a list of all nodes
    std::vector<TrieNode*>   CollectAllNodes( TrieNode* node );

    // CVar
    int*   m_pVerboseCVarNamePaddingWidth;

 private:
    TrieNode* root;
    std::vector< std::string > m_vAcceptedSubstrings;
    std::vector< std::string > m_vNotAcceptedSubstrings;
    std::vector< std::string > m_vCVarNames; // Keep a list of CVar names
    bool m_bVerbose;
};

std::ostream &operator<<(std::ostream &stream, Trie &rTrie );
std::istream &operator>>(std::istream &stream, Trie &rTrie );


#endif
