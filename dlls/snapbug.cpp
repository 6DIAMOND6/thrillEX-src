//=========================================================
// snapbug.cpp - an alien insect
//=========================================================

// UNDONE: Clean up magic numbers!!!
// UNDONE: Is it necessary to store both the entity pointer and edict at the same time?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"
#include	"player.h"

#define		SF_SNAPBUG_TRAPMODE				1024
#define		SNAPBUG_PITHEIGHT				16
#define		SNAPBUG_SEARCH_RADIUS			40
#define		SNAPBUG_MAX_DIST_TO_TARGET		36

class CSnapBug : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );

	Vector Center( void ){ return Vector( pev->origin.x, pev->origin.y, pev->origin.z + 6 ); }
	Vector BodyTarget( const Vector &posSrc ){ return Center( ); }
	void PainSound( void ){ EMIT_SOUND_DYN( edict(), CHAN_VOICE, "snapbug/sb_pain1.wav", 1.0, ATTN_IDLE, 0, 100 ); }
//	void DeathSound( void ){ EMIT_SOUND_DYN( edict(), CHAN_VOICE, "snapbug/sb_die1.wav", 1.0, ATTN_IDLE, 0, 100 ); }
	void EXPORT SnapBugThink( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	void StartTask( void );
	int  Classify ( void );
	int  IRelationship( CBaseEntity *pTarget );

	int MainGib() { return MAINGIB_ALIEN; }
	BOOL HasHumanGibs() { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void EXPORT SnapBugSubterrThink( void );
	void FindNearestPrey( void );
	void ReleaseTarget( void );
	void CatchTarget( void );

	virtual float GetDamageAmount( void ) { return gSkillData.snapbugDmgBite; }
	Schedule_t	*GetSchedule( void );
//	Schedule_t  *GetScheduleOfType ( int Type );
	BOOL FCanCheckAttacks ( void ) { return FALSE; }

	int	Save( CSave &save ); 
	virtual int Restore( CRestore &restore );

	//CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fCatchPlayer;
	BOOL m_fCaughtPrey;
	float m_flNextCapture;
	Vector m_vecOldOrigin;

	CBaseEntity *pTouchEnt;
	edict_t *m_pVictimEdict;

	BOOL m_fTrapActivated;
};

LINK_ENTITY_TO_CLASS( monster_snapbug, CSnapBug );

TYPEDESCRIPTION	CSnapBug::m_SaveData[] = 
{
	DEFINE_FIELD( CSnapBug, m_flNextCapture, FIELD_TIME ),
	DEFINE_FIELD( CSnapBug, m_fCatchPlayer, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSnapBug, m_fCaughtPrey, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSnapBug, m_pVictimEdict, FIELD_EDICT ),
	DEFINE_FIELD( CSnapBug, m_fTrapActivated, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSnapBug, m_vecOldOrigin, FIELD_VECTOR ),
};

//IMPLEMENT_SAVERESTORE( CSnapBug, CBaseMonster );

//=========================================================
// Spawn
//=========================================================
void CSnapBug :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/snapbug.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 8));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.snapbugHealth;

	// leave this like that; after SetEyePosition fix these values made snapbug visible to everyone when he was hiding underground
	// pev->view_ofs		= Vector ( 0, 0, 20 );
	m_flFieldOfView		= 0.5;
	m_MonsterState		= MONSTERSTATE_NONE;
	pev->yaw_speed		= 60;

	MonsterInit();

	SetActivity ( ACT_IDLE );

	pev->nextthink = gpGlobals->time + 0.5;

	UTIL_SetOrigin ( pev, pev->origin );

	SetTouch ( &CSnapBug::LeapTouch );

	if ( FBitSet( pev->spawnflags, SF_SNAPBUG_TRAPMODE ) )
	{
		m_vecOldOrigin = pev->origin;
		pev->origin.z -= SNAPBUG_PITHEIGHT;
		SetThink ( &CSnapBug::SnapBugSubterrThink );
		pev->movetype		= MOVETYPE_NOCLIP;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSnapBug :: Classify ( void )
{
	return	CLASS_INSECT;
}

int CSnapBug::IRelationship( CBaseEntity *pTarget )
{
	if ( (pTarget->IsPlayer()) )
		if ( m_afMemory & bits_MEMORY_PROVOKED )
			return R_HT;
		else
			return R_NO;
	switch (pTarget->Classify())
	{
	case CLASS_HUMAN_PASSIVE:
	case CLASS_PLAYER_ALLY:
	case CLASS_INSECT:
	case CLASS_NONE:
	case CLASS_BARNACLE:
		return R_NO;
	default:
		return R_HT;
	}
	return CBaseMonster::IRelationship( pTarget );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSnapBug :: Precache()
{
//	PRECACHE_SOUND("snapbug/sb_die1.wav");

	PRECACHE_SOUND("snapbug/catch.wav");
	PRECACHE_MODEL("models/snapbug.mdl");

	PRECACHE_SOUND("snapbug/sb_pain1.wav");
	PRECACHE_SOUND("snapbug/sb_dig1.wav");
}	

void CSnapBug :: LeapTouch ( CBaseEntity *pOther )
{
	// Hits when on ground
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		return;
	}

	if ( m_Activity == ACT_WALK || m_Activity == ACT_RUN )
	{
		return;
	}

	if ( !pOther->pev->takedamage )
	{
		return;
	}

	// don't bite the target that is below you
	if ( pOther->pev->origin.z < pev->origin.z )
	{
		return;
	}

	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "snapbug/catch.wav", 1, ATTN_NORM );

	// Show some interest to the target
	MakeIdealYaw(pOther->pev->origin);
	ChangeYaw(pev->yaw_speed);

	SetThink ( &CSnapBug::SnapBugThink );
	m_fCaughtPrey = TRUE;

	if ( pOther->IsPlayer() )
	{
		TraceResult tr;
		Vector vecEnd;

		if (g_iSkillLevel != SKILL_EASY)
			pOther->TakeDamage( pev, pev, GetDamageAmount(), DMG_SLASH );

		vecEnd = pOther->pev->origin;
		UTIL_TraceHull(pev->origin, vecEnd, dont_ignore_monsters, human_hull, edict(), &tr);
		if ( tr.flFraction != 1.0 )
		{
			ALERT( at_aiconsole, "CSnapbug: Can't capture target as it will get stuck!\n" );
			m_flNextCapture = gpGlobals->time + 1.0;
			SetTouch( NULL );
			return;
		}

		m_flNextCapture = gpGlobals->time + 10.0;

		pOther->pev->velocity.x *= 0.0;
		pOther->pev->velocity.y *= 0.0;

		pTouchEnt = pOther;
		m_pVictimEdict = pOther->edict();
		m_fCatchPlayer = TRUE;
		SetTouch( NULL );

		CatchTarget();

		return;
	}

	if ( FClassnameIs(pOther->pev, "monster_headcrab") || FClassnameIs(pOther->pev, "monster_chumtoad") || FClassnameIs(pOther->pev, "monster_cockroach") )
	{
//		ALERT( at_aiconsole, "Gibbed the crab!\n" );
		pOther->Killed( pev, GIB_ALWAYS );
		m_flNextCapture = gpGlobals->time + 1.0;
		SetTouch( NULL );

		return;
	}

	pOther->TakeDamage( pev, pev, GetDamageAmount(), DMG_SLASH );
	m_flNextCapture = gpGlobals->time + 1.0;
	SetTouch( NULL );
}

void CSnapBug :: ReleaseTarget( void )
{
	pTouchEnt->pev->flags &= ~FL_IMMUNE_LAVA;

	pTouchEnt = NULL;
	m_fCatchPlayer = FALSE;
	m_pVictimEdict = NULL;

	ALERT( at_aiconsole, "CSnapBug: Released the target!\n" );
}

void CSnapBug :: CatchTarget( void )
{
	// NOTE: this is a hack! 
	// Physinfo was annoying to deal with and caused anomalous behaviour, 
	// so it was ditched in favor of this quake leftover flag
	pTouchEnt->pev->flags |= FL_IMMUNE_LAVA;											
}

//=========================================================
// SnapBugThink - snapbug prevents its target from moving.
// In this function we assume that our target is player.
//=========================================================
void CSnapBug :: SnapBugThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.0;

	if ( m_fCaughtPrey )
	{
		 if ( gpGlobals->time >= m_flNextCapture )
		 {
			m_fCaughtPrey = FALSE;
			SetTouch ( &CSnapBug::LeapTouch );
		 }
	}

	if ( !m_fCatchPlayer )
	{
		MonsterThink();
		return;
	}

	if ( pTouchEnt->pev->movetype == MOVETYPE_NOCLIP )
	{
		ReleaseTarget();
		m_flNextCapture = gpGlobals->time + 1.5;
		return;
	}

	Vector vecDistToTarget = pTouchEnt->pev->origin - pev->origin;
	vecDistToTarget.z = 0;

	// Do a check if we are too far from the target. If so, then leave it alone.
	if ( pTouchEnt->pev->origin.z <= pev->origin.z || (pTouchEnt->pev->origin.z - VEC_HUMAN_HULL_MAX.z > pev->origin.z) || vecDistToTarget.Length() > SNAPBUG_MAX_DIST_TO_TARGET )
	{
		ALERT( at_aiconsole, "CSnapBug: Target's too far!\n" );
		ReleaseTarget();
		m_flNextCapture = gpGlobals->time + 1.5;
		return;
	}

	if ( gpGlobals->time < m_flNextCapture - 1.5 )
	{
	//	ALERT( at_aiconsole, "Here's my dinner!\n" );
		
		// FIXME: sometimes it leads to target stucking in bsp geometry
		// ... or does it? turning this off doesn't fix the issue
		// if player clips through the bug, they wont be able to walk away after the block will end.
		// to avoid this, we will push them out of the snapbug's bounding box.
		if (pTouchEnt->pev->origin.z - pev->origin.z < 80 || ( FBitSet(pTouchEnt->pev->flags,FL_DUCKING) && pTouchEnt->pev->origin.z - pev->origin.z < 44 ) )
			pTouchEnt->pev->origin.z += 1.0;
	}
	else
	{
		ReleaseTarget();
	}
	
}

int CSnapBug :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	m_afMemory |= bits_MEMORY_PROVOKED;

	if ( m_fCatchPlayer )
	{
		ReleaseTarget();
	}

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SnapBugSubterrThink - smoothly move buggie out of the pit
//=========================================================
void CSnapBug :: SnapBugSubterrThink ( void )
{
	if ( !m_fTrapActivated ) FindNearestPrey();

	if ( pev->origin.z <= m_vecOldOrigin.z && m_fTrapActivated )	pev->origin.z += .75;		// .5 -> .75

	if ( pev->origin.z >= m_vecOldOrigin.z )
	{
		SetThink ( &CSnapBug::SnapBugThink );
		m_fTrapActivated = FALSE;
		pev->movetype = MOVETYPE_STEP;
		SetTouch ( &CSnapBug::LeapTouch );
	}

	if ( !m_fTrapActivated )
		pev->nextthink = gpGlobals->time + .1;
	else
		pev->nextthink = gpGlobals->time;
}

void CSnapBug :: FindNearestPrey( void )
{
	CBaseEntity *pEntity = NULL;

	pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 40.0 );

	if ( pEntity == NULL || !pEntity->Classify() || !pEntity->IsAlive() )	// only react to live entities
		return;

	if ( FClassnameIs ( pEntity->pev, "monster_snapbug" ) )
		return;
		
	m_fTrapActivated = TRUE;
	ALERT( at_aiconsole, "SnapTrap: Found the target!\n" );
	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "snapbug/sb_dig1.wav", 1, ATTN_NORM );
	SetTouch ( &CSnapBug::LeapTouch );

	ClearBits( pev->spawnflags, SF_SNAPBUG_TRAPMODE );
}

Schedule_t *CSnapBug :: GetSchedule ( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		// dead enemy
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster :: GetSchedule();
		}

		if ( HasConditions ( bits_COND_HEAR_SOUND ) )
		{
			return GetScheduleOfType( SCHED_COMBAT_FACE );
		}

		if ( HasConditions(bits_COND_NEW_ENEMY) )
		{
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		else
		if ( HasConditions(bits_COND_LIGHT_DAMAGE) || HasConditions(bits_COND_HEAVY_DAMAGE) )
		{
			ALERT( at_aiconsole, "CSnapBug: Take cover!\n" );
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ORIGIN );	
		}
		else if ( !HasConditions ( bits_COND_SEE_FEAR ) && !HasConditions ( bits_COND_SEE_ENEMY ) )
		{
		//	ALERT( at_aiconsole, "CSnapBug: Face!\n" );
			return GetScheduleOfType ( SCHED_COMBAT_FACE );
		}
		else
		{
			ALERT( at_aiconsole, "CSnapBug: Take cover from enemy!\n" );
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}				
		break;
	}	

	return CBaseMonster :: GetSchedule();
}


int CSnapBug::Save( CSave &save )
{
	if ( !CBaseMonster::Save(save) )
		return 0;

	return save.WriteFields( "SNAPBUG", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}

int CSnapBug::Restore( CRestore &restore )
{
	if ( !CBaseMonster::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "SNAPBUG", this, m_SaveData, ARRAYSIZE(m_SaveData) );

	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;

	// a way to prevent snapbug from disappearing on certain maps
	if ( FBitSet( pev->spawnflags, SF_SNAPBUG_TRAPMODE ) && !m_fTrapActivated )
	{
		ALERT( at_aiconsole, "SnapBug: Restored origin!\n" );
		pev->origin = m_vecOldOrigin;
		pev->origin.z -= SNAPBUG_PITHEIGHT;
	}

	// find player by saved edict
	if (m_fCatchPlayer)
	{
		if (m_pVictimEdict)
		{
			pTouchEnt = Instance(m_pVictimEdict);
			CatchTarget();	// lock player once again
		}
	}

	return status;
}