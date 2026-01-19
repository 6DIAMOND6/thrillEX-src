//=========================================================
// Mr. Friendly - lustful bullchik's ripoff
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"
#include	"decals.h"
#include	"game.h"
#include	"weapons.h"
#include	"spit.h"

//=========================================================
// Friendly's spit projectile
//=========================================================
class CFriendlySpit : public CSpit
{
public:
	void EXPORT Animate( void );
	void Touch( CBaseEntity *pOther );
	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Splat( void );
	void Splash( void );

	BOOL m_fDirectHit;
};

LINK_ENTITY_TO_CLASS( friendlyspit, CFriendlySpit );

void CFriendlySpit :: Animate( void )
{
	CBaseEntity *pEntity = NULL;
	CSpit::Animate();

	pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 8 );

	if ( pEntity && pEntity->Classify() )
	{
		Splat();
		Splash();

		m_fDirectHit = FALSE;
		SetThink ( &CFriendlySpit::SUB_Remove );
	}

	pev->nextthink = gpGlobals->time;
}

void CFriendlySpit::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CFriendlySpit *pSpit = GetClassPtr( (CFriendlySpit *)NULL );
	pSpit->Spawn();
	pSpit->pev->classname = MAKE_STRING( "friendlyspit" );
	pSpit->m_fDirectHit = TRUE;

	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles( vecVelocity );

	pSpit->SetThink ( &CFriendlySpit::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CFriendlySpit :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;
	
	Splat();

	if ( !pOther->pev->takedamage )
	{
	//	RadiusDamage( pev->origin, pev, pev, 75, 64.0, CLASS_NONE, DMG_PARALYZE );
		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_YBLOOD2 + RANDOM_LONG(0,1));

	}

	if ( m_fDirectHit )
		Splash();

	SetThink ( &CFriendlySpit::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CFriendlySpit :: Splat ( void )
{
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );	

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}
}

void CFriendlySpit :: Splash ( void )
{
	entvars_t *pevOwner = VARS( pev->owner );

	RadiusDamage( pev->origin, pevOwner, pev, gSkillData.friendlyDmgSpit, 32.0, CLASS_NONE, DMG_PARALYZE | DMG_NEVERGIB );
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		FRIENDLY_AE_SPIT	( 1 )
#define		FRIENDLY_AE_SLASH	( 2 )
#define		FRIENDLY_AE_STOMP	( 3 )

class CFriendly : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	
	void IdleSound ( void );
	void AlertSound ( void );

	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	
	BOOL IsLightDamage( float flDamage ) { return FALSE; } // never stop schedules due to light damage
	Activity GetDeathActivity ( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	Schedule_t *GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );

	BOOL FCanCheckAttacks ( void );
	void StompAttack( void );

	BOOL ShouldThrowCorpse( void ) { return FALSE; }

	CUSTOM_SCHEDULES;
	
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

private:
	float m_flNextSpitTime;
	float m_flNextRumble;
	BOOL m_flStrongPunch;	// if Friendly is hit by something strong, we can play brutal death animation
};
LINK_ENTITY_TO_CLASS( monster_friendly, CFriendly );
LINK_ENTITY_TO_CLASS( monster_mrfriendly, CFriendly );

const char *CFriendly::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CFriendly::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CFriendly :: Classify ( void )
{
	return	CLASS_ALIEN_PREDATOR;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CFriendly :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}

//=========================================================
// Spawn
//=========================================================
void CFriendly :: Spawn()
{
	Precache( );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;

	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextSpitTime = gpGlobals->time;

	SET_MODEL(ENT(pev), "models/friendly.mdl");
//	UTIL_SetSize( pev, Vector( -48, -48, 0 ), Vector( 48, 48, 64 ) );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

//	pev->health	= 600;
	pev->health = gSkillData.friendlyHealth;
	m_flFieldOfView		= 0.2;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFriendly :: Precache()
{
	PRECACHE_MODEL("models/friendly.mdl");
	PRECACHE_MODEL("models/spit.mdl");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");

	PRECACHE_SOUND("friendly/fr_alert1.wav");
	PRECACHE_SOUND("friendly/fr_attack1.wav");
	PRECACHE_SOUND("friendly/fr_idle1.wav");
	PRECACHE_SOUND("friendly/fr_idle2.wav");

	int i;

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);
}

void CFriendly :: IdleSound ( void )
{
	if ( RANDOM_LONG(0,1) )
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "friendly/fr_idle1.wav", 1, ATTN_IDLE );
	else
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "friendly/fr_idle2.wav", 1, ATTN_IDLE );
}

void CFriendly :: AlertSound ( void )
{
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "friendly/fr_alert1.wav", 1, ATTN_NORM );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
//=========================================================
Task_t	tlFriendlyRangeAttack1[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_FAIL	},
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slFriendlyRangeAttack1[] =
{
	{ 
		tlFriendlyRangeAttack1,
		ARRAYSIZE ( tlFriendlyRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|		// always finish the attack so it won't be abused by player
		bits_COND_ENEMY_DEAD,
//		bits_COND_HEAVY_DAMAGE,		
		0,
		"Friendly Range Attack1"
	},
};

//=========================================================
// secondary range attack.
//=========================================================
Task_t	tlFriendlyRangeAttack2[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CHASE_ENEMY	},
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
};

Schedule_t	slFriendlyRangeAttack2[] =
{
	{ 
		tlFriendlyRangeAttack2,
		ARRAYSIZE ( tlFriendlyRangeAttack2 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE,		
		0,
		"Friendly Range Attack2"
	},
};

DEFINE_CUSTOM_SCHEDULES( CFriendly )
{
	slFriendlyRangeAttack1,
	slFriendlyRangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CFriendly, CBaseMonster );

BOOL CFriendly :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 128 && flDot >= 0.7 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CFriendly :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 640 )
	{
		return FALSE;
	}

	if ( flDist > 144 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime )
	{
		if ( m_hEnemy != NULL )
		{
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 512 )
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 1;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 0.5;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL CFriendly :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( m_flNextRumble > gpGlobals->time )
		return FALSE;

	if ( flDist <= 512 )
		return TRUE;

	return FALSE;
}

//=========================================================
// GetDeathActivity - determines the best type of death
// anim to play.
//=========================================================
Activity CFriendly :: GetDeathActivity ( void )
{
	if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
	{
		float		flDot;
		TraceResult	tr;
		Vector		vecSrc;
		
		vecSrc = Center();

		UTIL_MakeVectors ( pev->angles );
		flDot = DotProduct ( gpGlobals->v_forward, g_vecAttackDir * -1 );

		// make sure there's room to fall backward
		UTIL_TraceHull ( vecSrc, vecSrc - gpGlobals->v_forward * 272, dont_ignore_monsters, head_hull, edict(), &tr );
		
		if ( tr.flFraction == 1.0 && flDot <= -0.3 )
		return ACT_DIEBACKWARD;

		ALERT( at_console, "Friendly: Failed to play ACT_DIEBACKWARD, forcing ACT_DIESIMPLE\n" );
	}
	
	return ACT_DIESIMPLE;
}

int CFriendly :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType == DMG_BULLET )
	flDamage *= 0.3;

	if ( bitsDamageType == DMG_CLUB )
	flDamage *= 0.7;

	if ( bitsDamageType == DMG_BLAST )
	flDamage *= 0.6;

	if ( bitsDamageType == DMG_PARALYZE )
	flDamage = 0;

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CFriendly :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case FRIENDLY_AE_SPIT:
		{
			Vector	vecSpitOffset;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			vecSpitOffset = pev->origin + gpGlobals->v_forward * 64.0;
			vecSpitOffset.z += 32.0;

			vecSpitDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - vecSpitOffset);
			float gravity = g_psv_gravity->value;
			float time = vecSpitDir.Length( ) / 2100;
			vecSpitDir = vecSpitDir * (1.0 / time);

			// adjust upward toss to compensate for gravity loss
			vecSpitDir.z += gravity * time * 0.5;
	
			CFriendlySpit::Shoot( pev, vecSpitOffset, vecSpitDir );

			// spew the spittle particle streams.
			Vector vecStreamDir = gpGlobals->v_forward * 400.0 + gpGlobals->v_up * 400.0;
			vecStreamDir.Normalize();

			for ( int i = 0; i < 4; i++ )
			{
				vecStreamDir.x += RANDOM_FLOAT( -0.6, 0.6 );
				vecStreamDir.y += RANDOM_FLOAT( -0.6, 0.6 );
				vecStreamDir.z += RANDOM_FLOAT( -0.6, 0.6 );

				UTIL_BloodStream( vecSpitOffset, vecStreamDir, BLOOD_COLOR_GREEN, 400 );
			}
		}
		break;

		case FRIENDLY_AE_SLASH:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 128, gSkillData.friendlyDmgSlash, DMG_SLASH );
			
			if ( pHurt )
			{
				//pHurt->pev->punchangle.z = -15;
				//pHurt->pev->punchangle.x = -45;
				//pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
				pHurt->pev->velocity = g_vecZero;

				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

		}
		break;

		case FRIENDLY_AE_STOMP:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 128, 5, DMG_SLASH );

			if ( pHurt && pHurt->Classify() )
			{
				if ( pHurt->IsPlayer() && !(pHurt->pev->flags & FL_GODMODE) )	// don't ignore godmode
				{
					pHurt->pev->health = 0;	// !!!
					pHurt->Killed(pev, GIB_NEVER);	// mr. friendly raped its target and it commits suicide.
				}
				// Play a random attack hit sound
			//	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}

			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "friendly/fr_attack1.wav", 1, ATTN_NORM, 0, 100 );
		}
		break;
	}
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule. 
//=========================================================
void CFriendly :: StartTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK2:
		{
			pev->sequence = LookupSequence( "attack" );
			break;
		}
	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

//=========================================================
// RunTask 
//=========================================================
void CFriendly :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK2:
		{
			if ( m_fSequenceFinished )
			{
				StompAttack();
				m_Activity = ACT_RESET;
				TaskComplete();
			}
			break;
		}
	default:
		CBaseMonster::RunTask(pTask);
		break;
	}
}

Schedule_t *CFriendly :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				return CBaseMonster :: GetSchedule();
			}
			
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				AlertSound();
			}

			if ( !HasConditions(bits_COND_SEE_ENEMY) )
			{
				// we can't see the enemy
				if ( !HasConditions(bits_COND_ENEMY_OCCLUDED) )
				{
					// enemy is unseen, but not occluded!
					// turn to face enemy
					return GetScheduleOfType( SCHED_COMBAT_FACE );
				}
				else
				{
					if ( HasConditions(bits_COND_CAN_RANGE_ATTACK2) )
						return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
					// chase!
					return GetScheduleOfType( SCHED_CHASE_ENEMY );
				}
			}
			
			if ( !HasConditions(bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1) )
			{
				if ( HasConditions(bits_COND_CAN_RANGE_ATTACK2) )
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );

				return GetScheduleOfType( SCHED_CHASE_ENEMY );
			}
		}
		break;
	}

	return CBaseMonster::GetSchedule();
}

Schedule_t *CFriendly :: GetScheduleOfType ( int Type )
{
	switch ( Type )
	{
		case SCHED_RANGE_ATTACK1:
		{
			return &slFriendlyRangeAttack1[ 0 ];
		}
		case SCHED_RANGE_ATTACK2:
		{
			return &slFriendlyRangeAttack2[ 0 ];
		}
		case SCHED_CHASE_ENEMY_FAILED:
		{
			return CBaseMonster :: GetScheduleOfType ( SCHED_ALERT_STAND );
		}
		default:
		{
			return CBaseMonster :: GetScheduleOfType ( Type );
		}
	}
}

BOOL CFriendly :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}

	return FALSE;
}

void CFriendly :: StompAttack ( void )
{
	m_flNextRumble = gpGlobals->time + RANDOM_FLOAT ( 5, 15 );
//	::RadiusDamage( pev->origin, pev, pev, 15, 512.0, CLASS_NONE, edict(), DMG_BLAST );
}