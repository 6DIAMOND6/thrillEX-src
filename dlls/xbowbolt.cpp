#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "xbowbolt.h"

LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt );

TYPEDESCRIPTION	CCrossbowBolt::m_SaveData[] = 
{
	DEFINE_FIELD( CCrossbowBolt, m_iBoltType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CCrossbowBolt, CBaseEntity );

CCrossbowBolt *CCrossbowBolt::BoltCreate( int iBoltType )
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = GetClassPtr( (CCrossbowBolt *)NULL );
	pBolt->pev->classname = MAKE_STRING("bolt");
	if ( iBoltType == CROSSBOW_BOLT_STANDARD )
	{
	//	ALERT( at_console, "CCrossbowBolt: Created standard bolt!\n" );
		pBolt->m_iBoltType = CROSSBOW_BOLT_STANDARD;
	}

	if ( iBoltType == CROSSBOW_BOLT_POISONED )
	{
	//	ALERT( at_console, "CCrossbowBolt: Created paralyze bolt!\n" );
		pBolt->m_iBoltType = CROSSBOW_BOLT_POISONED;
	}

	if ( iBoltType == CROSSBOW_BOLT_SPEAR )
	{
	//	ALERT( at_console, "CCrossbowBolt: Created spear!\n" );
		pBolt->m_iBoltType = CROSSBOW_BOLT_SPEAR;
	}

	if ( iBoltType == CROSSBOW_BOLT_BLOATER )
	{
	//	ALERT( at_console, "CCrossbowBolt: Created bloater projectile!\n" );
		pBolt->m_iBoltType = CROSSBOW_BOLT_BLOATER;
	}

	pBolt->Spawn();

	return pBolt;
}

void CCrossbowBolt::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	if ( m_iBoltType == CROSSBOW_BOLT_SPEAR )
		SET_MODEL(ENT(pev), "models/spear.mdl");
	else if ( m_iBoltType == CROSSBOW_BOLT_BLOATER )
		SET_MODEL(ENT(pev), "models/spearsmall.mdl");
	else
		SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch( &CCrossbowBolt::BoltTouch );
	SetThink( &CCrossbowBolt::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.2;
}


void CCrossbowBolt::Precache( )
{
	PRECACHE_MODEL ("models/crossbow_bolt.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("weapons/xbow_hit2.wav");
	PRECACHE_SOUND("fvox/beep.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
	PRECACHE_MODEL ("models/spear.mdl");
	PRECACHE_MODEL ("models/spearsmall.mdl");
}


int	CCrossbowBolt :: Classify ( void )
{
	return	CLASS_NONE;
}

void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );
/*
		if ( pOther->IsPlayer() )
		{
			switch ( m_iBoltType )
			{
				case CROSSBOW_BOLT_STANDARD:
				default:
				pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowMonster, pev->velocity.Normalize(), &tr, DMG_NEVERGIB | DMG_NEVERGIB );
				break;
				case CROSSBOW_BOLT_POISONED:
			//	ALERT( at_console, "CCrossbowBolt: Paralyzed someone!\n" );
				pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowMonster, pev->velocity.Normalize(), &tr, DMG_PARALYZE | DMG_NEVERGIB );
			}
		}
		else
		{	*/
			switch ( m_iBoltType )
			{
				case CROSSBOW_BOLT_STANDARD:
				default:
				pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowMonster, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB ); 
				break;
				case CROSSBOW_BOLT_POISONED:
				ALERT( at_console, "CCrossbowBolt: Paralyzed someone!\n" );
				pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowMonster, pev->velocity.Normalize(), &tr, DMG_PARALYZE | DMG_NEVERGIB );
				break;
				case CROSSBOW_BOLT_BLOATER:
				pOther->TraceAttack(pevOwner, gSkillData.bloaterDmgBolt, pev->velocity.Normalize(), &tr, DMG_PARALYZE | DMG_NEVERGIB );				
			}
//		}

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = Vector( 0, 0, 0 );
		// play body "thwack" sound
		switch( RANDOM_LONG(0,1) )
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM); break;
		}

		if ( !g_pGameRules->IsMultiplayer() )
		{
			Killed( pev, GIB_NEVER );
		}
	}
	else
	{
		switch( RANDOM_LONG(0,1) )
		{
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,7)); break;
		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/xbow_hit2.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,7)); break;
		}
		SetThink( &CCrossbowBolt::SUB_Remove );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if ( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize( );
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0,360);
			pev->nextthink = gpGlobals->time + 10.0;
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			Vector myPos;

			// this sprite is noticeably off-center, so tune it a bit
			myPos = pev->origin - gpGlobals->v_forward * 10;
			myPos = myPos + gpGlobals->v_right * 6;
			myPos = myPos - gpGlobals->v_up * 4;

			UTIL_Spark( myPos );
		}
	}

	if ( g_pGameRules->IsMultiplayer() )
	{
		SetThink( &CCrossbowBolt::ExplodeThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CCrossbowBolt::BubbleThink( void )
{
//	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
	{
		if ( m_iBoltType != CROSSBOW_BOLT_SPEAR && m_iBoltType != CROSSBOW_BOLT_BLOATER )
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );

				WRITE_BYTE( TE_BEAMFOLLOW );
				WRITE_SHORT(entindex());	// entity
				WRITE_SHORT(m_iTrail );	// model
				WRITE_BYTE( 10 ); // life
				WRITE_BYTE( 2.15 );  // width
				WRITE_BYTE( 245 );   // r, g, b
				WRITE_BYTE( 245 );   // r, g, b
				WRITE_BYTE( 118 );   // r, g, b
				WRITE_BYTE( 128 );	// brightness

			MESSAGE_END();

			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_fly1.wav", .5, ATTN_NORM);
		}

	}
	else UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );	
}

void CCrossbowBolt::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;
	
	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION);		
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		if (iContents != CONTENTS_WATER)
			WRITE_BYTE( iScale ); // scale * 10
		else
			WRITE_BYTE( 0 );	// no sprite
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	if (iContents == CONTENTS_WATER)
		PLAYBACK_EVENT_FULL( NULL, NULL, g_sBubbleXplo, 0.0, (float*)&pev->origin, (float*)&g_vecZero, 0.0, 0.0, 16, 0, 0, 0);

	entvars_t *pevOwner;

	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );

	UTIL_Remove(this);
}