//=========================================================
// diablo.cpp - the predator
//=========================================================

// UNDONE: crawl when enemy is not facing you

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"
#include	"nodes.h"

#define DIABLO_JUMP_DIST_MIN	128
#define DIABLO_JUMP_DIST_MAX	448

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define DIABLO_AE_CLAW_LEFT				( 1 )
#define DIABLO_AE_CLAW_RIGHT			( 2 )
#define DIABLO_AE_JUMP					( 3 )

class CDiablo : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	void IdleSound( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void EXPORT LeapTouch ( CBaseEntity *pOther );

	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );

	Schedule_t  *GetScheduleOfType ( int Type );
	void Jump( void );
	void Claw( int iClaw );

	CUSTOM_SCHEDULES;

	static TYPEDESCRIPTION m_SaveData[];

	float m_flLastHurtTime;
	float m_flNextMelee;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
};

LINK_ENTITY_TO_CLASS( monster_panther, CDiablo );
LINK_ENTITY_TO_CLASS( monster_diablo, CDiablo );

TYPEDESCRIPTION	CDiablo::m_SaveData[] = 
{
	DEFINE_FIELD( CDiablo, m_flLastHurtTime, FIELD_TIME ),
	DEFINE_FIELD( CDiablo, m_flNextMelee, FIELD_TIME ),
};

const char *CDiablo::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CDiablo::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDiablo :: SetYawSpeed ( void )
{
	pev->yaw_speed = 200;
}

void CDiablo :: Spawn ( void )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/panther.mdl");

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_PANTHER;
	pev->effects		= 0;

	m_MonsterState		= MONSTERSTATE_NONE;

//	UTIL_SetSize( pev, Vector( -24, -32, 0 ), Vector( 24, 32, 48 ) );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );
	pev->health			= gSkillData.diabloHealth;
	m_flFieldOfView		= 0.4;

	m_flDistDieForward	= 96;
	m_flDistDieBackward	= 272;

	MonsterInit();
}


void CDiablo :: Precache ( void )
{
	PRECACHE_MODEL("models/panther.mdl");
	PRECACHE_SOUND("panthereye/pa_idle1.wav");
	PRECACHE_SOUND("panthereye/pa_idle2.wav");
	PRECACHE_SOUND("panthereye/pa_idle3.wav");
	PRECACHE_SOUND("panthereye/pa_idle4.wav");
	
	int i;	

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);
}


int CDiablo :: Classify ( void )
{
	return	CLASS_ALIEN_PREDATOR;
}

void CDiablo :: IdleSound ( void )
{
	switch ( RANDOM_LONG(0,3) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "panthereye/pa_idle1.wav", 1, ATTN_NORM );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "panthereye/pa_idle2.wav", 1, ATTN_NORM );	
		break;	
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "panthereye/pa_idle3.wav", 1, ATTN_NORM );	
		break;
	case 3:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "panthereye/pa_idle4.wav", 1, ATTN_NORM );	
		break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CDiablo :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case DIABLO_AE_CLAW_LEFT:
		{
			Claw(DIABLO_AE_CLAW_LEFT);
		}
		break;

		case DIABLO_AE_CLAW_RIGHT:
		{
			Claw(DIABLO_AE_CLAW_RIGHT);
		}
		break;

		case DIABLO_AE_JUMP:
		{
			Jump();
		}	
		break;
	}
}

//=========================================================
// IgnoreConditions 
//=========================================================
int CDiablo::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ( gpGlobals->time - m_flLastHurtTime <= 20 )
	{
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	if ( m_hEnemy != NULL )
	{
		if ( FClassnameIs( m_hEnemy->pev, "monster_headcrab" ) || FClassnameIs( m_hEnemy->pev, "monster_chumtoad" ) )
		{
			iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
		}
	}


	return iIgnore;
}

//=========================================================
// TakeDamage - overridden for diablo so we can keep track
// of how much time has passed since it was injured
//=========================================================
int CDiablo :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{

	if ( !FClassnameIs ( pevAttacker, "monster_headcrab" ) )
	{
		m_flLastHurtTime = gpGlobals->time;
	}

	if ( bitsDamageType & DMG_BULLET )
		flDamage *= 0.7;

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// LeapTouch - diablo's touch function for jump attack
// borrowed from headcrab
//=========================================================
void CDiablo :: LeapTouch ( CBaseEntity *pOther )
{
	if ( !pOther->pev->takedamage )
	{
		return;
	}

	// Don't hit if back on ground
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		
		pOther->TakeDamage( pev, pev, gSkillData.diabloDmgJump, DMG_SLASH );
	}

	SetTouch( NULL );
}

//=========================================================
// RunTask 
//=========================================================
void CDiablo :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
				SetTouch( NULL );
				m_IdealActivity = ACT_IDLE;
			}
			break;
		}
	default:
		{
			CBaseMonster :: RunTask(pTask);
		}
	}
}

void CDiablo :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
		//	SetTouch ( &CDiablo::LeapTouch );
			break;
		}
	default:
		{
			CBaseMonster :: StartTask( pTask );
		}
	}
}

Task_t	tlDiabloRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slDiabloRangeAttack1[] =
{
	{ 
		tlDiabloRangeAttack1,
		ARRAYSIZE ( tlDiabloRangeAttack1 ), 
		0,
		0,						// no interruptions, if we decided to jump, we jump no matter what
		"Diablo Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( CDiablo )
{
	slDiabloRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CDiablo, CBaseMonster );

Schedule_t* CDiablo :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK1:
		{
			return &slDiabloRangeAttack1[ 0 ];
		}
	default:
		{
			return CBaseMonster :: GetScheduleOfType ( Type );
		}
	}
}

BOOL CDiablo :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( m_flNextMelee > gpGlobals->time )
		return FALSE;

	if ( CBaseMonster::CheckMeleeAttack1( flDot, flDist ) == TRUE )
	{
		m_flNextMelee = gpGlobals->time + RANDOM_FLOAT(2.5, 3.5);
		return TRUE;
	}

	return FALSE;
}

BOOL CDiablo :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	// double claw is preferred
	if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		return FALSE;

	return CBaseMonster::CheckMeleeAttack2( flDot, flDist );
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CDiablo :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
		return FALSE;

	if ( flDist > DIABLO_JUMP_DIST_MAX )
		return FALSE;

	if ( flDist < DIABLO_JUMP_DIST_MIN )
		return FALSE;

	if ( flDot < 0.65 )
		return FALSE;

	if ( m_flNextAttack > gpGlobals->time )
		return FALSE;

	TraceResult tr;

	// get a rough idea about being able to reach enemy
	UTIL_TraceLine(pev->origin, m_hEnemy->EyePosition(), ignore_monsters, ignore_glass, edict(), &tr);

	if ( tr.flFraction == 1.0 )
		return TRUE;
	
	return FALSE;
}

void CDiablo :: Jump ( void )
{
	SetTouch ( &CDiablo::LeapTouch );

	ClearBits( pev->flags, FL_ONGROUND );

	UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
	UTIL_MakeVectors ( pev->angles );

	Vector vecJumpDir;
	if (m_hEnemy != NULL)
	{
		// Headcrab jump formula
		float gravity = g_psv_gravity->value;
		if (gravity <= 1)
			gravity = 1;

		float height = ( m_hEnemy->EyePosition().z - pev->origin.z);
		if (height < 16)
			height = 16;

		float speed = sqrt( 2 * gravity * height );
		float time = speed / gravity;

		vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
		vecJumpDir = vecJumpDir * ( 1.0 / time );

		vecJumpDir.z = speed;
		
		// Give some falloff to it
		vecJumpDir = vecJumpDir * 0.8;
	}
	else
	{
		// jump hop, don't care where
		vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
	}

	pev->velocity = vecJumpDir;
	m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(3.0, 7.5);
}

void CDiablo :: Claw ( int iClaw )
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.diabloDmgSlash, DMG_SLASH );

	if ( pHurt )
	{
		if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
		{
			int direction;

			if ( iClaw == DIABLO_AE_CLAW_RIGHT )
				direction = 1;
			else
				direction = -1;

			pHurt->pev->punchangle.z = -18;
			pHurt->pev->punchangle.x = 5 * direction;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100 * direction;
		}
		// Play a random attack hit sound
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
	}
	else // Play a random attack miss sound
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}
