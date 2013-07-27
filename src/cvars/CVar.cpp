// Ŭnicode please 
#include "stdafx.h"
#include "cvars/CVar.h"

// Single global instance
Trie* g_pCVarTrie = nullptr;

////////////////////////////////////////////////////////////////////////////////
// This can get called many times -- esp when global constructors with defalut
// parameters are use, because they can be called by the compiler *before* main 
// runs -- basically, the CVarUtils functions that depend on g_pCVarTrie can be
// called before main runs, so they must all call this function to be sure
// g_pCVarTrie is allocated.
void InitCVars()
{
	static Trie CVarTrieLocal;
    if( g_pCVarTrie == NULL ){
		g_pCVarTrie = &CVarTrieLocal;
        g_pCVarTrie->Init();
    }
}