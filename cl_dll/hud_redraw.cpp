/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// hud_redraw.cpp
//
#include <math.h>
#include "hud.h"
#include "cl_util.h"
#include "bench.h"

#include "vgui_TeamFortressViewport.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

#define MAX_LOGO_FRAMES 56

int grgLogoFrame[MAX_LOGO_FRAMES] = 
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 13, 13, 13, 13, 12, 11, 10, 9, 8, 14, 15,
	16, 17, 18, 19, 20, 20, 20, 20, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
	29, 29, 29, 29, 29, 28, 27, 26, 25, 24, 30, 31 
};


extern int g_iVisibleMouse;

float HUD_GetFOV( void );

extern cvar_t *sensitivity;

// Update local version of the ScreenInfo and setup HUD's scale
void CHud::UpdateScreenInfo( void )
{
	// setup screen info
	m_scrinfo.iSize = sizeof( m_scrinfo );
	GetScreenInfo(&m_scrinfo);

	if ( m_pCvarScale->value && IEngineStudio.IsHardware() )
	{
		float xscale;
		if (m_scrinfo.iHeight < 700)
			xscale = 1;
		else if (m_scrinfo.iHeight < 1000)
			xscale = 0.75;
		else if (m_scrinfo.iHeight < 1400)
			xscale = 0.5;
		else
			xscale = 0.3;

		m_iHudScaleWidth  =  m_scrinfo.iWidth * xscale;
		m_iHudScaleHeight = m_scrinfo.iHeight * xscale;
	}
	else
	{
		m_iHudScaleWidth  = m_scrinfo.iWidth;
		m_iHudScaleHeight = m_scrinfo.iHeight;
	}

}

// Think
void CHud::Think(void)
{
//	m_scrinfo.iSize = sizeof(m_scrinfo);
//	GetScreenInfo(&m_scrinfo);

	UpdateScreenInfo();

	int newfov;
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if (pList->p->m_iFlags & HUD_ACTIVE)
			pList->p->Think();
		pList = pList->pNext;
	}

	if ( m_pCvarFovAdjust->value )
	{
		if ( (float)ScreenWidth / (float)ScreenHeight < 1.7 )
			gEngfuncs.Cvar_SetValue("default_fov", 90);
		else
			gEngfuncs.Cvar_SetValue("default_fov", 106);
	}
	
	newfov = HUD_GetFOV();
	if ( newfov == 0 )
	{
		m_iFOV = default_fov->value;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if ( m_iFOV == default_fov->value )
	{  
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{  
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)default_fov->value) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	// think about default fov
	if ( m_iFOV == 0 )
	{  // only let players adjust up in fov,  and only if they are not overriden by something else
		m_iFOV = max( default_fov->value, 90 );  
	}
	
	if ( gEngfuncs.IsSpectateOnly() )
	{
		m_iFOV = gHUD.m_Spectator.GetFOV();	// default_fov->value;
	}

	Bench_CheckStart();
}

// Redraw
// step through the local data,  placing the appropriate graphics & text as appropriate
// returns 1 if they've changed, 0 otherwise
int CHud :: Redraw( float flTime, int intermission )
{
	m_fOldTime = m_flTime;	// save time of previous redraw
	m_flTime = flTime;
	m_flTimeDelta = (double)m_flTime - m_fOldTime;
	static float m_flShotTime = 0;
	
	// Clock was reset, reset delta
	if ( m_flTimeDelta < 0 )
		m_flTimeDelta = 0;

	// Bring up the scoreboard during intermission
	if (gViewPort)
	{
		if ( m_iIntermission && !intermission )
		{
			// Have to do this here so the scoreboard goes away
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideScoreBoard();
			gViewPort->UpdateSpectatorPanel();
		}
		else if ( !m_iIntermission && intermission )
		{
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideVGUIMenu();
			gViewPort->ShowScoreBoard();
			gViewPort->UpdateSpectatorPanel();

			// Take a screenshot if the client's got the cvar set
			if ( CVAR_GET_FLOAT( "hud_takesshots" ) != 0 )
				m_flShotTime = flTime + 1.0;	// Take a screenshot in a second
		}
	}

	if (m_flShotTime && m_flShotTime < flTime)
	{
		gEngfuncs.pfnClientCmd("snapshot\n");
		m_flShotTime = 0;
	}

	m_iIntermission = intermission;

	// if no redrawing is necessary
	// return 0;
	
	// draw all registered HUD elements
	if ( m_pCvarDraw->value )
	{
		HUDLIST *pList = m_pHudList;

		while (pList)
		{
			if ( !Bench_Active() )
			{
				if ( !intermission )
				{
					if ( (pList->p->m_iFlags & HUD_ACTIVE) && !(m_iHideHUDDisplay & HIDEHUD_ALL) )
						pList->p->Draw(flTime);
				}
				else
				{  // it's an intermission,  so only draw hud elements that are set to draw during intermissions
					if ( pList->p->m_iFlags & HUD_INTERMISSION )
						pList->p->Draw( flTime );
				}
			}
			else
			{
				if ( ( pList->p == &m_Benchmark ) &&
					 ( pList->p->m_iFlags & HUD_ACTIVE ) &&
					 !( m_iHideHUDDisplay & HIDEHUD_ALL ) )
				{
					pList->p->Draw(flTime);
				}
			}

			pList = pList->pNext;
		}
	}

	// are we in demo mode? do we need to draw the logo in the top corner?
	if (m_iLogo)
	{
		int x, y, i;

		if (m_hsprLogo == 0)
			m_hsprLogo = LoadSprite("sprites/%d_logo.spr");

		SPR_Set(m_hsprLogo, 250, 250, 250 );
		
		x = SPR_Width(m_hsprLogo, 0);
		x = ScreenWidth - x;
		y = SPR_Height(m_hsprLogo, 0)/2;

		// Draw the logo at 20 fps
		int iFrame = (int)(flTime * 20) % MAX_LOGO_FRAMES;
		i = grgLogoFrame[iFrame] - 1;

		SPR_DrawAdditive(i, x, y, NULL);
	}

	/*
	if ( g_iVisibleMouse )
	{
		void IN_GetMousePos( int *mx, int *my );
		int mx, my;

		IN_GetMousePos( &mx, &my );
		
		if (m_hsprCursor == 0)
		{
			char sz[256];
			sprintf( sz, "sprites/cursor.spr" );
			m_hsprCursor = SPR_Load( sz );
		}

		SPR_Set(m_hsprCursor, 250, 250, 250 );
		
		// Draw the logo at 20 fps
		SPR_DrawAdditive( 0, mx, my, NULL );
	}
	*/

	return 1;
}

void ScaleColors( int &r, int &g, int &b, int a )
{
	float x = (float)a / 255;
	r = (int)(r * x);
	g = (int)(g * x);
	b = (int)(b * x);
}

int CHud :: DrawHudString(int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b )
{
	return xpos + gEngfuncs.pfnDrawString( xpos, ypos, szIt, r, g, b);
}

int CHud :: DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b )
{
	char szString[32];
	sprintf( szString, "%d", iNumber );
	return DrawHudStringReverse( xpos, ypos, iMinX, szString, r, g, b );

}

// draws a string from right to left (right-aligned)
int CHud :: DrawHudStringReverse( int xpos, int ypos, int iMinX, char *szString, int r, int g, int b )
{
	return xpos - gEngfuncs.pfnDrawStringReverse( xpos, ypos, szString, r, g, b);
}

//==========================================
// DrawHudNumber
//
// Modified version of DrawHudNumber that
// supports different fonts and holes
// drawing. - Serecky 1.11.26
//==========================================

int CHud :: DrawHudNumber( int x, int y, int iFlags, int iNumber, int r, int g, int b, int a, int iSprite )
{
	int iWidth = GetSpriteRect(iSprite).right - GetSpriteRect(iSprite).left;
	int iHoles = (iFlags & DHN_HOLES) ? 1 : 0;
	int r1, g1, b1, k;

	r1 = r; g1 = g; b1 = b;
	a = (iHoles) ? 255 : a;

	ScaleColors(r1, g1, b1, (iHoles) ? a : (a * 0.5f));
	ScaleColors(r, g, b, a);

	if (iNumber > 0)
	{
		// SPR_Draw 100's
		if (iNumber >= 100)
		{
			k = iNumber/100;
			SPR_Set(GetSprite(iSprite + k), r, g, b );

			if (iHoles)
				SPR_DrawHoles( 0, x, y, &GetSpriteRect(iSprite + k));
			else
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect(iSprite + k));

			x += iWidth;
		}
		else if ((iFlags & DHN_3DIGITS) && !(iFlags & DHN_NOLZERO))
		{
			SPR_Set(GetSprite(iSprite), r1, g1, b1 );

			if (iHoles)
				SPR_DrawHoles( 0, x, y, &GetSpriteRect(iSprite));
			else
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect(iSprite));

			x += iWidth;
		}

		// SPR_Draw 10's
		if (iNumber >= 10)
		{
			k = (iNumber % 100)/10;
			SPR_Set(GetSprite(iSprite + k), r, g, b );

			if (iHoles)
				SPR_DrawHoles( 0, x, y, &GetSpriteRect(iSprite + k));
			else
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect(iSprite + k));

			x += iWidth;
		}
		else if ((iFlags & (DHN_3DIGITS | DHN_2DIGITS)) && !(iFlags & DHN_NOLZERO))
		{
			SPR_Set(GetSprite(iSprite), r1, g1, b1 );

			if (iHoles)
				SPR_DrawHoles( 0, x, y, &GetSpriteRect(iSprite));
			else
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect(iSprite));

			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set(GetSprite(iSprite + k), r, g, b );

		if (iHoles)
			SPR_DrawHoles( 0, x, y, &GetSpriteRect(iSprite + k));
		else
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(iSprite + k));

		x += iWidth;
	} 
	else if (iFlags & DHN_DRAWZERO) 
	{
		SPR_Set(GetSprite(iSprite), r1, g1, b1 );

		// SPR_Draw 100's
		if ((iFlags & DHN_3DIGITS) && !(iFlags & DHN_NOLZERO))
		{
			if (iHoles)
				SPR_DrawHoles( 0, x, y, &GetSpriteRect(iSprite));
			else
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect(iSprite));

			x += iWidth;
		}

		if ((iFlags & (DHN_3DIGITS | DHN_2DIGITS)) && !(iFlags & DHN_NOLZERO))
		{
			if (iHoles)
				SPR_DrawHoles( 0, x, y, &GetSpriteRect(iSprite));
			else
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect(iSprite));

			x += iWidth;
		}

		// SPR_Draw ones
		if (iHoles)
			SPR_DrawHoles( 0, x, y, &GetSpriteRect(iSprite));
		else
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(iSprite));

		x += iWidth;
	}

	return x;
}

int CHud::GetNumWidth( int iNumber, int iFlags )
{
	return 3;
/*
	if (iFlags & (DHN_3DIGITS))
		return 3;

	if (iFlags & (DHN_2DIGITS))
		return 2;

	if (iNumber <= 0)
	{
		if (iFlags & (DHN_DRAWZERO))
			return 1;
		else
			return 0;
	}

	if (iNumber < 10)
		return 1;

	if (iNumber < 100)
		return 2;

	return 3;
*/
}	


