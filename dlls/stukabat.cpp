//=========================================================
// Stukabat - alien bat that flies in squads
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"flyingmonster.h"
#include	"animation.h"
#include	"soundent.h"
#include	"decals.h"
#include	"weapons.h"
#include	"game.h"
#include	"spit.h"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_STUKA_TAKEOFF = LAST_COMMON_SCHEDULE + 1,
	SCHED_STUKA_FALL,
	SCHED_STUKA_DETACH,
	SCHED_STUKA_EAT,
	SCHED_STUKA_DIVE,
	SCHED_STUKA_DIEFALL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_STUKA_TAKEOFF = LAST_COMMON_TASK + 1,
	TASK_STUKA_FALL,
	TASK_STUKA_WAKE,
	TASK_STUKA_IDLE,
	TASK_STUKA_DIVE,
};	

#define STUKA_ASLEEP		0
#define STUKA_WAKING		1
#define STUKA_AWAKE			2

#define	STUKA_MAX_FLYSPEED	120
#define STUKA_DIVE_TIME		4

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
//#define STUKA_SWING1		( 8 )	// already implemented by CFlyingMonster
//#define STUKA_SWING2		( 9 )
#define	STUKA_BOMB			( 1 )
#define	STUKA_CLAW			( 2 )

// UNDONE: CONVERT TO THE SQUADMONSTER!!!
// UNDONE: stukas can't avoid glass!

class CStukaBomb : public CSpit
{
public:
	void Spawn();
	void Touch( CBaseEntity *pOther );
	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
};

class CStukaBat : public CFlyingMonster
{
	public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CallForHelp( float flDist );

	Activity GetStoppedActivity ( void );
	void SetTurnActivity ( void );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void Move(float flInterval);
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
//	void Stop( void );
	int CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );
//	void RouteClear ( void );
	MONSTERSTATE GetIdealState ( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t* GetScheduleOfType ( int Type );
//	void MonsterThink();
	void RunAI( void );
	float ChangeYaw( int yawSpeed );

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	Activity GetFlyActivity( void );
	void Wake ( void );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Killed( entvars_t *pevAttacker, int iGib );

//	void Bomb( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];
	static const char *pAttackHitSounds[];

//	BOOL m_fAsleep;
	BOOL m_fDeathMidAir;
	BOOL m_fStukaDmgFall;
	BOOL m_fDiving;
	float m_flNextTakeoff;
	int	m_iSleepState;
	float m_flNextBomb;
	float m_momentum;
	float m_flNextPathUpdate;
	float m_flLastDiveAttack;
//	float m_flNextDiveAttack;
};

LINK_ENTITY_TO_CLASS( monster_stukabat, CStukaBat );

TYPEDESCRIPTION	CStukaBat::m_SaveData[] = 
{
//	DEFINE_FIELD( CStukaBat, m_fAsleep, FIELD_BOOLEAN ),
	DEFINE_FIELD( CStukaBat, m_fDeathMidAir, FIELD_BOOLEAN ),
	DEFINE_FIELD( CStukaBat, m_fDiving, FIELD_BOOLEAN ),
	DEFINE_FIELD( CStukaBat, m_flNextTakeoff, FIELD_TIME ),
	DEFINE_FIELD( CStukaBat, m_flNextBomb, FIELD_TIME ),
	DEFINE_FIELD( CStukaBat, m_flLastDiveAttack, FIELD_TIME ),
//	DEFINE_FIELD( CStukaBat, m_flNextDiveAttack, FIELD_TIME ),
	DEFINE_FIELD( CStukaBat, m_iSleepState, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CStukaBat, CFlyingMonster );

const char *CStukaBat::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

//=========================================================
// Stuka fail
//=========================================================
Task_t	tlStukaFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_STUKA_IDLE,			0				},
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slStukaFail[] =
{
	{
		tlStukaFail,
		ARRAYSIZE ( tlStukaFail ),
		bits_COND_CAN_ATTACK,
		0,
		"Stuka Fail"
	},
};

Task_t	tlStukaWake[] =
{
	{ TASK_STUKA_WAKE,			0					},
//	{ TASK_STOP_MOVING,			(float)0			},
//	{ TASK_WAIT,				(float)2			},
};

Schedule_t	slStukaWake[] =
{
	{
		tlStukaWake,
		ARRAYSIZE ( tlStukaWake ),
		0,
		0,
		"Stuka Wake"
	},
};

//=========================================================
//	Attack Schedules
//=========================================================

// primary range attack
Task_t	tlStukaRangeAttack1[] =
{
//	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slStukaRangeAttack1[] =
{
	{ 
		tlStukaRangeAttack1,
		ARRAYSIZE ( tlStukaRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Stuka Range Attack1"
	},
};

// secondary range attack
Task_t	tlStukaRangeAttack2[] =
{
//	{ TASK_STOP_MOVING,			0				},
//	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
};

Schedule_t	slStukaRangeAttack2[] =
{
	{ 
		tlStukaRangeAttack2,
		ARRAYSIZE ( tlStukaRangeAttack2 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Stuka Range Attack2"
	},
};

Task_t	tlStukaTakeoff[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_STUKA_TAKEOFF,		0				},

};

Schedule_t	slStukaTakeoff[] =
{
	{ 
		tlStukaTakeoff,
		ARRAYSIZE ( tlStukaTakeoff ), 
		0,
		0,
		"Stuka Take off"
	},
};

Task_t	tlStukaFall[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_STUKA_FALL,			(float)0			},
//	{ TASK_WAIT_RANDOM,			(float)2.5			},
};

Schedule_t	slStukaFall[] =
{
	{
		tlStukaFall,
		ARRAYSIZE ( tlStukaFall ),
		0,
		0,
		"Stuka Fall"
	},
};

//=========================================================
// AlertIdle Schedules
//=========================================================
Task_t	tlStukaAlertStand[] =
{
	{ TASK_STOP_MOVING,			0						 },
	{ TASK_STUKA_IDLE,			0						 },
	{ TASK_WAIT,				(float)5				 },
	{ TASK_SUGGEST_STATE,		(float)MONSTERSTATE_IDLE },
};

Schedule_t	slStukaAlertStand[] =
{
	{ 
		tlStukaAlertStand,
		ARRAYSIZE ( tlStukaAlertStand ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_SEE_ENEMY				|
		bits_COND_SEE_FEAR				|
		bits_COND_LIGHT_DAMAGE			|
		bits_COND_HEAVY_DAMAGE			|
		bits_COND_PROVOKED				|
		bits_COND_SMELL					|
		bits_COND_SMELL_FOOD			|
		bits_COND_HEAR_SOUND,

		bits_SOUND_COMBAT		|// sound flags
		bits_SOUND_WORLD		|
		bits_SOUND_PLAYER		|
		bits_SOUND_DANGER		|

		bits_SOUND_MEAT			|// scent flags
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"Stuka Alert Stand"
	},
};

Task_t	tlStukaIdle[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_STUKA_IDLE,			0				},
	{ TASK_WAIT,				(float)5		},
};

Schedule_t	slStukaIdle[] =
{
	{ 
		tlStukaIdle,
		ARRAYSIZE ( tlStukaIdle ), 
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
		"Stuka Idle"
	},
};

Task_t	tlStukaAlertFace[] =
{
	{ TASK_STOP_MOVING,				0				},
	{ TASK_STUKA_IDLE,				0				},
	{ TASK_FACE_IDEAL,				(float)0		},
};

Schedule_t	slStukaAlertFace[] =
{
	{ 
		tlStukaAlertFace,
		ARRAYSIZE ( tlStukaAlertFace ),
		bits_COND_NEW_ENEMY		|
		bits_COND_SEE_FEAR		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_PROVOKED,
		0,
		"Stuka Alert Face"
	},
};

Task_t	tlStukaCombatFace[] =
{
	{ TASK_STOP_MOVING,				0				},
	{ TASK_STUKA_IDLE,				0				},
//	{ TASK_SET_ACTIVITY,			(float)ACT_FLY	},
	{ TASK_FACE_ENEMY,				(float)0		},
};

Schedule_t	slStukaCombatFace[] =
{
	{ 
		tlStukaCombatFace,
		ARRAYSIZE ( tlStukaCombatFace ), 
		bits_COND_CAN_ATTACK			|
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD,
		0,
		"Stuka Combat Face"
	},
};

Task_t tlStukaEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
};

Schedule_t slStukaEat[] =
{
	{
		tlStukaEat,
		ARRAYSIZE( tlStukaEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"SquidEat"
	}
};

Task_t tlStukaChase[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CHASE_ENEMY_FAILED	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0		},
	{ TASK_RUN_PATH,			(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
};

Schedule_t slStukaChase[] =
{
	{ 
		tlStukaChase,
		ARRAYSIZE ( tlStukaChase ),
		0,
		0,
		"Stuka Chase"
	},
};

Task_t tlStukaDive[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CHASE_ENEMY_FAILED	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_STUKA_DIVE,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slStukaDive[] =
{
	{ 
		tlStukaDive,
		ARRAYSIZE ( tlStukaDive ),
		0,
		0,
		"Stuka Dive"
	},
};

Task_t tlStukaDieFall[] = 
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_STUKA_FALL,				(float)0				},
	{ TASK_DIE,						(float)0				},
};

Schedule_t slStukaDieFall[] =
{
	{ 
		tlStukaDieFall,
		ARRAYSIZE ( tlStukaDieFall ),
		0,
		0,
		"Stuka Die Fall"
	},
};

DEFINE_CUSTOM_SCHEDULES( CStukaBat )
{
	slStukaFail,
	slStukaWake,
	slStukaRangeAttack1,
	slStukaRangeAttack2,
	slStukaTakeoff,
	slStukaIdle,
	slStukaFall,
	slStukaAlertStand,
	slStukaAlertFace,
	slStukaCombatFace,
	slStukaEat,
	slStukaChase,
	slStukaDive,
	slStukaDieFall,
};

IMPLEMENT_CUSTOM_SCHEDULES( CStukaBat, CFlyingMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CStukaBat :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CStukaBat :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
	case ACT_STAND:
	case ACT_LEAP:
		ys = 0;
		break;	// do not turn when on ceiling
	case ACT_HOVER:
		ys = 120;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// Spawn
//=========================================================
void CStukaBat :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/stukabat.mdl");
	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 18 ) );

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->flags			|= FL_FLY;
	m_bloodColor		= BLOOD_COLOR_YELLOW;
	pev->health			= gSkillData.stukaHealth;
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_MonsterState		= MONSTERSTATE_NONE;
	pev->view_ofs		= Vector(0, 0, 8);
	m_pFlapSound	= "zombie/claw_miss1.wav";
	
	SetFlyingSpeed( 275 );
	SetFlyingMomentum( 2.5 );

	MonsterInit();

	Vector Forward;
	UTIL_MakeVectorsPrivate(pev->angles, Forward, 0, 0);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CStukaBat :: Precache()
{
	PRECACHE_MODEL("models/stukabat.mdl");
	PRECACHE_MODEL("models/spit.mdl");

	// wing sounds
	PRECACHE_SOUND("zombie/claw_miss1.wav");
//	PRECACHE_SOUND("zombie/claw_miss2.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");
	PRECACHE_SOUND("bullchicken/bc_attack1.wav");

	int i ;

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CStukaBat :: HandleAnimEvent( MonsterEvent_t *pEvent )
{	

	switch( pEvent->event )
	{
	case STUKA_CLAW:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 96, gSkillData.stukaDmgSlash, DMG_SLASH );
			if ( pHurt && !FClassnameIs(pHurt->pev, "monster_stukabat") )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				
				m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(1,3);

				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
		}
	break;

	case STUKA_BOMB:
	{
		Vector	vecSpitOffset;
		Vector	vecSpitDir;

		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "bullchicken/bc_attack1.wav", 1, ATTN_NORM );
		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

		UTIL_MakeVectors ( pev->angles );

		vecSpitOffset = pev->origin + gpGlobals->v_forward * 32.0;
		
		vecSpitDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - vecSpitOffset);
		float gravity = g_psv_gravity->value;
		float time = vecSpitDir.Length( ) / 900;
		vecSpitDir = vecSpitDir * (1.0 / time);

		// adjust upward toss to compensate for gravity loss
		vecSpitDir.z += gravity * time * 0.5;

		CStukaBomb::Shoot( pev, vecSpitOffset, vecSpitDir );

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

		m_flNextBomb = gpGlobals->time + RANDOM_FLOAT(5,10);
	}
	break;

	default:
		CFlyingMonster::HandleAnimEvent( pEvent );
		break;
	}
	
}

//=========================================================
// RunAI
//=========================================================
void CStukaBat :: RunAI ( void )
{
	if ( ( m_MonsterState == MONSTERSTATE_IDLE || m_MonsterState == MONSTERSTATE_ALERT ) && RANDOM_LONG(0,99) == 0 && !(pev->flags & SF_MONSTER_GAG) )
	{
		IdleSound();
	}

	if ( m_MonsterState != MONSTERSTATE_NONE	&& 
		 m_MonsterState != MONSTERSTATE_PRONE   && 
		 m_MonsterState != MONSTERSTATE_DEAD )// don't bother with this crap if monster is prone. 
	{
	
		if ( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) || ( m_MonsterState == MONSTERSTATE_COMBAT ) )
		{
			if ( m_iSleepState > STUKA_ASLEEP )
				Look( m_flDistLook );	// sleep with eyes closed

			Listen();// check for audible sounds. 

			// now filter conditions.
			ClearConditions( IgnoreConditions() );

			GetEnemy();
		}

		// do these calculations if monster has an enemy.
		if ( m_hEnemy != NULL )
		{
			CheckEnemy( m_hEnemy );
		}
	}

	FCheckAITrigger();

	PrescheduleThink();

	MaintainSchedule();

	// this is a rough way of getting rid of enemyLKP path when it becomes obsolete.
	// find a better way of doing it
	if ( m_hEnemy != NULL && HasConditions(bits_COND_SEE_ENEMY) && pev->movetype == MOVETYPE_FLY && m_flNextPathUpdate < gpGlobals->time )
	{
		RouteClear();
		m_flNextPathUpdate = gpGlobals->time + 1.0;
		BuildRoute ( m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy );	// can we do better than this?
	}

	if ( m_IdealActivity != ACT_FLY && pev->angles.z != 0 )
		pev->angles.z = 0;

}

float CStukaBat :: ChangeYaw( int yawSpeed )
{
//	ALERT(at_aiconsole, "Stukabat: ChangeYaw\n");
	
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlYawDiff();
		float target = 0;

		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if ( diff < -20 )
				target = 90;
			else if ( diff > 20 )
				target = -90;
		}
		else //FIXME: this is not right
		//pev->angles.y = UTIL_Approach( pev->ideal_yaw, pev->angles.y, 700.0 * gpGlobals->frametime );
		{
			float move, current, speed;

			current = UTIL_AngleMod( pev->angles.y );

			if (current != pev->ideal_yaw)
			{
				speed = (float)yawSpeed * gpGlobals->frametime * 10;
				move = pev->ideal_yaw - current;

				if (pev->ideal_yaw > current)
				{
					if (move >= 180)
						move = move - 360;
				}
				else
				{
					if (move <= -180)
						move = move + 360;
				}

				if (move > 0)
				{
					if (move > speed)
						move = speed;
				}
				else
				{
					if (move < -speed)
						move = -speed;
				}

				pev->angles.y = UTIL_AngleMod (current + move);
			}
		}

		pev->angles.z = UTIL_Approach( target, pev->angles.z, 220.0 * gpGlobals->frametime );
	}
	return CBaseMonster::ChangeYaw( yawSpeed );
}

//=========================================================
// CheckRangeAttack1 - stukabat's "bombing" attack, called
// when gliding 
//=========================================================
BOOL CStukaBat :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( pev->movetype != MOVETYPE_FLY )
		return FALSE; // defenseless on ground

	if ( m_flightSpeed < 50 )
		return FALSE;

	if ( m_flNextBomb > gpGlobals->time )
		return FALSE;

//	if ( !NoFriendlyFire() )
//		return FALSE;

	if ( flDist > 96 && flDist <= 1024 && flDot > 0.9 )
		return TRUE;

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - stukabat's close range attack, a
// replacer for melee attack
//=========================================================
BOOL CStukaBat :: CheckRangeAttack2 ( float flDot, float flDist )
{
//	if ( !m_flightSpeed && m_Activity == ACT_HOVER )
	if ( pev->movetype != MOVETYPE_FLY )
		return FALSE; // defenseless on ground

	if ( m_flNextAttack > gpGlobals->time )
		return FALSE;

//	if ( flDist > 96 )
//		return FALSE;

//	if ( pev->origin.z > m_hEnemy->EyePosition().z || pev->origin.z < m_hEnemy->pev->origin.z )
//		return FALSE;

/*	if ( flDist <= 64 )
	{
		pev->velocity = g_vecZero;
		return TRUE;
	}*/

	if ( flDist <= 48 )
		return TRUE;

	return FALSE;
}

void CStukaBat :: CallForHelp( float flDist )
{
	CBaseEntity *pEntity = NULL;

	ALERT(at_aiconsole, "Stukabat: Help!\n");

	while ( (pEntity = UTIL_FindEntityByClassname(pEntity, STRING(pev->classname))) != NULL )
	{
		if ( pEntity->edict() == edict() )
			continue;

		float d = (pev->origin - pEntity->pev->origin).Length();
		
		if (d > flDist)
			continue;

		CStukaBat *pStuka = (CStukaBat *)Instance(pEntity->edict());

		ALERT(at_aiconsole, "Stukabat: grabbed a buddie\n");
		if ( pStuka->m_iSleepState == STUKA_ASLEEP )
			pStuka->Wake();
	}
}

//=========================================================
// GetStoppedActivity - switch between air and ground activities
//=========================================================
Activity CStukaBat :: GetStoppedActivity( void )
{ 
//	ALERT(at_aiconsole, "Stukabat: GetStoppedActivity\n");
	if ( pev->movetype != MOVETYPE_FLY )		
		return ACT_CROUCHIDLE;

	return ACT_HOVER; 
}

void CStukaBat :: SetTurnActivity ( void )
{
	float flYD;
	flYD = FlYawDiff();

	if ( flYD <= -45 && LookupActivity ( ACT_FLY_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
	{// big right turn
		m_IdealActivity = ACT_FLY_RIGHT;
	}
	else if ( flYD > 45 && LookupActivity ( ACT_FLY_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{// big left turn
		m_IdealActivity = ACT_FLY_LEFT;
	}
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule. 
//=========================================================
void CStukaBat :: StartTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_STUKA_TAKEOFF:
		{
			RouteClear();
			m_IdealActivity = ACT_LEAP;
			break;
		}
	case TASK_DIE:
		{
			RouteClear();
			pev->frame = 0;
			
			if ( !m_fDeathMidAir ) 
			{
				ALERT(at_aiconsole, "Stukabat: Ground death\n");
				pev->sequence = LookupSequence("Die_on_ground");
			}
			else
			{
				ALERT(at_aiconsole, "Stukabat: Fall death\n");
				if ( RANDOM_LONG(1,10) > 5 )
					pev->sequence = LookupSequence("Death_fall_simple");
				else
					pev->sequence = LookupSequence("Death_fall_violent");
			}

			ResetSequenceInfo( );
			pev->deadflag = DEAD_DYING;
			break;	
		}
	case TASK_STUKA_FALL:
		{
			m_IdealActivity = ACT_FALL;
			pev->movetype = MOVETYPE_TOSS;				
			break;
		}
	case TASK_STUKA_WAKE:
		{
			m_IdealActivity = ACT_STAND;
			break;
		}
	case TASK_STUKA_IDLE:
		{
			if ( m_iSleepState > STUKA_ASLEEP )
				m_IdealActivity = GetStoppedActivity();
			else
				m_IdealActivity = ACT_IDLE;
			
			TaskComplete();
			break;
		}
	case TASK_STOP_MOVING:
		{
			if ( m_IdealActivity == m_movementActivity )
			{
				m_IdealActivity = GetStoppedActivity();
			}

			pev->velocity = g_vecZero;

			RouteClear();
			TaskComplete();
			break;
		}
	case TASK_STUKA_DIVE:
		{
			m_fDiving = TRUE;
			break;
		}
	default:
		{
			CFlyingMonster::StartTask( pTask );
			break;
		}
	}
} 

//=========================================================
// RunTask 
//=========================================================
void CStukaBat :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_STUKA_TAKEOFF:
		{
			if ( m_fSequenceFinished )
			{
				pev->flags |= FL_FLY;
				pev->movetype = MOVETYPE_FLY;
				m_IdealActivity = ACT_HOVER;
				pev->velocity.z += 48.0;
				m_flNextBomb = gpGlobals->time + RANDOM_FLOAT ( 5, 10 );
				m_flLastDiveAttack = gpGlobals->time; // don't start with diveattack!
				TaskComplete();
			}
			break;
		}	
	case TASK_DIE:
		{
			CFlyingMonster::RunTask(pTask);

			break;
		}
	case TASK_STUKA_FALL:
		{
			if ( FBitSet ( pev->flags, FL_ONGROUND ) )
			{
				pev->flags &= ~FL_FLY;

				if ( pev->waterlevel == 3 )
					Killed(pev, GIB_NEVER);

				if ( !m_fDeathMidAir )
				{
					m_IdealActivity = ACT_CROUCHIDLE;
					pev->movetype = MOVETYPE_STEP;
					m_flNextTakeoff = gpGlobals->time + RANDOM_FLOAT ( 5, 15 );
				}
				TaskComplete();
			}
			break;
		}
	case TASK_STUKA_WAKE:
		{
			if ( m_fSequenceFinished )
			{
				m_iSleepState = STUKA_AWAKE;
				pev->origin.z -= 12;
				pev->velocity.z += 64.0;
				m_flNextBomb = gpGlobals->time + RANDOM_FLOAT(2,5);	// don't start with bomb attack
				m_flLastDiveAttack = gpGlobals->time;

				CallForHelp( 256 );

				TaskComplete();
			}
			break;
		}
	case TASK_STUKA_DIVE:
		{
			MakeIdealYaw ( m_vecEnemyLKP );
			ChangeYaw ( pev->yaw_speed );

			if ( HasConditions(bits_COND_CAN_RANGE_ATTACK2) || m_flLastDiveAttack + STUKA_DIVE_TIME < gpGlobals->time )
			{
				ALERT(at_aiconsole, "Stukabat: Stopped diving!\n");
				m_fDiving = FALSE;
				m_Activity = ACT_RESET;
				TaskComplete();
			}
			break;
		}
	default:
		{
			CFlyingMonster :: RunTask(pTask);
			break;
		}
	}
}	

//=========================================================
void CStukaBat::Move(float flInterval)
{
//	ALERT(at_aiconsole, "Stukabat: Move\n");
	if ( pev->movetype == MOVETYPE_FLY )
		CFlyingMonster::Move( flInterval );
	else
		CBaseMonster::Move( flInterval );
}

void CStukaBat::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		if ( gpGlobals->time - m_stopTime > 1.0 )
		{
			if ( m_IdealActivity != GetFlyActivity() )
			{
				m_IdealActivity = GetFlyActivity();
				m_flGroundSpeed = m_flightSpeed = 200;
			}
		}
		Vector vecMove = pev->origin + (( vecDir + (m_vecTravel * m_momentum) ).Normalize() * (m_flGroundSpeed * flInterval));

		if ( m_IdealActivity != GetFlyActivity() )
		{
			m_flightSpeed = UTIL_Approach( 100, m_flightSpeed, 75 * gpGlobals->frametime );
			if ( m_flightSpeed < 100 )
				m_stopTime = gpGlobals->time;
		}
		else
			m_flightSpeed = UTIL_Approach( 20, m_flightSpeed, 300 * gpGlobals->frametime );

		if ( pTargetEnt != NULL && (pTargetEnt->pev->origin - pev->origin).Length() > 64 )
			m_flightSpeed += 75;

		if ( m_IdealActivity != ACT_SPECIAL_ATTACK1 )
		{
			if ( m_flightSpeed > STUKA_MAX_FLYSPEED )
				m_flightSpeed = STUKA_MAX_FLYSPEED;
		}
		else m_flGroundSpeed = m_flightSpeed = 300;

		if ( CheckLocalMove ( pev->origin, vecMove, pTargetEnt, NULL ) == LOCALMOVE_VALID )
		{
			m_IdealActivity = GetFlyActivity();
			m_vecTravel = (vecMove - pev->origin);
			m_vecTravel = m_vecTravel.Normalize();
			UTIL_MoveToOrigin(ENT(pev), vecMove, (m_flGroundSpeed * flInterval), MOVE_STRAFE);
		//	ALERT(at_aiconsole, "Flying Monster: LOCALMOVE_VALID\n");
		//	ALERT( at_aiconsole, "Speed is %.0f Speed * flInterval is %.0f\n", m_flightSpeed, (m_flGroundSpeed * flInterval) );
		}
		else
		{
			m_IdealActivity = GetStoppedActivity();
			m_stopTime = gpGlobals->time;
			m_vecTravel = g_vecZero;
			pev->velocity = g_vecZero;
		//	ALERT(at_aiconsole, "Stukabat: LOCALMOVE_INVALID\n");
		}		
	}
	else
	{
		CBaseMonster::MoveExecute( pTargetEnt, vecDir, flInterval );
		return;
	}
	
	pev->velocity = m_flightSpeed * m_vecTravel;
}

int CStukaBat :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
//	ALERT(at_aiconsole, "Stukabat: CheckLocalMove\n");
	TraceResult tr;

//	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, point_hull, edict(), &tr );
	TRACE_MONSTER_HULL(edict(), vecStart + Vector(0, 0, 16), vecEnd + Vector(0, 0, 16), dont_ignore_monsters, edict(), &tr);

	// ALERT( at_aiconsole, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
	// ALERT( at_aiconsole, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

	// ALERT( at_aiconsole, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if ( tr.flFraction >= 1.0 )
		return LOCALMOVE_VALID;

	if (tr.fStartSolid || tr.flFraction < 1.0)
	{
	//	if ( pTarget && pTarget->edict() == tr.pHit )
		if ( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
		ALERT( at_aiconsole, "LOCALMOVE_INVALID\n" );
	}

	return LOCALMOVE_VALID;
}

MONSTERSTATE CStukaBat :: GetIdealState ( void )
{
//	int	iConditions;

//	iConditions = IScheduleFlags();

	switch ( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		{
			if ( m_iSleepState < STUKA_AWAKE )
			{
				// need to wake up first
				return MONSTERSTATE_ALERT;
			}

			if ( m_hEnemy == NULL )
			{
				m_IdealMonsterState = MONSTERSTATE_IDLE;
			}

			return CFlyingMonster::GetIdealState();
			break;
		}
	default:
		{
			return CFlyingMonster::GetIdealState();
			break;
		}
	}

}

//=========================================================
// GetScheduleOfType - returns a pointer to one of the 
// monster's available schedules of the indicated type.
//=========================================================
Schedule_t* CStukaBat :: GetScheduleOfType ( int Type ) 
{
//	ALERT ( at_aiconsole, "Sched Type:%d\n", Type );
	
	switch	( Type )
	{
	case SCHED_FAIL:
	case SCHED_CHASE_ENEMY_FAILED:
		{
			ALERT ( at_aiconsole, "Stukabat: Fail\n" );

			return &slStukaFail[ 0 ];
		}
	case SCHED_RANGE_ATTACK1:
		{
			return &slStukaRangeAttack1[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slStukaRangeAttack2[ 0 ];
		}
	case SCHED_STUKA_TAKEOFF:
		{
			ALERT ( at_aiconsole, "Stukabat: Taking off!\n" );
			return &slStukaTakeoff[ 0 ];
		}
	case SCHED_STUKA_DETACH:
		{
			ALERT ( at_aiconsole, "Stukabat: Wake\n" );
			m_iSleepState = STUKA_WAKING;
			return &slStukaWake[ 0 ];
		}
	case SCHED_IDLE_STAND:
		{
			return &slStukaIdle[ 0 ];
		}
	case SCHED_STUKA_FALL:
		{
			return &slStukaFall[ 0 ];
		}
	case SCHED_ALERT_STAND:
		{
			return &slStukaAlertStand[ 0 ];
		}
	case SCHED_ALERT_FACE:
		{
			return &slStukaAlertFace[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slStukaCombatFace[ 0 ];
		}
	case SCHED_STUKA_EAT:
		{
			return &slStukaEat[ 0 ];
		}
	case SCHED_CHASE_ENEMY:
		{
			return &slStukaChase[ 0 ];
		}
	case SCHED_STUKA_DIVE:
		{
			return &slStukaDive[ 0 ];
		}
	case SCHED_STUKA_DIEFALL:
		{
			return &slStukaDieFall[ 0 ];
		}
	default:
		{
			return CFlyingMonster::GetScheduleOfType ( Type );
		}
	}

}	

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CStukaBat :: GetSchedule ( void )
{
//	ALERT ( at_aiconsole, "StukaBat: GetSchedule\n" );
	switch	( m_MonsterState )
	{
		case MONSTERSTATE_IDLE:
		{
			if ( m_iSleepState == STUKA_ASLEEP )
			{
				return GetScheduleOfType( SCHED_IDLE_STAND );
			}
			else if ( HasConditions ( bits_COND_HEAR_SOUND ) && pev->movetype != MOVETYPE_FLY )
			{
				return GetScheduleOfType( SCHED_ALERT_FACE );
			}
			else if ( FRouteClear() )
			{
				// no valid route!
				return GetScheduleOfType( SCHED_IDLE_STAND );
			}
			else
			{
				// valid route. Get moving
				return GetScheduleOfType( SCHED_IDLE_WALK );
			}
			
			break;
		}	
		case MONSTERSTATE_ALERT:
		{
			if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			{
				if( m_iSleepState == STUKA_ASLEEP )
					return GetScheduleOfType( SCHED_STUKA_DETACH );

				return CFlyingMonster::GetSchedule();
			}
			else
			if ( HasConditions ( bits_COND_HEAR_SOUND ) || HasConditions ( bits_MEMORY_PROVOKED ) )
			{
				if ( m_iSleepState == STUKA_ASLEEP )
					return GetScheduleOfType( SCHED_STUKA_DETACH );

				return GetScheduleOfType( SCHED_ALERT_FACE );
			}
			else if ( HasConditions(bits_COND_SMELL_FOOD) && FShouldEat() )
			{	
				ALERT ( at_aiconsole, "Stukabat: Found food!\n" );
				// food is right out in the open. Just go get it.
				return GetScheduleOfType( SCHED_STUKA_EAT );
			}
			else return GetScheduleOfType( SCHED_ALERT_STAND );

			break;
		}
		case MONSTERSTATE_COMBAT:
		{
			if ( m_iSleepState == STUKA_ASLEEP )
			{
				// if somehow is still asleep then its about time to wake up
				return GetScheduleOfType( SCHED_STUKA_DETACH );
			}	

			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				return CFlyingMonster::GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType ( SCHED_COMBAT_FACE );
			}
			else
			if ( m_fStukaDmgFall )
			{
				m_fStukaDmgFall = FALSE;
				return GetScheduleOfType( SCHED_STUKA_FALL );
			}
			else
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
					if ( pev->movetype != MOVETYPE_FLY )
					{
					//	if ( FBitSet ( pev->flags, FL_ONGROUND ) && m_flNextTakeoff < gpGlobals->time )
					//		return GetScheduleOfType ( SCHED_STUKA_TAKEOFF );

						return GetScheduleOfType ( SCHED_IDLE_STAND );
					}
					// chase!
					return GetScheduleOfType( SCHED_CHASE_ENEMY );
				}
			}
			else  
			{
				// we can see the enemy
				ALERT ( at_aiconsole, "Stukabat: I can see the enemy!\n" );

				if ( HasConditions(bits_COND_CAN_RANGE_ATTACK2) )
				{
					ALERT ( at_aiconsole, "Stukabat: Claw!\n" );
					pev->velocity.z = g_vecZero.z;
					return GetScheduleOfType ( SCHED_RANGE_ATTACK2 );
				}
	
				if ( !HasConditions(bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2) )
				{
					if ( pev->movetype != MOVETYPE_FLY )
					{
						if ( FBitSet ( pev->flags, FL_ONGROUND ) && m_flNextTakeoff < gpGlobals->time )
							return GetScheduleOfType ( SCHED_STUKA_TAKEOFF );

						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}

					if ( !m_fDiving && m_flLastDiveAttack + 15 < gpGlobals->time && m_hEnemy != NULL )
					{
						TraceResult tr;
						Vector vecToEnemy;

						ALERT ( at_aiconsole, "Stukabat: Dive check\n" );

						vecToEnemy = (m_hEnemy->pev->origin - pev->origin).Normalize();

						if ( vecToEnemy.Length() > 512.0 && DotProduct( vecToEnemy, gpGlobals->v_forward.Normalize() ) > 0 )
						{
							CBaseEntity* pEntity;

							UTIL_TraceLine(pev->origin, m_hEnemy->pev->origin ,dont_ignore_monsters, edict(), &tr);

							pEntity = (CBaseEntity*)Instance(tr.pHit);
							if (pEntity == m_hEnemy)
							{	
								ALERT ( at_aiconsole, "Stukabat: Diving!\n" );
								m_flLastDiveAttack = gpGlobals->time;
								return GetScheduleOfType( SCHED_STUKA_DIVE );
							}
						}
						
					}
					// chase them until you can attack
					return GetScheduleOfType( SCHED_CHASE_ENEMY );
				}
				else if ( !FacingIdeal() )
				{
					//turn
					return GetScheduleOfType( SCHED_COMBAT_FACE );
				}
				else
				{
					ALERT ( at_aiconsole, "Stukabat: Chase!\n" );
					// keep chasing as attack sequences are played on fly
					return GetScheduleOfType( SCHED_CHASE_ENEMY );
				}
			}

		//	return CFlyingMonster::GetSchedule();
			break;
		}
		case MONSTERSTATE_DEAD:
		{
			if ( m_fDeathMidAir )
			{
				ALERT(at_aiconsole, "Stukabat: Death fall\n");
				return GetScheduleOfType( SCHED_STUKA_DIEFALL);
			}

			return GetScheduleOfType( SCHED_DIE );
			break;
		}
		default:
		{
			return CFlyingMonster::GetSchedule();
		}
	}

}

int CStukaBat::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType == DMG_POISON )
		flDamage = 0;

	if ( flDamage > 0 )
	{
		int iPercent = RANDOM_LONG(0,99);

		if ( iPercent >= 10 )
			m_fStukaDmgFall = TRUE;
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

Activity CStukaBat :: GetFlyActivity( void )
{ 
	if ( HasConditions(bits_COND_CAN_RANGE_ATTACK1) && !m_fDiving )
		return ACT_RANGE_ATTACK1;
	else if ( HasConditions(bits_COND_CAN_RANGE_ATTACK2) )
		return ACT_RANGE_ATTACK2;

	if ( m_fDiving )
		return ACT_SPECIAL_ATTACK1;
	
	if ( m_flightSpeed > 40 )
	{
	//	if ( m_hEnemy != NULL && ((m_hEnemy->pev->origin - pev->origin).Length() < 64) ) 
	//	return ACT_HOVER;

		return ACT_FLY;
	}
	
	return ACT_HOVER; 
}

void CStukaBat :: Killed( entvars_t *pevAttacker, int iGib )
{
/*	TraceResult tr;
	Vector vecEnd;

	vecEnd = pev->origin;
	vecEnd.z -= 32.0;
	UTIL_TraceLine(pev->origin, vecEnd, dont_ignore_monsters, edict(), &tr);

	if ( tr.flFraction == 1.0 )
	{
		m_fDeathMidAir = TRUE;
	}*/

	if ( !FBitSet ( pev->flags, FL_ONGROUND ) )
		m_fDeathMidAir = TRUE;

	CFlyingMonster :: Killed(pevAttacker, iGib);
}

void CStukaBat :: Wake ( void )
{
//	ALERT(at_aiconsole, "Stukabat: Provoked!\n");

	m_IdealMonsterState = MONSTERSTATE_ALERT;
	SetConditions ( bits_MEMORY_PROVOKED );
}

void CStukaBat :: Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
//	ALERT(at_aiconsole, "Stukabat: Using!\n");

	if ( useType != USE_ON )
		return;

	if ( m_iSleepState == STUKA_ASLEEP )
		Wake();
}

LINK_ENTITY_TO_CLASS( stukabomb, CStukaBomb );

void CStukaBomb :: Spawn( void )
{
	CSpit::Spawn();
	pev->classname = MAKE_STRING( "stukabomb" );
}

void CStukaBomb::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CSpit *pSpit = GetClassPtr( (CStukaBomb *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles( vecVelocity );

	pSpit->SetThink ( &CSpit::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CStukaBomb :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );	

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}

	if ( !pOther->pev->takedamage )
	{
		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_YBLOOD2 + RANDOM_LONG(0,1));
	}
	
	RadiusDamage( pev->origin, pev, pev, gSkillData.stukaDmgBomb, 64.0, CLASS_NONE, DMG_POISON ); // don't use acid as it won't be healed with antitoxine!!!

	SetThink ( &CStukaBomb::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}
