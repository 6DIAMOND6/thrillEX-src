//=========================================================
// Control Sphere - living Xen turret
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SPHERE_DEPLOY = LAST_COMMON_SCHEDULE + 1,
	SCHED_SPHERE_RETIRE,
	SCHED_SPHERE_ANGRY_IDLE,
};

class CXenLaser : public CBaseAnimating
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( xenlaser, CXenLaser );

void CXenLaser :: Spawn( void )
{
	SET_MODEL(ENT(pev), "models/laser.mdl");
//	UTIL_SetSize(pev, Vector( -1, -1, -1 ), Vector( 1, 1, 1 )  ); // slightly bigger than point size
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
}

void CXenLaser::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CXenLaser *pLaser = GetClassPtr( (CXenLaser *)NULL );
	pLaser->Spawn();
	
	UTIL_SetOrigin( pLaser->pev, vecStart );
	pLaser->pev->velocity = vecVelocity;
	pLaser->pev->owner = ENT(pevOwner);
	pLaser->pev->angles = UTIL_VecToAngles( vecVelocity );
}

void CXenLaser :: Touch ( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage )
		pOther->TakeDamage ( pev, pev, gSkillData.slaveDmgZap, DMG_ENERGYBEAM );

	UTIL_EmitAmbientSound( ENT(pev), pev->origin, "weapons/electro4.wav", 0.3, ATTN_NORM, 0, RANDOM_LONG( 90, 99 ) );

	SetThink ( &CXenLaser::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define		SPHERE_LASER_SPEED	1200 // 700

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SPHERE_AE_SHOOT		( 1 )

class CXenSphere : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );

	void SetYawSpeed ( void ) { pev->yaw_speed = 100; }
	
	int	Classify ( void ) { return CLASS_ALIEN_MILITARY; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	
	void EXPORT SphereUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	Vector GetGunPosition( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void CheckAmmo ( void );

	void RunAI();

//	void StartTask ( Task_t *pTask );
//	void RunTask ( Task_t *pTask );

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );
	void Killed( entvars_t *pevAttacker, int iGib );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;

	static TYPEDESCRIPTION m_SaveData[];

	int m_iBodyGibs;
	BOOL m_fOn;
};

LINK_ENTITY_TO_CLASS( monster_alien_sphere, CXenSphere );

TYPEDESCRIPTION	CXenSphere::m_SaveData[] = 
{
	DEFINE_FIELD( CXenSphere, m_fOn, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CXenSphere, CBaseMonster );

void CXenSphere :: Spawn( void )
{
	m_fOn = FALSE;

	Precache();

	SET_MODEL(ENT(pev), "models/sphere.mdl");
	UTIL_SetSize(pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 )  );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_TOSS;
	pev->health			= 75;
	pev->effects		= 0;
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_bloodColor		= BLOOD_COLOR_PANTHER;

	MonsterInit();

	// UNDONE: set this in qc file
	pev->view_ofs		= Vector ( 0, 0, 16 );

	SetUse( &CXenSphere::SphereUse );
}

void CXenSphere :: Precache( void )
{
	PRECACHE_MODEL("models/sphere.mdl");
	PRECACHE_MODEL("models/laser.mdl");

	m_iBodyGibs = PRECACHE_MODEL( "models/shrapnel.mdl" );

	PRECACHE_SOUND("debris/beamstart4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
}

void CXenSphere :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case SPHERE_AE_SHOOT:
		{
			Vector	vecShootOffset;
			Vector	vecShootDir;

			UTIL_MakeVectors ( pev->angles );

			vecShootOffset = GetGunPosition();

			vecShootDir = (m_hEnemy->BodyTarget(vecShootOffset) - vecShootOffset);
			UTIL_DrawBeam( GetGunPosition(), m_hEnemy->BodyTarget(vecShootOffset), 128, 8, 32 );

		//	vecShootDir = vecShootDir + gpGlobals->v_forward * SPHERE_LASER_SPEED;
			float time = vecShootDir.Length( ) / 2100;
			vecShootDir = vecShootDir * (1.0 / time);

			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "debris/beamstart4.wav", 1, ATTN_NORM );

			CXenLaser::Shoot( pev, vecShootOffset, vecShootDir );
			m_flNextAttack = gpGlobals->time + 0.5;
			m_cAmmoLoaded = 0;

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_DLIGHT);		
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y + 16 );
				WRITE_COORD( pev->origin.z );
				WRITE_BYTE( 12 );	// radius * 0.1
				WRITE_BYTE( 255 );
				WRITE_BYTE( 48 );
				WRITE_BYTE( 192 );
				WRITE_BYTE( 4 );	// time * 10
				WRITE_BYTE( 8 );	// decay * 0.1
			MESSAGE_END();
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// TakeDamage - negate damage when shelled
//=========================================================
int CXenSphere :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( m_IdealActivity == ACT_IDLE_ANGRY )
		flDamage *= 0.6;

	return CBaseMonster :: TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SphereUse - turn on and off by trigger
//=========================================================
void CXenSphere :: SphereUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType == USE_ON )
	{
		m_fOn = TRUE;
		return;
	}

	if ( useType == USE_OFF )
	{
		m_hEnemy = NULL;
		m_fOn = FALSE;
		return;
	}

	if (m_fOn)
	{
		m_hEnemy = NULL;
		m_fOn = FALSE;
	}
	else m_fOn = TRUE;
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

Task_t	tlSphereDeploy[] =
{
	{ TASK_PLAY_SEQUENCE,		(float)ACT_ARM	},
};

Schedule_t	slSphereDeploy[] =
{
	{ 
		tlSphereDeploy,
		ARRAYSIZE ( tlSphereDeploy ), 
		0,
		0,
		"Sphere Deploy"
	},
};

Task_t	tlSphereRetire[] =
{
	{ TASK_PLAY_SEQUENCE,		(float)ACT_DISARM	},
};

Schedule_t	slSphereRetire[] =
{
	{ 
		tlSphereRetire,
		ARRAYSIZE ( tlSphereRetire ), 
		0,
		0,
		"Sphere Retire"
	},
};

//=========================================================
//	Idle Schedules
//=========================================================
Task_t	tlSphereAngryIdle[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE_ANGRY	},
	{ TASK_WAIT_INDEFINITE,		(float)0				},
};

Schedule_t	slSphereAngryIdle[] =
{
	{ 
		tlSphereAngryIdle,
		ARRAYSIZE ( tlSphereAngryIdle ), 
		bits_COND_CAN_ATTACK			|
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD,
		0,		

		"Sphere Angry Idle"
	},
};

//=========================================================
// AlertFace Schedules
//=========================================================
Task_t	tlSphereAlertFace[] =
{
	{ TASK_STOP_MOVING,				0						},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE_ANGRY	},
	{ TASK_FACE_IDEAL,				(float)0				},
};

Schedule_t	slSphereAlertFace[] =
{
	{ 
		tlSphereAlertFace,
		ARRAYSIZE ( tlSphereAlertFace ),
		bits_COND_NEW_ENEMY		|
		bits_COND_SEE_FEAR		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_PROVOKED,
		0,
		"Sphere Alert Face"
	},
};

//=========================================================
// CombatFace Schedule
//=========================================================
Task_t	tlSphereCombatFace[] =
{
	{ TASK_STOP_MOVING,				0						},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE_ANGRY	},
	{ TASK_FACE_ENEMY,				(float)0				},
};

Schedule_t	slSphereCombatFace[] =
{
	{ 
		tlSphereCombatFace,
		ARRAYSIZE ( tlSphereCombatFace ), 
		bits_COND_CAN_ATTACK			|
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD,
		0,
		"Sphere Combat Face"
	},
};

DEFINE_CUSTOM_SCHEDULES( CXenSphere ) 
{
	slSphereDeploy,
	slSphereRetire,
	slSphereAngryIdle,
	slSphereCombatFace,
	slSphereAlertFace
};

IMPLEMENT_CUSTOM_SCHEDULES( CXenSphere, CBaseMonster );

Vector CXenSphere :: GetGunPosition( )
{
	return EyePosition() + gpGlobals->v_forward * 16.0;
}

BOOL CXenSphere :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && m_flNextAttack < gpGlobals->time )
	{
		TraceResult	tr;
			
		Vector vecSrc = GetGunPosition();

		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
		//	ALERT(at_console, "Sphere: Can Range Attack\n"); 
			m_cAmmoLoaded = 1;
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckAmmo - this function is basically forcing range attack
// to be interrupted 
//=========================================================
void CXenSphere :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

void CXenSphere :: RunAI()
{
//	ALERT(at_console,"%i\n", m_IdealMonsterState);

	if ( m_MonsterState != MONSTERSTATE_NONE	&& 
	 m_MonsterState != MONSTERSTATE_PRONE   && 
	 m_MonsterState != MONSTERSTATE_DEAD )// don't bother with this crap if monster is prone. 
	{
		if ( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) || ( m_MonsterState == MONSTERSTATE_COMBAT ) )
		{
		//	can't see when carapace is closed
			if ( m_fOn )
				Look( m_flDistLook );

			Listen();

			ClearConditions( IgnoreConditions() );
			GetEnemy();
		}

		if ( m_hEnemy != NULL )
		{
			CheckEnemy( m_hEnemy );
		}

		CheckAmmo();

		FCheckAITrigger();

		PrescheduleThink();
	}

	MaintainSchedule();
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CXenSphere :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
		{
			if ( m_fOn )
			{
				ALERT(at_console, "Sphere: Retire\n");
				m_fOn = FALSE;
				return GetScheduleOfType ( SCHED_SPHERE_RETIRE );
			}

			return GetScheduleOfType( SCHED_IDLE_STAND );
		}
	case MONSTERSTATE_ALERT:
		{
			ALERT(at_console, "Steven\n");
			if ( !m_fOn )
			{
				ALERT(at_console, "Sphere: Deploy\n");
				m_fOn = TRUE;
				return GetScheduleOfType ( SCHED_SPHERE_DEPLOY );
			}

			if ( HasConditions ( bits_COND_HEAR_SOUND ) )
			{
				return GetScheduleOfType( SCHED_ALERT_FACE );
			}

			return GetScheduleOfType( SCHED_SPHERE_ANGRY_IDLE );

			break;
		}
	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_SEE_ENEMY) )
			{
				if ( HasConditions(bits_COND_CAN_RANGE_ATTACK1) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}

				return GetScheduleOfType( SCHED_COMBAT_FACE );
			}

			ALERT(at_console, "Sphere: Angry Idle\n");
			return GetScheduleOfType ( SCHED_SPHERE_ANGRY_IDLE );

			break;
		}
	}
	return CBaseMonster :: GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CXenSphere :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_SPHERE_DEPLOY:
		return &slSphereDeploy[ 0 ];
		break;
	case SCHED_SPHERE_RETIRE:
		return &slSphereRetire[ 0 ];
		break;
	case SCHED_SPHERE_ANGRY_IDLE:
		return &slSphereAngryIdle[ 0 ];
		break;
	case SCHED_CHASE_ENEMY:
	case SCHED_WAKE_ANGRY:
	case SCHED_COMBAT_FACE:
		return &slSphereCombatFace[ 0 ];
	case SCHED_ALERT_FACE:
		return &slSphereAlertFace[ 0 ];
		break;
	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}

void CXenSphere :: Killed( entvars_t *pevAttacker, int iGib )
{
	Vector vecSpot, vecDir;

	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	vecDir = gpGlobals->v_up * 16;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z );

			// size
			WRITE_COORD( pev->size.x );
			WRITE_COORD( pev->size.y );
			WRITE_COORD( pev->size.z );

			// velocity
			WRITE_COORD( vecDir.x ); 
			WRITE_COORD( vecDir.y );
			WRITE_COORD( vecDir.z );

			// randomization
			WRITE_BYTE( 10 ); 

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 12 );

			// duration
			WRITE_BYTE( 30 );

			// flags

			WRITE_BYTE( BREAK_METAL | FTENT_SMOKETRAIL );
	MESSAGE_END();

	Vector vecStreamDir = gpGlobals->v_up * 256.0;
	vecStreamDir.Normalize();

	for ( int i = 0; i < 4; i++ )
	{
		vecStreamDir.x += RANDOM_FLOAT( -0.6, 0.6 );
		vecStreamDir.y += RANDOM_FLOAT( -0.6, 0.6 );
		vecStreamDir.z += RANDOM_FLOAT( -0.6, 0.6 );

		UTIL_BloodStream( pev->origin, vecStreamDir, m_bloodColor, 400 );
	}

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);

	SetThink(&CXenSphere::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
