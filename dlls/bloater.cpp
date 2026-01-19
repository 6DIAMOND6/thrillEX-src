/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Bloater - controller's smaller brother
//=========================================================

// TODO: Add proper group behavior.

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"xbowbolt.h"
#include	"weapons.h"
#include	"soundent.h"


//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	BLOATER_MAX_SPEED			150
#define DIST_TO_CHECK				200

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BLOATER_AE_ATTACK_MELEE1		0x01

class CBloater : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void MakeIdealPitch( Vector vecTarget );
	float FlPitchDiff( void );
	float ChangePitch( int speed );
	int  Classify ( void );
	int IRelationship( CBaseEntity *pTarget );
	void CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation );
//	void HandleAnimEvent( MonsterEvent_t *pEvent );
	Vector EyePosition( ) { return pev->origin + Vector(0,0,36); }
	Vector BodyTarget( const Vector &posSrc ){ return Center( ); }

//	void PainSound( void );
	void AlertSound( void );
//	void IdleSound( void );
//	void AttackSnd( void );

	void RunAI ( void );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	Schedule_t* GetSchedule ( void );
	Schedule_t* GetScheduleOfType ( int Type );
//	void SetActivity ( Activity NewActivity );

	BOOL ShouldAdvanceRoute( float flWaypointDist );
	int  CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );
	void Move ( float flInterval );
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void Stop( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }

	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist ) { return FALSE; }
	void Shoot( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void Killed( entvars_t *pevAttacker, int iGib );

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_bloater, CBloater );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBloater :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

int CBloater::IRelationship( CBaseEntity *pTarget )
{
	if ( (pTarget->IsPlayer()) )
		if ( ! (m_afMemory & bits_MEMORY_PROVOKED ))
			if ( (pev->origin - pTarget->pev->origin).Length() > 512 )
				return R_NO;
			else m_afMemory |= bits_MEMORY_PROVOKED;

	return CSquadMonster::IRelationship( pTarget );
}

//=========================================================
// ALertSound - scream
//=========================================================
void CBloater :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		CallForHelp( "monster_bloater", 768, m_hEnemy, m_vecEnemyLKP );
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBloater :: SetYawSpeed ( void )
{
	pev->yaw_speed = 60;
}

void CBloater :: MakeIdealPitch( Vector vecTarget )
{
	Vector vecToForward, vecToTarget;
	float pitchCos;

	vecToTarget = vecTarget - pev->origin;
	vecToForward = vecToTarget;
	vecToForward.z = pev->origin.z;

	pitchCos = (pow(vecToForward.Length(), 2) + pow(vecToTarget.Length(),2) - ( vecToTarget.Length() - vecToForward.Length() ))/(2 * vecToForward.Length() * vecToTarget.Length());
	
	pev->idealpitch = acos(pitchCos);
}

float CBloater::FlPitchDiff( void )
{
	float	flPitchDiff;
	float	flCurrentPitch;

	flCurrentPitch = UTIL_AngleMod( pev->angles.z );

	if ( flCurrentPitch == pev->idealpitch )
	{
		return 0;
	}

	flPitchDiff = pev->idealpitch - flCurrentPitch;

	if ( pev->idealpitch > flCurrentPitch )
	{
		if (flPitchDiff >= 180)
			flPitchDiff = flPitchDiff - 360;
	}
	else 
	{
		if (flPitchDiff <= -180)
			flPitchDiff = flPitchDiff + 360;
	}
	return flPitchDiff;
}

float CBloater :: ChangePitch( int speed )
{
	float diff = FlPitchDiff();
	float target = 0;
	if ( m_IdealActivity != GetStoppedActivity() )
	{
		if (diff < -20)
			target = 45;
		else if (diff > 20)
			target = -45;
	}
	pev->angles.x = UTIL_Approach(target, pev->angles.x, 220.0 * 0.1 );
	
	return 0;
}

int CBloater :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType & DMG_ACID )
		return 0;

	m_afMemory |= bits_MEMORY_PROVOKED;

//	PainSound();
	return CSquadMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

#if 0
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBloater :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case BLOATER_AE_ATTACK_MELEE1:
		{
			// do stuff for this event.
			AttackSnd();
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}
#endif

//=========================================================
// Spawn
//=========================================================
void CBloater :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/floater.mdl");
	UTIL_SetSize( pev, Vector(-16, -16, 0), Vector(16, 16, 48 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->spawnflags		|= FL_FLY;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= gSkillData.bloaterHealth;	// is 40 too much?
	pev->view_ofs		= Vector( 0, 0, 32 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_SQUAD | bits_CAP_RANGE_ATTACK1 | bits_CAP_MELEE_ATTACK1;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBloater :: Precache()
{
	PRECACHE_MODEL("models/floater.mdl");
	PRECACHE_SOUND("debris/bustflesh2.wav");
	PRECACHE_SOUND("bullchicken/bc_attack2.wav");
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


// Chase enemy schedule
Task_t tlBloaterChaseEnemy[] = 
{
	{ TASK_GET_PATH_TO_ENEMY_LKP,	(float)128		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},

};

Schedule_t slBloaterChaseEnemy[] =
{
	{ 
		tlBloaterChaseEnemy,
		ARRAYSIZE ( tlBloaterChaseEnemy ),
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_TASK_FAILED,
		0,
		"BloaterChaseEnemy"
	},
};



Task_t	tlBloaterStrafe[] =
{
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_GET_PATH_TO_ENEMY,		(float)128					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_WAIT,					(float)1					},
};

Schedule_t	slBloaterStrafe[] =
{
	{ 
		tlBloaterStrafe,
		ARRAYSIZE ( tlBloaterStrafe ), 
		bits_COND_NEW_ENEMY,
		0,
		"BloaterStrafe"
	},
};


Task_t	tlBloaterTakeCover[] =
{
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_WAIT,					(float)1					},
};

Schedule_t	slBloaterTakeCover[] =
{
	{ 
		tlBloaterTakeCover,
		ARRAYSIZE ( tlBloaterTakeCover ), 
		bits_COND_NEW_ENEMY,
		0,
		"BloaterTakeCover"
	},
};



DEFINE_CUSTOM_SCHEDULES( CBloater )
{
	slBloaterChaseEnemy,
	slBloaterStrafe,
	slBloaterTakeCover,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBloater, CSquadMonster );

//=========================================================
// RunAI
//=========================================================
void CBloater :: RunAI ( void )
{
//	ALERT ( at_aiconsole, "CBloater: RunAI!\n" );
	
	// FIXME: for some reason bloater fails to pass FIND_CLIENT_IN_PVS check in the original RunAI
	// and i can't know why since its an engine function
	// which is why AI functions behind that check are called from there

	if ( m_MonsterState != MONSTERSTATE_NONE	&& 
		 m_MonsterState != MONSTERSTATE_PRONE   && 
		 m_MonsterState != MONSTERSTATE_DEAD )	// don't call them twice!
	{
		Look( m_flDistLook );
		Listen();// check for audible sounds. 

		// now filter conditions.
		ClearConditions( IgnoreConditions() );

		GetEnemy();

		if ( m_hEnemy != NULL )
		{
			CheckEnemy( m_hEnemy );
		}
	}

	FCheckAITrigger();

	PrescheduleThink();

	MaintainSchedule();

	if (HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
	{
		Shoot();
	}
}

//=========================================================
// StartTask
//=========================================================
void CBloater :: StartTask ( Task_t *pTask )
{
//	CSquadMonster :: StartTask ( pTask );
//	return;
	switch ( pTask->iTask )
	{
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if ( pEnemy == NULL )
			{
				TaskFail();
				return;
			}

			if (BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, 64, (pEnemy->pev->origin - pev->origin).Length() ))
			{
				TaskComplete();
			}
			else 
			if ( BuildRoute ( pEnemy->pev->origin, bits_MF_TO_ENEMY, pEnemy ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "CBloater: GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
	default:
		CSquadMonster :: StartTask ( pTask );
		break;
	}
}

//=========================================================
// RunTask 
//=========================================================
void CBloater :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
	case TASK_WAIT_PVS:
		if ( m_hEnemy != NULL )
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			MakeIdealPitch( m_vecEnemyLKP );
			ChangePitch( pev->yaw_speed );
		}

		CSquadMonster :: RunTask ( pTask );

		break;
	default: 
		CSquadMonster :: RunTask ( pTask );
		break;
	}	
}

//=========================================================
//=========================================================
Schedule_t *CBloater :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				return CSquadMonster::GetSchedule( );
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType ( SCHED_WAKE_ANGRY );
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
					// chase!
					return GetScheduleOfType( SCHED_CHASE_ENEMY );
				}
			}
			else  
			{
				if ( HasConditions(bits_COND_CAN_MELEE_ATTACK1) )	// we've got too close
				{
					ALERT ( at_aiconsole, "CBloater: Got too close!\n" );
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				// we can see the enemy
				if ( HasConditions(bits_COND_CAN_RANGE_ATTACK1) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
			
				if ( !HasConditions(bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1) )
				{
					return GetScheduleOfType( SCHED_CHASE_ENEMY );
				}
				else if ( !FacingIdeal() )
				{
					//turn
					return GetScheduleOfType( SCHED_COMBAT_FACE );
				}
				else
				{
					ALERT ( at_aiconsole, "No suitable combat schedule!\n" );
				}
			}
			break;

		}
	}

	return CSquadMonster::GetSchedule( );
}

//=========================================================
//=========================================================
Schedule_t* CBloater :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK1:
	case SCHED_CHASE_ENEMY:
		return slBloaterChaseEnemy;
	case SCHED_TAKE_COVER_FROM_ENEMY:
//	case SCHED_RANGE_ATTACK1:
	case SCHED_MELEE_ATTACK1:
		return slBloaterTakeCover;
/*	case SCHED_WAKE_ANGRY:
		{
			ALERT ( at_aiconsole, "CBloater: Wake!\n");
			return CSquadMonster::GetScheduleOfType( Type );
		}*/
	}

	return CSquadMonster :: GetScheduleOfType( Type );
}

#ifdef _DEBUG
extern void DrawRoute( entvars_t *pev, WayPoint_t *m_Route, int m_iRouteIndex, int r, int g, int b );
#endif

void CBloater :: Move ( float flInterval ) 
{
//	ALERT( at_aiconsole, "CBloater: Move\n" );

//	CSquadMonster :: Move (flInterval);
//	return;

	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	float		flMoveDist;
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if ( FRouteClear() )
	{
		ALERT( at_aiconsole, "Tried to move with no route!\n" );
		TaskFail();
		return;
	}
	
	if ( m_flMoveWaitFinished > gpGlobals->time )
		return;

	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	if (m_flGroundSpeed == 0)
	{
		m_flGroundSpeed = 50;
		// TaskFail( );
		// return;
	}

	if (m_flGroundSpeed > BLOATER_MAX_SPEED)
			m_flGroundSpeed -= BLOATER_MAX_SPEED;

	flMoveDist = m_flGroundSpeed * flInterval;

	do 
	{
		// local move to waypoint.
		vecDir = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize();
		flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length();
		
		// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
		if ( flWaypointDist < DIST_TO_CHECK )
		{
			flCheckDist = flWaypointDist;
		}
		else
		{
			flCheckDist = DIST_TO_CHECK;
		}
		
		if ( (m_Route[ m_iRouteIndex ].iType & (~bits_MF_NOT_TO_MASK)) == bits_MF_TO_ENEMY )
		{
			// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
			pTargetEnt = m_hEnemy;
		}
		else if ( (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_TARGETENT )
		{
			MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );
			ChangeYaw ( pev->yaw_speed );
			pTargetEnt = m_hTargetEnt;
		}

		// !!!BUGBUG - CheckDist should be derived from ground speed.
		// If this fails, it should be because of some dynamic entity blocking this guy.
		// We've already checked this path, so we should wait and time out if the entity doesn't move
		flDist = 0;
		if ( CheckLocalMove ( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist ) != LOCALMOVE_VALID )
		{
			CBaseEntity *pBlocker;

			// Can't move, stop
			Stop();
			// Blocking entity is in global trace_ent
			pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
			if (pBlocker)
			{
				DispatchBlocked( edict(), pBlocker->edict() );
			}
			if ( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time-m_flMoveWaitFinished) > 3.0 )
			{
				// Can we still move toward our target?
				if ( flDist < m_flGroundSpeed )
				{
					// Wait for a second
					m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
	//				ALERT( at_aiconsole, "Move %s!!!\n", STRING( pBlocker->pev->classname ) );
					return;
				}
			}
			else 
			{
				// try to triangulate around whatever is in the way.
				if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist, pTargetEnt, &vecApex ) )
				{
					InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
					RouteSimplify( pTargetEnt );
				}
				else
				{
	 			    ALERT ( at_aiconsole, "Couldn't Triangulate\n" );
					Stop();
					if ( m_moveWaitTime > 0 )
					{
						FRefreshRoute();
						m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime * 0.5;
					}
					else
					{
						TaskFail();
						ALERT( at_aiconsole, "Failed to move!\n" );
						//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, (pev->origin + (vecDir * flCheckDist)).z, m_Route[m_iRouteIndex].vecLocation.z );
					}
					return;
				}
			}
		}

		if (flCheckDist < flMoveDist)
		{
			MoveExecute( pTargetEnt, vecDir, flCheckDist / m_flGroundSpeed );

			// ALERT( at_aiconsole, "%.02f\n", flInterval );
			AdvanceRoute( flWaypointDist );
			flMoveDist -= flCheckDist;
		}
		else
		{
			MoveExecute( pTargetEnt, vecDir, flMoveDist / m_flGroundSpeed );

			if ( ShouldAdvanceRoute( flWaypointDist - flMoveDist ) )
			{
				AdvanceRoute( flWaypointDist );
			}
			flMoveDist = 0;
		}

		if ( MovementIsComplete() )
		{
			Stop();
			RouteClear();
		}
	} while (flMoveDist > 0 && flCheckDist > 0);

	// cut corner?
	if (flWaypointDist < 128)
	{
		if ( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();

		if (m_flGroundSpeed > BLOATER_MAX_SPEED)
			m_flGroundSpeed -= 40;
	}
	else
	{
		if (m_flGroundSpeed < BLOATER_MAX_SPEED)
			m_flGroundSpeed += 10;
	}
}

BOOL CBloater:: ShouldAdvanceRoute( float flWaypointDist )
{
	if ( flWaypointDist <= 32  )
	{
		return TRUE;
	}

	return FALSE;
}


int CBloater :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	TraceResult tr;

	UTIL_TraceHull( vecStart + Vector( 0, 0, 24), vecEnd + Vector( 0, 0, 24), dont_ignore_monsters, head_hull, edict(), &tr );

	// ALERT( at_aiconsole, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
	// ALERT( at_aiconsole, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

	if (pflDist)
	{
		*pflDist = ( (tr.vecEndPos - Vector( 0, 0, 24 )) - vecStart ).Length();// get the distance.
	}

	// ALERT( at_aiconsole, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if (tr.fStartSolid || tr.flFraction < 1.0)
	{
		if ( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}

	return LOCALMOVE_VALID;
}

void CBloater::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
//	ALERT( at_aiconsole, "CBloater: MoveExecute\n" );
	Vector2D vec2DirToPoint, vec2Forward;
	vec2DirToPoint = ( m_Route[ 0 ].vecLocation - pev->origin ).Make2D().Normalize();
	vec2Forward = gpGlobals->v_forward.Make2D().Normalize();

	if ( DotProduct ( vec2DirToPoint, vec2Forward ) <= 0 )
		m_flGroundSpeed = 40;	// no speedstrafing	

	pev->velocity = m_flGroundSpeed * vecDir;
}

void CBloater::Stop( void ) 
{ 
	pev->velocity = g_vecZero; 
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBloater :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (!NoFriendlyFire())
	{
		return FALSE;
	}

	if ( !HasConditions(bits_COND_SEE_ENEMY) || HasConditions(bits_COND_ENEMY_DEAD) )
	{
		return FALSE;
	}

	if ( flDist <= 768 && flDot >= 0.5 )
	{
	//	ALERT( at_aiconsole, "CBloater: Can rangeattack1!! \n" );
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CBloater :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 96 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}

void CBloater::Shoot( void )
{
	if (m_flNextAttack > gpGlobals->time)
		return;

	Vector vecShootOrigin;
	Vector vecShootDir;
	Vector vecSpread = VECTOR_CONE_1DEGREES;

	vecShootOrigin = pev->origin + gpGlobals->v_forward*8 + gpGlobals->v_up*24;
	vecShootDir = ShootAtEnemy( vecShootOrigin );

	// get circular gaussian spread
	float x, y, z;
	do 
	{
		x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		z = x*x+y*y;
	} 
	while (z > 1);

	vecShootDir = vecShootDir +	x * vecSpread.x * gpGlobals->v_right +
					y * vecSpread.y * gpGlobals->v_up;

	// using crossbow bolt as a placeholder
	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate(CROSSBOW_BOLT_BLOATER);
	pBolt->pev->origin = vecShootOrigin;
	pBolt->pev->angles = UTIL_VecToAngles( vecShootDir );
	pBolt->pev->owner = edict();

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "bullchicken/bc_attack2.wav", 1.0, ATTN_NORM);
	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	pBolt->pev->velocity = vecShootDir * 1000;
	pBolt->pev->speed = 1000;

	pBolt->pev->avelocity.z = 10;

	m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(1.5, 3.5);
	
//	UTIL_BloodStream( pev->origin + Vector( 0, 0, 16 ), UTIL_RandomBloodVector(), BloodColor(), RANDOM_LONG( 80, 150 ));
}

void CBloater :: CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation )
{
	ALERT( at_aiconsole, "CBloater: Help!! \n" );

	// skip ones not on my netname
	if ( FStringNull( pev->netname ))
		return;

	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ))) != NULL)
	{
		float d = (pev->origin - pEntity->pev->origin).Length();
		if (d < flDist)
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer( );
			if (pMonster)
			{
				pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
				pMonster->PushEnemy( hEnemy, vecLocation );
			}
		}
	}
}

//=========================================================
// Killed.
//=========================================================
void CBloater :: Killed( entvars_t *pevAttacker, int iGib )
{
//	CSquadMonster::Killed(pevAttacker, iGib);
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->deadflag = DEAD_DEAD;
	
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "debris/bustflesh2.wav", 0.7, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );
	
	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if ( pOwner != NULL )
	{
		pOwner->DeathNotice( pev );
	}

	CGib::SpawnRandomGibs( pev, 3, MainGib() );

	for ( int i = 0; i < 6; i++ )	// acid blast
	{
		UTIL_BloodStream( pev->origin + Vector( 0, 0, 48 ), UTIL_RandomBloodVector(), BloodColor(), RANDOM_LONG( 80, 150 ));
	}

	RadiusDamage ( pev->origin + Vector( 0, 0, 24 ), pev, pev, 15, CLASS_NONE, DMG_ACID);

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_PARTICLEBURST );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( 50 );
		WRITE_BYTE( (unsigned short)192 );
		WRITE_BYTE( 5 );
	MESSAGE_END();

	SetThink ( &CSquadMonster::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}
