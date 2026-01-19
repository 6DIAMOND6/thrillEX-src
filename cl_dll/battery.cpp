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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Battery, Battery)

int CHudBattery::Init(void)
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;

	HOOK_MESSAGE(Battery);

	gHUD.AddHudElem(this);

	return 1;
};


int CHudBattery::VidInit(void)
{
	int HUD_suit_empty = gHUD.GetSpriteIndex( "battery_empty" );
	int HUD_suit_full = gHUD.GetSpriteIndex( "battery_full" );

//	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_hSprite1 = gHUD.GetSprite(HUD_suit_empty);
	m_hSprite2 = gHUD.GetSprite(HUD_suit_full);
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_iHeight = m_prc2->right - m_prc1->left;
	m_fFade = 0;
	return 1;
};

int CHudBattery:: MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;
	
	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

#if defined( _TFC )
	int y = READ_SHORT();

	if ( x != m_iBat || y != m_iBatMax )
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
		m_iBatMax = y;
	}
#else
	if ( x != m_iBat )
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
	}
#endif

	return 1;
}

int CHudBattery::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	int r, g, b, x, y, a;

	UnpackRGB(r,g,b, RGB_GREENISH);

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	a = MIN_ALPHA;	

	ScaleColors(r, g, b, a );
	
	int iOffset = 50 - m_iBat/2;

	y = ScreenHeight - 72 - (m_prc1->bottom - m_prc1->top);
	x = 14;

	SPR_Set(m_hSprite1, r, g, b );
	SPR_DrawAdditive( 0,  x, y, m_prc1);	

	wrect_t rc;

	rc = *m_prc2;
	rc.left += m_iHeight * ((float)(100 - (min(100, m_iBat))) * 0.01);

	if (rc.right > rc.left)
	{
		SPR_Set(m_hSprite2, r, g, b );
		SPR_DrawAdditive( 0, x + 4 , y + 4, &rc );
	}	

	return 1;
}