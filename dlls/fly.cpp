//=========================================================
// fly - literally a flying leech
//=========================================================
#include	"float.h"
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"

// TODO: food chase behavior

// Movement constants

#define		FLY_ACCELERATE		10
#define		FLY_CHECK_DIST		45
#define		FLY_SPEED		350//300
#define		FLY_ACCEL		80
#define		FLY_SWIM_DECEL		10
#define		FLY_TURN_RATE		90
#define		FLY_SIZEX			4
#define		FLY_FRAMETIME		0.1



#define DEBUG_BEAMS		0

#if DEBUG_BEAMS
#include "effects.h"
#endif


class CFly : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );

	void EXPORT FlyThink( void );
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
		pev->absmin = pev->origin + Vector(-8,-8,0);
		pev->absmax = pev->origin + Vector(8,8,2);
	}

	void UpdateMotion( void );
	float ObstacleDistance( CBaseEntity *pTarget );
	void MakeVectors( void );
	void RecalculateHeight( void );
	
	// Base entity functions
	void IdleSound( void );
	int	BloodColor( void ) { return BLOOD_COLOR_YELLOW; }
	void EXPORT KillUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Killed( entvars_t *pevAttacker, int iGib );
	void Activate( void );
	int	ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int	Classify( void ) { return CLASS_INSECT; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	float	m_flTurning;// is this boid turning?
	BOOL	m_fPathBlocked;// TRUE if there is an obstacle ahead
	float	m_flAccelerate;
	float	m_obstacle;
	float	m_top;
	float	m_bottom;
	float	m_height;
	float	m_flNextRecalculate;
	float	m_sideTime;		// Timer to randomly check clearance on sides
	float	m_zTime;

	static const char *pIdleSounds[];

#if DEBUG_BEAMS
	CBeam	*m_pb;
	CBeam	*m_pt;
#endif
};



LINK_ENTITY_TO_CLASS( monster_fly, CFly );

TYPEDESCRIPTION	CFly::m_SaveData[] = 
{
	DEFINE_FIELD( CFly, m_flTurning, FIELD_FLOAT ),
	DEFINE_FIELD( CFly, m_fPathBlocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFly, m_flAccelerate, FIELD_FLOAT ),
	DEFINE_FIELD( CFly, m_obstacle, FIELD_FLOAT ),
	DEFINE_FIELD( CFly, m_top, FIELD_FLOAT ),
	DEFINE_FIELD( CFly, m_bottom, FIELD_FLOAT ),
	DEFINE_FIELD( CFly, m_height, FIELD_FLOAT ),
	DEFINE_FIELD( CFly, m_flNextRecalculate, FIELD_TIME ),
	DEFINE_FIELD( CFly, m_sideTime, FIELD_TIME ),
	DEFINE_FIELD( CFly, m_zTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CFly, CBaseMonster );


const char *CFly::pIdleSounds[] = 
{
	"Fly/FLY1.WAV",
	"Fly/FLY2.WAV",
	"Fly/FLY3.WAV",
};

void CFly::Spawn( void )
{
	Precache();
	SET_MODEL(ENT(pev), "models/fly.mdl");
	
//	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetSize( pev, Vector(-1,-1,-1), Vector(1,1,1));
	
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	SetBits(pev->flags, FL_FLY);
	pev->health			= 1;

	m_flFieldOfView		= -0.5;	// 180 degree FOV
	m_flDistLook		= 750;
	MonsterInit();
	SetThink( &CFly::FlyThink );
	SetUse( &CFly::KillUse );
	SetTouch( NULL );
	pev->view_ofs = g_vecZero;

	m_flTurning = 0;
	m_fPathBlocked = FALSE;
	SetActivity( ACT_FLY );
}


void CFly::Activate( void )
{
	RecalculateHeight();
}



void CFly::RecalculateHeight( void )
{
	// Calculate boundaries
	Vector vecTest = pev->origin - Vector(0,0,400);

	TraceResult tr;

	UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
	if ( tr.flFraction != 1.0 )
		m_bottom = tr.vecEndPos.z + 1;
	else
		m_bottom = vecTest.z;

	for (int i = 0; i < abs((int)m_bottom); i++ )
	{
		Vector vecContent = pev->origin;

		vecContent.z -= i;

		if ( UTIL_PointContents ( vecContent ) == CONTENTS_WATER )
		{
			// Stop right ther. We don't want flies to venture into the water
			m_bottom = vecContent.z + 1;
			ALERT ( at_aiconsole, "Fly: Found water underneath!\n" );
		}
	}

	vecTest = pev->origin + Vector(0,0,200);

	UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);

	if ( tr.flFraction != 1.0 )
		m_top = tr.vecEndPos.z - 1;
	else
		m_top = vecTest.z;

	// Chop off 20% of the outside range
	float newBottom = m_bottom * 0.8 + m_top * 0.2;
	m_top = m_bottom * 0.2 + m_top * 0.8;
	m_bottom = newBottom;
	m_height = RANDOM_FLOAT( m_bottom, m_top );

	m_flNextRecalculate = gpGlobals->time + RANDOM_FLOAT( 3, 5 );

	UTIL_DrawBeam( pev->origin, Vector(pev->origin.x, pev->origin.y, m_top), 160, 10, 80);
	UTIL_DrawBeam( pev->origin, Vector(pev->origin.x, pev->origin.y, m_bottom), 160, 10, 80);
}

void CFly::Precache( void )
{
	PRECACHE_MODEL("models/fly.mdl");

	PRECACHE_SOUND("roach/rch_smash.wav");
	PRECACHE_SOUND("Fly/FLY1.WAV");
	PRECACHE_SOUND("Fly/FLY2.WAV");
	PRECACHE_SOUND("Fly/FLY3.WAV");
}


void CFly::MakeVectors( void )
{
	Vector tmp = pev->angles;
	tmp.x = -tmp.x;
	UTIL_MakeVectors ( tmp );
}


//
// ObstacleDistance - returns normalized distance to obstacle
//
float CFly::ObstacleDistance( CBaseEntity *pTarget )
{
	TraceResult		tr;
	Vector			vecTest, vecProbe;

	// use VELOCITY, not angles, not all boids point the direction they are flying
	//Vector vecDir = UTIL_VecToAngles( pev->velocity );
	MakeVectors();

	// check for obstacle ahead
	vecTest = pev->origin + gpGlobals->v_forward * FLY_CHECK_DIST;
	UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);

	if ( tr.fStartSolid )
	{
		pev->speed = -FLY_SPEED * 0.5;
//		ALERT( at_aiconsole, "Stuck from (%f %f %f) to (%f %f %f)\n", pev->oldorigin.x, pev->oldorigin.y, pev->oldorigin.z, pev->origin.x, pev->origin.y, pev->origin.z );
//		UTIL_SetOrigin( pev, pev->oldorigin );
	}

	vecProbe = pev->origin - tr.vecEndPos;

#if 0
	for (int i = 1; i < (int)vecProbe.Length(); i++)
	{
		Vector vecContent = pev->origin + gpGlobals->v_forward * i;

		if ( UTIL_PointContents ( vecContent ) == CONTENTS_WATER )
		{
			ALERT( at_aiconsole, "Fly: Found water at a distance of %f!\n", vecContent.Length() );
			pev->effects = EF_BRIGHTFIELD;
			if ( vecProbe.Length() != 0 )
				return vecContent.Length()/vecProbe.Length();
			else
				return tr.flFraction;
		}
	}
#endif

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
		vecTest = pev->origin + gpGlobals->v_right * FLY_SIZEX * 2 + gpGlobals->v_forward * FLY_CHECK_DIST;
		UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
		if (tr.flFraction != 1.0)
			return tr.flFraction;

		vecTest = pev->origin - gpGlobals->v_right * FLY_SIZEX * 2 + gpGlobals->v_forward * FLY_CHECK_DIST;
		UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
		if (tr.flFraction != 1.0)
			return tr.flFraction;

		// Didn't hit either side, so stop testing for another 0.5 - 1 seconds
		m_sideTime = gpGlobals->time + RANDOM_FLOAT(0.5,1);
	}
	return 1.0;
}


void CFly::UpdateMotion( void )
{
	float flapspeed = (pev->speed - m_flAccelerate) / FLY_ACCELERATE;
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

	if ( pev->avelocity.y > 150 || pev->avelocity.y < -150 )
		m_IdealActivity = ACT_IDLE;
	else
		m_IdealActivity = ACT_FLY;

	// lean
	float targetPitch, delta;
	delta = m_height - pev->origin.z;

	if ( delta < -10 )
		targetPitch = -30;
	else if ( delta > 10 )
		targetPitch = 30;
	else
		targetPitch = 0;

	pev->angles.x = UTIL_Approach( targetPitch, pev->angles.x, 60 * FLY_FRAMETIME );

	// bank
	pev->avelocity.z = - (pev->angles.z + (pev->avelocity.y * 0.25));

	if ( m_Activity != m_IdealActivity )
	{
		SetActivity ( m_IdealActivity );
	}
	float flInterval = StudioFrameAdvance();

#if DEBUG_BEAMS
	if ( !m_pb )
		m_pb = CBeam::BeamCreate( "sprites/laserbeam.spr", 5 );
	if ( !m_pt )
		m_pt = CBeam::BeamCreate( "sprites/laserbeam.spr", 5 );
	m_pb->PointsInit( pev->origin, pev->origin + gpGlobals->v_forward * FLY_CHECK_DIST );
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


void CFly::FlyThink( void )
{
	TraceResult		tr;
	float			flLeftSide;
	float			flRightSide;
	float			targetSpeed;
	float			targetYaw = 0;
	CBaseEntity		*pTarget;

	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
	{
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(1,1.5);
		pev->velocity = g_vecZero;
		return;
	}
	else
		pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->waterlevel )
	{
		ALERT(at_aiconsole, "Fly: HELP\n");
		if ( pev->movetype != MOVETYPE_BOUNCE )
		{
			pev->velocity = g_vecZero;
			pev->movetype = MOVETYPE_BOUNCE;
		}
	}
	else if ( pev->movetype == MOVETYPE_BOUNCE )
	{
		pev->movetype = MOVETYPE_FLY;
	}

	if ( RANDOM_LONG(0,99) <= 5 )
		IdleSound();

	targetSpeed = FLY_SPEED;

	if ( m_flNextRecalculate < gpGlobals->time )
		RecalculateHeight();
	
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
		// If fly didn't move, there must be something blocking it, so try to turn
		m_sideTime = 0;
	}

	m_obstacle = ObstacleDistance( pTarget );
	pev->oldorigin = pev->origin;
	if ( m_obstacle < 0.1 )
		m_obstacle = 0.1;

	// is the way ahead clear?
	if ( m_obstacle == 1.0 )
	{
		// if the fly is turning, stop the trend.
		if ( m_flTurning != 0 )
		{
			m_flTurning = 0;
		}

		m_fPathBlocked = FALSE;
		pev->speed = UTIL_Approach( targetSpeed, pev->speed, FLY_ACCEL * FLY_FRAMETIME );
		pev->velocity = gpGlobals->v_forward * pev->speed;

	}
	else
	{
		m_obstacle = 1.0 / m_obstacle;
		// IF we get this far in the function, the leader's path is blocked!
		m_fPathBlocked = TRUE;

		if ( m_flTurning == 0 )// something in the way and fly is not already turning to avoid
		{
			Vector vecTest;
			// measure clearance on left and right to pick the best dir to turn
			vecTest = pev->origin + (gpGlobals->v_right * FLY_SIZEX) + (gpGlobals->v_forward * FLY_CHECK_DIST);
			UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
			flRightSide = tr.flFraction;

			vecTest = pev->origin + (gpGlobals->v_right * -FLY_SIZEX) + (gpGlobals->v_forward * FLY_CHECK_DIST);
			UTIL_TraceLine(pev->origin, vecTest, missile, edict(), &tr);
			flLeftSide = tr.flFraction;

			// turn left, right or random depending on clearance ratio
			float delta = (flRightSide - flLeftSide);
			if ( delta > 0.1 || (delta > -0.1 && RANDOM_LONG(0,100)<50) )
				m_flTurning = -FLY_TURN_RATE;
			else
				m_flTurning = FLY_TURN_RATE;
		}
		pev->speed = UTIL_Approach( -(FLY_SPEED*0.5), pev->speed, FLY_SWIM_DECEL * FLY_FRAMETIME * m_obstacle );
		pev->velocity = gpGlobals->v_forward * pev->speed;
	}
	pev->ideal_yaw = m_flTurning + targetYaw;
	UpdateMotion();
}

void CFly :: IdleSound ( void )
{
	int pitch = 100 + RANDOM_LONG(-5,5);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

//=========================================================
// KillUse - Gordon attempts to kill fly with his hands
//=========================================================
void CFly :: KillUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// if it's not a player, ignore
	if ( !pActivator->IsPlayer() )
		return;
	
	if ( RANDOM_LONG(0,1) )
		CFly::Killed(pCaller->pev, GIB_ALWAYS);
//	else
//		ALERT( at_aiconsole, "CFly: Try again, LOSER!!!\n" );
}

//=========================================================
// Killed.
//=========================================================
void CFly :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->solid = SOLID_NOT;

	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "roach/rch_smash.wav", 0.7, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );
	
//	CSoundEnt::InsertSound ( bits_SOUND_WORLD, pev->origin, 128, 1 );
	SetUse( NULL );

	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if ( pOwner )
	{
		pOwner->DeathNotice( pev );
	}
	UTIL_Remove( this );
}


