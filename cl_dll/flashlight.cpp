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
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>



DECLARE_MESSAGE(m_Flash, FlashBat)
DECLARE_MESSAGE(m_Flash, Flashlight)

#define BAT_NAME "sprites/%d_Flashlight.spr"

int CHudFlashlight::Init(void)
{
	m_fOn = 0;

	HOOK_MESSAGE(Flashlight);
	HOOK_MESSAGE(FlashBat);

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
};

void CHudFlashlight::Reset(void)
{
	m_fOn = 0;
	m_fCanDraw = 0;

	m_iCoord[0] = 0;
	m_iCoord[1] = 0;
}

int CHudFlashlight::VidInit(void)
{
	int HUD_flash_on = gHUD.GetSpriteIndex( "flash_on" );
	int HUD_flash_on_a = gHUD.GetSpriteIndex( "a_flash_on" );

	int HUD_flash_off = gHUD.GetSpriteIndex( "flash_off" );
	int HUD_flash_off_a = gHUD.GetSpriteIndex( "a_flash_off" );

	m_hSpriteOn[0] = gHUD.GetSprite(HUD_flash_on);
	m_hSpriteOn[1] = gHUD.GetSprite(HUD_flash_on_a);

	m_hSpriteOff[0] = gHUD.GetSprite(HUD_flash_off);
	m_hSpriteOff[1] = gHUD.GetSprite(HUD_flash_off_a);

	m_prcOn = &gHUD.GetSpriteRect(HUD_flash_on);
	m_prcOff = &gHUD.GetSpriteRect(HUD_flash_off);

	return 1;
};

int CHudFlashlight:: MsgFunc_FlashBat(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ((float)x)/100.0;

	return 1;
}

int CHudFlashlight:: MsgFunc_Flashlight(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE();
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ((float)x)/100.0;

	return 1;
}

// Rewrote this to work with the current Apparatus setup...
// - serecky 1.13.26

int CHudFlashlight::Draw(float flTime)
{
	int r, g, b, x, y;
	int iWidth, iHeight;
	int isAlpha;

	if (!gHUD.m_Ammo.m_pCvarWListStyle)
		return 1;

	isAlpha = gHUD.m_Ammo.m_pCvarWListStyle->value ? 1 : 0;

	x = m_iCoord[0];
	y = m_iCoord[1];

	if (isAlpha)
	{
		iWidth = iHeight = 32;
	}
	else
	{
		iWidth = m_prcOn->right - m_prcOn->left;
		iHeight = abs(m_prcOn->top - m_prcOn->bottom);
	}

	UnpackRGB(r,g,b, RGB_GREENISH);
	ScaleColors(r, g, b, 255);

	if( m_fOn )
	{
		if (isAlpha)
		{
			SPR_Set( m_hSpriteOn[1], 255, 255, 255 );
			SPR_DrawHoles( 0, m_iCoord[0], m_iCoord[1], NULL );
			UnpackRGB(r,g,b, RGB_GREENISH);
			FillRGBA(x, y, iWidth, iHeight, r, g, b, 96);
		}
		else
		{
			SPR_Set( m_hSpriteOn[0], r, g, b );
			SPR_DrawAdditive( 0, m_iCoord[0], m_iCoord[1], m_prcOn );
			UnpackRGB(r,g,b, RGB_GREENISH);
			FillRGBA(x, y, iWidth, iHeight, r, g, b, 64);
		}
	}
	else if ( m_fCanDraw )
	{
		if (isAlpha)
		{
			SPR_Set( m_hSpriteOff[1], 255, 255, 255 );
			SPR_DrawHoles( 0, m_iCoord[0], m_iCoord[1], NULL );
			UnpackRGB(r,g,b, RGB_GREENISH);
			FillRGBA(x, y, iWidth, iHeight, r, g, b, 96);
		}
		else
		{
			SPR_Set( m_hSpriteOff[0], r, g, b );
			SPR_DrawAdditive( 0, m_iCoord[0], m_iCoord[1], m_prcOff );
			UnpackRGB(r,g,b, RGB_GREENISH);
			FillRGBA(x, y, iWidth, iHeight, r, g, b, 64);
		}
	}

	return 1;
}

int CHudFlashlight::IsOn( void )
{
	return m_fOn;
}