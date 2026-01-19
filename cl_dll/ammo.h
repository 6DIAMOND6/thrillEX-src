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

#ifndef __AMMO_H__
#define __AMMO_H__

#define MAX_WEAPON_NAME 128


#define WEAPON_FLAGS_SELECTONEMPTY	1

#define WEAPON_IS_ONTARGET 0x40

struct WEAPON
{
	char	szName[MAX_WEAPON_NAME];
	int		iAmmoType;
	int		iAmmo2Type;
	int		iMax1;
	int		iMax2;
	int		iSlot;
	int		iSlotPos;
	int		iFlags;
	int		iId;
	int		iClip;
	int		iMaxClip;

	int		iCount;		// # of itesm in plist

	HSPRITE hAlphaIcon;		// NEW - serecky 1.2.26
	HSPRITE hQuakeIcon;		// SERECKY JAN-18-26: NEW!!!
	HSPRITE hQuakeIcon2;	// SERECKY JAN-18-26: NEW!!!

	HSPRITE hActive;
	wrect_t rcActive;
	HSPRITE hInactive;
	wrect_t rcInactive;
	HSPRITE	hAmmo;
	wrect_t rcAmmo;
	HSPRITE hAmmo2;
	wrect_t rcAmmo2;
	HSPRITE hCrosshair;
	wrect_t rcCrosshair;
	HSPRITE hAutoaim;
	wrect_t rcAutoaim;
	HSPRITE hZoomedCrosshair;
	wrect_t rcZoomedCrosshair;
	HSPRITE hZoomedAutoaim;
	wrect_t rcZoomedAutoaim;
};

#define ITEM_ALWAYS_DRAW	1
#define ITEM_BAR_COUNT		2
#define ITEM_NUMBER_COUNT	4


typedef struct hud_item_s
{
	char		szName[MAX_WEAPON_NAME];
	int			iCount;
	int			iMax;
	int			iFlags;

	HSPRITE		hIcon;
	wrect_t		rcIcon;
	HSPRITE		hAlphaIcon;
	wrect_t		rcAlphaIcon;
} HUD_ITEM;

typedef int AMMO;


#endif