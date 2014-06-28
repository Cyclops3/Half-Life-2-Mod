/*
** Stack-based Gmod script enforcer bypass
** (c) TEAM METALSLAVE RAGE CO.
**
** REVISION HISTORY
**
** 00/00/0000 initial implementation in private release as MSSEB.DLL
**
** MEET THE SCRIPT ENFORCER
**
** Due to a large amount of cheaters in Gmod servers running scripts like aimbots, wallhacks
** and so on, Garry Jewman finally decided to implement something called the Script Enforcer.
** Unfortunately, like everything he's created, it didn't work out as well as he had hoped.
**
** The function that blocks scripts is:
**
** bool __thiscall CScriptEnforcer::CanLoadScript( void* );
**
** THE BYPASS EXPLAINED
**
** Before the script enforcer can block a script, it first notifies us in the console via
** Msg. Msg will pass through some formatting functions before eventually landing at whatever
** has been initialized as the SpewOutputFunc. We change the SpewOutputFunc on it and when it
** is called, we check for returns to the script enforcer and rewrite them so the program returns
** into code that is specified in our bypass DLL (see below).
**
** When the Msg() call to the string is called, we catch it and redirect the code. When returning
** we pretend that the script was actually blocked by setting the "blocked" flag but returning 1,
** allowing the script to load.
**
** This only works on scripts that the server has blocked entirely. It does not work on
** scripts that are different from the server's due to multiple CRC and MD5 checks that
** run in-game.
**
** msseb.lib is cleared for public release.
**
*/

#include <stdio.h>
#include <windows.h>
#include "convar.h"
#include "dbg.h"

static SpewOutputFunc_t ZeOldSpewOutput = 0; // the old function that handles debug msgs.
static UINT32 se_blockscriptaddr = 0; // the address where the script is blocked
static ConVar seb_enable ( "seb_enable", "0", FCVAR_SERVER_CANNOT_QUERY, "enable dat scriptenforcer bypass mufugga");


// GetStackPtr: obfuscated utility function for getting the stack pointer.
// this doesn't return the *exact* stack frame, but hey, it works.
static UINT32 * __stdcall GetStackPointer() {
	UINT32* crap ;
	__asm {
		mov eax, esp
		mov crap, eax
    };

	return crap;
}

UINT8 SEBypass[20] = {
		0x83, 0xC4, 0x08,						// add esp, 8h
		0xC6, 0x46, 0x44, 0x01,					// set scriptenforce flag as blocked
		0xB0, 0x01,								// mov al, 1h to return TRUE
		0x5E,									// pop esi
		0x81, 0xC4, 0x08, 0x01, 0x00, 0x00,		// add esp, 108h
		0xC2, 0x08, 0x00,						// retn 8
};


/*
** ZeNewSpewOutput
** This is our new debug message handler. It just so happens that the
** Script Enforcer calls this before blocking a script.
*/ 

static SpewRetval_t ZeNewSpewOutput( SpewType_t type, const char* msg ) {


	if (strstr(msg,"ScriptEnforce:") && !strstr(msg, "CRC")) {

		Msg("ScriptEnforcer is attempting to block a script.\n");
		// get the stack pointer
		UINT32* theSp = GetStackPointer();
		Msg("ESP = %p\n", theSp);

		// If the script enforcer is trying to block something, step a few stack
		// frames and rewrite anything that's returning to the scriptenforcer
		// validation so it goes to the bypass code when retn is hit in Msg().
		// This verification is done to prevent false positives jumping into
		// code that crashes the thing hilariously.		
		int isFound = 0;
		for (int i = 0; i < 0x1000; i ++) {
			if (theSp[i] == se_blockscriptaddr)   {
				theSp[i] = (UINT32)SEBypass; 
				Msg("*** ScriptEnforcer tried to block a script. Stack modified.\n");
				isFound = 1;
				break;
			}
		}
		if (!isFound) Msg("ERROR: Couldn't find the return address in the stack!\n");
	}
	

	// jump off to the old spew output function
	return ZeOldSpewOutput( type, msg );
}

void SEBypasser_Init() {

	// get the return address following the Msg()
	se_blockscriptaddr = (UINT32)GetModuleHandleA("client.dll");
	se_blockscriptaddr += 0x1CC22A;
	

	// get the original spew fcn using Msg (there's a pointer in there
	// to the output function). this is yet another module garry has no
	// control over so we can care less about what changes here.
	ZeOldSpewOutput = GetSpewOutputFunc();

	if (!ZeOldSpewOutput) {
		Msg("Old spew function was ZERO\n");
		return;
	}

	// Switch the output on us. All messages now go through our filter.
	SpewOutputFunc( ZeNewSpewOutput );

	// just let the user know that we initialized the module OK.
	Msg("*** msseb.lib built " __DATE__ " " __TIME__" ***\n");
	Msg("(c) 2010 TEAM METALSLAVE RAGE CO.\n");
	Msg("this software is provided to you without warranty or terms.\n");
	Msg("see msseb.txt for further information.\n");
}
