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
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "pm_shared.h"

#include <string.h>
#include <stdio.h>

#include "ammohistory.h"
#include "vgui_TeamFortressViewport.h"

#include "r_studioint.h"
extern engine_studio_api_t IEngineStudio;

WEAPON *gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
						// this points to the active weapon menu item
WEAPON *gpLastSel;		// Last weapon menu selection 

client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);

WeaponsResource gWR;

int g_weaponselect = 0;

cvar_t *cl_bonusflash;

//=============================================
// Hardcoded Item stuff. Was thinking about
// making these more modular, like where you
// can add weapons with UserMsgs, but there's
// no time for that!!!
// - serecky 1.11.26
//=============================================

HUD_ITEM item_airtank =
{
	"airtank",
	0,
	12,
	ITEM_ALWAYS_DRAW | ITEM_BAR_COUNT,
	NULL, NULL, NULL, NULL
};

HUD_ITEM item_longjump =
{
	"longjump",
	0,
	5,
	ITEM_BAR_COUNT,
	NULL, NULL, NULL, NULL
};

HUD_ITEM item_radiation =
{
	"radiation",
	0,
	5,
	ITEM_NUMBER_COUNT,
	NULL, NULL, NULL, NULL
};

HUD_ITEM item_antidote =
{
	"antidote",
	0,
	5,
	ITEM_NUMBER_COUNT,
	NULL, NULL, NULL, NULL
};

HUD_ITEM item_adrenaline =
{
	"adrenaline",
	0,
	1,
	0,
	NULL, NULL, NULL, NULL
};

#define MAX_HUD_ITEMS	5

HUD_ITEM* hud_items[MAX_HUD_ITEMS] =
{
	&item_airtank,
	&item_longjump,
	&item_antidote,
	&item_radiation,
	&item_adrenaline
};

//=============================================
//	LoadItemSprites
//=============================================

void LoadItemSprites(HUD_ITEM *pItem)
{
	memset( &pItem->rcIcon, 0, sizeof(wrect_t) );
	memset( &pItem->rcAlphaIcon, 0, sizeof(wrect_t) );

	pItem->hIcon = 0;
	pItem->hAlphaIcon = 0;

	if (pItem->szName == NULL)
		return;

	int iIcon;

	iIcon = gHUD.GetSpriteIndex(pItem->szName);
	pItem->hIcon = gHUD.GetSprite(iIcon);
	pItem->rcIcon = gHUD.GetSpriteRect(iIcon);

	//gEngfuncs.pfnConsolePrint(va("%s\n", pItem->szName));

	char iAlphaSprite[192];
	_snprintf(iAlphaSprite, sizeof(iAlphaSprite), "a_%s", pItem->szName);

	iIcon = gHUD.GetSpriteIndex(iAlphaSprite);
	pItem->hAlphaIcon = gHUD.GetSprite(iIcon);
	pItem->rcAlphaIcon = gHUD.GetSpriteRect(iIcon);

	//gEngfuncs.pfnConsolePrint(va("%s\n", iAlphaSprite));
}

//=============================================
//	LoadAllItemSprites
//=============================================

void LoadAllItemSprites(void)
{
	for (int i = 0; i < MAX_HUD_ITEMS; i++)
	{
		LoadItemSprites(hud_items[i]);
	}
}

//=============================================



//=============================================

void WeaponsResource :: LoadAllWeaponSprites( void )
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if ( rgWeapons[i].iId )
			LoadWeaponSprites( &rgWeapons[i] );
	}
}

int WeaponsResource :: CountAmmo( int iId ) 
{ 
	if ( iId < 0 )
		return 0;

	return riAmmo[iId];
}

int WeaponsResource :: HasAmmo( WEAPON *p )
{
	if ( !p )
		return FALSE;

	// weapons with no max ammo can always be selected
	if ( p->iMax1 == -1 )
		return TRUE;

	return (p->iAmmoType == -1) || p->iClip > 0 || CountAmmo(p->iAmmoType) 
		|| CountAmmo(p->iAmmo2Type) || ( p->iFlags & WEAPON_FLAGS_SELECTONEMPTY );
}


void WeaponsResource :: LoadWeaponSprites( WEAPON *pWeapon )
{
	int i, iRes;

	if (ScreenWidth < 640)
		iRes = 320;
	else
		iRes = 640;

	char sz[128];

	if ( !pWeapon )
		return;

	memset( &pWeapon->rcActive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcInactive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo2, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo2, 0, sizeof(wrect_t) );
	memset( &pWeapon->hAlphaIcon, 0, sizeof(wrect_t) );

	pWeapon->hInactive = 0;
	pWeapon->hActive = 0;
	pWeapon->hAmmo = 0;
	pWeapon->hAmmo2 = 0;
	pWeapon->hAlphaIcon = 0;

	sprintf(sz, "sprites/%s.txt", pWeapon->szName);
	client_sprite_t *pList = SPR_GetList(sz, &i);

	if (!pList)
		return;

	client_sprite_t *p;

	static wrect_t nullrc;
	pWeapon->hCrosshair = SPR_Load("sprites/plushair.spr");

//  x: 322, y: 250
	
	// manually set this since GetSpriteRect() requires an hud.txt entry

	pWeapon->rcCrosshair.left = 0;
	pWeapon->rcCrosshair.right = 24;
	pWeapon->rcCrosshair.top = 0;
	pWeapon->rcCrosshair.bottom = 48;

	p = GetSpriteList(pList, "weapon", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hInactive = SPR_Load(sz);
		pWeapon->rcInactive = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hInactive = 0;

	p = GetSpriteList(pList, "weapon_s", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hActive = SPR_Load(sz);
		pWeapon->rcActive = p->rc;
	}
	else
		pWeapon->hActive = 0;

	p = GetSpriteList(pList, "ammo", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo = SPR_Load(sz);
		pWeapon->rcAmmo = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hAmmo = 0;

	p = GetSpriteList(pList, "ammo2", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo2 = SPR_Load(sz);
		pWeapon->rcAmmo2 = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hAmmo2 = 0;


	// New stuff for the ALPHA styled weapon icons..
	// Making these call 320 only coz' yeah.. Also,
	// since the weapon icons basically have the same
	// size as each other at 320 and above, I'm not going
	// to be using a wrect_t...
	// - serecky 1/2/26

	p = GetSpriteList(pList, "weapon_icon", 320, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAlphaIcon = SPR_Load(sz);
	}
	else
	{
		pWeapon->hAlphaIcon = 0;
	}

	// SERECKY JAN-18-26: NEW!!!
	p = GetSpriteList(pList, "quake_icon", 320, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hQuakeIcon = SPR_Load(sz);
	}
	else
	{
		pWeapon->hQuakeIcon = 0;
	}

	p = GetSpriteList(pList, "quake_icon2", 320, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hQuakeIcon2 = SPR_Load(sz);
	}
	else
	{
		pWeapon->hQuakeIcon2 = 0;
	}
}

// Returns the first weapon for a given slot.
WEAPON *WeaponsResource :: GetFirstPos( int iSlot )
{
	WEAPON *pret = NULL;

	for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
	{
		if ( rgSlots[iSlot][i] && HasAmmo( rgSlots[iSlot][i] ) )
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}

// Returns number of weapon slots that we can draw.

int WeaponsResource::ReturnDrawableSlots( void )
{
	int count = 0;

	for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		for (int j = 0; j < MAX_WEAPON_POSITIONS; j++)
		{
			if (rgSlots[i][j])
			{
				count++;
				break;
			}
		}
	}
	//gEngfuncs.pfnConsolePrint(va("drawable count: %d\n", count));
	return count;
}

// Return 1 if our slot is drawable, and return 0 if not!!

int WeaponsResource::IsSlotDrawable( int iSlot )
{
	for (int j = 0; j < MAX_WEAPON_POSITIONS; j++)
	{
		if (rgSlots[iSlot][j])
		{
			return 1;
		}
	}
	return 0;
}

WEAPON* WeaponsResource :: GetNextActivePos( int iSlot, int iSlotPos )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	WEAPON *p = gWR.rgSlots[ iSlot ][ iSlotPos+1 ];
	
	if ( !p || !gWR.HasAmmo(p) )
		return GetNextActivePos( iSlot, iSlotPos + 1 );

	return p;
}


int giBucketHeight, giBucketWidth, giABHeight, giABWidth; // Ammo Bar width and height
int giItemOfs[2]; // SERECKY JAN-17-26: NEW

HSPRITE ghsprBuckets;					// Sprite for top row of weapons menu

DECLARE_MESSAGE(m_Ammo, CurWeapon );	// Current weapon and clip
DECLARE_MESSAGE(m_Ammo, WeaponList);	// new weapon type
DECLARE_MESSAGE(m_Ammo, AmmoX);			// update known ammo type's count
DECLARE_MESSAGE(m_Ammo, AmmoPickup);	// flashes an ammo pickup record
DECLARE_MESSAGE(m_Ammo, WeapPickup);    // flashes a weapon pickup record
DECLARE_MESSAGE(m_Ammo, HideWeapon);	// hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE(m_Ammo, ItemPickup);
DECLARE_MESSAGE(m_Ammo, RadPickup);
DECLARE_MESSAGE(m_Ammo, AntPickup);
DECLARE_MESSAGE(m_Ammo, AdrPickup);
DECLARE_MESSAGE(m_Ammo, Airtank);
DECLARE_MESSAGE(m_Ammo, LonJumBat);

DECLARE_COMMAND(m_Ammo, Slot1);
DECLARE_COMMAND(m_Ammo, Slot2);
DECLARE_COMMAND(m_Ammo, Slot3);
DECLARE_COMMAND(m_Ammo, Slot4);
DECLARE_COMMAND(m_Ammo, Slot5);
DECLARE_COMMAND(m_Ammo, Slot6);
DECLARE_COMMAND(m_Ammo, Slot7);
DECLARE_COMMAND(m_Ammo, Slot8);
DECLARE_COMMAND(m_Ammo, Slot9);
DECLARE_COMMAND(m_Ammo, Slot10);
DECLARE_COMMAND(m_Ammo, Close);
DECLARE_COMMAND(m_Ammo, NextWeapon);
DECLARE_COMMAND(m_Ammo, PrevWeapon);

// width of ammo fonts
#define AMMO_SMALL_WIDTH 10
#define AMMO_LARGE_WIDTH 20

#define HISTORY_DRAW_TIME	"2"

int CHudAmmo::Init(void)
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(CurWeapon);
	HOOK_MESSAGE(WeaponList);
	HOOK_MESSAGE(AmmoPickup);
	HOOK_MESSAGE(WeapPickup);
	HOOK_MESSAGE(ItemPickup);
	HOOK_MESSAGE(HideWeapon);
	HOOK_MESSAGE(AmmoX);
	HOOK_MESSAGE(RadPickup);
	HOOK_MESSAGE(AntPickup);
	HOOK_MESSAGE(AdrPickup);
	HOOK_MESSAGE(Airtank);
	HOOK_MESSAGE(LonJumBat);

	HOOK_COMMAND("slot1", Slot1);
	HOOK_COMMAND("slot2", Slot2);
	HOOK_COMMAND("slot3", Slot3);
	HOOK_COMMAND("slot4", Slot4);
	HOOK_COMMAND("slot5", Slot5);
	HOOK_COMMAND("slot6", Slot6);
	HOOK_COMMAND("slot7", Slot7);
	HOOK_COMMAND("slot8", Slot8);
	HOOK_COMMAND("slot9", Slot9);
	HOOK_COMMAND("slot10", Slot10);
	HOOK_COMMAND("cancelselect", Close);
	HOOK_COMMAND("invnext", NextWeapon);
	HOOK_COMMAND("invprev", PrevWeapon);

	Reset();

	CVAR_CREATE( "hud_drawhistory_time", HISTORY_DRAW_TIME, FCVAR_ARCHIVE );
	CVAR_CREATE( "hud_fastswitch", "0", FCVAR_ARCHIVE );		// controls whether or not weapons can be selected in one keypress
	
	m_pCvarWListStyle = CVAR_CREATE( "hud_wlist_style", "0", FCVAR_ARCHIVE ); // New - serecky 1.1.26
	cl_bonusflash = gEngfuncs.pfnRegisterVariable( "cl_bonusflash", "1", FCVAR_ARCHIVE);


	m_iFlags |= HUD_ACTIVE; //!!!

	gWR.Init();
	gHR.Init();

	return 1;
};

void CHudAmmo::Reset(void)
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!

	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;
	item_longjump.iFlags = ITEM_BAR_COUNT;

	gWR.Reset();
	gHR.Reset();
}

int CHudAmmo::VidInit(void)
{
	gHUD.plushairindex = gHUD.GetSpriteIndex( "crosshairplus" );

	// Load sprites for buckets (top row of weapon menu)
	m_HUD_small0 = gHUD.GetSpriteIndex( "small_1" );
	m_HUD_bucket0 = gHUD.GetSpriteIndex( "bucket1" );
	m_HUD_selection = gHUD.GetSpriteIndex( "selection" );

	m_prcSmallNum = &gHUD.GetSpriteRect( m_HUD_small0 );

	ghsprBuckets = gHUD.GetSprite(m_HUD_bucket0);
	giBucketWidth = gHUD.GetSpriteRect(m_HUD_bucket0).right - gHUD.GetSpriteRect(m_HUD_bucket0).left;
	giBucketHeight = gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top;

	gHR.iHistoryGap = max( gHR.iHistoryGap, gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top);

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();
	LoadAllItemSprites();

	if (ScreenWidth >= 640)
	{
		giABWidth = 20;
		giABHeight = 4;
		giItemOfs[0] = 8;
	}
	else
	{
		giABWidth = 10;
		giABHeight = 2;
		giItemOfs[0] = 4;
	}

	giItemOfs[1] = 8;

	return 1;
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think(void)
{
	if ( gHUD.m_fPlayerDead )
		return;

	if ( gHUD.m_iWeaponBits != gWR.iOldWeaponBits )
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;

		for (int i = MAX_WEAPONS-1; i > 0; i-- )
		{
			WEAPON *p = gWR.GetWeapon(i);

			if ( p )
			{
				if ( gHUD.m_iWeaponBits & ( 1 << p->iId ) )
					gWR.PickupWeapon( p );
				else
					gWR.DropWeapon( p );
			}
		}
	}

	if (!gpActiveSel)
		return;

	// has the player selected one?
	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		if (gpActiveSel != (WEAPON *)1)
		{
			ServerCmd(gpActiveSel->szName);
			g_weaponselect = gpActiveSel->iId;
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;

		PlaySound("common/wpn_select.wav", 1);
	}

}

//
// Helper function to return a Ammo pointer from id
//

HSPRITE* WeaponsResource :: GetAmmoPicFromWeapon( int iAmmoId, wrect_t& rect )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( rgWeapons[i].iAmmoType == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo;
			return &rgWeapons[i].hAmmo;
		}
		else if ( rgWeapons[i].iAmmo2Type == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo2;
			return &rgWeapons[i].hAmmo2;
		}
	}

	return NULL;
}


// Menu Selection Code

void WeaponsResource :: SelectSlot( int iSlot, int fAdvance, int iDirection )
{
	if ( gHUD.m_Menu.m_fMenuDisplayed && (fAdvance == FALSE) && (iDirection == 1) )	
	{ // menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem( iSlot + 1 );  // slots are one off the key numbers
		return;
	}

	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	if ( gHUD.m_fPlayerDead || gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
		return;

	WEAPON *p = NULL;
	bool fastSwitch = CVAR_GET_FLOAT( "hud_fastswitch" ) != 0;

	if ( (gpActiveSel == NULL) || (gpActiveSel == (WEAPON *)1) || (iSlot != gpActiveSel->iSlot) )
	{
		PlaySound( "common/wpn_hudon.wav", 1 );
		p = GetFirstPos( iSlot );

		if ( p && fastSwitch ) // check for fast weapon switch mode
		{
			// if fast weapon switch is on, then weapons can be selected in a single keypress
			// but only if there is only one item in the bucket
			WEAPON *p2 = GetNextActivePos( p->iSlot, p->iSlotPos );
			if ( !p2 )
			{	// only one active item in bucket, so change directly to weapon
				ServerCmd( p->szName );
				g_weaponselect = p->iId;
				return;
			}
		}
	}
	else
	{
		PlaySound("common/wpn_moveselect.wav", 1);
		if ( gpActiveSel )
			p = GetNextActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );
		if ( !p )
			p = GetFirstPos( iSlot );
	}

	
	if ( !p )  // no selection found
	{
		// just display the weapon list, unless fastswitch is on just ignore it
		if ( !fastSwitch )
			gpActiveSel = (WEAPON *)1;
		else
			gpActiveSel = NULL;
	}
	else 
		gpActiveSel = p;
}

//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

//
// AmmoX  -- Update the count of a known type of ammo
// 
int CHudAmmo::MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	gWR.SetAmmo( iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	// Add ammo to the history
	gHR.AddToHistory( HISTSLOT_AMMO, iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_WEAP, iIndex );

	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *szName = READ_STRING();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_ITEM, szName );

	return 1;
}


int CHudAmmo::MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	gHUD.m_iHideHUDDisplay = READ_BYTE();

	if (gEngfuncs.IsSpectateOnly())
		return 1;

	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
	{
		static wrect_t nullrc;
		gpActiveSel = NULL;
		SetCrosshair( 0, nullrc, 0, 0, 0 );
	}
	else
	{
		if ( m_pWeapon )
		
		SetCrosshair( m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255 );
	}

	return 1;
}

// 
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type 
//  numbers match a real ammo type.
//
int CHudAmmo::MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf )
{
	static wrect_t nullrc;
	int fOnTarget = FALSE;

	BEGIN_READ( pbuf, iSize );

	int iState = READ_BYTE();
	int iId = READ_CHAR();
	int iClip = READ_CHAR();

	// detect if we're also on target
	if ( iState > 1 )
	{
		fOnTarget = TRUE;
	}

	if ( iId < 1 )
	{
		SetCrosshair(0, nullrc, 0, 0, 0);
		return 0;
	}

	if ( g_iUser1 != OBS_IN_EYE )
	{
		// Is player dead???
		if ((iId == -1) && (iClip == -1))
		{
			gHUD.m_fPlayerDead = TRUE;
			gpActiveSel = NULL;
			return 1;
		}
		gHUD.m_fPlayerDead = FALSE;
	}

	WEAPON *pWeapon = gWR.GetWeapon( iId );

	if ( !pWeapon )
		return 0;

	if ( iClip < -1 )
		pWeapon->iClip = abs(iClip);
	else
		pWeapon->iClip = iClip;


	if ( iState == 0 )	// we're not the current weapon, so update no more
		return 1;

	m_pWeapon = pWeapon;

	SetCrosshair(m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255);

	gHUD.m_GeneralHud.m_iAmmoFade = 255.0f; //!!!
	m_iFlags |= HUD_ACTIVE;
	
	return 1;
}

//
// WeaponList -- Tells the hud about a new weapon type.
//
int CHudAmmo::MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	WEAPON Weapon;

	strcpy( Weapon.szName, READ_STRING() );
	Weapon.iAmmoType = (int)READ_CHAR();	
	
	Weapon.iMax1 = READ_BYTE();
	if (Weapon.iMax1 == 255)
		Weapon.iMax1 = -1;

	Weapon.iAmmo2Type = READ_CHAR();
	Weapon.iMax2 = READ_BYTE();
	if (Weapon.iMax2 == 255)
		Weapon.iMax2 = -1;

	Weapon.iSlot = READ_CHAR();
	Weapon.iSlotPos = READ_CHAR();
	Weapon.iId = READ_CHAR();
	Weapon.iFlags = READ_BYTE();
	Weapon.iMaxClip = READ_CHAR();
	Weapon.iClip = 0;

	gWR.AddWeapon( &Weapon );

	return 1;

}

int CHudAmmo::MsgFunc_RadPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_radiationCount = READ_SHORT();
	item_radiation.iCount = m_radiationCount;

	return 1;
}

int CHudAmmo::MsgFunc_AntPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_antidoteCount = READ_SHORT();
	item_antidote.iCount = m_antidoteCount;

	return 1;
}

int CHudAmmo::MsgFunc_AdrPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_adrenalineCount = READ_SHORT();
	item_adrenaline.iCount = m_adrenalineCount;

	return 1;
}

int CHudAmmo::MsgFunc_Airtank( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

	m_iAirtank = x;
	item_airtank.iCount = m_iAirtank;

	return 1;
}

int CHudAmmo::MsgFunc_LonJumBat( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

	m_iLongJumpBat = x;
	item_longjump.iCount = m_iLongJumpBat;

	if (!( item_longjump.iFlags & ITEM_ALWAYS_DRAW ) && m_iLongJumpBat > -1)
		item_longjump.iFlags |= ITEM_ALWAYS_DRAW;

	return 1;
}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
// Slot button pressed
void CHudAmmo::SlotInput( int iSlot )
{
	if ( gViewPort && gViewPort->SlotInput( iSlot ) )
		return;

	gWR.SelectSlot(iSlot, FALSE, 1);
}

void CHudAmmo::UserCmd_Slot1(void)
{
	SlotInput( 0 );
}

void CHudAmmo::UserCmd_Slot2(void)
{
	SlotInput( 1 );
}

void CHudAmmo::UserCmd_Slot3(void)
{
	SlotInput( 2 );
}

void CHudAmmo::UserCmd_Slot4(void)
{
	SlotInput( 3 );
}

void CHudAmmo::UserCmd_Slot5(void)
{
	SlotInput( 4 );
}

void CHudAmmo::UserCmd_Slot6(void)
{
	SlotInput( 5 );
//	PlaySound("PLAYER/hoot1.wav", 1);
}

void CHudAmmo::UserCmd_Slot7(void)
{
	SlotInput( 6 );
}

void CHudAmmo::UserCmd_Slot8(void)
{
	SlotInput( 7 );
}

void CHudAmmo::UserCmd_Slot9(void)
{
	SlotInput( 8 );
}

void CHudAmmo::UserCmd_Slot10(void)
{
	SlotInput( 9 );
}

void CHudAmmo::UserCmd_Close(void)
{
	if (gpActiveSel)
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		PlaySound("common/wpn_hudoff.wav", 1);
	}
	else
		EngineClientCmd("escape");
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = 0;
	int slot = 0;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos + 1;
		slot = gpActiveSel->iSlot;
	}

	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot < MAX_WEAPON_SLOTS; slot++ )
		{
			for ( ; pos < MAX_WEAPON_POSITIONS; pos++ )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if ( wsp && gWR.HasAmmo(wsp) )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = 0;
		}

		slot = 0;  // start looking from the first slot again
	}

	gpActiveSel = NULL;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = MAX_WEAPON_POSITIONS-1;
	int slot = MAX_WEAPON_SLOTS-1;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos - 1;
		slot = gpActiveSel->iSlot;
	}
	
	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot >= 0; slot-- )
		{
			for ( ; pos >= 0; pos-- )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if ( wsp && gWR.HasAmmo(wsp) )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS-1;
		}
		
		slot = MAX_WEAPON_SLOTS-1;
	}

	gpActiveSel = NULL;
}



//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------

int CHudAmmo::Draw(float flTime)
{
	if ( (gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )) )
		return 1;

	// Draw Weapon Menu
	if (m_pCvarWListStyle && m_pCvarWListStyle->value) // NEW - serecky 1.1.26
		DrawAlphaWList(flTime);
	else
		DrawWList(flTime);

	// Draw the Quake-ified console stuff...
	// - serecky 1.13.26
	gHR.DrawAmmoHistory(flTime);

	if (!(m_iFlags & HUD_ACTIVE))
		return 0;

	if (!m_pWeapon)
		return 0;

	// Draw crosshair
	if ( gHUD.m_pCvarCrosshair->value )
	{
		if ( CVAR_GET_FLOAT( "crosshair" ) )
			gEngfuncs.pfnClientCmd( "crosshair 0" );

		if (m_pWeapon)
			DrawCrosshairScalable(m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255);
	}

	return 1;
}

//
// Draw Weapon Menu
//

int CHudAmmo::DrawWList(float flTime)
{
	int r, g, b, x, y, a, i, j, iWidth, iHeight;

	bool shouldDraw;

	if ( !gpActiveSel && !(gHUD.m_pCvarApparatus->value) )
		shouldDraw = false;
	else
		shouldDraw = true;

	//
	// Draw Apparatus
	//

	// Rewrote the Apparatus code to be slightly more cleaner...
	// - serecky 1.12.26

	int iItemCount = 0;

	UnpackRGB(r,g,b, RGB_GREENISH);
	ScaleColors(r, g, b, 255);

	if (ScreenWidth >= 640)
	{
		iWidth = iHeight = 60;
		x = gHUD.m_iHudScaleWidth - (iWidth + 10);
		y = 8;
	}
	else
	{
		iWidth = iHeight = 30;
		x = gHUD.m_iHudScaleWidth - (iWidth + 20);
		y = 4;
	}

	for (j = 0; j < MAX_HUD_ITEMS; j++)
	{
		if (!shouldDraw || !hud_items[j])
			continue;

		if ( ( hud_items[j]->iCount > 0 ) || ( hud_items[j]->iFlags & ITEM_ALWAYS_DRAW ) )
		{
			SPR_Set(hud_items[j]->hIcon, r, g, b );
			SPR_DrawAdditive(0, x, y, &hud_items[j]->rcIcon);

			if ( hud_items[j]->iFlags & ITEM_NUMBER_COUNT )
			{
				int iNumHeight = abs(m_prcSmallNum->top - m_prcSmallNum->bottom);
				int iNewY = y + iHeight - iNumHeight;
				int iCount = hud_items[j]->iCount - 1;

				SPR_Set( gHUD.GetSprite( m_HUD_small0 + iCount ), r, g, b );
				SPR_DrawAdditive( 0, x, iNewY, &gHUD.GetSpriteRect( m_HUD_small0 + iCount ) );
			}

			if ( hud_items[j]->iFlags & ITEM_BAR_COUNT )
			{
				if (( hud_items[j]->iCount > 0 ) && ( hud_items[j]->iMax > 0 ))
				{
					float flOffset = (float)hud_items[j]->iCount / (float)hud_items[j]->iMax;
					int iNewHeight = iHeight * flOffset;
					int iNewBarY = y + iHeight - iNewHeight;

					FillRGBA(x, iNewBarY, iWidth, iNewHeight, r, g, b, 32);
				}
				FillRGBA(x, y, iWidth, iHeight, r, g, b, 16);
			}
			else
			{
				FillRGBA(x, y, iWidth, iHeight, r, g, b, 48);
			}

			y += iHeight + giItemOfs[0];
		}
		else
		{
			continue;
		}
	}

	if (gHUD.m_Flash.IsOn() || shouldDraw)
	{
		gHUD.m_Flash.m_iCoord[0] = x;
		gHUD.m_Flash.m_iCoord[1] = y;
		gHUD.m_Flash.m_fCanDraw = 1;
	}
	else
	{
		gHUD.m_Flash.m_fCanDraw = 0;
	}

	if ( !shouldDraw )
		return 0;

	// Weapon-List code.

	if ( !gpActiveSel )
		return 0;

	int iActiveSlot;

	if ( gpActiveSel == (WEAPON *)1 )
		iActiveSlot = -1;	// current slot has no weapons
	else 
		iActiveSlot = gpActiveSel->iSlot;

	x = 10; //!!!
	y = 10; //!!!
	
	// Ensure that there are available choices in the active slot
	if ( iActiveSlot > 0 )
	{
		if ( !gWR.GetFirstPos( iActiveSlot ) )
		{
			gpActiveSel = (WEAPON *)1;
			iActiveSlot = -1;
		}
	}
		
	// Draw top line
	for ( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		int iWidth;

		UnpackRGB(r,g,b, RGB_GREENISH);
	
		if ( iActiveSlot == i )
			a = 255;
		else
			a = 192;

		ScaleColors(r, g, b, 255);
		SPR_Set(gHUD.GetSprite(m_HUD_bucket0 + i), r, g, b );

		// make active slot wide enough to accomodate gun pictures
		if ( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos(iActiveSlot);
			if ( p )
				iWidth = p->rcActive.right - p->rcActive.left;
			else
				iWidth = giBucketWidth;
		}
		else
			iWidth = giBucketWidth;

		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_bucket0 + i));
		
		x += iWidth + 5;
	}


	a = 128; //!!!
	x = 10;

	// Draw all of the buckets
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		y = giBucketHeight + 10;

		// If this is the active slot, draw the bigger pictures,
		// otherwise just draw boxes
		if ( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos( i );
			int iWidth = giBucketWidth;
			if ( p )
				iWidth = p->rcActive.right - p->rcActive.left;

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				p = gWR.GetWeaponSlot( i, iPos );

				if ( !p || !p->iId )
					continue;

				UnpackRGB( r,g,b, RGB_GREENISH );
			
				// if active, then we must have ammo.

				if ( gpActiveSel == p )
				{
					SPR_Set(p->hActive, r, g, b );
					SPR_DrawAdditive(0, x, y, &p->rcActive);

					SPR_Set(gHUD.GetSprite(m_HUD_selection), r, g, b );
					SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_selection));
				}
				else
				{
					// Draw Weapon if Red if no ammo

					if ( gWR.HasAmmo(p) )
						ScaleColors(r, g, b, 192);
					else
					{
						UnpackRGB(r,g,b, RGB_REDISH);
						ScaleColors(r, g, b, 128);
					}

					SPR_Set( p->hInactive, r, g, b );
					SPR_DrawAdditive( 0, x, y, &p->rcInactive );
				}		
				y += p->rcActive.bottom - p->rcActive.top + 5;
			}
			x += iWidth + 5;
		}
		else
		{
			// Draw Row of weapons.

			UnpackRGB(r,g,b, RGB_GREENISH);

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				WEAPON *p = gWR.GetWeaponSlot( i, iPos );
				
				if ( !p || !p->iId )
					continue;

				if ( gWR.HasAmmo(p) )
				{
					UnpackRGB(r,g,b, RGB_GREENISH);
					a = 128;
				}
				else
				{
					UnpackRGB(r,g,b, RGB_REDISH);
					a = 96;
				}

				FillRGBA( x, y, giBucketWidth, giBucketHeight, r, g, b, a );

				y += giBucketHeight + 5;
			}

			x += giBucketWidth + 5;
		}
	}	
	
	return 1;

}

//=================================
//
//	DrawAlphaWList
//
//	Here's my drawing code for a Half-Life Alpha
//	styled weapon list that I ripped out of SereckyMod... 
//	Basically just makes all the weapon icons wrap 
//	around the selected weapon, which this piece of
//	code tries to keep in the center...
//
//	It's a little ugly, but whatevs...
//
//	- serecky 1.1.26
//
//=================================

int GetNumberOfItems(void)
{
	int iCount = 0;

	for (int i = 0; i < MAX_HUD_ITEMS; i++)
	{
		if (!hud_items[i])
			continue;

		if ( ( hud_items[i]->iCount > 0 ) || ( hud_items[i]->iFlags & ITEM_ALWAYS_DRAW ) )
			iCount++;
		else
			continue;
	}

	//gEngfuncs.pfnConsolePrint(va("%d\n", iCount));
	return iCount;
}

int CHudAmmo::DrawAlphaWList(float flTime)
{
	int r, g, b, x, y, a, i, j;
	bool shouldDraw;

	int iAvailableSlots = gWR.ReturnDrawableSlots();
	int iItemCount = GetNumberOfItems();
	int iWidth, iHeight;

	if ( !gpActiveSel && !gHUD.m_pCvarApparatus->value )
		shouldDraw = false;
	else
		shouldDraw = true;

	// Draw Alpha styled apparatus

	iWidth = iHeight = 32;

	x = gHUD.m_iHudScaleWidth - ( iWidth + 8 );

	if (shouldDraw)
		y = (gHUD.m_iHudScaleHeight - (iItemCount * (iHeight + giItemOfs[1]))) * 0.5f;
	else
		y = (gHUD.m_iHudScaleHeight - (iHeight + giItemOfs[1])) * 0.5f;

	// SERECKY JAN-18-26: HACK FOR ALPHA HUD

	if (gHUD.m_GeneralHud.m_pCvarHudStyle->value == 1)
		y = 8;

	for (j = 0; j < MAX_HUD_ITEMS; j++)
	{
		if (!shouldDraw || !hud_items[j])
			continue;

		if ( ( hud_items[j]->iCount > 0 ) || ( hud_items[j]->iFlags & ITEM_ALWAYS_DRAW ) )
		{
			SPR_Set(hud_items[j]->hAlphaIcon, 255, 255, 255 );
			SPR_DrawHoles(0, x, y, &hud_items[j]->rcAlphaIcon);

			if ( hud_items[j]->iFlags & ITEM_NUMBER_COUNT )
			{
				int iTextHeight, iTextWidth, iNewY;

				GetConsoleStringSize("0", &iTextWidth, &iTextHeight);
				iNewY = y + (iHeight - iTextHeight);

				TRI_DrawConsoleString(x, iNewY, va("%d", hud_items[j]->iCount), iTextWidth, iTextHeight);
			}

			if ( hud_items[j]->iFlags & ITEM_BAR_COUNT )
			{
				if (( hud_items[j]->iCount > 0 ) && ( hud_items[j]->iMax > 0 ))
				{
					float flOffset = (float)hud_items[j]->iCount / (float)hud_items[j]->iMax;
					int iNewHeight = iHeight * flOffset;
					int iNewBarY = y + iHeight - iNewHeight;

					a = 96;
					UnpackRGB( r, g, b, RGB_GREENISH );
					FillRGBA(x, iNewBarY, iWidth, iNewHeight, r, g, b, a);
				}
			}
			else
			{
				a = 96;
				UnpackRGB( r, g, b, RGB_GREENISH );
				FillRGBA(x, y, iWidth, iHeight, r, g, b, a);
			}
			y += iHeight + giItemOfs[1];
		}
		else
		{
			continue;
		}
	}

	if (gHUD.m_Flash.IsOn() || shouldDraw)
	{
		gHUD.m_Flash.m_iCoord[0] = x;
		gHUD.m_Flash.m_iCoord[1] = y;
		gHUD.m_Flash.m_fCanDraw = 1;
	}
	else
	{
		gHUD.m_Flash.m_fCanDraw = 0;
	}

	if ( !gpActiveSel )
		return 0;

	static int iActiveSlot = 0;
	static int iRealActiveSlot = 0;

	if ( gpActiveSel != (WEAPON *)1 )
		iActiveSlot = gpActiveSel->iSlot;

	// Ensure that there are available choices in the active slot
	if ( iActiveSlot > 0 )
	{
		if ( !gWR.GetFirstPos( iActiveSlot ) )
			gpActiveSel = (WEAPON *)1;
	}

	int iRealSlot = 0;

	// Decouple getting actual active slot from
	// main drawing loop. -serecky 1.9.26
	for ( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		if (!gWR.IsSlotDrawable(i))
			continue;

		// Make active slot the first useable slot
		// if it's zero'd and we don't have a crowbar... 
		// - serecky 12.18.25

		if (iActiveSlot == 0 && !gWR.IsSlotDrawable(0))
			iActiveSlot = i;

		if (i == iActiveSlot && iRealActiveSlot != iRealSlot)
			iRealActiveSlot = iRealSlot;

		iRealSlot++;
	}

	iWidth = 32;
	int iOffset = iWidth + 8;
	int iNewOffset, iPosCount;

	wrect_t rect;

	rect.top = 0; rect.left = 0;
	rect.bottom = 32; rect.right = 32;

	iRealSlot = 0; // Reset this here for drawing purposes.

	for ( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		if (!gWR.IsSlotDrawable(i))
			continue;

		if (iAvailableSlots)
		{
			iNewOffset = iRealSlot - iRealActiveSlot;

			if ((iAvailableSlots % 2) > 0)
			{
				if (iNewOffset > (iAvailableSlots * 0.5f))
					iNewOffset -= iAvailableSlots;
				else if (iNewOffset < (iAvailableSlots * -0.5f))
					iNewOffset += iAvailableSlots;
			}
			else
			{
				if (iNewOffset >= (int)(iAvailableSlots * 0.5f))
					iNewOffset -= iAvailableSlots;
				else if (iNewOffset < (int)(iAvailableSlots * -0.5f))
					iNewOffset += iAvailableSlots;
			}

			x = (gHUD.m_iHudScaleWidth * 0.5f) + (iNewOffset * iOffset);
		}

		iRealSlot += 1;
		iPosCount = 0;

		for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
		{
			WEAPON *p = gWR.GetWeaponSlot( i, iPos );

			if ( !p )
			{
				//gEngfuncs.pfnConsolePrint(va("refusing to draw slot: %d pos: %d\n", i, iPos));
				continue;
			}

			y = (gHUD.m_iHudScaleHeight * 0.5f - (iWidth * 0.5f)) + (iPosCount * iOffset);

			iPosCount++; // Added this to prevent weird gaps from happening... - serecky 1.2.26

			if ( gpActiveSel == p && gWR.HasAmmo(p) ) 
			{
				r = 255; g = 255; b = 0; a = 192;
				ScaleColors(r, g, b, a);
			}
			else 
			{
				a = 144;
				if ( gWR.HasAmmo(p) ) 
				{
					UnpackRGB(r, g, b, RGB_GREENISH);
					ScaleColors(r, g, b, a);
				}
				else 
				{
					UnpackRGB(r, g, b, RGB_REDISH);
					ScaleColors(r, g, b, a);
				}
			}

			FillRGBA(x, y, 32, 32, r, g, b, a);

			r = 255; g = 255; b = 255;
			SPR_Set( p->hAlphaIcon, r, g, b );
			SPR_DrawHoles( 0, x, y, &rect );

			//gEngfuncs.pfnDrawConsoleString(x, y, va("%d\n", i));
		}
		//gEngfuncs.pfnConsolePrint(va("Slot number:%d ofs: %d\n",i, iNewOffset));
		//gEngfuncs.pfnConsolePrint(va("available: %d\n", iAvailableSlots));
	}	
	//gEngfuncs.pfnConsolePrint(va("Selected: %d\n", iActiveSlot));
	//gEngfuncs.pfnConsolePrint(va("real active:%d Hud active: %d Actual slots: %d\n",iActiveSlot, iRealActiveSlot, iAvailableSlots));

	return 1;
}

/* =================================
	DrawCrosshairScalable

This non-engine function is able to draw
a crosshair with different size and placement
on user's request. It is regulated by 
"hud_crosshair" cvar.
================================= */

void CHudAmmo :: DrawCrosshairScalable(HSPRITE spr, wrect_t prc, int r, int g, int b)
{
	int x,y;
	
	if ( gHUD.m_pCvarCrosshair->value == 1 )	// user-friendly variant: crosshair is placed right in the center of the screen
	{
		x = gHUD.m_iHudScaleWidth/2 - prc.right/2;
		y = gHUD.m_iHudScaleHeight/2 - prc.bottom/2 - prc.bottom/3;
	}
	else if ( gHUD.m_pCvarCrosshair->value >= 2 )	// alpha-accurate variant
	{
		x = gHUD.m_iHudScaleWidth/2 - prc.right/2 + 2;
		y = gHUD.m_iHudScaleHeight/2 - prc.bottom/2 + 10;
	}

	SPR_Set( spr, r, g, b );
	SPR_DrawHoles( 0, x, y, &prc );
}

/* =================================
	GetSpriteList

Finds and returns the matching 
sprite name 'psz' and resolution 'iRes'
in the given sprite list 'pList'
iCount is the number of items in the pList

================================= */
client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount)
{
	if (!pList)
		return NULL;

	int i = iCount;
	client_sprite_t *p = pList;

	while(i--)
	{
		if ((!strcmp(psz, p->szName)) && (p->iRes == iRes))
			return p;
		p++;
	}

	return NULL;
}
