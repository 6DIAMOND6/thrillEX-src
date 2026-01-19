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
// Health.cpp
//
// implementation of CHudHealth class
//

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>

#include "screenfade.h"
#include "shake.h"

extern cvar_t	v_kicktime;
extern cvar_t	v_kickroll;
extern cvar_t	v_kickpitch;

extern cshift_t	cshifts[];
extern float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

DECLARE_MESSAGE(m_Health, Health )
DECLARE_MESSAGE(m_Health, Battery)
DECLARE_MESSAGE(m_Health, Damage )

#define PAIN_NAME "sprites/%d_pain.spr"
#define DAMAGE_NAME "sprites/%d_dmg.spr"

int giDmgHeight, giDmgWidth;

int giDmgFlags[NUM_DMG_TYPES] = 
{
	DMG_POISON,
	DMG_ACID,
	DMG_FREEZE|DMG_SLOWFREEZE,
	DMG_DROWN,
	DMG_BURN|DMG_SLOWBURN,
	DMG_NERVEGAS, 
	DMG_RADIATION,
	DMG_SHOCK,
	DMG_CALTROP,
	DMG_TRANQ,
	DMG_CONCUSS,
	DMG_HALLUC
};

int CHudHealth::Init(void)
{
	HOOK_MESSAGE(Health);
	HOOK_MESSAGE(Battery);
	HOOK_MESSAGE(Damage);
	m_iHealth = 100;
	m_fFade = 0;
	m_iBat = 0;
	m_iFlags = 0;
	m_bitsDamage = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;
	m_pCvarPainFade = CVAR_CREATE("hud_dmgfade", "0", FCVAR_ARCHIVE );

	memset(m_dmg, 0, sizeof(DAMAGE_IMAGE) * NUM_DMG_TYPES);


	gHUD.AddHudElem(this);
	return 1;
}

void CHudHealth::Reset( void )
{
	// make sure the pain compass is cleared when the player respawns
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;


	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	for ( int i = 0; i < NUM_DMG_TYPES; i++ )
	{
		m_dmg[i].fExpire = 0;
	}
}

int CHudHealth::VidInit(void)
{
	m_hSprite = 0;

	m_HUD_dmg_bio = gHUD.GetSpriteIndex( "dmg_bio" ) + 1;

	giDmgHeight = gHUD.GetSpriteRect(m_HUD_dmg_bio).right - gHUD.GetSpriteRect(m_HUD_dmg_bio).left;
	giDmgWidth = gHUD.GetSpriteRect(m_HUD_dmg_bio).bottom - gHUD.GetSpriteRect(m_HUD_dmg_bio).top;
	
	int HUD_suit_empty = gHUD.GetSpriteIndex( "battery_empty" );
	int HUD_suit_full = gHUD.GetSpriteIndex( "battery_full" );

	m_hSprite1 = gHUD.GetSprite(HUD_suit_empty);
	m_hSprite2 = gHUD.GetSprite(HUD_suit_full);
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_iHeight = m_prc2->right - m_prc2->left;
	m_fFade = 0;
	return 1;
}

int CHudHealth:: MsgFunc_Health(const char *pszName,  int iSize, void *pbuf )
{
	// TODO: update local health data
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if (x != m_iHealth)
	{
		gHUD.m_GeneralHud.m_iHealthFade = 200;
		m_iHealth = x;
	}

	return 1;
}

int CHudHealth:: MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;
	
	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

	if ( x != m_iBat )
	{
	//	m_fFade = FADE_TIME;
		m_iBat = x;
	}

	return 1;
}


int CHudHealth:: MsgFunc_Damage(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int		armor, blood;
	Vector	from;
	int		i;
	float	count;
	long	bitsDamage;

	BEGIN_READ(pbuf, iSize);
	armor = READ_BYTE();
	blood = READ_BYTE();
	bitsDamage = READ_LONG();

	for (i = 0; i < 3; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5f) + (armor * 0.5f);

	if (count < 10)
		count = 10;

	cl_entity_t *ent = gEngfuncs.GetViewModel();

	if (ent)
	{
		vec3_t forward, right, up;
		float side;

		VectorSubtract(from, ent->origin, from);
		VectorNormalize(from);

		AngleVectors(ent->angles, forward, right, up);

		side = DotProduct(from, right);
		v_dmg_roll = count * side * v_kickroll.value;

		side = DotProduct(from, forward);
		v_dmg_pitch = count * side * v_kickpitch.value;

		if ( blood > 0 || armor > 0 )
			v_dmg_time = v_kicktime.value;
	}

	UpdateTiles(gHUD.m_flTime, bitsDamage);

	// Actually took damage?

	if ( blood > 0 || armor > 0 )
	{
		CalcDamageDirection(from);
		
		// SERECKY JAN-18-26: NEW!!!
		gHUD.m_GeneralHud.m_flFacePainTime = gHUD.m_flTime + 0.2f;

		if ( (m_pCvarPainFade->value != 0) )
		{
			cshifts[CSHIFT_DAMAGE].percent += 3 * count;
			if (cshifts[CSHIFT_DAMAGE].percent < 0)
				cshifts[CSHIFT_DAMAGE].percent = 0;
			if (cshifts[CSHIFT_DAMAGE].percent > 150)
				cshifts[CSHIFT_DAMAGE].percent = 150;

			if (armor > blood)
			{
				cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
				cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
				cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
			}
			else if (armor)
			{
				cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
				cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
				cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
			}
			else
			{
				cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
				cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
				cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
			}
		}
		else
		{
			cshifts[CSHIFT_DAMAGE].percent = 0;
		}
	}

	return 1;
}


// Returns back a color from the
// Green <-> Yellow <-> Red ramp
void CHudHealth::GetPainColor( int &r, int &g, int &b )
{
	int iHealth = m_iHealth;

	if (iHealth > 25)
		iHealth -= 25;
	else if ( iHealth < 0 )
		iHealth = 0;

	if (m_iHealth > 25)
	{
		g = iHealth * 255 / 100;
		r = 255 - g;
		b = 0;
	}
	else
	{
		r = 250;
		g = 0;
		b = 0;
	}
}

int CHudHealth::Draw(float flTime)
{
	if ( (gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) || gEngfuncs.IsSpectateOnly() )
		return 1;

	if ( !m_hSprite )
		m_hSprite = LoadSprite(PAIN_NAME);

	DrawDamage(flTime);

	if ( m_iHealth <= 0 )
	{
		if (m_fFade < 150)
			m_fFade += 5;

		FillRGBA(0, 0, gHUD.m_iHudScaleWidth, gHUD.m_iHudScaleHeight, 225, 0, 0, m_fFade);
	}
	else
	{
		return DrawPain(flTime);
	}
	
	return 1;
}

void CHudHealth::CalcDamageDirection(vec3_t vecFrom)
{
	vec3_t	forward, right, up;
	float	side, front;
	vec3_t vecOrigin, vecAngles;

	if (!vecFrom[0] && !vecFrom[1] && !vecFrom[2])
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
		return;
	}


	memcpy(vecOrigin, gHUD.m_vecOrigin, sizeof(vec3_t));
	memcpy(vecAngles, gHUD.m_vecAngles, sizeof(vec3_t));


	VectorSubtract (vecFrom, vecOrigin, vecFrom);

	float flDistToTarget = vecFrom.Length();

	vecFrom = vecFrom.Normalize();
	AngleVectors (vecAngles, forward, right, up);

	front = DotProduct (vecFrom, right);
	side = DotProduct (vecFrom, forward);

	if (flDistToTarget <= 50)
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
	}
	else 
	{
		if (side > 0)
		{
			if (side > 0.3)
				m_fAttackFront = max(m_fAttackFront, side);
		}
		else
		{
			float f = fabs(side);
			if (f > 0.3)
				m_fAttackRear = max(m_fAttackRear, f);
		}

		if (front > 0)
		{
			if (front > 0.3)
				m_fAttackRight = max(m_fAttackRight, front);
		}
		else
		{
			float f = fabs(front);
			if (f > 0.3)
				m_fAttackLeft = max(m_fAttackLeft, f);
		}
	}
}

int CHudHealth::DrawPain(float flTime)
{
	int r, g, b;
	int x, y, a, shade;

	if (m_fFade > 0 )
	{
		a = m_fFade;
		m_fFade -= (gHUD.m_flTimeDelta * 100);

		r = 225;
		g = 0;
		b = 0;
		FillRGBA(0, 0, gHUD.m_iHudScaleWidth, gHUD.m_iHudScaleHeight, r, g, b, a);
	}

	if (!(m_fAttackFront || m_fAttackRear || m_fAttackLeft || m_fAttackRight))
		return 1;

	// TODO:  get the shift value of the health
	a = 255;	// max brightness until then

	float fFade = gHUD.m_flTimeDelta * 2;
	
	// SPR_Draw top
	if (m_fAttackFront > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackFront, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = gHUD.m_iHudScaleWidth/2 - SPR_Width(m_hSprite, 0)/2;
		y = gHUD.m_iHudScaleHeight/2 - SPR_Height(m_hSprite,0) * 3;
		SPR_DrawAdditive(0, x, y, NULL);
		m_fAttackFront = max( 0, m_fAttackFront - fFade );
	} else
		m_fAttackFront = 0;

	if (m_fAttackRight > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackRight, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = gHUD.m_iHudScaleWidth/2 + SPR_Width(m_hSprite, 1) * 2;
		y = gHUD.m_iHudScaleHeight/2 - SPR_Height(m_hSprite,1)/2;
		SPR_DrawAdditive(1, x, y, NULL);
		m_fAttackRight = max( 0, m_fAttackRight - fFade );
	} else
		m_fAttackRight = 0;

	if (m_fAttackRear > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackRear, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = gHUD.m_iHudScaleWidth/2 - SPR_Width(m_hSprite, 2)/2;
		y = gHUD.m_iHudScaleHeight/2 + SPR_Height(m_hSprite,2) * 2;
		SPR_DrawAdditive(2, x, y, NULL);
		m_fAttackRear = max( 0, m_fAttackRear - fFade );
	} else
		m_fAttackRear = 0;

	if (m_fAttackLeft > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackLeft, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = gHUD.m_iHudScaleWidth/2 - SPR_Width(m_hSprite, 3) * 3;
		y = gHUD.m_iHudScaleHeight/2 - SPR_Height(m_hSprite,3)/2;
		SPR_DrawAdditive(3, x, y, NULL);

		m_fAttackLeft = max( 0, m_fAttackLeft - fFade );
	} else
		m_fAttackLeft = 0;

	return 1;
}

int CHudHealth::DrawDamage(float flTime)
{
	int r, g, b, a;
	DAMAGE_IMAGE *pdmg;

	if (!m_bitsDamage)
		return 1;

//	UnpackRGB(r,g,b, RGB_GREENISH);
	GetPainColor(r,g,b);
	
	a = (int)( fabs(sin(flTime*2)) * 256.0);

	ScaleColors(r, g, b, a);

	// Draw all the items
	int i;
	for ( i = 0; i < NUM_DMG_TYPES; i++)
	{
		if (m_bitsDamage & giDmgFlags[i])
		{
			pdmg = &m_dmg[i];
			SPR_Set(gHUD.GetSprite(m_HUD_dmg_bio + i), r, g, b );
			SPR_DrawAdditive(0, pdmg->x, pdmg->y - giDmgHeight/2, &gHUD.GetSpriteRect(m_HUD_dmg_bio + i));
		}
	}


	// check for bits that should be expired
	for ( i = 0; i < NUM_DMG_TYPES; i++ )
	{
		DAMAGE_IMAGE *pdmg = &m_dmg[i];

		if ( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg->fExpire = min( flTime + DMG_IMAGE_LIFE, pdmg->fExpire );

			if ( pdmg->fExpire <= flTime		// when the time has expired
				&& a < 40 )						// and the flash is at the low point of the cycle
			{
				pdmg->fExpire = 0;

				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;

				// move everyone above down
				for (int j = 0; j < NUM_DMG_TYPES; j++)
				{
					pdmg = &m_dmg[j];
					if ((pdmg->y) && (pdmg->y < y))
						pdmg->y += giDmgHeight;

				}

				m_bitsDamage &= ~giDmgFlags[i];  // clear the bits
			}
		}
	}

	return 1;
}
 

void CHudHealth::UpdateTiles(float flTime, long bitsDamage)
{	
	DAMAGE_IMAGE *pdmg;

	// Which types are new?
	long bitsOn = ~m_bitsDamage & bitsDamage;
	
	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		pdmg = &m_dmg[i];

		// Is this one already on?
		if (m_bitsDamage & giDmgFlags[i])
		{
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE; // extend the duration
			if (!pdmg->fBaseline)
				pdmg->fBaseline = flTime;
		}

		// Are we just turning it on?
		if (bitsOn & giDmgFlags[i])
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth/8;
			pdmg->y = gHUD.m_iHudScaleHeight - giDmgHeight * 2;
			pdmg->fExpire=flTime + DMG_IMAGE_LIFE;
			
			// move everyone else up
			for (int j = 0; j < NUM_DMG_TYPES; j++)
			{
				if (j == i)
					continue;

				pdmg = &m_dmg[j];
				if (pdmg->y)
					pdmg->y -= giDmgHeight;

			}
			pdmg = &m_dmg[i];
		}	
	}	

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bitsDamage;
}
