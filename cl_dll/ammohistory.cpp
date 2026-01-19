/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  ammohistory.cpp
//


#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "ammohistory.h"

HistoryResource gHR;

extern cvar_t *cl_bonusflash;
extern cvar_t *hud_weaponhinter;
extern void V_BonusFlash_f(void);
int HISTORY_DRAW_TIME = 5;

// this is horrible but WHATEVER!!!!

#define WEAPON_NONE				0
#define WEAPON_CROWBAR			1
#define	WEAPON_GLOCK			2
#define WEAPON_PYTHON			3
#define WEAPON_MP5				4
#define WEAPON_CHAINGUN			5
#define WEAPON_CROSSBOW			6
#define WEAPON_SHOTGUN			7
#define WEAPON_RPG				8
#define WEAPON_GAUSS			9
#define WEAPON_EGON				10
#define WEAPON_HORNETGUN		11
#define WEAPON_HANDGRENADE		12
#define WEAPON_TRIPMINE			13
#define	WEAPON_SATCHEL			14
#define	WEAPON_SNARK			15
#define WEAPON_CHUB				16
#define WEAPON_FLARE			17

const char *ReturnGunName(int iId)
{
	switch (iId)
	{
		case WEAPON_CROWBAR:		return "Crowbar";
		case WEAPON_GLOCK:			return "9mm Semi-Automatic Handgun";
		case WEAPON_PYTHON:			return ".44 Caliber Handgun";
		case WEAPON_MP5:			return "9mm Assualt Rifle w/ Grenade Launcher";
		case WEAPON_CROSSBOW:		return "High-Velocity Stealth Crossbow";
		case WEAPON_SHOTGUN:		return "Combat Shotgun";
		case WEAPON_RPG:			return "Laser-Guided Rocket Launcher";
		case WEAPON_GAUSS:			return "Expirimental Hypervelocity Projectile Weapon";
		case WEAPON_EGON:			return "Expirimental Energy Weapon";
		case WEAPON_HORNETGUN:		return "Unidentified Projectile Bio-Weapon";
		case WEAPON_HANDGRENADE:	return "Frag. Grenade";
		case WEAPON_TRIPMINE:		return "Tripmine";
		case WEAPON_SATCHEL:		return "Satchel charges";
		case WEAPON_SNARK:			return "Unidentified Bio-Grenade";
		case WEAPON_CHUB:			return "Unidentified Bio-Creature";
	}

	return "Missing gun name";
}

const char *ReturnAmmoName(int iId)
{
	switch (iId)
	{
		case 1:		return "Shotgun Shell";
		case 2:		return "9mm Bullet";
		case 3:		return "Assault Rifle Grenade";
		case 4:		return ".44 Caliber Round";
		case 5:		return "Energy Weapon Recharge Unit";
		case 6:		return "Rocket";
		case 7:		return "Crossbow Dart";
		case 8:		return "Tripmine";
		case 9:		return "Satchel Charge";
		case 10:	return "Frag. Grenade";
		case 11:	return "Unidentified Bio-Grenade";
		case 12:	return "Unidentified Bio-Projectile";
		case 13:	return "Unidentified Bio-Creature";
	}

	return "Missing ammo name";
}

void HistoryResource :: AddToHistory( int iType, int iId, int iCount )
{
	if ( iType == HISTSLOT_AMMO && !iCount )
		return;  // no amount, so don't add

	if ((cl_bonusflash != NULL) && cl_bonusflash->value)
		V_BonusFlash_f();

	if (!hud_weaponhinter->value)
		return;

	if ( iCurrentHistorySlot >= MAX_HISTORY )
		iCurrentHistorySlot = 0;
	
	HIST_ITEM *freeslot = &rgAmmoHistory[iCurrentHistorySlot++];

	if ( iType == HISTSLOT_AMMO )
	{
		if (iCount > 1)
			_snprintf(freeslot->szMessage, sizeof(freeslot->szMessage), "You got %d %ss!\n", iCount, ReturnAmmoName(iId));
		else
			_snprintf(freeslot->szMessage, sizeof(freeslot->szMessage), "You got %d %s!\n", iCount, ReturnAmmoName(iId));
	}
	else
	{
		_snprintf(freeslot->szMessage, sizeof(freeslot->szMessage), "You got the %s!\n", ReturnGunName(iId));
	}

	//gEngfuncs.pfnConsolePrint(freeslot->szMessage);

	HISTORY_DRAW_TIME = CVAR_GET_FLOAT( "hud_drawhistory_time" );
	freeslot->DisplayTime = gHUD.m_flTime + HISTORY_DRAW_TIME;
	freeslot->iOn = 1;
}



void HistoryResource :: AddToHistory( int iType, const char *szName, int iCount )
{
	if ( iType != HISTSLOT_ITEM )
		return;

	if ((cl_bonusflash != NULL) && cl_bonusflash->value)
		V_BonusFlash_f();

	if (!hud_weaponhinter->value)
		return;

	if ( iCurrentHistorySlot >= MAX_HISTORY )
		iCurrentHistorySlot = 0;

	HIST_ITEM *freeslot = &rgAmmoHistory[iCurrentHistorySlot++];

	_snprintf(freeslot->szMessage, sizeof(freeslot->szMessage), "You got the %s!\n", szName);

	//gEngfuncs.pfnConsolePrint(freeslot->szMessage);

	HISTORY_DRAW_TIME = CVAR_GET_FLOAT( "hud_drawhistory_time" );
	freeslot->DisplayTime = gHUD.m_flTime + HISTORY_DRAW_TIME;
	freeslot->iOn = 1;
}


void HistoryResource :: CheckClearHistory( void )
{
	for ( int i = 0; i < MAX_HISTORY; i++ )
	{
		if ( rgAmmoHistory[i].iOn )
			return;
	}

	iCurrentHistorySlot = 0;
}

// Rewrote the DrawAmmoHistory Code to behave sorta like
// Quake1's console since Half-Life doesn't show "print_notifiy"
// in regular mode. - serecky 1.13.26

int HistoryResource :: DrawAmmoHistory( float flTime )
{
	int TextHeight, TextWidth;
	int x, y;

	y = 0;

	for ( int i = 0; i < MAX_HISTORY; i++ )
	{
		if ( rgAmmoHistory[i].iOn )
		{
			if ( rgAmmoHistory[i].DisplayTime <= flTime )
			{
				memset( &rgAmmoHistory[i], 0, sizeof(HIST_ITEM) );
				CheckClearHistory();
			}
			else
			{
				GetConsoleStringSize( rgAmmoHistory[i].szMessage, &TextWidth, &TextHeight );

				x = (ScreenWidth * 0.5f) - (TextWidth * 0.5f);
				DrawConsoleString(x, y, rgAmmoHistory[i].szMessage);
				y += TextHeight;
			}
		}
	}

	return 1;
}


