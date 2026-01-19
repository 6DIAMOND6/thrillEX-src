//=========================================================
// chumtoad.cpp - jumpy alien frog
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"
#include	"weapons.h"
#include	"player.h"
#include	"soundent.h"
#include	"gamerules.h"
#include	"chumtoad.h"

#define		CHUB_CROAK_RANGE		384

#define		CHUB_EYE_OPEN			0
#define		CHUB_EYE_SEMI			1
#define		CHUB_EYE_CLOSED			2

// Chub Toad monster begins here

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_CHUB_PLAYDEAD = LAST_COMMON_SCHEDULE + 1,
	SCHED_CHUB_STOP_PLAYDEAD,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_CHUB_IDLE = LAST_COMMON_TASK + 1,
	TASK_CHUB_START_PLAYDEAD,
	TASK_CHUB_PLAYDEAD,
	TASK_CHUB_STOP_PLAYDEAD,
};

//=========================================================
//	Idle Schedules
//=========================================================
Task_t	tlChubIdleStand1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_CHUB_IDLE,			0				},
	{ TASK_WAIT,				(float)5		},// repick IDLESTAND every five seconds. gives us a chance to pick an active idle, fidget, etc.
};

Schedule_t	slChubIdleStand[] =
{
	{ 
		tlChubIdleStand1,
		ARRAYSIZE ( tlChubIdleStand1 ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_SEE_FEAR		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL_FOOD	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags
		bits_SOUND_WORLD		|
		bits_SOUND_PLAYER		|
		bits_SOUND_DANGER		|

		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

Task_t	tlChubStand[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_STAND			},
	{ TASK_CHUB_STOP_PLAYDEAD,		(float)0					},
};

Schedule_t	slChubStand[] =
{
	{ 
		tlChubStand,
		ARRAYSIZE ( tlChubStand ), 
		bits_COND_NEW_ENEMY,
		0,
		"Chub Fear"
	},
};

Task_t	tlChubPlayDead[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_CHUB_START_PLAYDEAD,		(float)0					},
	{ TASK_CHUB_PLAYDEAD,			(float)0					},
	{ TASK_CHUB_STOP_PLAYDEAD,		(float)0					},
};

Schedule_t	slChubPlayDead[] =
{
	{ 
		tlChubPlayDead,
		ARRAYSIZE ( tlChubPlayDead ), 
		0,
		0,
		"Chub Playdead"
	},
};

Task_t	tlChubStopPlayDead[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_CHUB_STOP_PLAYDEAD,		(float)0					},
};

Schedule_t	slChubStopPlayDead[] =
{
	{ 
		tlChubStopPlayDead,
		ARRAYSIZE ( tlChubStopPlayDead ), 
		0,
		0,
		"Chub Stop Playdead"
	},
};

DEFINE_CUSTOM_SCHEDULES( CChub )
{
	slChubIdleStand,
	slChubStand,
	slChubPlayDead,
	slChubStopPlayDead,
};

IMPLEMENT_CUSTOM_SCHEDULES( CChub, CBaseMonster );

LINK_ENTITY_TO_CLASS( monster_chumtoad, CChub );
LINK_ENTITY_TO_CLASS( monster_chubtoad, CChub );	// both spellings are right

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CChub :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_STAND:
	case ACT_TWITCH:
	case ACT_FEAR_DISPLAY:
		ys = 0;
		break;
	case ACT_SWIM:
		ys = 150;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

void CChub :: Spawn (void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/chumtoad.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));	

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_TOSS;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.chubtoadHealth;
	pev->view_ofs		= Vector ( 0, 0, 16 );
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

//	SetThink (&CChub::AmphibiousThink);
	SetUse (&CChub::ChubUse);
}

void CChub :: Precache (void)
{
	PRECACHE_SOUND("chub/frog.wav");
	PRECACHE_MODEL("models/chumtoad.mdl");
}

int	CChub :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
//	return	CLASS_ALIEN_PREY;
}

int CChub::IRelationship( CBaseEntity *pTarget )
{
	if ( pTarget->IsPlayer() && HasMemory(bits_MEMORY_PROVOKED) )
		return R_FR;
	return CBaseMonster::IRelationship( pTarget );
}

void CChub :: IdleSound (void)
{
}

//=========================================================
// StartTask
//=========================================================
void CChub :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_SET_ACTIVITY:
	{
		Activity activity = (Activity)(int)pTask->flData;

		if ( activity == ACT_IDLE )
		{
			if ( !pev->waterlevel )
			{
				activity = ACT_IDLE;
			}
			else
			{
				activity = ACT_SWIM;
			}
		}

		m_IdealActivity = activity;
		TaskComplete();
	}
	break;
	case TASK_CHUB_IDLE:
	{
		if ( m_fPlayingDead )
		{
			m_IdealActivity = ACT_TWITCH;
		}
		else if ( !pev->waterlevel )
		{
			m_IdealActivity = ACT_IDLE;
			pev->movetype = MOVETYPE_STEP;
		//	ALERT( at_console, "Chub: Not in water!\n");
		}
		else
		{
			m_IdealActivity = ACT_SWIM;
		//	pev->movetype = MOVETYPE_FLY;
		}
		TaskComplete();
	}
	break;
	case TASK_CHUB_START_PLAYDEAD:
		{
			ALERT( at_console, "Chub: Playing dead!\n" );
			m_fPlayingDead = TRUE;
			pev->skin = CHUB_EYE_CLOSED;
			m_IdealActivity = ACT_FEAR_DISPLAY;
			break;
		}
	case TASK_CHUB_PLAYDEAD:
		{
			m_IdealActivity = ACT_TWITCH;
			break;
		}
	case TASK_CHUB_STOP_PLAYDEAD:
		{
			ALERT( at_console, "Chub: Stopped playdead!\n" );
			Forget ( bits_MEMORY_INCOVER );
			m_fPlayingDead = FALSE; 
			pev->deadflag = DEAD_NO;
			m_flPlayDeadTime = NULL;
			TaskComplete();
			break;
		}
	default:
		CBaseMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CChub :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CHUB_START_PLAYDEAD:
		{
			if (m_fSequenceFinished)
			{
				pev->deadflag = DEAD_DEAD;
				m_flPlayDeadTime = gpGlobals->time + RANDOM_FLOAT(5,10);
			//	ALERT( at_console, "Chub: Finished playdead2!\n");
				TaskComplete();
			}
			break;
		}
	case TASK_CHUB_PLAYDEAD:
		{
			if ( m_flPlayDeadTime < gpGlobals->time )
			{
				TaskComplete();	
			}
			break;
		}
	case TASK_DIE:
		{
			if ( m_fSequenceFinished || pev->frame >= 255 )
			{	
				pev->skin = CHUB_EYE_CLOSED;
				SetUse(NULL);
				CBaseMonster :: RunTask( pTask );
			}
		}
	default:
		{
			CBaseMonster :: RunTask( pTask );
			break;
		}
	}
}

void EXPORT CChub :: ChubUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// if it's not a player, ignore
	if ( !pActivator->IsPlayer() )
		return;

	if ( HasMemory(bits_MEMORY_PROVOKED))
		return;	// im not gonna cooperate with you!

	// do not revive dead toads!!!
	if ( pev->deadflag != DEAD_NO && !m_fPlayingDead )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pActivator;

	// can I have this?
	if ( !g_pGameRules->CanHaveAmmo( pPlayer, "Chubtoads", CHUB_MAX_CARRY ) )
	{
		return;
	}

	pPlayer->GiveNamedItem( "weapon_chub" );
	EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if ( pOwner != NULL )	// tell the monstermaker that this thing is gone
	{
		pOwner->DeathNotice( pev );
	}

	UTIL_Remove(this);
}

// overriden for Chubbie. see Classify() for why
void CChub :: GibMonster( void )
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);
	CGib::SpawnRandomGibs( pev, 2, MAINGIB_ALIEN );
	SetThink ( &CBaseMonster::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

int CChub :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	int iTakeDamage;

	if ( bitsDamageType & ( DMG_RADIATION ) ) flDamage = 0;

	if ( flDamage > 0 )
	{
		Forget(bits_MEMORY_INCOVER);
		if ( m_flPlayDeadTime )
			m_flPlayDeadTime = NULL;
	}

	if ( pevInflictor && pevInflictor->flags & FL_CLIENT )
	{
		Remember( bits_MEMORY_PROVOKED );
	}

	// HACK: change deadflag so we can pass IsAlive check
	if ( m_fPlayingDead )
		pev->deadflag = DEAD_NO;

	iTakeDamage = CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );

	if ( m_fPlayingDead )
		pev->deadflag = DEAD_DEAD;

	return iTakeDamage;
}

//=========================================================
// GetScheduleOfType - returns a pointer to one of the 
// monster's available schedules of the indicated type.
//=========================================================
Schedule_t* CChub :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_IDLE_STAND:
		{
			return &slChubIdleStand[ 0 ];
		}
	case SCHED_CHUB_PLAYDEAD:
		{
			return &slChubPlayDead[ 0 ];
		}
	case SCHED_CHUB_STOP_PLAYDEAD:
		{
			return &slChubStopPlayDead[ 0 ];
		}
	default:
		{
			return CBaseMonster :: GetScheduleOfType ( Type );
		}
	}
}

Schedule_t *CChub :: GetSchedule ( void )
{
	switch( m_MonsterState )
	{
		case MONSTERSTATE_IDLE:
		{
			// hop around randomly
			if ( RANDOM_LONG(0,99) <= 50 )
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ORIGIN);

			return CBaseMonster::GetSchedule();
		break;
		}
		case MONSTERSTATE_COMBAT:
		{
		//	ALERT( at_console, "Chub: MONSTERSTATE_COMBAT!\n" );
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				if ( m_fPlayingDead ) 
					return GetScheduleOfType( SCHED_CHUB_STOP_PLAYDEAD );

				return CBaseMonster::GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType ( SCHED_WAKE_ANGRY );
			}

			if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE) )
			{
				if ( !m_fPlayingDead )
				{
					return CBaseMonster::GetSchedule();
				}
				else return GetScheduleOfType( SCHED_CHUB_STOP_PLAYDEAD );
			}

			if ( HasMemory(bits_MEMORY_INCOVER) )
			{
				if ( !m_fPlayingDead )
				{
					return GetScheduleOfType( SCHED_CHUB_PLAYDEAD );
				}
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				if ( !m_fPlayingDead )
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}

				return GetScheduleOfType( SCHED_IDLE_STAND );
			}
			else
			{
				return GetScheduleOfType( SCHED_COMBAT_FACE );
			}
		break;
		}
		default:
		{
			return CBaseMonster::GetSchedule();
		}
	}
}

#define	CHUB_CHECK_DIST		72
#define CHUB_SWIM_SPEED		50
#define CHUB_SWIM_ACCEL		90
#define CHUB_TURN_RATE		70

// UNDONE: implement proper obstacle avoidance behavior
void CChub :: Swim ( void )
{
	TraceResult		tr;
	Vector			vecTest;
	BOOL			fStuck = FALSE;

	if ( m_Activity != ACT_SWIM )
		SetActivity(ACT_SWIM);
	
	Vector tmp = pev->angles;
	tmp.x = -tmp.x;
	UTIL_MakeVectors ( tmp );

	vecTest = pev->origin + gpGlobals->v_forward * RANDOM_LONG(CHUB_CHECK_DIST/2, CHUB_CHECK_DIST);

	UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
	UTIL_DrawBeam(pev->origin, tr.vecEndPos, 16, 128, 16);
	UTIL_DrawBeam(pev->origin, vecTest, 128, 0, 0);

	if ( tr.flFraction <= 0.97 )
		fStuck = TRUE;

	TRACE_MONSTER_HULL(edict(), tr.vecEndPos, tr.vecEndPos, missile, edict(), &tr);

	if ( tr.fStartSolid )
		fStuck = TRUE;

	if ( !fStuck )
	{
	//	ALERT(at_console, "VICTORY %f\n", tr.flFraction);
		pev->speed = UTIL_Approach( CHUB_SWIM_SPEED, pev->speed, CHUB_SWIM_ACCEL * .1 );
		pev->velocity = gpGlobals->v_forward * pev->speed;
		pev->avelocity = g_vecZero;
		pev->angles.y += 2.5 * RANDOM_LONG(-1, 1);

	//	UTIL_ParticleEffect(pev->origin, Vector(0,0,0), 224, 16);

		pev->nextthink = gpGlobals->time + .1;
	}
	else
	{
	//	ALERT(at_console, "%f\n", tr.flFraction);

		pev->velocity = g_vecZero;
		pev->angles.y += 5;

		pev->nextthink = gpGlobals->time + .05;
	}
}

void CChub :: MonsterThink ( void )
{
	if ((pev->movetype == MOVETYPE_TOSS) && (pev->flags & FL_ONGROUND))
		pev->movetype = MOVETYPE_STEP;

	if (m_flBlink < gpGlobals->time && (!m_fPlayingDead || m_fInWater))
	{
		if ( pev->skin < CHUB_EYE_CLOSED )
		{
			pev->skin++;
			m_flBlink = gpGlobals->time + 0.15;
		}
		else
		{
			pev->skin = CHUB_EYE_OPEN;
			m_flBlink = gpGlobals->time + RANDOM_FLOAT(2,5);
		}
	}

	if ( m_fInWater )
	{
		if ( pev->health <= 0 )
		{
		//	ALERT(at_console, "dead\n");
			SetActivity(ACT_DIESIMPLE);
			UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

			SetThink(&CChub::WaterDeathThink);
			pev->nextthink = gpGlobals->time + .1;
			pev->gravity = 0.1;
		}
		else if ( pev->waterlevel != 3 )
		{
			pev->deadflag = DEAD_NO;
			pev->movetype = MOVETYPE_STEP;
			pev->velocity = pev->avelocity = g_vecZero;
			m_fInWater = m_fPlayingDead = FALSE;
		}
		else Swim(); //	just dumbly swim around
		
		return;
	}
	else if ( pev->waterlevel == 3 )
	{
		pev->deadflag = DEAD_DEAD;	// enemies ignore chub when it swims
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->avelocity = g_vecZero;
		m_fInWater = m_fPlayingDead = TRUE;
	}

	if (!m_flNextCroak)
		m_flNextCroak = gpGlobals->time + RANDOM_FLOAT(1.5, 5.0);
	else if (gpGlobals->time >= m_flNextCroak)
	{
		BOOL fShouldCroak = !m_fPlayingDead && !pev->waterlevel;

		if ( fShouldCroak )
		{
			EMIT_SOUND_DYN( edict(), CHAN_VOICE, "chub/frog.wav", 1.0, ATTN_NORM, 0, RANDOM_LONG(90,110) );
			// TEMP soundent
			CSoundEnt::InsertSound ( bits_SOUND_PLAYER, pev->origin, CHUB_CROAK_RANGE, 0.5 );
		}

		m_flNextCroak = NULL;
	}

	CBaseMonster::MonsterThink();
}

void CChub :: WaterDeathThink( void )
{
	if ( pev->skin < CHUB_EYE_CLOSED )
		pev->skin++;

	if ( pev->waterlevel == 3 )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.5;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 4;
	}
	else pev->movetype = MOVETYPE_BOUNCE;

	pev->nextthink = gpGlobals->time + .1;
}

// Chub Toad monster ends here
