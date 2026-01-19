/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

class CAirtank : public CBaseMonster
{
	void Spawn( void );
	void Precache( void );
	void EXPORT TankThink( void );
	void EXPORT TankTouch( CBaseEntity *pOther );
	int	 BloodColor( void ) { return DONT_BLEED; };
	void Killed( entvars_t *pevAttacker, int iGib );

	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	int	 m_state;
};


LINK_ENTITY_TO_CLASS( item_airtank, CAirtank );
TYPEDESCRIPTION	CAirtank::m_SaveData[] = 
{
	DEFINE_FIELD( CAirtank, m_state, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CAirtank, CBaseMonster );


void CAirtank :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_oxygen.mdl");
	UTIL_SetSize(pev, Vector( -16, -16, 0), Vector(16, 16, 36));
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CAirtank::TankTouch );
	SetThink( &CAirtank::TankThink );

	pev->takedamage		= DAMAGE_YES;
	pev->health			= 20;
	pev->dmg			= 50;
	m_state				= 1;
}

void CAirtank::Precache( void )
{
	PRECACHE_MODEL("models/w_oxygen.mdl");
	PRECACHE_SOUND("items/airtank1.wav");
}


void CAirtank :: Killed( entvars_t *pevAttacker, int iGib )
{
	Vector vecSpot;
	float flFlags;

	vecSpot = pev->origin;
	vecSpot.z += 16;

	flFlags = TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOPARTICLES;

	PLAYBACK_EVENT_FULL( NULL, NULL, g_sBubbleXplo, 0.0, (float*)&vecSpot, (float*)&g_vecZero, 0.0, 0.0, 48, 0, 0, 0);

	// make an explosion sound
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 0 ); // scale * 10
			WRITE_BYTE( 0  ); // framerate
			WRITE_BYTE( flFlags );
		MESSAGE_END();

	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	RadiusDamage ( pev, pevAttacker, pev->dmg, CLASS_NONE, DMG_BLAST );

	if ( !FStringNull(pev->target) )
	{
		// quick hack to remove the target
		m_iszKillTarget = pev->target;

		SUB_UseTargets( this, USE_TOGGLE, 0 );
	}

	SetThink( &CBaseMonster::SUB_Remove );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;
}


void CAirtank::TankThink( void )
{
	// Fire trigger
	m_state = 1;
	SUB_UseTargets( this, USE_TOGGLE, 0 );
}


void CAirtank::TankTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
		return;

	if (!m_state)
	{
		// "no oxygen" sound
		EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 1.0, ATTN_NORM );
		return;
	}
		
	// give player 12 more seconds of air
	pOther->pev->air_finished = gpGlobals->time + 12;

	// suit recharge sound
//	EMIT_SOUND( ENT(pev), CHAN_VOICE, "doors/aliendoor3.wav", 1.0, ATTN_NORM );
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "items/airtank1.wav", 1.0, ATTN_NORM );

	// recharge airtank in 30 seconds
	pev->nextthink = gpGlobals->time + 10;
	m_state = 0;
	SUB_UseTargets( this, USE_TOGGLE, 1 );
}
