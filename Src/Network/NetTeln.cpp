/*=============================================================================
	NetTeln.cpp: Unreal Internet telnet interface

	Copyright 1997 Epic MegaGames, Inc. This software is a trade secret.
	Compiled with Visual C++ 4.0. Best viewed with Tabs=4.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#pragma warning (disable : 4201) /* nonstandard extension used : nameless struct/union */

#include <windows.h>
#include <windowsx.h>
#include "UnBuild.h"
#include "Net.h"
#include "NetPrv.h"
#include "NetINet.h"
#include "Resource.h"

int UNENGINE_API GEngineExec(const char *Cmd,FOutputDevice *Out);

/*------------------------------------------------------------------------------
	Telnet task definition and implementation
------------------------------------------------------------------------------*/

//
// Execute a command in a Telnet task context.  If the
// command isn't one recognized by the telnet console,
// passes it to the engine for processing.
//
int FTaskTelnet::Exec(const char *Cmd,FOutputDevice *Out)
	{
	const char *Str=Cmd;
	//
	if (NetGetCMD(&Str,"LOGIN") || NetGetCMD(&Str,"_LOGIN"))
		{
		char Name[NET_NAME_SIZE], Password[NET_NAME_SIZE];
		char MatchName[NET_NAME_SIZE], MatchPassword[NET_NAME_SIZE];
		//
		GApp->GetProfileValue("UnrealServer","TelnetName",    MatchName,    "god",NET_NAME_SIZE);
		GApp->GetProfileValue("UnrealServer","TelnetPassword",MatchPassword,"",NET_NAME_SIZE);
		//
		if (NetGrabSTRING(Str,Name,256) && NetGrabSTRING(Str,Password,256))
			{
			if ((!stricmp(MatchName,Name)) && (!stricmp(MatchPassword,Password)) && MatchPassword[0])
				{
				Guest=0;
				strcpy(UserName,MatchName);
				Out->Logf("Welcome to Unreal, %s",MatchName);
				}
			else Out->Logf("Incorrect name or password",Name,Password);
			};
		return 1;
		}
	else return GEngineExec(Cmd,Out);
	};

//
// Send a string across the telnet console.
//
void FTaskTelnet::Send(const char *Str)
	{
	send(Socket,Str,strlen(Str),0);
	};

//
// Implementation of FOutputDevice-derived Log function for Telnet console.
//
void FTaskTelnet::Write(const void *Data,int Length,ELogType MsgType)
	{
	Send((char *)Data);
	Send("\r\n");
	};

//
// Initialize a telnet console task.
//
void FTaskTelnet::Init(SOCKET NewSocket,SOCKADDR_IN NewSockAddr)
	{
	strcpy(UserName,"Guest");
	strcpy(Input,"");
	//
	Socket		= NewSocket;
	Guest		= 1;
	SockAddr	= NewSockAddr;
	//
	GTaskManager->AddTask(this,NULL,GApp,PRIORITY_Realtime,0);
	//
	Logf(CONSOLE_SPAWN_1);
	Logf(CONSOLE_SPAWN_2);
	Logf("Guest access granted.  Type 'LOGIN name password' for regular access.");
	Logf("");
	Send("(> ");
	};

//
// Handle a timer tick on the telnet console.  This is the one and only
// place where the FTaskTelnet object can be destroyed.
//
void FTaskTelnet::TaskTick( FLOAT DeltaSeconds )
	{
	guard(FTaskTelnet::TaskTick);
	//
	char Buffer[4096];
	int Result = recv(Socket,Buffer,4096,0);
	if (Result==0) // Connection was closed
		{
		GTaskManager->KillTask(this);
		delete this;
		return;
		}
	else if (Result!=SOCKET_ERROR) // Received data
		{
		Buffer[Result]=0;
		char *c = &Buffer[0];
		while (*c)
			{
			int Len = strlen(Input);
			if ((*c>=32)&&(*c<127))
				{
				if (Len<MAX_INPUT_LEN)
					{
					Input[Len]=*c;
					Input[Len+1]=0;
					Send(&Input[Len]);
					};
				}
			else if ((*c==8)||(*c==127))
				{
				if (Len>0)
					{
					Input[Len-1]=0;
					Send("\b \b");
					};
				}
			else if (*c==27)
				{
				for (int i=0; i<Len; i++) Send("\b \b");
				Input[0]=0;
				}
			else if (*c==13)
				{
				if (Input[0])
					{
					char Temp[MAX_INPUT_LEN+2]="";
					if (Guest) strcat(Temp,"_"); // Prevent executing regular commands
					strcat(Temp,Input);
					Input[0]=0;
					//
					Send("\r\n");
					Exec(Temp,this);
					Send("\r\n(> ");
					};
				};
			c++;
			};
		};
	unguard;
	};

//
// Do all cleanup work for the telnet console.
//
void FTaskTelnet::TaskExit()
	{
	guard(FTaskTelnet::TaskExit);
	//
	closesocket(Socket);
	//
	unguard;
	};

//
// Return the status of the telnet console.
//
char *FTaskTelnet::TaskStatus(char *Name,char *Desc)
	{
	guard(FTaskTelnet::TaskStatus);
	//
	sprintf(Name,"Telnet");
	sprintf(Desc,UserName);
	return Name;
	//
	unguard;
	};

/*------------------------------------------------------------------------------
	The End
------------------------------------------------------------------------------*/
