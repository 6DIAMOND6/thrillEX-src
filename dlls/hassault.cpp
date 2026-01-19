//=========================================================
// Human sergeant
//=========================================================

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"

// UNDONE: chase after supressing, wait before chase

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HASSAULT_AE_SHOOT		( 1 )
#define		HASSAULT_AE_GUN_PUNCH	( 10 )
#define		HASSAULT_AE_KICK		( 11 )
#define		HASSAULT_AE_ARM_PUNCH	( 12 )
#define		HASSAULT_AE_STEP		( 13 )

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SPINUP = LAST_COMMON_SCHEDULE + 1,
	SCHED_HASSAULT_SHOOT,
	SCHED_SPINDOWN,
	SCHED_HASSAULT_SUPPRESS,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_SPINDOWN = LAST_COMMON_TASK + 1,
	TASK_HASSAULT_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_HASSAULT_NOFIRE	( bits_COND_SPECIAL1 )

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	HASSAULT_NOT_SPINNING				0
#define	HASSAULT_SPINUP_BEGAN				1
#define	HASSAULT_SPINNING					2
#define HASSAULT_SHOOT_AT_EYEPOS			1
#define HASSAULT_SHOOT_AT_CENTER			2
#define HASSAULT_SHOOT_AT_FEET				3

class CHassault : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void MonsterInit ( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	
	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );
	void AlertSound ( void );
	Vector GetGunPosition( void );
	void Shoot ( void );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );

	Schedule_t *GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );

	void EXPORT MonsterThink ( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );

	void Killed( entvars_t *pevAttacker, int iGib );
//	BOOL HasGruntGibs( void ){return TRUE;}
	BOOL HasHumanGibs( void ){return FALSE;}
	int SpecialGib( void ) { return SPECIALGIB_HGRUNT; }
	void SpinDown( void );
	void MeleeAttack( float flDamage );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	BOOL FVisible ( CBaseEntity *pEntity );
	Vector ShootAtEnemy( const Vector &shootOrigin );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

private:
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	int		m_iBrassShell;
	int		m_iSentence;
	int		m_iSpinState;
	int		m_iShootTarget;

	float m_flSpinupDelay;
	float m_flWaitForEnemy;

	static const char *pAttackMissSounds[];
	static const char *pHassaultStepSounds[];
};

LINK_ENTITY_TO_CLASS( monster_human_assault, CHassault );

TYPEDESCRIPTION	CHassault::m_SaveData[] = 
{
	DEFINE_FIELD( CHassault, m_iSpinState, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHassault, CSquadMonster );

const char *CHassault::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CHassault::pHassaultStepSounds[] =
{
	"hgrunt/gr_step1.wav",
	"hgrunt/gr_step2.wav",
	"hgrunt/gr_step3.wav",
	"hgrunt/gr_step4.wav",
};

void CHassault :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/hassault.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.hassaultHealth;
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_MonsterState		= MONSTERSTATE_COMBAT;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_DOORS_GROUP;

	CTalkMonster::g_talkWaitTime = 0;

	m_iSpinState = HASSAULT_NOT_SPINNING;

	m_flSpinupDelay = 0.0;

	MonsterInit();
}

void CHassault :: Precache()
{
	PRECACHE_MODEL("models/hassault.mdl");

	PRECACHE_SOUND( "hassault/hw_gun4.wav" );
	PRECACHE_SOUND( "hassault/hw_spin.wav" );
	PRECACHE_SOUND( "hassault/hw_spinup.wav" );
	PRECACHE_SOUND( "hassault/hw_spindown.wav" );
	PRECACHE_SOUND( "hassault/hw_alert.wav" );

	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );

	PRECACHE_SOUND( "hgrunt/gr_pain1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain5.wav" );

	for ( int i = 0; i < ARRAYSIZE( pHassaultStepSounds ); i++ )
		PRECACHE_SOUND((char *)pHassaultStepSounds[i]);

	PRECACHE_SOUND( "hw_gun3.wav" );

	PRECACHE_SOUND( "player/hoot1.wav" );
	PRECACHE_SOUND( "player/hoot2.wav" );
	PRECACHE_SOUND( "player/hoot3.wav" );

	PRECACHE_SOUND( "hgrunt/makesquad.wav" );

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event
}

//=========================================================
// MonsterInit - after a monster is spawned, it needs to 
// be dropped into the world, checked for mobility problems,
// and put on the proper path, if any. This function does
// all of those things after the monster spawns. Any
// initialization that should take place for all monsters
// goes here.
//=========================================================
void CHassault :: MonsterInit ( void )
{
	CBaseMonster :: MonsterInit ( );
	m_flDistTooFar		= 2048.0;
}

void CHassault :: IdleSound( void )
{
	SENTENCEG_PlayRndSz(ENT(pev), "HG_RADIO", 0.35, ATTN_NORM, 0, 100);
}

//=========================================================
// PainSound
//=========================================================
void CHassault :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain4.wav", 1, ATTN_NORM );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain5.wav", 1, ATTN_NORM );	
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain1.wav", 1, ATTN_NORM );	
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain2.wav", 1, ATTN_NORM );	
			break;
		}
		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CHassault :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// AlertSound 
//=========================================================
void CHassault :: AlertSound ( void )
{
	if ( IsLeader() && RANDOM_LONG(0,1) )
	{
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/makesquad.wav", 1, ATTN_IDLE );	
	}
	else EMIT_SOUND( ENT(pev), CHAN_VOICE, "hassault/hw_alert.wav", 1, ATTN_IDLE );	
}

int	CHassault :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

void CHassault :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:	
		ys = 150;		
		break;
	case ACT_TURN_LEFT:
		 ys = 180;
		 break;
	case ACT_TURN_RIGHT:	
		ys = 180;
		break;
	case ACT_WALK:	
		ys = 180;		
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 120;	
		break;
	default:
		ys = 150;
		break;
	}
	pev->yaw_speed = ys;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CHassault :: GetGunPosition( )
{
	return pev->origin + Vector( 0, 0, 48 );
}

//=========================================================
// Shoot
//=========================================================
void CHassault :: Shoot ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir;

	vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecSpread;
	switch (g_iSkillLevel)
	{
	case SKILL_EASY:
		vecSpread = VECTOR_CONE_7DEGREES;
		break;
	case SKILL_MEDIUM:
		vecSpread = VECTOR_CONE_6DEGREES;
		break;
	case SKILL_HARD:
		vecSpread = VECTOR_CONE_4DEGREES;
		break;
	}

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 
	FireBullets(1, vecShootOrigin, vecShootDir, vecSpread, 4096, BULLET_MONSTER_12MM, gSkillData.hassaultDmgCannon ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

//	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hw_gun3.wav", 1, ATTN_NORM );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHassault :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case HASSAULT_AE_SHOOT:
		{
			Shoot();
		//	EMIT_SOUND ( ENT(pev), CHAN_ITEM, "hassault/hw_spin.wav", 1, ATTN_NORM );
		}
		break;
		case HASSAULT_AE_GUN_PUNCH:
		{
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/hoot2.wav", 1, ATTN_NORM );
			MeleeAttack(15);
		}
		break;
		case HASSAULT_AE_KICK:
		{
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/hoot1.wav", 1, ATTN_NORM );
			MeleeAttack(10);
		}
		break;
		case HASSAULT_AE_ARM_PUNCH:
		{
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/hoot3.wav", 1, ATTN_NORM );
			MeleeAttack(5);
		}
		break;
		case HASSAULT_AE_STEP:
		{
			EMIT_SOUND_DYN ( ENT(pev), CHAN_ITEM, pHassaultStepSounds[ RANDOM_LONG(0,ARRAYSIZE(pHassaultStepSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
		}
		break;
		default:
		CSquadMonster::HandleAnimEvent( pEvent );
		break;
	}

}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// HassaultSpinup
//=========================================================
Task_t	tlHassaultSpinup[] =
{
	{ TASK_FACE_ENEMY,			(float)0		},
//	{ TASK_STOP_MOVING,			0				},
	{ TASK_WAIT,					(float)0.9	},
};

Schedule_t	slHassaultSpinup[] =
{
	{
		tlHassaultSpinup,
		ARRAYSIZE ( tlHassaultSpinup ),
		bits_COND_HEAVY_DAMAGE,
		0,
		"Sergeant Spinup"
	},
};

//=========================================================
// HassaultFail
//=========================================================
Task_t	tlHassaultFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slHassaultFail[] =
{
	{
		tlHassaultFail,
		ARRAYSIZE ( tlHassaultFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Hassault Fail"
	},
};

//=========================================================
// HassaultSpindown
//=========================================================
Task_t	tlHassaultSpindown[] =
{
	{ TASK_SPINDOWN,			0				},
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_WAIT,					(float)0.75	},
};

Schedule_t	slHassaultSpindown[] =
{
	{
		tlHassaultSpindown,
		ARRAYSIZE ( tlHassaultSpindown ),
		bits_COND_HEAVY_DAMAGE,
		0,
		"Hassault Spindown"
	},
};

// Chase enemy schedule
Task_t tlHassaultChase[] = 
{
	{ TASK_SPINDOWN,			0				},
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CHASE_ENEMY_FAILED	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0		},
	{ TASK_RUN_PATH,			(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
};

Schedule_t slHassaultChase[] =
{
	{ 
		tlHassaultChase,
		ARRAYSIZE ( tlHassaultChase ),
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_ENEMY			|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_TASK_FAILED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Hassault Chase Enemy"
	},
};

// primary melee attack
Task_t	tlHassaultMelee[] =
{
	{ TASK_SPINDOWN,			0				},
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slHassaultMelee[] =
{
	{ 
		tlHassaultMelee,
		ARRAYSIZE ( tlHassaultMelee ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Hassault Melee Attack"
	},
};

// primary range attack
Task_t	tlHassaultSuppress[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slHassaultSuppress[] =
{
	{ 
		tlHassaultSuppress,
		ARRAYSIZE ( tlHassaultSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Hassault Suppress"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
//=========================================================
Task_t	tlHassaultRangeAttack1[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TAKE_COVER_FROM_ENEMY	},
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
//	{ TASK_HASSAULT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slHassaultRangeAttack1[] =
{
	{ 
		tlHassaultRangeAttack1,
		ARRAYSIZE ( tlHassaultRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_HASSAULT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( CHassault )
{
	slHassaultSpinup,
	slHassaultFail,
	slHassaultSpindown,
	slHassaultChase,
	slHassaultMelee,
	slHassaultSuppress,
	slHassaultRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHassault, CSquadMonster );

//=========================================================
// StartTask
//=========================================================
void CHassault :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_HASSAULT_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_HASSAULT_NOFIRE );
			if ( m_iSpinState > HASSAULT_NOT_SPINNING )
				SpinDown();
		}
		TaskComplete();
		break;
	case TASK_RANGE_ATTACK1_NOTURN:
	case TASK_RANGE_ATTACK1:
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_gun4.wav", 1, ATTN_NORM, 0, 100);
			if ( m_Activity != ACT_RANGE_ATTACK1 )
				m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
	case TASK_SPINDOWN:
		{
			if ( m_iSpinState > HASSAULT_NOT_SPINNING )
				SpinDown();

			TaskComplete();
			break;
		}
	default: 
	CSquadMonster :: StartTask( pTask );
	break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CHassault :: RunTask ( Task_t *pTask )
{

	switch ( pTask->iTask )
	{
/*	case TASK_SPINDOWN:
	{
		TaskComplete();
		break;
	}	*/
	case TASK_WAIT_FOR_MOVEMENT:
		{
			if (MovementIsComplete() || HasConditions(bits_COND_CAN_RANGE_ATTACK1))
			{
				TaskComplete();
				RouteClear();		// Stop moving
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}

}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CHassault :: GetSchedule( void )
{
// TODO: Add a delay before sergeant will start chasing player so player wont abuse it too much

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if ( m_iSpinState > HASSAULT_SPINUP_BEGAN )
			return GetScheduleOfType ( SCHED_SPINDOWN );
		else return CSquadMonster :: GetSchedule();
		break;

	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				if ( m_iSpinState > HASSAULT_SPINUP_BEGAN )
					SpinDown();
				return CSquadMonster::GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
			/*	if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1) )
				{
					return GetScheduleOfType ( SCHED_HASSAULT_SUPPRESS );
				}
				else
				{
					return GetScheduleOfType ( SCHED_CHASE_ENEMY );
				}*/
				return GetScheduleOfType ( SCHED_WAKE_ANGRY );
			}
			else if (HasConditions(bits_COND_LIGHT_DAMAGE) && !HasMemory( bits_MEMORY_FLINCHED) )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}
			else if (HasConditions(bits_COND_HEAVY_DAMAGE) && !HasMemory( bits_MEMORY_FLINCHED) )
			{
				SpinDown();
				return GetScheduleOfType( SCHED_BIG_FLINCH );
			}
			else if ( !HasConditions(bits_COND_SEE_ENEMY) )
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
					if ( m_iSpinState > HASSAULT_SPINUP_BEGAN && m_flWaitForEnemy < gpGlobals->time )
					{
					//	if ( RANDOM_LONG (1,3) > 1 )
					//	{
							ALERT ( at_aiconsole, "HSarge: Supressing!\n");
							m_flWaitForEnemy = gpGlobals->time + 8;
							return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
					//	}
					}

					// chase!
					return GetScheduleOfType( SCHED_CHASE_ENEMY );
				}
			}
			else  
			{
				// we can see the enemy
				if ( HasConditions(bits_COND_CAN_RANGE_ATTACK1) && m_iSpinState > HASSAULT_SPINUP_BEGAN )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( m_iSpinState < HASSAULT_SPINUP_BEGAN )
				{
					return GetScheduleOfType ( SCHED_SPINUP );
				}

				if ( HasConditions(bits_COND_CAN_MELEE_ATTACK1) )
				{
					ALERT ( at_aiconsole, "HSarge: Melee Attack!\n" );
					return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
				}

				if ( !HasConditions(bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1) )
				{
					// if we can see enemy but can't use either attack type, we must need to get closer to enemy
					return GetScheduleOfType( SCHED_CHASE_ENEMY );
				}
			}
			break;
		break;
		}
	}

	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

Schedule_t *CHassault :: GetScheduleOfType ( int Type )
{
	switch ( Type )
	{
	case SCHED_SPINUP:
		{
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "hassault/hw_spinup.wav", 1, ATTN_NORM );
			RouteClear();

			m_flSpinupDelay = gpGlobals->time + gSkillData.hassaultSpinupDelay;
		//	ALERT ( at_console, "Time is: %.2f Delay is %.2f \n", gpGlobals->time, m_flSpinupDelay);
			m_iSpinState = HASSAULT_SPINUP_BEGAN;
			return &slHassaultSpinup[ 0 ];
		}
	case SCHED_FAIL:
		{
			return &slHassaultFail[ 0 ];
		}
	case SCHED_SPINDOWN:
		{
			return &slHassaultSpindown[ 0 ];
		}
	case SCHED_CHASE_ENEMY:
		{
			return &slHassaultChase[ 0 ];
		}
	case SCHED_MELEE_ATTACK1:
		{
			return &slHassaultMelee[ 0 ];
		}
	case SCHED_RANGE_ATTACK1:
		{
			return &slHassaultRangeAttack1[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}

void CHassault :: MonsterThink ( void )
{
	if ( m_iSpinState <= HASSAULT_SPINUP_BEGAN && m_flSpinupDelay != 0.0 && m_flSpinupDelay <= gpGlobals->time )
	{
		m_flSpinupDelay = 0.0;
		m_iSpinState = HASSAULT_SPINNING;
	}

	CBaseMonster::MonsterThink();
}

void CHassault :: Killed( entvars_t *pevAttacker, int iGib )
{
	if ( m_iSpinState > HASSAULT_NOT_SPINNING )
		SpinDown();

	CSquadMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// CheckRangeAttack1 
//=========================================================
BOOL CHassault :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 2048 && flDot >= 0.5 )	// occluded checks are done in GetSchedule
	{
		TraceResult	tr;

		if ( flDist <= 64 )
		{
			return FALSE;
		}

		if ( m_iSpinState < HASSAULT_SPINNING && HasConditions( bits_COND_ENEMY_OCCLUDED ) )	// no wallhacking; 
			return FALSE;																	// occluded checks are overriden only for suppressing fire

		if ( FVisible ( m_hEnemy ) )
			return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - see if we have to stop chasing enemy.
// Can't do this in CheckRangeAttack1 because of the spinning thing 
//=========================================================
BOOL CHassault :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 )
	{
		return TRUE;
	}

	return FALSE;
}

void CHassault :: SpinDown( void )
{
	STOP_SOUND( ENT(pev), CHAN_WEAPON, "hassault/hw_gun4.wav" );
	STOP_SOUND( ENT(pev), CHAN_ITEM, "hassault/hw_spin.wav" );

	ALERT ( at_aiconsole, "HSarge: Spin down!\n");
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/null.wav", 1, ATTN_NORM);

	m_iSpinState = HASSAULT_NOT_SPINNING;

	EMIT_SOUND( ENT(pev), CHAN_ITEM, "hassault/hw_spindown.wav", 1, ATTN_NORM );
	m_flSpinupDelay = 0.0;
}

void CHassault :: MeleeAttack( float flDamage )
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 70, flDamage, DMG_SLASH );
	if ( pHurt )
	{
		if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
		{
			pHurt->pev->punchangle.z = -18;
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * -100;
		}
	}
	else // Play a random attack miss sound
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

int CHassault :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if (g_iSkillLevel == SKILL_HARD)
		flDamage *= 0.75;	// tanky behavior on hard level

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// FVisible - unlike the original function, this one does
// two additional checks for target's center and feet position
//=========================================================
BOOL CHassault :: FVisible ( CBaseEntity *pEntity )
{
	TraceResult tr;
	Vector		vecLookerOrigin;
	Vector		vecTargetOrigin;
	
	if (FBitSet( pEntity->pev->flags, FL_NOTARGET ))
		return FALSE;

	// don't look through water
	if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3) 
		|| (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
		return FALSE;

	vecLookerOrigin = GetGunPosition();

	for ( int i = 0; i <= 2; i++ )
	{
		switch (i)
		{
		case 0:
			m_iShootTarget = HASSAULT_SHOOT_AT_EYEPOS;
			vecTargetOrigin = pEntity->EyePosition();
			break;
		case 1:
			m_iShootTarget = HASSAULT_SHOOT_AT_CENTER;
			vecTargetOrigin = pEntity->Center();
			break;
		case 2:	// can we see target's feet?
			m_iShootTarget = HASSAULT_SHOOT_AT_FEET;
			vecTargetOrigin = Vector( pEntity->Center().x, pEntity->Center().y, pEntity->pev->absmin.z );	
			break;
		}

		UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT(pev)/*pentIgnore*/, &tr);
		
		if ( tr.flFraction == 1.0 )
			return TRUE;
	}
	
	return FALSE;
}

Vector CHassault :: ShootAtEnemy( const Vector &shootOrigin )
{
	CBaseEntity *pEnemy = m_hEnemy;
	Vector		vecTargetOrigin;

	// since hsarge isn't too mobile, he can't afford too much of ELOF's.
	// that's why we keep him able to fire at enemy as long as possible, even if they are trying to hide their eyepos.
	if ( pEnemy )
	{
		switch ( m_iShootTarget )
		{
		case HASSAULT_SHOOT_AT_EYEPOS:
			vecTargetOrigin = pEnemy->EyePosition();
			break;
		case HASSAULT_SHOOT_AT_CENTER:
			vecTargetOrigin = pEnemy->Center();
			break;
		case HASSAULT_SHOOT_AT_FEET:
			vecTargetOrigin = Vector( pEnemy->Center().x, pEnemy->Center().y, pEnemy->pev->absmin.z );
			break;
		}
		return ( (vecTargetOrigin - pEnemy->pev->origin) + m_vecEnemyLKP - shootOrigin ).Normalize();
	//	return ( vecTargetOrigin - shootOrigin).Normalize();
	}
	else
		return gpGlobals->v_forward;
}