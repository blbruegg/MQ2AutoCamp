// MQ2AutoCamp.cpp : RedGuides exclusive plugin to camp out when dead and afk to save rez timer
// v1.00 :: Sym - 2013-04-09
// v1.01 :: Sym - 2013-04-11 :: Added reset delay to aborted checks due to buff lingering, /endmac on death toggle
// v1.02 :: Sym - 2013-04-13 :: Added check for rez in same zone as bind point to cancel camping
// v1.03 :: Sym - 2013-04-24 :: Added ini support, start/end macros on death/rez

#include <mq/Plugin.h>

PreSetup("MQ2AutoCamp");
PLUGIN_VERSION(1.03);

constexpr int SKIP_PULSES = 100;

bool bAutoCampEnabled = false;
bool bCamp = false;
bool bEndMacro = false;
bool bEndMacroWhenDead = false;
bool bStartMacro = false;
bool bInitDone = false;

char szMacroName[MAX_STRING];
char szTemp[MAX_STRING];
char szHold[MAX_STRING];

int WarningCount = 0;
int CampCount = 15;
long SkipPulse = 0;

time_t StartTime;
time_t WarningTime;
time_t ResetTime;

void SaveINI()
{
	// write on/off settings
	char ToStr[16];
	sprintf_s(szTemp,"%s_Settings",GetCharInfo()->Name);
	WritePrivateProfileSection(szTemp, "", INIFileName);
	WritePrivateProfileString(szTemp,"Enabled",bAutoCampEnabled? "1" : "0",INIFileName);
	sprintf_s(ToStr,"%d",CampCount);
	WritePrivateProfileString(szTemp,"WaitMinutes",ToStr,INIFileName);
	WritePrivateProfileString(szTemp,"Endmac",bEndMacro? "1" : "0",INIFileName);
	WritePrivateProfileString(szTemp,"EndmacWhenDead",bEndMacroWhenDead? "1" : "0",INIFileName);
	WritePrivateProfileString(szTemp,"StartMacro",bStartMacro? "1" : "0",INIFileName);
	WritePrivateProfileString(szTemp,"StartMacroName",szMacroName,INIFileName);

	WriteChatf("MQ2AutoCamp :: Settings updated");
}

void LoadINI()
{
	// get on/off settings
	sprintf_s(szTemp,"%s_Settings",GetCharInfo()->Name);
	bAutoCampEnabled = GetPrivateProfileInt(szTemp, "Enabled", 0, INIFileName) > 0 ? true : false;
	CampCount = GetPrivateProfileInt(szTemp, "WaitMinutes", 15, INIFileName);
	bEndMacro = GetPrivateProfileInt(szTemp, "Endmac", 0, INIFileName) > 0 ? true : false;
	bEndMacroWhenDead = GetPrivateProfileInt(szTemp, "EndmacWhenDead", 0, INIFileName) > 0 ? true : false;
	bStartMacro = GetPrivateProfileInt(szTemp, "StartMacro", 0, INIFileName) > 0 ? true : false;
	GetPrivateProfileString(szTemp,"StartMacroName","",szMacroName,2047,INIFileName);

	if (!strlen(szMacroName)) {
		bStartMacro = false;
	}

	bInitDone = true;
}

void ShowHelp()
{
	WriteChatf("\atMQ2AutoCamp :: v%1.2f :: by Sym for RedGuides.com\ax", MQ2Version);
	WriteChatf("/autocamp :: Lists command syntax");
	WriteChatf("/autocamp load :: Loads ini file.");
	WriteChatf("/autocamp save :: Saves settings to ini. Changes \arDO NOT\ax auto save.");
	WriteChatf("/autocamp on|off :: Toggles camping to desktop when dead. Default is \arOFF\ax");
	WriteChatf("/autocamp endmac on|off :: Toggles /endmac before camping. Default is \arOFF\ax");
	WriteChatf("/autocamp endmac whendead on|off :: Toggles /endmac as soon as dead. Default is \arOFF\ax");
	WriteChatf("/autocamp startmac on|off|clear|set macroname :: Toggles /mac macroname after rez. Default is \arOFF\ax");
	WriteChatf("/autocamp abort :: Aborts processing the currently detected death.");
	WriteChatf("/autocamp wait # :: Number of minutes to wait before camping to desktop. Default is 15.");
	WriteChatf("/autocamp status :: Shows the current status.");

}

void ShowStatus()
{
	WriteChatf("\atMQ2AutoCamp :: v%1.2f :: by Sym for RedGuides.com\ax", MQ2Version);
	WriteChatf("MQ2AutoCamp :: %s", bAutoCampEnabled?"\agENABLED\ax":"\arDISABLED\ax");
	WriteChatf("MQ2AutoCamp :: Waiting \ag%d minutes\ax before camping",CampCount);
	WriteChatf("MQ2AutoCamp :: /endmac when dead is %s", bEndMacroWhenDead ?"\agON\ax":"\arOFF\ax");
	WriteChatf("MQ2AutoCamp :: Starting macro after rez is %s", bStartMacro?"\agON\ax":"\arOFF\ax");
	if (bStartMacro) WriteChatf("MQ2AutoCamp :: Macro to start after rez is \ag%s\ax", szMacroName);
}

void AutoCampCommand(PSPAWNINFO pChar, PCHAR zLine)
{
	char szArg1[MAX_STRING],szArg2[MAX_STRING],szArg3[MAX_STRING];

	GetArg(szArg1, zLine, 1);
	GetArg(szArg2, zLine, 2);
	GetArg(szArg3, zLine, 3);

	if(!_strnicmp(szArg1,"load",4)) {
		LoadINI();
		WriteChatf("MQ2AutoCamp :: \agSettings loaded\ax");
		ShowStatus();
		return;
	}
	if(!_strnicmp(szArg1,"save",4)) {
		SaveINI();
		return;
	}
	if(!_strnicmp(szArg1,"on",2)) {
		bAutoCampEnabled = true;
		WriteChatf("MQ2AutoCamp :: \agEnabled\ax");
	}
	else if(!_strnicmp(szArg1,"off",2)) {
		bAutoCampEnabled = false;
		WriteChatf("MQ2AutoCamp :: \arDisabled\ax");
	}
	else if(!_strnicmp(szArg1,"abort",2)) {
		bCamp = false;
		ResetTime = time(NULL);
		WriteChatf("MQ2AutoCamp :: Camp to desktop \arABORTED\ax");
	}
	else if (!_stricmp(szArg1, "status")) {
		ShowStatus();
	} else if(!_strcmpi(szArg1,"wait")) {
		if(!_strcmpi(szArg2, "")) {
			WriteChatf("Usage: /autocamp wait #");
			WriteChatf("       Number of minutes to wait before camping to desktop.");
			return;
		} else {
			CampCount = atoi(szArg2) > 0 ? atoi(szArg2) : 15 ;
			WriteChatf("MQ2AutoCamp :: Camping to desktop %s minutes after death.",szArg2);
		}
	} else if(!_strcmpi(szArg1,"endmac")) {
		if(!_strcmpi(szArg2, "")) {
			WriteChatf("Usage: /autocamp endmac on|off");
			WriteChatf("       Toggles /endmac before camping. Default is \arOFF\ax");
			return;
		}
		if(!_strcmpi(szArg2, "on")) {
			bEndMacro = true;
		}
		else if(!_strcmpi(szArg2, "off")) {
			bEndMacro = false;
		}
		else if(!_strcmpi(szArg2, "whendead")) {
			if(!_strcmpi(szArg3, "")) {
				WriteChatf("Usage: /autocamp endmac whendead on|off");
				WriteChatf("       Toggles /endmac as soon as death is detected. Default is \arOFF\ax");
				return;
			}
			if(!_strcmpi(szArg3, "on")) {
				bEndMacroWhenDead = true;
			}
			else if(!_strcmpi(szArg3, "off")) {
				bEndMacroWhenDead = false;
			}
			WriteChatf("MQ2AutoCamp :: /endmac when dead is %s", bEndMacroWhenDead ?"\agON\ax":"\arOFF\ax");
			return;
		}
		WriteChatf("MQ2AutoCamp :: /endmac before camping is %s", bEndMacro?"\agON\ax":"\arOFF\ax");
	} else if(!_strcmpi(szArg1,"startmac")) {
		if(!_strcmpi(szArg2, "")) {
			WriteChatf("Usage: /autocamp startmac on|off");
			WriteChatf("       Toggles /mac macroname after getting a rez. Default is \arOFF\ax");
			return;
		}
		if(!_strcmpi(szArg2, "on")) {
			bStartMacro = true;
		}
		else if(!_strcmpi(szArg2, "off")) {
			bStartMacro = false;
		}
		else if(!_strcmpi(szArg2, "set")) {
			sprintf_s(szMacroName,"%s",szArg3);
			if(!_strcmpi(szArg3, "")) {
				WriteChatf("Usage: /autocamp startmac set \"macroname param1 param2 ... paramN\" ");
				WriteChatf("       Sets macroname to be started after getting a rez.");
			} else {
				WriteChatf("MQ2AutoCamp :: Macro to start after rez is \ag%s\ax", szMacroName);
			}
			return;
		}
		else if(!_strcmpi(szArg2, "clear")) {
			sprintf_s(szMacroName,"%s",szArg3);
			WriteChatf("MQ2AutoCamp :: Macro to start after rez has been removed.");
			return;
		}
		WriteChatf("MQ2AutoCamp :: Starting macro after rez is %s", bStartMacro?"\agON\ax":"\arOFF\ax");
	} else {
		ShowHelp();
	}

}

// Called once, when the plugin is to initialize
PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2AutoCamp");
	AddCommand("/autocamp",AutoCampCommand);
}

// Called once, when the plugin is to shutdown
PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2AutoCamp");
	RemoveCommand("/autocamp");
}


// Called once directly after initialization, and then every time the gamestate changes
PLUGIN_API void SetGameState(int GameState)
{
	DebugSpewAlways("MQ2AutoCamp::SetGameState()");
	if(GameState==GAMESTATE_INGAME) {
		if (!bInitDone) {
			LoadINI();
		}
	} else if(GameState!=GAMESTATE_LOGGINGIN) {
		if (bInitDone) bInitDone=false;
	}
}


void PopUpWarning(const char* szText)
{
	pTextOverlay->DisplayText(szText,13,10,255,100,500,30000);
}

// This is called every time MQ pulses
PLUGIN_API void OnPulse()
{
	if (GetGameState() != GAMESTATE_INGAME || !bAutoCampEnabled) return;
	SkipPulse++;
	if (SkipPulse == SKIP_PULSES) {
		SkipPulse = 0;
		// death hasn't been detected yet
		if (!bCamp) {
			// do we have revival sickness?
			sprintf_s(szTemp,"${Me.Buff[Revival Sickness].ID}");
			ParseMacroData(szTemp, sizeof(szTemp));
			// and its been more than 2 minutes since we last saw revival sickness
			//		since a zone without rez or an abort command will not clear the buff
			if (atoi(szTemp) > 0 && (time(NULL) > ResetTime + 120)) {
				// if so, start timers and show notifications
				WriteChatf("MQ2AutoCamp :: \arI am dead. Camping to desktop in %d minutes.\ax",CampCount);
				WriteChatf("MQ2AutoCamp :: \arZone or \at/autocamp abort\ar to abort\ax");
				sprintf_s(szTemp,". . . Camping to desktop in %d minutes . . .", CampCount);
				PopUpWarning(szTemp);
				bCamp = true;
				StartTime = time(NULL);
				WarningTime = time(NULL);
				WarningCount = CampCount-1;
				if (bEndMacroWhenDead && gRunning) DoCommand(GetCharInfo()->pSpawn,"/endmac");
			}
		}
		// we've seen death and another minute has passed, issue warning on pending camp
		if (bCamp && (time(NULL) > WarningTime + 60) && WarningCount > 0) {
			WriteChatf("MQ2AutoCamp :: \arI am dead. Camping to desktop in %d minutes.\ax", WarningCount);
			WriteChatf("MQ2AutoCamp :: \arZone or \at/autocamp abort\ar to abort\ax");
			sprintf_s(szTemp,". . . Camping to desktop in %d minutes . . .",WarningCount);
			PopUpWarning(szTemp);
			WarningCount--;
			WarningTime = time(NULL);
		}

		// we've seen death and CampCount minutes have passed so camp to desktop
		if (bCamp && (time(NULL) > StartTime + (CampCount * 60))) {
			// reset start time so we camp again if we are still in game in 60 seconds.
			StartTime += 60;
			WriteChatf("MQ2AutoCamp :: \arI am dead. Camping to desktop NOW.\ax");
			PopUpWarning(". . . . Camping to desktop NOW . . . . ");
			if (bEndMacro && gRunning) DoCommand(GetCharInfo()->pSpawn,"/endmac");
			DoCommand(GetCharInfo()->pSpawn,"/camp desktop");
		}

		sprintf_s(szTemp,"${Me.Buff[Resurrection Sickness].ID}");
		ParseMacroData(szTemp, sizeof(szTemp));
		if (atoi(szTemp) > 0) {
			bCamp = false;
			ResetTime = time(NULL);
			sprintf_s(szHold,"%s", "");
			sprintf_s(szHold,"/mac %s", szMacroName);
			if (bStartMacro && !gRunning) {
				WriteChatf("MQ2AutoCamp :: \agRez Detected. Executing %s\ax", szHold);
				DoCommand(GetCharInfo()->pSpawn,szHold);
			}
		}
	}
}


PLUGIN_API void OnEndZone() {
	// if we zone assume manual control or rez and abort auto camping
	if (bCamp) {
		bCamp = false;
		WriteChatf("MQ2AutoCamp :: Camp to desktop \arABORTED\ax because of zoning");
		ResetTime = time(nullptr);
	}
}
