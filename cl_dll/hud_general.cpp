
//======================================
//	hud_general.cpp
//
//	Code for drawing health, battery,
//	ammo and etc put all into one file.
// 
//	History:
//	1.11.26: Started. Green & P.F HUD
//	is in.
//	1.12.26: Battery drawing works now.
//	1.17.26: Added option for HUD to
//	remove certian elements based on the
//	given "viewsize"
//	1.17.26: Started work on adding the
//	alpha hud.
//	1.18.26: Added Quake hud, ammo &
//	health fades. Finished Alpha hud.
//
//	TODO JAN-18-26 : Make HUD display
//	properly at resolutions below 640x
//	for the lital people that use 1 B.C
//	hardware
// 
//======================================

#include "hud.h"
#include "cl_util.h"

#include "ammo.h"
#include "ammohistory.h"

#include <string.h>
#include <stdio.h>
#include "r_studioint.h"

extern WeaponsResource gWR;
extern int SCR_GetViewSize(void);
extern engine_studio_api_t IEngineStudio;
cvar_t* test_cvar;

//=========================
// Init
//=========================

int CHudGeneral::Init(void)
{
	m_pCvarHudStyle = CVAR_CREATE( "hud_style", "3", FCVAR_ARCHIVE );
	test_cvar = CVAR_CREATE( "test_cvar", "255", 0 );

	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);

	return 1;
}

//=========================
// VidInit
// Various "VidInit" functions calling for the
// precache of HUD sprites.
//=========================

void CHudGeneral::Quake_VidInit(void)
{
	m_hSBar[0] = gHUD.GetSprite( gHUD.GetSpriteIndex( "sbar" ) );
	m_hSBar[1] = gHUD.GetSprite( gHUD.GetSpriteIndex( "ibar" ) );
	m_hArmor	= gHUD.GetSprite( gHUD.GetSpriteIndex( "suit" ) );

	for ( int j = 0; j < 5; j++ )
	{
		m_hFace[j][0] = gHUD.GetSprite( gHUD.GetSpriteIndex(va("face%d", j + 1)) ); // Normal
		m_hFace[j][1] = gHUD.GetSprite( gHUD.GetSpriteIndex(va("face_p%d", j + 1)) ); // Pain
	}
}

void CHudGeneral::Alpha_VidInit(void)
{
	int iAlphaBars[4];
	int iAlphaTint[4];

	// 0 - Bottom left
	// 1 - Bottom Right
	// 2 - Middle Right
	// 3 - Top Left

	iAlphaBars[0]	= gHUD.GetSpriteIndex( "bar_btm_lf" );
	iAlphaBars[1]	= gHUD.GetSpriteIndex( "bar_btm_rt" );
	iAlphaBars[2]	= gHUD.GetSpriteIndex( "bar_mid_rt" );
	iAlphaBars[3]	= gHUD.GetSpriteIndex( "bar_up_lf" );

	iAlphaTint[0]	= gHUD.GetSpriteIndex( "tint_btm_lf" );
	iAlphaTint[1]	= gHUD.GetSpriteIndex( "tint_btm_rt" );
	iAlphaTint[2]	= gHUD.GetSpriteIndex( "tint_mid_rt" );
	iAlphaTint[3]	= gHUD.GetSpriteIndex( "tint_up_lf" );

	m_hAlphaBars[0] = gHUD.GetSprite( iAlphaBars[0] );
	m_hAlphaBars[1] = gHUD.GetSprite( iAlphaBars[1] );
	m_hAlphaBars[2] = gHUD.GetSprite( iAlphaBars[2] );
	m_hAlphaBars[3] = gHUD.GetSprite( iAlphaBars[3] );

	m_hAlphaTints[0]	= gHUD.GetSprite( iAlphaTint[0] );
	m_hAlphaTints[1]	= gHUD.GetSprite( iAlphaTint[1] );
	m_hAlphaTints[2]	= gHUD.GetSprite( iAlphaTint[2] );
	m_hAlphaTints[3]	= gHUD.GetSprite( iAlphaTint[3] );

	wrect_t rcSprite;

	for ( int i = 0; i < 4; i++ )
	{
		rcSprite = gHUD.GetSpriteRect(iAlphaBars[i]);

		m_iAlphaWidth[i] = rcSprite.right - rcSprite.left;
		m_iAlphaHeight[i] = rcSprite.bottom - rcSprite.top;
	}
}

int CHudGeneral::VidInit(void)
{
	// Reset "CShift"
	m_flScreenTint[0] = 0.0f;
	m_flScreenTint[1] = 0.0f;
	m_flScreenTint[2] = 0.0f;
	m_flScreenTint[3] = 0.0f;

	int iIndex_Divider[2];
	int iIndex_EmptyBat[2];
	int iIndex_FullBat[2];
	wrect_t rcSprite;

	// Divider

	iIndex_Divider[0]	= gHUD.GetSpriteIndex( "Hud1_Divider" ); // P.F
	iIndex_Divider[1]	= gHUD.GetSpriteIndex( "Hud2_Divider" ); // Green

	m_hDivider[0]		= gHUD.GetSprite(iIndex_Divider[0]);
	m_hDivider[1]		= gHUD.GetSprite(iIndex_Divider[1]);

	// Battery

	iIndex_EmptyBat[0]	= gHUD.GetSpriteIndex( "Hud1_Battery_Empty" ); // P.F
	iIndex_EmptyBat[1]	= gHUD.GetSpriteIndex( "Hud2_Battery_Empty" ); // Green

	iIndex_FullBat[0]	= gHUD.GetSpriteIndex( "Hud1_Battery_Full" ); // P.F
	iIndex_FullBat[1]	= gHUD.GetSpriteIndex( "Hud2_Battery_Full" ); // Green

	m_hBatEmpty[0]		= gHUD.GetSprite(iIndex_EmptyBat[0]);
	m_hBatEmpty[1]		= gHUD.GetSprite(iIndex_EmptyBat[1]);

	m_hBatFull[0]		= gHUD.GetSprite(iIndex_FullBat[0]);
	m_hBatFull[1]		= gHUD.GetSprite(iIndex_FullBat[1]);

	m_rcBatFull			= &gHUD.GetSpriteRect(iIndex_EmptyBat[0]);
	m_iBatFullWidth		= m_rcBatFull->right - m_rcBatFull->left;

	m_iBatWidth			= SPR_Width(gHUD.GetSprite(iIndex_EmptyBat[0]), 0);
	m_iBatHeight		= SPR_Height(gHUD.GetSprite(iIndex_EmptyBat[0]), 0);

	// Numbers

	m_gHUD_num0[0]		= gHUD.GetSpriteIndex( "num_0" ); // Quake
	m_gHUD_num0[1]		= gHUD.GetSpriteIndex( "Hud1_Num0" ); // Alpha
	m_gHUD_num0[2]		= gHUD.GetSpriteIndex( "Hud1_Num0" ); // P.F
	m_gHUD_num0[3]		= gHUD.GetSpriteIndex( "Hud2_Num0" ); // Green

	m_gHUD_anum0[0]		= gHUD.GetSpriteIndex( "anum_0" ); // Quake
	m_gHUD_anum0[1]		= gHUD.GetSpriteIndex( "Hud1_ANum0" ); // Alpha
	m_gHUD_anum0[2]		= gHUD.GetSpriteIndex( "Hud1_ANum0" ); // P.F
	m_gHUD_anum0[3]		= gHUD.GetSpriteIndex( "Hud2_ANum0" ); // Green

	for ( int i = 0; i < 4; i++ )
	{
		rcSprite = gHUD.GetSpriteRect(m_gHUD_num0[i]);

		m_iNumWidth[i]		= rcSprite.right - rcSprite.left;
		m_iNumHeight[i]		= rcSprite.bottom - rcSprite.top;

		rcSprite = gHUD.GetSpriteRect(m_gHUD_anum0[i]);

		m_iANumWidth[i]		= rcSprite.right - rcSprite.left;
		m_iANumHeight[i]	= rcSprite.bottom - rcSprite.top;
	}

	Quake_VidInit();
	Alpha_VidInit();

	m_iHealthFade = 0;
	m_iAmmoFade = 0;

	return 1;
}

//=========================
// Draw
//
// Draw the appropiate hud based
// on the defined "hud_style"
// 0 - Quake
// 1 - Alpha
// 2 - Prelim
// 3 - Green
//=========================

int CHudGeneral::Draw(float flTime)
{
	if ( gEngfuncs.IsSpectateOnly() )
		return 1;

	switch ((int)m_pCvarHudStyle->value)
	{
		case 0: DrawQuakeHud(); break;
		case 1: DrawAlphaHud(); break;
		case 2: DrawGreenHud(); break;
		case 3: DrawGreenHud(); break;
	}

	DrawCShift();

	return 1;
}

//=========================
// DrawQuakeFace
//=========================

void CHudGeneral::DrawQuakeFace(int x, int y)
{
	int m_iHealth = gHUD.m_Health.m_iHealth;

	if (m_iHealth > 0)
		m_iHealth = 4 - min((m_iHealth / 100.0f) * 4, 4);
	else
		m_iHealth = 0;

	if (m_flFacePainTime >= gHUD.m_flTime)
		SPR_Set(m_hFace[m_iHealth][1], 255, 255, 255);
	else
		SPR_Set(m_hFace[m_iHealth][0], 255, 255, 255);

	SPR_DrawHoles(0, x, y, NULL);
}

//=========================
// DrawQuakeHud
// SERECKY JAN-18-26: What can I say? I LOVE the
// Quake-ish unused Alpha HUD and I figured what
// better than to bring this HUD BACK!!
//=========================

void CHudGeneral::DrawQuakeHud(void)
{
	int iSize = SCR_GetViewSize();
	int x, y;
	int iFlags = DHN_3DIGITS | DHN_HOLES | DHN_NOLZERO | DHN_DRAWZERO;

	if (iSize > 140)
	{
		DrawBasicQuakeHud();
		return;
	}

	if ( iSize < 110 || iSize > 130 )
	{
		if (iSize < 110)
		{
			x = 0;
			y = gHUD.m_iHudScaleHeight - 48;

			if (IEngineStudio.IsHardware())
				FillRGBA(x, y, gHUD.m_iHudScaleWidth, 24, 0, 0, 0, 255, 0);
		}

		x = (gHUD.m_iHudScaleWidth * 0.5f) - 160;
		y = gHUD.m_iHudScaleHeight - 48;

		SPR_Set(m_hSBar[1], 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);
	}

	if ( iSize < 120 || iSize > 120 )
	{
		if (iSize < 120)
		{
			x = 0;
			y = gHUD.m_iHudScaleHeight - 24;

			if (IEngineStudio.IsHardware())
				FillRGBA(x, y, gHUD.m_iHudScaleWidth, 24, 0, 0, 0, 255, 0);
		}

		x = (gHUD.m_iHudScaleWidth * 0.5f) - 160;
		y = gHUD.m_iHudScaleHeight - 24;

		SPR_Set(m_hSBar[0], 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);

		int iHealth = gHUD.m_Health.m_iHealth;
		int iBat = gHUD.m_Health.m_iBat;

		// Battery.

		x = (gHUD.m_iHudScaleWidth * 0.5f) - 160;
		SPR_Set(m_hArmor, 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);

		x = (gHUD.m_iHudScaleWidth * 0.5f) - 160 + 24;
		gHUD.DrawHudNumber(x, y, iFlags, iBat, 255, 255, 255, 255, iBat <= 25 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);

		// Health.

		x = (gHUD.m_iHudScaleWidth * 0.5f) - 160 + 112;
		DrawQuakeFace(x, y);

		x = (gHUD.m_iHudScaleWidth * 0.5f) - 160 + 136;
		gHUD.DrawHudNumber(x, y, iFlags, iHealth, 255, 255, 255, 255, iHealth <= 25 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);

		// Ammo.
		if (gHUD.m_Ammo.m_pWeapon)
		{
			WEAPON *pw = gHUD.m_Ammo.m_pWeapon;

			if ((pw->iAmmoType < 0) && (pw->iAmmo2Type < 0))
			{
				x = (gHUD.m_iHudScaleWidth * 0.5f) - 160 + 248;
				y = gHUD.m_iHudScaleHeight - 24;
				gHUD.DrawHudNumber(x, y, iFlags, 0, 255, 255, 255, 255, m_gHUD_anum0[0]);
				return;
			}

			// Does weapon have any ammo at all?
			if (pw->iAmmoType >= 0)
			{
				if ( pw->hQuakeIcon )
				{
					x = (gHUD.m_iHudScaleWidth * 0.5f) - 160 + 224;
					y = gHUD.m_iHudScaleHeight - 24;
					SPR_Set( pw->hQuakeIcon, 255, 255, 255 );
					SPR_DrawHoles( 0, x, y, NULL );
				}

				if (pw->iClip >= 0)
				{
					int iTextHeight, iTextWidth;

					GetConsoleStringSize("0", &iTextWidth, &iTextHeight);

					x = (gHUD.m_iHudScaleWidth * 0.5f) - 160 + 248;
					y = gHUD.m_iHudScaleHeight - iTextHeight;	
					TRI_DrawConsoleString(x, y, va("%d", gWR.CountAmmo(pw->iAmmoType)), iTextWidth, iTextHeight);

					y = gHUD.m_iHudScaleHeight - 24;
					gHUD.DrawHudNumber(x, y, iFlags, pw->iClip, 255, 255, 255, 255, pw->iClip <= 10 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);
				}
				else
				{
					x = (gHUD.m_iHudScaleWidth * 0.5f) - 160 + 248;
					y = gHUD.m_iHudScaleHeight - 24;
					gHUD.DrawHudNumber(x, y, iFlags, gWR.CountAmmo(pw->iAmmoType), 255, 255, 255, 255, gWR.CountAmmo(pw->iAmmoType) <= 10 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);
				}
			}

			// Does weapon have seconday ammo?
			if (pw->iAmmo2Type > 0) 
			{
				if ((pw->iAmmo2Type != 0) && (gWR.CountAmmo(pw->iAmmo2Type) >= 0) && (iSize < 110 || iSize > 130))
				{
					if ( pw->hQuakeIcon2 )
					{
						x = (gHUD.m_iHudScaleWidth * 0.5f) - 160 + 224;
						y = gHUD.m_iHudScaleHeight - 48;
						SPR_Set( pw->hQuakeIcon2, 255, 255, 255 );
						SPR_DrawHoles( 0, x, y, NULL );
					}

					x = (gHUD.m_iHudScaleWidth * 0.5f) - 160 + 248;
					y = gHUD.m_iHudScaleHeight - 48;
					gHUD.DrawHudNumber(x, y, iFlags, gWR.CountAmmo(pw->iAmmo2Type), 255, 255, 255, 255, gWR.CountAmmo(pw->iAmmoType) <= 10 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);
				}
			}
		}
		else
		{
			x = (gHUD.m_iHudScaleWidth * 0.5f) - 160 + 248;
			y = gHUD.m_iHudScaleHeight - 24;
			gHUD.DrawHudNumber(x, y, iFlags, 0, 255, 255, 255, 255, m_gHUD_anum0[0]);
		}
	}
}

//=========================
// DrawBasicQuakeHud
// SERECKY JAN-18-26: Added the basic modern
// styled Quake HUDS you find in Quake Remastered & 64
// for the people that like it... (me sometimes)
//=========================

void CHudGeneral::DrawBasicQuakeHud(void)
{
	int x, y;
	int iFlags = DHN_3DIGITS | DHN_HOLES | DHN_NOLZERO | DHN_DRAWZERO;

	int iHealth = gHUD.m_Health.m_iHealth;
	int iBat = gHUD.m_Health.m_iBat;

	// Health.

	x = 20;
	y = gHUD.m_iHudScaleHeight - 24 - 8;
	DrawQuakeFace(x, y);

	x += 24;
	gHUD.DrawHudNumber(x, y, iFlags, iHealth, 255, 255, 255, 255, iHealth <= 25 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);

	// Battery.

	x = 20;
	y -= 24 + 2;

	SPR_Set(m_hArmor, 255, 255, 255);
	SPR_DrawHoles(0, x, y, NULL);

	x += 24;
	gHUD.DrawHudNumber(x, y, iFlags, iBat, 255, 255, 255, 255, iBat <= 25 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);

	// Ammo.
	if (gHUD.m_Ammo.m_pWeapon)
	{
		WEAPON *pw = gHUD.m_Ammo.m_pWeapon;

		if ((pw->iAmmoType < 0) && (pw->iAmmo2Type < 0))
			return;

		// Does weapon have any ammo at all?
		if (pw->iAmmoType >= 0)
		{
			x = gHUD.m_iHudScaleWidth - 20 - 24;
			y = gHUD.m_iHudScaleHeight - 24 - 8;

			if ( pw->hQuakeIcon )
			{
				SPR_Set( pw->hQuakeIcon, 255, 255, 255 );
				SPR_DrawHoles( 0, x, y, NULL );
			}

			if (pw->iClip >= 0)
			{
				x -= 24 * 3;
				gHUD.DrawHudNumber(x, y, iFlags, gWR.CountAmmo(pw->iAmmoType), 255, 255, 255, 255, gWR.CountAmmo(pw->iAmmoType) <= 10 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);

				x -= 24 * 4;
				gHUD.DrawHudNumber(x, y, iFlags, pw->iClip, 255, 255, 255, 255, pw->iClip <= 10 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);
			}
			else
			{
				x -= 24 * 3;
				gHUD.DrawHudNumber(x, y, iFlags, gWR.CountAmmo(pw->iAmmoType), 255, 255, 255, 255, gWR.CountAmmo(pw->iAmmoType) <= 10 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);
			}
		}

		// Does weapon have seconday ammo?
		if (pw->iAmmo2Type > 0) 
		{
			if ((pw->iAmmo2Type != 0) && (gWR.CountAmmo(pw->iAmmo2Type) >= 0))
			{
				x = gHUD.m_iHudScaleWidth - 20 - 24;
				y -= 24 + 2;

				if ( pw->hQuakeIcon2 )
				{
					SPR_Set( pw->hQuakeIcon2, 255, 255, 255 );
					SPR_DrawHoles( 0, x, y, NULL );
				}
				
				x -= 24 * 3;
				gHUD.DrawHudNumber(x, y, iFlags, gWR.CountAmmo(pw->iAmmo2Type), 255, 255, 255, 255, gWR.CountAmmo(pw->iAmmoType) <= 10 ? m_gHUD_anum0[0] : m_gHUD_num0[0]);
			}
		}
	}
}

//=========================
// DrawAlphaHud
// SERECKY JAN-18-26: Drawing routines for the alpha
// hud.. A little messy yes, but atleast it works.
// May or may not add numbers to this some day.
//=========================

void CHudGeneral::DrawAlphaHud(void)
{
	int x, y, r, g, b, a, i;
	int iSize = SCR_GetViewSize();
	int iHealth, iBat, iAmmo1, iAmmo2;
	int iHealthMax, iBatMax, iAmmo1Max, iAmmo2Max;
	int iHeight, iSizeHack;

	if (ScreenWidth >= 640)
	{
		iAmmo1Max = 25;
		iAmmo2Max = 15;
		iHealthMax = 15;
		iBatMax = 8;
	}
	else
	{
		iAmmo1Max = 25;
		iAmmo2Max = 8;
		iHealthMax = 8;
		iBatMax = 6;
	}

	// Battery meter.

	if (iSize < 130)
	{
		x = 0;
		y = 0;

		SPR_Set(m_hAlphaBars[3], 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);

		SPR_Set(m_hAlphaTints[3], 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);

		// SERECKY JAN-18-26: Force hud to show 1 meter if we've got
		// less than 10 percent of our armor and it's not zero.

		if ((gHUD.m_Health.m_iBat < 10) && (gHUD.m_Health.m_iBat > 0))
			iBat = 1;
		else
			iBat = ceil((gHUD.m_Health.m_iBat * iBatMax) * 0.01f);

		r = 255; g = 156; b = 39; a = 255;

		x = 20;
		y = 138;

		for ( i = 0; i < iBat; i++ )
		{
			FillRGBA(x, y, 30, 8, r, g, b, a, 0);
			y -= 10;
		}
	}

	// Health meter.

	if (iSize < 150)
	{
		x = 0;
		y = gHUD.m_iHudScaleHeight - m_iAlphaHeight[0];

		SPR_Set(m_hAlphaBars[0], 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);

		SPR_Set(m_hAlphaTints[0], 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);

		if (ScreenWidth >= 640)
		{
			x = 88;
			y += 84;
		}
		else
		{
			x = 44;
			y += 36;
		}

		// SERECKY JAN-18-26: Force hud to show 1 meter if we've got
		// less than 10 percent of our health and we're not dead.

		if ((gHUD.m_Health.m_iHealth < 10) && (gHUD.m_Health.m_iHealth > 0))
			iHealth = 1;
		else
			iHealth = ceil((gHUD.m_Health.m_iHealth * iHealthMax) * 0.01f);

		r = 255; g = 156; b = 39; a = 255;

		for ( i = 0; i < iHealth; i++ )
		{
			FillRGBA(x, y, 8, 8, r, g, b, a, 0);
			x += 10;
		}
	}

	// Ammo meter.

	if (iSize < 140)
	{
		x = gHUD.m_iHudScaleWidth - m_iAlphaWidth[1];
		y = gHUD.m_iHudScaleHeight - m_iAlphaHeight[1];

		SPR_Set(m_hAlphaBars[1], 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);

		SPR_Set(m_hAlphaTints[1], 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);

		x = gHUD.m_iHudScaleWidth - m_iAlphaWidth[2];
		y -= m_iAlphaHeight[2];

		SPR_Set(m_hAlphaBars[2], 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);

		SPR_Set(m_hAlphaTints[2], 255, 255, 255);
		SPR_DrawHoles(0, x, y, NULL);

		if (!gHUD.m_Ammo.m_pWeapon)
			return;

		WEAPON *pw = gHUD.m_Ammo.m_pWeapon;
		r = 255; g = 156; b = 39; a = 255;

		// SERECKY JAN-18-26: Added code for drawing ammo bars, health, and batteries. The code here isn't the
		// most pretty, but it looks good enough and I'd like to move the codebase to the 2.3 SDK soon.

		// Does weapon have any ammo at all?
		if (pw->iAmmoType >= 0)
		{
			if (pw->iClip >= 0)
			{
				// Get height for a single meter.
				iHeight = (pw->iMaxClip > 0) ? ( (( iAmmo1Max * 10 ) / (float)pw->iMaxClip ) - 2 ) : 0;
				
				x = gHUD.m_iHudScaleWidth - 50;
				y = gHUD.m_iHudScaleHeight - 50 - iHeight;

				if (iHeight >= 3)
				{
					for ( i = 0; i < pw->iClip; i++ )
					{
						// SERECKY JAN-18-26: HACK for meters that exceed the
						// max height.

						if ( i == (pw->iMaxClip - 1) )
						{
							iSizeHack = (iHeight + 2) * pw->iMaxClip;
							if (iSizeHack >= (iAmmo1Max * 10))
							{
								iSizeHack -= (iAmmo1Max * 10);
								iHeight -= iSizeHack;
								y += iSizeHack;
							}
							else
							{
								iSizeHack = ((iAmmo1Max * 10) - iSizeHack);
								iHeight += iSizeHack;
								y -= iSizeHack;
							}
						}

						FillRGBA(x, y, 8, iHeight, r, g, b, a, 0);
						y -= iHeight + 2;
					}
				}
				else
				{
					// Get height for a bar meter.
					iHeight = ( (float)pw->iClip / (float)pw->iMaxClip ) * (iAmmo1Max * 10);
					x = gHUD.m_iHudScaleWidth - 50;
					y = gHUD.m_iHudScaleHeight - 50 - iHeight;

					FillRGBA(x, y, 8, iHeight, r, g, b, a, 0);
				}

				// Get height for a bar meter.
				iHeight = ( (float)gWR.CountAmmo(pw->iAmmoType) / (float)pw->iMax1 ) * (iAmmo1Max * 10);
				x += 8 + 4;
				y = gHUD.m_iHudScaleHeight - 50 - iHeight;

				FillRGBA(x, y, 8, iHeight, r, g, b, a, 0);
			}
			else
			{
				// Get height for a single meter.
				iHeight = (pw->iMax1 > 0) ? ( (( iAmmo1Max * 10 ) / (float)pw->iMax1 ) - 2 ) : 0;

				x = gHUD.m_iHudScaleWidth - 50;
				y = gHUD.m_iHudScaleHeight - 50 - iHeight;
				iAmmo1 = gWR.CountAmmo(pw->iAmmoType);

				if (iHeight >= 3)
				{
					for ( i = 0; i < iAmmo1; i++ )
					{
						// SERECKY JAN-18-26: HACK for meters that exceed the
						// max height.

						if ( i == (pw->iMax1 - 1) )
						{
							iSizeHack = (iHeight + 2) * pw->iMax1;
							if (iSizeHack >= (iAmmo1Max * 10))
							{
								iSizeHack -= (iAmmo1Max * 10);
								iHeight -= iSizeHack;
								y += iSizeHack;
							}
							else
							{
								iSizeHack = ((iAmmo1Max * 10) - iSizeHack);
								iHeight += iSizeHack;
								y -= iSizeHack;
							}
						}

						FillRGBA(x, y, 22, iHeight, r, g, b, a, 0);
						y -= iHeight + 2;
					}
				}
				else
				{
					// Get height for a bar meter.
					iHeight = ( (float)iAmmo1 / (float)pw->iMax1 ) * (iAmmo1Max * 10);
					y = gHUD.m_iHudScaleHeight - 50 - iHeight;
					FillRGBA(x, y, 22, iHeight, r, g, b, a, 0);
				}
			}
		}

		// Does weapon have seconday ammo?
		if (pw->iAmmo2Type > 0) 
		{
			if ((pw->iAmmo2Type != 0) && (gWR.CountAmmo(pw->iAmmo2Type) >= 0))
			{
				iHeight = (pw->iMax2 > 0) ? ( (( iAmmo2Max * 10 ) / (float)pw->iMax2 ) - 2 ) : 0;

				x = gHUD.m_iHudScaleWidth - 95;
				y = gHUD.m_iHudScaleHeight - 44;

				iAmmo2 = gWR.CountAmmo(pw->iAmmo2Type);

				for ( i = 0; i < iAmmo2; i++ )
				{
					if ( i == (pw->iMax2 - 1) )
					{
						iSizeHack = (iHeight + 2) * pw->iMax2;
						if (iSizeHack >= (iAmmo2Max * 10))
						{
							iSizeHack -= (iAmmo2Max * 10);
							iHeight -= iSizeHack;
							x += iSizeHack;
						}
						else
						{
							iSizeHack = ((iAmmo2Max * 10) - iSizeHack);
							iHeight += iSizeHack;
							x -= iSizeHack;
						}
					}

					FillRGBA(x, y, iHeight, 8, r, g, b, a, 0);
					x -= iHeight + 2;
				}

			}
		}
	}
}

//=========================
// DrawGreenHud
//=========================

#define MIN_AMMO_ALPHA		240
#define MIN_HEALTH_ALPHA	144

void CHudGeneral::DrawGreenHud(void)
{
	if (!m_pCvarHudStyle)
		return;

	int iIndex_Num = (m_pCvarHudStyle->value == 3) ? 3 : 2;
	int iIndex_DivBat = (m_pCvarHudStyle->value == 3) ? 1 : 0;
	int iFlags = DHN_3DIGITS | DHN_DRAWZERO;
	int x, y, r, g, b, a;
	int iSize = SCR_GetViewSize();

	wrect_t rc;

	rc = *m_rcBatFull;
	rc.right  -= m_iBatWidth * ((float)(100-(min(100,gHUD.m_Health.m_iBat))) * 0.01);	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	// Health display.

	x = 14;
	y = gHUD.m_iHudScaleHeight - m_iNumHeight[iIndex_Num] - 20;
	a = max(m_iHealthFade - (gHUD.m_flTimeDelta * 20), MIN_HEALTH_ALPHA);

	gHUD.m_Health.GetPainColor(r, g, b);

	if (iSize < 150)
		gHUD.DrawHudNumber(x, y, iFlags, gHUD.m_Health.m_iHealth, r, g, b, a, m_gHUD_num0[iIndex_Num]);

	// Battery display.

	if ( (gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT))) && (iSize < 130) )
	{
		y -= m_iBatHeight + 12;

		ScaleColors(r, g, b, a);

		SPR_Set(m_hBatEmpty[iIndex_DivBat], r, g, b);
		SPR_DrawAdditive(0, x, y, NULL);

		if (m_pCvarHudStyle->value == 3)
		{
			x += 5;
			y += 5;
		}
		else
		{
			x += 4;
			y += 4;
		}

		if (rc.right > rc.left)
		{
			SPR_Set(m_hBatFull[iIndex_DivBat], r, g, b);
			SPR_DrawAdditive(0, x, y, &rc);
		}
	}

	// Ammo display.

	if (iSize < 140)
	{
		x = gHUD.m_iHudScaleWidth - (m_iANumWidth[iIndex_Num] * 3) - 16;
		y = gHUD.m_iHudScaleHeight - m_iANumHeight[iIndex_Num] - 33;
		a = max(m_iAmmoFade - (gHUD.m_flTimeDelta * 20), MIN_AMMO_ALPHA);

		if (!gHUD.m_Ammo.m_pWeapon)
		{
			UnpackRGB(r, g, b, RGB_GREENISH);
			gHUD.DrawHudNumber(x, y, iFlags, 255, r, g, b, a, m_gHUD_anum0[iIndex_Num]);
			return;
		}

		WEAPON *pw = gHUD.m_Ammo.m_pWeapon;

		// Draw the infamous "255"
		if ((pw->iAmmoType < 0) && (pw->iAmmo2Type < 0))
		{
			UnpackRGB(r, g, b, RGB_GREENISH);
			gHUD.DrawHudNumber(x, y, iFlags, 255, r, g, b, a, m_gHUD_anum0[iIndex_Num]);
			return;
		}

		// Does weapon have any ammo at all?
		if (pw->iAmmoType >= 0)
		{
			if (pw->iClip >= 0)
			{
				UnpackRGB(r, g, b, RGB_GREENISH);
				gHUD.DrawHudNumber(x, y, iFlags, gWR.CountAmmo(pw->iAmmoType), r, g, b, a, m_gHUD_anum0[iIndex_Num]);

				x -= m_iANumWidth[iIndex_Num];
				
				ScaleColors(r, g, b, a);
				SPR_Set(m_hDivider[iIndex_DivBat], r, g, b);
				SPR_DrawAdditive(0, x, y, NULL);

				x -= m_iANumWidth[iIndex_Num] * 3;

				UnpackRGB(r, g, b, RGB_GREENISH);
				gHUD.DrawHudNumber(x, y, iFlags, pw->iClip, r, g, b, a, m_gHUD_anum0[iIndex_Num]);
			}
			else
			{
				UnpackRGB(r, g, b, RGB_GREENISH);
				gHUD.DrawHudNumber(x, y, iFlags, gWR.CountAmmo(pw->iAmmoType), r, g, b, a, m_gHUD_anum0[iIndex_Num]);
			}
		}

		// Does weapon have seconday ammo?
		if (pw->iAmmo2Type > 0) 
		{
			if ((pw->iAmmo2Type != 0) && (gWR.CountAmmo(pw->iAmmo2Type) >= 0))
			{
				x = gHUD.m_iHudScaleWidth - (m_iANumWidth[iIndex_Num] * 3) - 32;
				y = gHUD.m_iHudScaleHeight - m_iANumHeight[iIndex_Num] - 6;

				UnpackRGB(r, g, b, RGB_GREENISH);
				gHUD.DrawHudNumber(x, y, iFlags, gWR.CountAmmo(pw->iAmmo2Type), r, g, b, a, m_gHUD_anum0[iIndex_Num]);
			}
		}
	}
}

//=========================
// DrawCShift
//=========================

void CHudGeneral::DrawCShift(void)
{
	int r, g, b, a;

	r = (int)m_flScreenTint[0];
	g = (int)m_flScreenTint[1];
	b = (int)m_flScreenTint[2];
	a = (int)m_flScreenTint[3];

	FillRGBA(0, 0, gHUD.m_iHudScaleWidth, gHUD.m_iHudScaleHeight, r, g, b, a);
}