//=========================================================
// Kingpin - 3d archvile reincarnation
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"game.h"

#define KINGPIN_MELEE_RANGE			96

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define KINGPIN_AE_MELEE			( 1 )
#define	KINGPIN_AE_RANGE_CHARGE		( 2 )
#define	KINGPIN_AE_RANGE_ATTACK		( 3 )
#define KINGPIN_AE_BLOWUP			( 4 )

class CKingpin : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify ( void );
	int IRelationship( CBaseEntity *pTarget );
	
	void AlertSound( void );
	void IdleSound( void );

	void SetYawSpeed ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void RunTask ( Task_t *pTask );
	Schedule_t *GetSchedule( void );
	Schedule_t* GetScheduleOfType ( int Type ) ;
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	void RangeAttack( CBaseEntity *pTarget );

	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	void EXPORT DyingThink( void );
	void EXPORT Blowup( void );

	CUSTOM_SCHEDULES;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
};

LINK_ENTITY_TO_CLASS ( monster_kingpin, CKingpin );

const char *CKingpin::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CKingpin::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

void CKingpin :: Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/kingpin.mdl");

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	m_afCapability		= bits_CAP_RANGE_ATTACK1 | bits_CAP_MELEE_ATTACK1;

	m_MonsterState		= MONSTERSTATE_NONE;

	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 112 ) );

	pev->health			= gSkillData.kingpinHealth;
	m_flFieldOfView		= VIEW_FIELD_FULL;	// kingpin has four eyes, so it can see around 360 degrees

	MonsterInit();
}

void CKingpin :: Precache( void )
{
	PRECACHE_MODEL("models/kingpin.mdl");
	PRECACHE_SOUND("kingpin/ki_alert1.wav");

	PRECACHE_SOUND("kingpin/ki_idle1.wav");
	PRECACHE_SOUND("kingpin/ki_idle2.wav");
	PRECACHE_SOUND("kingpin/ki_attack1.wav");

	int i;

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	UTIL_PrecacheOther( "test_effect" );
}

int CKingpin :: Classify( void )
{
	return	CLASS_ALIEN_MONSTER;
}

int CKingpin::IRelationship( CBaseEntity *pTarget )
{
	if ( (pTarget->IsPlayer()) )
		if ( (pev->spawnflags & SF_MONSTER_WAIT_UNTIL_PROVOKED ) && ! (m_afMemory & bits_MEMORY_PROVOKED ))
			return R_NO;
	return CBaseMonster::IRelationship( pTarget );
}

void CKingpin :: AlertSound( void )
{
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "kingpin/ki_alert1.wav", 1, ATTN_NORM );
}

void CKingpin :: IdleSound( void )
{
	if ( RANDOM_LONG(0,1) )
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "kingpin/ki_idle1.wav", 1, ATTN_NORM );
	else
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "kingpin/ki_idle2.wav", 1, ATTN_NORM );
}

void CKingpin :: SetYawSpeed ( void )
{
	pev->yaw_speed = 100;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CKingpin :: HandleAnimEvent( MonsterEvent_t *pEvent )
{	

	switch( pEvent->event )
	{
	case KINGPIN_AE_MELEE:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( KINGPIN_MELEE_RANGE, gSkillData.kingpinDmgClaw, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		}
		break;
	case KINGPIN_AE_RANGE_CHARGE:
		if ( m_hEnemy != NULL )
		{
		}
		break;
	case KINGPIN_AE_RANGE_ATTACK:
		if ( m_hEnemy != NULL )
			RangeAttack(m_hEnemy);
		break;
	case KINGPIN_AE_BLOWUP:
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}

}

// primary range attack
Task_t	tlKingpinRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slKingpinRangeAttack1[] =
{
	{ 
		tlKingpinRangeAttack1,
		ARRAYSIZE ( tlKingpinRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"Kingpin Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( CKingpin )
{
	slKingpinRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CKingpin, CBaseMonster );

void CKingpin :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_RANGE_ATTACK1:
		{
			Vector vecSrc, vecDir;

			if ( m_hEnemy != NULL )
			{
				if ( m_hEnemy->pev->flags & FL_CLIENT )
				{
					m_hEnemy->pev->velocity.x /= 2;
					m_hEnemy->pev->velocity.y /= 2;
				}
				else if ( m_hEnemy->pev->flags & FL_MONSTER )
				{
					// stun enemies
					m_hEnemy->MyMonsterPointer()->m_MonsterState = MONSTERSTATE_NONE;
				}
				
				vecSrc = m_hEnemy->EyePosition();
				vecSrc.z += 8;
				vecDir = pev->origin - m_hEnemy->pev->origin;

				UTIL_ParticleEffect(vecSrc, vecDir, 238, 32);

				MakeIdealYaw( m_vecEnemyLKP );

				ChangeYaw( pev->yaw_speed );
			}
			else TaskFail();

			if ( m_fSequenceFinished )
			{
				m_Activity = ACT_RESET;
				TaskComplete();
			}
			break;
		}
		default:
		{
			CBaseMonster :: RunTask(pTask);
			break;
		}
	}
}

Schedule_t *CKingpin :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if (HasConditions(bits_COND_LIGHT_DAMAGE) || HasConditions(bits_COND_HEAVY_DAMAGE))
				return GetScheduleOfType( SCHED_COMBAT_FACE );	// dont flinch

			return CBaseMonster :: GetSchedule();
		}
		break;
	default:
		{
			return CBaseMonster :: GetSchedule();
		}
		break;
	}
}

Schedule_t* CKingpin :: GetScheduleOfType ( int Type ) 
{
	if ( Type == SCHED_RANGE_ATTACK1 )
		return &slKingpinRangeAttack1[ 0 ];
	
	return CBaseMonster::GetScheduleOfType(Type);
}

BOOL CKingpin :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist < 128 )
		return TRUE;

	return FALSE;
}

void CKingpin :: RangeAttack( CBaseEntity *pTarget )
{
	Vector vecDir;
	entvars_t *pevTarget;

	vecDir = pev->origin + gpGlobals->v_forward * 192;
	vecDir.z = gpGlobals->v_up.z * 128;

	pevTarget = VARS(pTarget->edict());
	pevTarget->punchangle.z = -18;
	pevTarget->punchangle.x = 5;
	pevTarget->basevelocity = vecDir;
	pTarget->TakeDamage( pev, pev, gSkillData.kingpinDmgZap, DMG_GENERIC );

	EMIT_SOUND( ENT(pev), CHAN_VOICE, "kingpin/ki_attack1.wav", 1, ATTN_NORM );
}

//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CKingpin :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if ( pevAttacker->flags & FL_CLIENT ) // i trusted you, but you betrayed me
		m_afMemory |= bits_MEMORY_PROVOKED;

	if ( bitsDamageType == DMG_CLUB || bitsDamageType == DMG_BULLET )
		flDamage *= 0.7;

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CKingpin :: Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster :: Killed (pevAttacker, GIB_NEVER);
	
	CBaseEntity *pPort = CBaseEntity::Create( "test_effect", Vector ( pev->origin.x, pev->origin.y, pev->origin.z + 32.0 ), g_vecZero, NULL );
	pPort->Use(this, this, USE_ON, 0);

	pev->renderamt = 255;
	pev->rendermode = kRenderTransTexture;
	pev->takedamage = DAMAGE_NO;

	SetThink(&CKingpin::DyingThink);
	pev->nextthink = gpGlobals->time;
}

void CKingpin :: DyingThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->renderamt > 80 )
	{
		pev->renderamt -= 5;

	//	keep the scheduling going
		RunAI();

		return;
	}

	pev->renderfx = kRenderFxExplode;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 0;
	pev->rendercolor.z = 0;	

	SetThink(&CKingpin::Blowup);
}

void CKingpin :: Blowup( void )
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);
	pev->solid = SOLID_NOT;

	CGib::SpawnRandomGibs( pev, 9, MainGib() );

	for ( int i = 0; i < 8; i++ )
	{	
		UTIL_BloodStream( pev->origin + Vector( 0, 0, 48 ), UTIL_RandomBloodVector(), BloodColor(), RANDOM_LONG( 80, 150 ));
	}

	SetThink(&CKingpin::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}