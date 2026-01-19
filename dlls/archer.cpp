//=========================================================
// archer - funny fish from the outer world
//=========================================================

#include	"float.h"
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"

// Animation events
#define ARCHER_AE_ATTACK		1
#define ARCHER_AE_FLOP			2
#define ARCHER_BEAM_ATTACK		3
#define ARCHER_BEAM_DONE		4

#define DEBUG_BEAMS		0

#if DEBUG_BEAMS
#include "effects.h"
#endif

// Movement constants

#define		ARCHER_ACCELERATE		10
#define		ARCHER_CHECK_DIST		45
#define		ARCHER_SWIM_SPEED		50
#define		ARCHER_SWIM_ACCEL		90
#define		ARCHER_SWIM_DECEL		15
#define		ARCHER_TURN_RATE		100//90
#define		ARCHER_SIZEX			10
#define		ARCHER_FRAMETIME		0.1
#define		ARCHER_MAX_BEAMS		2
#define		ARCHER_BEAM_LIFETIME	0.4

class CArcher : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );

	void EXPORT SwimThink( void );
	void EXPORT DeadThink( void );
	void Touch( CBaseEntity *pOther )
	{
		if ( pOther->IsPlayer() )
		{
			// If the client is pushing me, give me some base velocity
			if ( gpGlobals->trace_ent && gpGlobals->trace_ent == edict() )
			{
				pev->basevelocity = pOther->pev->velocity;
				pev->flags |= FL_BASEVELOCITY;
			}
		}
	}
	
	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector(-32,-12,-11);
		pev->absmax = pev->origin + Vector(32,12,11);
	}

	void AttackSound( void );
	void AlertSound( void );
	void UpdateMotion( void );
	float ObstacleDistance( CBaseEntity *pTarget );
	void MakeVectors( void );
	void RecalculateWaterlevel( void );
	void SwitchArcherState( void );

	// Base entity functions
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int	BloodColor( void ) { return BLOOD_COLOR_GREEN; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void Activate( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	int	Classify( void ) { return CLASS_INSECT; }
	int IRelationship( CBaseEntity *pTarget );

	void BeamAttack( int side );
	void ClearBeams( );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];

	private:
	// UNDONE: Remove unused boid vars, do group behavior
	float	m_flTurning;// is this boid turning?
	BOOL	m_fPathBlocked;// TRUE if there is an obstacle ahead
	float	m_flAccelerate;
	float	m_obstacle;
	float	m_top;
	float	m_bottom;
	float	m_height;
	float	m_waterTime;
	float	m_sideTime;		// Timer to randomly check clearance on sides
	float	m_zTime;
	float	m_stateTime;
	float	m_attackSoundTime;

	CBeam *m_pBeam[ARCHER_MAX_BEAMS];

	int		m_iBeams;
	float	m_flNextBeamCleanup;
};

LINK_ENTITY_TO_CLASS( monster_archer, CArcher );
LINK_ENTITY_TO_CLASS( monster_archerfish, CArcher );	// beta backward compatibility

TYPEDESCRIPTION	CArcher::m_SaveData[] = 
{
	DEFINE_FIELD( CArcher, m_flTurning, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_fPathBlocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( CArcher, m_flAccelerate, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_obstacle, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_top, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_bottom, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_height, FIELD_FLOAT ),
	DEFINE_FIELD( CArcher, m_waterTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_sideTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_zTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_stateTime, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_attackSoundTime, FIELD_TIME ),
	DEFINE_ARRAY( CArcher, m_pBeam, FIELD_CLASSPTR, ARCHER_MAX_BEAMS ),
	DEFINE_FIELD( CArcher, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CArcher, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CArcher, m_flNextBeamCleanup, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CArcher, CBaseMonster );

const char *CArcher::pAttackSounds[] =	
{
	"snapbug/catch.wav",
};

const char *CArcher::pAlertSounds[] =
{
	"leech/leech_alert1.wav",
	"leech/leech_alert2.wav",
};

void CArcher::Spawn( void )
{
	Precache();
	SET_MODEL(ENT(pev), "models/archer.mdl");
	UTIL_SetSize( pev, Vector(-12, -12, 0), Vector(12, 12, 12));
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	SetBits(pev->flags, FL_SWIM);
	pev->health			= gSkillData.archerHealth;

	m_flFieldOfView		= VIEW_FIELD_FULL;	
	m_flDistLook		= 750;
	MonsterInit();
	SetThink( &CArcher::SwimThink );
	SetUse( NULL );
	SetTouch( NULL );
	pev->view_ofs = g_vecZero;

	m_flTurning = 0;
	m_fPathBlocked = FALSE;
	SetActivity( ACT_WALK );
	SetState( MONSTERSTATE_IDLE );
	m_stateTime = gpGlobals->time + RANDOM_FLOAT( 1, 5 );
}

void CArcher::Activate( void )
{
	RecalculateWaterlevel();
}

void CArcher::RecalculateWaterlevel( void )
{
	// Calculate boundaries
	Vector vecTest = pev->origin - Vector(0,0,400);

	TraceResult tr;

	UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
	if ( tr.flFraction != 1.0 )
		m_bottom = tr.vecEndPos.z + 1;
	else
		m_bottom = vecTest.z;

	m_top = UTIL_WaterLevel( pev->origin, pev->origin.z, pev->origin.z + 400 ) - 1;

	// Chop off 20% of the outside range
	float newBottom = m_bottom * 0.8 + m_top * 0.2;
	m_top = m_bottom * 0.2 + m_top * 0.8;
	m_bottom = newBottom;
	m_height = RANDOM_FLOAT( m_bottom, m_top );
	m_waterTime = gpGlobals->time + RANDOM_FLOAT( 5, 7 );
}

void CArcher::SwitchArcherState( void )
{
	m_stateTime = gpGlobals->time + RANDOM_FLOAT( 2, 4 );
	if ( m_MonsterState == MONSTERSTATE_COMBAT )
	{
		m_hEnemy = NULL;
		SetState( MONSTERSTATE_IDLE );
		// We may be up against the player, so redo the side checks
		m_sideTime = 0;
	}
	else
	{
		CBaseEntity *pEnemy = BestVisibleEnemy();
		//if ( pEnemy && pEnemy->pev->waterlevel != 0 )
		if ( pEnemy && pEnemy->IsAlive() )
		{
			m_hEnemy = pEnemy;
			SetState( MONSTERSTATE_COMBAT );
			m_stateTime = gpGlobals->time + RANDOM_FLOAT( 18, 25 );
			AlertSound();
		}
	}
}

int CArcher::IRelationship( CBaseEntity *pTarget )
{
	if ( pTarget->IsPlayer() )
		return R_DL;
	return CBaseMonster::IRelationship( pTarget );
}

void CArcher::AttackSound( void )
{
	if ( gpGlobals->time > m_attackSoundTime )
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, PITCH_NORM );
		m_attackSoundTime = gpGlobals->time + 0.5;
	}
}


void CArcher::AlertSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM * 0.5, 0, PITCH_NORM );
}

void CArcher::Precache( void )
{
	int i;

	PRECACHE_MODEL("models/archer.mdl");

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);
	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_MODEL("sprites/lgtning.spr");
}

int CArcher::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	pev->velocity = g_vecZero;

	if ( pevInflictor )
	{
		pev->velocity = (pev->origin - pevInflictor->origin).Normalize() * 25;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CArcher::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ARCHER_AE_ATTACK:
		AttackSound();
		CBaseEntity *pEnemy;

		pEnemy = m_hEnemy;
		if ( pEnemy != NULL )
		{
			Vector dir, face;

			UTIL_MakeVectorsPrivate( pev->angles, face, NULL, NULL );
			face.z = 0;
			dir = (pEnemy->pev->origin - pev->origin);
			dir.z = 0;
			dir = dir.Normalize();
			face = face.Normalize();

			
			if ( DotProduct(dir, face) > 0.9 )		// Only take damage if the leech is facing the prey
				pEnemy->TakeDamage( pev, pev, gSkillData.archerDmgBite, DMG_SLASH );
		}
		m_stateTime -= 2;
		break;
	
	case ARCHER_AE_FLOP:
		// Play flop sound
		break;

	case ARCHER_BEAM_ATTACK:
		{
	//	UTIL_ParticleEffect ( pev->origin, g_vecZero, 255, 25 );
	//	UTIL_EmitAmbientSound( ENT(pev), pev->origin, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
		ClearBeams( );

		ClearMultiDamage();

		UTIL_MakeAimVectors( pev->angles );

		BeamAttack( -1 );
		BeamAttack( 1 );

		Vector vecSrc;

		vecSrc = pev->origin;
		vecSrc.x += 24;
		vecSrc.z += 16;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(vecSrc.x);	// X
			WRITE_COORD(vecSrc.y);	// Y
			WRITE_COORD(vecSrc.z);	// Z
			WRITE_BYTE( 20 );		// radius * 0.1
			WRITE_BYTE( 32 );		// r
			WRITE_BYTE( 96 );		// g
			WRITE_BYTE( 255 );		// b
			WRITE_BYTE( 10 );		// time * 10
			WRITE_BYTE( 8 );		// decay * 0.1
		MESSAGE_END( );

		ApplyMultiDamage(pev, pev);

		m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( gSkillData.archerMinBeamDelay, gSkillData.archerMinBeamDelay + 5.0 );
		m_flNextBeamCleanup = gpGlobals->time + ARCHER_BEAM_LIFETIME;
		}
		break;

	case ARCHER_BEAM_DONE:
		ClearBeams();
		break;
	
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CArcher::MakeVectors( void )
{
	Vector tmp = pev->angles;
	tmp.x = -tmp.x;
	UTIL_MakeVectors ( tmp );
}

float CArcher::ObstacleDistance( CBaseEntity *pTarget )
{
	TraceResult		tr;
	Vector			vecTest;

	// use VELOCITY, not angles, not all boids point the direction they are flying
	//Vector vecDir = UTIL_VecToAngles( pev->velocity );
	MakeVectors();

	// check for obstacle ahead
	vecTest = pev->origin + gpGlobals->v_forward * ARCHER_CHECK_DIST;
	UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);

	if ( tr.fStartSolid )
	{
		pev->speed = -ARCHER_SWIM_SPEED * 0.5;
//		ALERT( at_console, "Stuck from (%f %f %f) to (%f %f %f)\n", pev->oldorigin.x, pev->oldorigin.y, pev->oldorigin.z, pev->origin.x, pev->origin.y, pev->origin.z );
//		UTIL_SetOrigin( pev, pev->oldorigin );
	}

	if ( tr.flFraction != 1.0 )
	{
		if ( (pTarget == NULL || tr.pHit != pTarget->edict()) )
		{
			return tr.flFraction;
		}
		else
		{
			if ( fabs(m_height - pev->origin.z) > 10 )
				return tr.flFraction;
		}
	}

	if ( m_sideTime < gpGlobals->time )
	{
		// extra wide checks
		vecTest = pev->origin + gpGlobals->v_right * ARCHER_SIZEX * 2 + gpGlobals->v_forward * ARCHER_CHECK_DIST;
		UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
		if (tr.flFraction != 1.0)
			return tr.flFraction;

		vecTest = pev->origin - gpGlobals->v_right * ARCHER_SIZEX * 2 + gpGlobals->v_forward * ARCHER_CHECK_DIST;
		UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
		if (tr.flFraction != 1.0)
			return tr.flFraction;

		// Didn't hit either side, so stop testing for another 0.5 - 1 seconds
		m_sideTime = gpGlobals->time + RANDOM_FLOAT(0.5,1);
	}
	return 1.0;
}

void CArcher::DeadThink( void )
{
	if ( m_fSequenceFinished )
	{
		if ( m_Activity == ACT_DIESIMPLE )
		{
			SetThink( NULL );
			StopAnimation();
			return;
		}
		else if ( pev->flags & FL_ONGROUND )
		{
			pev->solid = SOLID_NOT;
			SetActivity(ACT_DIESIMPLE);
		}
	}
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	// Apply damage velocity, but keep out of the walls
	if ( pev->velocity.x != 0 || pev->velocity.y != 0 )
	{
		TraceResult tr;

		// Look 0.5 seconds ahead
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 0.5, missile, edict(), &tr);
		if (tr.flFraction != 1.0)
		{
			pev->velocity.x = 0;
			pev->velocity.y = 0;
		}
	}
}

void CArcher::UpdateMotion( void )
{
	float flapspeed = (pev->speed - m_flAccelerate) / ARCHER_ACCELERATE;
	m_flAccelerate = m_flAccelerate * 0.8 + pev->speed * 0.2;

	if (flapspeed < 0) 
		flapspeed = -flapspeed;
	flapspeed += 1.0;
	if (flapspeed < 0.5) 
		flapspeed = 0.5;
	if (flapspeed > 1.9) 
		flapspeed = 1.9;

	pev->framerate = flapspeed;

	if ( !m_fPathBlocked )
		pev->avelocity.y = pev->ideal_yaw;
	else
		pev->avelocity.y = pev->ideal_yaw * m_obstacle;

	if ( pev->avelocity.y > 150 )
		m_IdealActivity = ACT_TURN_LEFT;
	else if ( pev->avelocity.y < -150 )
		m_IdealActivity = ACT_TURN_RIGHT;
	else if ( pev->speed > ARCHER_SWIM_SPEED )	
	{
		m_IdealActivity = ACT_RUN;
	}
	else
	{
		m_IdealActivity = ACT_WALK;
	}

	// lean
	float targetPitch, delta;
	delta = m_height - pev->origin.z;

	if ( delta < -10 )
		targetPitch = -30;
	else if ( delta > 10 )
		targetPitch = 30;
	else
		targetPitch = 0;

	pev->angles.x = UTIL_Approach( targetPitch, pev->angles.x, 60 * ARCHER_FRAMETIME );

	// bank
	pev->avelocity.z = - (pev->angles.z + (pev->avelocity.y * 0.25));

	BOOL canHit;

	if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && HasConditions( bits_COND_SEE_ENEMY) && m_flNextAttack < gpGlobals->time )
	{
		Vector vecSrc, vecAim;
		TraceResult tr;
		CBaseEntity* pEntity = NULL;

		GetAttachment ( 0, vecSrc, vecAim );
		vecAim = m_hEnemy->pev->origin - vecSrc;

		if( DotProduct( vecAim, gpGlobals->v_forward ) > 0 )
		{
			Vector vecSrc;

			vecSrc = pev->origin;
			vecSrc.x += 24;
			vecSrc.z += 16;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE(TE_DLIGHT);
				WRITE_COORD(vecSrc.x);	// X
				WRITE_COORD(vecSrc.y);	// Y
				WRITE_COORD(vecSrc.z);	// Z
				WRITE_BYTE( 10 );		// radius * 0.1
				WRITE_BYTE( 32 );		// r
				WRITE_BYTE( 96 );		// g
				WRITE_BYTE( 255 );		// b
				WRITE_BYTE( 5 );		// time * 10
				WRITE_BYTE( 16 );		// decay * 0.1
			MESSAGE_END( );
		}

		UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr);
		pEntity = CBaseEntity::Instance(tr.pHit);

		if ( pEntity != NULL && pEntity == m_hEnemy )
			canHit = TRUE;	
	}
	else canHit = FALSE;

	if ( m_MonsterState == MONSTERSTATE_COMBAT && canHit )
	{
		CBaseEntity *pEnemy;

		pEnemy = m_hEnemy;

	//	UTIL_ParticleEffect ( pev->origin, g_vecZero, 128, 25 );

		Vector dir, face;

		UTIL_MakeVectorsPrivate( pev->angles, face, NULL, NULL );
		face.z = 0;
		dir = (pEnemy->pev->origin - pev->origin);
		dir.z = 0;
		dir = dir.Normalize();
		face = face.Normalize();

		if ( DotProduct(dir, face) > 0.9 )
		{
			ClearBeams();
			m_IdealActivity = ACT_RANGE_ATTACK1;
		}
	}	
	else if ( m_MonsterState == MONSTERSTATE_COMBAT && HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
	{
		m_IdealActivity = ACT_MELEE_ATTACK1;
	}

	// Out of water check
	if ( !pev->waterlevel )
	{
		pev->movetype = MOVETYPE_TOSS;
		m_IdealActivity = ACT_TWITCH;
		pev->velocity = g_vecZero;

		// Animation will intersect the floor if either of these is non-zero
		pev->angles.z = 0;
		pev->angles.x = 0;

		if ( pev->framerate < 1.0 )
			pev->framerate = 1.0;
	}
	else if ( pev->movetype == MOVETYPE_TOSS )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->flags &= ~FL_ONGROUND;
		RecalculateWaterlevel();
		m_waterTime = gpGlobals->time + 2;	// Recalc again soon, water may be rising
	}

	if ( m_Activity != m_IdealActivity )
	{
		SetActivity ( m_IdealActivity );
	}
	float flInterval = StudioFrameAdvance();
	DispatchAnimEvents ( flInterval );

#if DEBUG_BEAMS
	if ( !m_pb )
		m_pb = CBeam::BeamCreate( "sprites/laserbeam.spr", 5 );
	if ( !m_pt )
		m_pt = CBeam::BeamCreate( "sprites/laserbeam.spr", 5 );
	m_pb->PointsInit( pev->origin, pev->origin + gpGlobals->v_forward * ARCHER_CHECK_DIST );
	m_pt->PointsInit( pev->origin, pev->origin - gpGlobals->v_right * (pev->avelocity.y*0.25) );
	if ( m_fPathBlocked )
	{
		float color = m_obstacle * 30;
		if ( m_obstacle == 1.0 )
			color = 0;
		if ( color > 255 )
			color = 255;
		m_pb->SetColor( 255, (int)color, (int)color );
	}
	else
		m_pb->SetColor( 255, 255, 0 );
	m_pt->SetColor( 0, 0, 255 );
#endif
}

void CArcher::SwimThink( void )
{
	TraceResult		tr;
	float			flLeftSide;
	float			flRightSide;
	float			targetSpeed;
	float			targetYaw = 0;
	CBaseEntity		*pTarget;

	if ( m_flNextBeamCleanup <= gpGlobals->time )
		ClearBeams();

	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
	{
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(1,1.5);
		pev->velocity = g_vecZero;
		return;
	}
	else
		pev->nextthink = gpGlobals->time + 0.1;

	targetSpeed = ARCHER_SWIM_SPEED;

	if ( m_waterTime < gpGlobals->time )
		RecalculateWaterlevel();

	if ( m_stateTime < gpGlobals->time )
		SwitchArcherState();

	ClearConditions( bits_COND_CAN_MELEE_ATTACK1 );
	ClearConditions( bits_COND_CAN_RANGE_ATTACK1 );

	Look( m_flDistLook );

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		pTarget = m_hEnemy;
		if ( !pTarget || !(pTarget->IsAlive()) )
			SwitchArcherState();
		else
		{
			// Chase the enemy's eyes
			m_height = pTarget->pev->origin.z + pTarget->pev->view_ofs.z - 5;
			// Clip to viable water area
			if ( m_height < m_bottom )
				m_height = m_bottom;
			else if ( m_height > m_top )
				m_height = m_top;
			Vector location = pTarget->pev->origin - pev->origin;
			location.z += (pTarget->pev->view_ofs.z);
			if ( location.Length() < 60 && pTarget->pev->waterlevel != 0 )
				SetConditions( bits_COND_CAN_MELEE_ATTACK1 );
			else if ( location.Length() >= 60 )
				SetConditions( bits_COND_CAN_RANGE_ATTACK1 );
			// Turn towards target ent
			targetYaw = UTIL_VecToYaw( location );

			targetYaw = UTIL_AngleDiff( targetYaw, UTIL_AngleMod( pev->angles.y ) );

			if ( targetYaw < (-ARCHER_TURN_RATE*0.75) )
				targetYaw = (-ARCHER_TURN_RATE*0.75);
			else if ( targetYaw > (ARCHER_TURN_RATE*0.75) )
				targetYaw = (ARCHER_TURN_RATE*0.75);
			else
				targetSpeed *= 3;
		}

		break;

	default:
		if ( m_zTime < gpGlobals->time )
		{
			float newHeight = RANDOM_FLOAT( m_bottom, m_top );
			m_height = 0.5 * m_height + 0.5 * newHeight;
			m_zTime = gpGlobals->time + RANDOM_FLOAT( 1, 4 );
		}
		if ( RANDOM_LONG( 0, 100 ) < 10 )
			targetYaw = RANDOM_LONG( -30, 30 );
		pTarget = NULL;
		// oldorigin test
		if ( (pev->origin - pev->oldorigin).Length() < 1 )
		{
			// If leech didn't move, there must be something blocking it, so try to turn
			m_sideTime = 0;
		}

		break;
	}

	m_obstacle = ObstacleDistance( pTarget );
	pev->oldorigin = pev->origin;
	if ( m_obstacle < 0.1 )
		m_obstacle = 0.1;

	// is the way ahead clear?
	if ( m_obstacle == 1.0 )
	{
		// if the leech is turning, stop the trend.
		if ( m_flTurning != 0 )
		{
			m_flTurning = 0;
		}

		m_fPathBlocked = FALSE;
		pev->speed = UTIL_Approach( targetSpeed, pev->speed, ARCHER_SWIM_ACCEL * ARCHER_FRAMETIME );
		pev->velocity = gpGlobals->v_forward * pev->speed;

	}
	else
	{
		m_obstacle = 1.0 / m_obstacle;
		// IF we get this far in the function, the leader's path is blocked!
		m_fPathBlocked = TRUE;

		if ( m_flTurning == 0 )// something in the way and leech is not already turning to avoid
		{
			Vector vecTest;
			// measure clearance on left and right to pick the best dir to turn
			vecTest = pev->origin + (gpGlobals->v_right * ARCHER_SIZEX) + (gpGlobals->v_forward * ARCHER_CHECK_DIST);
			UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
			flRightSide = tr.flFraction;

			vecTest = pev->origin + (gpGlobals->v_right * -ARCHER_SIZEX) + (gpGlobals->v_forward * ARCHER_CHECK_DIST);
			UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
			flLeftSide = tr.flFraction;

			// turn left, right or random depending on clearance ratio
			float delta = (flRightSide - flLeftSide);
			if ( delta > 0.1 || (delta > -0.1 && RANDOM_LONG(0,100)<50) )
				m_flTurning = -ARCHER_TURN_RATE;
			else
				m_flTurning = ARCHER_TURN_RATE;
		}
		pev->speed = UTIL_Approach( -(ARCHER_SWIM_SPEED*0.5), pev->speed, ARCHER_SWIM_DECEL * ARCHER_FRAMETIME * m_obstacle );
		pev->velocity = gpGlobals->v_forward * pev->speed;
	}
	pev->ideal_yaw = m_flTurning + targetYaw;
	UpdateMotion();
}

//=========================================================
// BeamAttack - borrowed from ISlave
//=========================================================
void CArcher::BeamAttack( int side )
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	if (m_iBeams >= ARCHER_MAX_BEAMS)
		return;

//	vecSrc = pev->origin + gpGlobals->v_up * 36;
	GetAttachment ( 0, vecSrc, vecAim );
//	vecAim = ShootAtEnemy( vecSrc );
//	vecAim = gpGlobals->v_forward * 1;
	vecAim = m_hEnemy->pev->origin - vecSrc;
//	float deflection = 0.01;
//	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( 0 );
	m_pBeam[m_iBeams]->SetColor( 32, 96, 255 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && pEntity->pev->takedamage)
	{
		pEntity->TraceAttack( pev, gSkillData.archerDmgZap, vecAim, &tr, DMG_SHOCK );
	}
	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CArcher :: ClearBeams( )
{
	for (int i = 0; i < ARCHER_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;

	STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
}

void CArcher::Killed(entvars_t *pevAttacker, int iGib)
{
	Vector			vecSplatDir;
	TraceResult		tr;

	ClearBeams();

	if	( ShouldGibMonster( iGib ) )
	{
		CallGibMonster();
		return;
	}

	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if (pOwner)
		pOwner->DeathNotice(pev);

	// When we hit the ground, play the "death_end" activity
	if ( pev->waterlevel )
	{
		pev->angles.z = 0;
		pev->angles.x = 0;
		pev->origin.z += 1;
		pev->avelocity = g_vecZero;
		if ( RANDOM_LONG( 0, 99 ) < 70 )
			pev->avelocity.y = RANDOM_LONG( -720, 720 );

		pev->gravity = 0.02;
		ClearBits(pev->flags, FL_ONGROUND);
		SetActivity( ACT_DIESIMPLE );
	}
	else
		SetActivity( ACT_DIEFORWARD );
	
	pev->movetype = MOVETYPE_TOSS;
	SetThink( &CArcher::DeadThink );
}
