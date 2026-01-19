//=========================================================
// Stealth - flying M44's brother
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"

class CStealth : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void ) { return CLASS_NONE; }
	int	BloodColor( void ) { return DONT_BLEED; }

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );
	void BreakModel( Vector vecSrc, Vector vecVel, int iModel, int iAmount );

	void EXPORT MonsterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT StealthThink( void );
	void EXPORT CrashTouch( CBaseEntity *pOther );
	void EXPORT DyingThink( void );

	void StopRolling( void );

	int m_iSoundState;
	int m_iSpriteTexture;
	int m_iExplode;
	int m_iBodyGibs;
};

void CStealth :: Spawn( void )
{
	Precache();
	SET_MODEL(ENT(pev), "models/stealth.mdl");
	UTIL_SetSize( pev, Vector(-128,-128,0), Vector(128,128,84));
	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY;
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_YES;

	// if stealth has a target, assume it's high in the sky and requires
	// a light effect so the model will never show as full black
	if ( !FStringNull(pev->target) )
	{
		pev->effects		|= EF_BRIGHTLIGHT;
		pev->health			= 100;
	}
	else pev->health		= 500;	// stayput stealth is harder to destroy

//	MonsterInit();
//	SetThink( &CStealth::MonsterThink );
	SetUse ( &CStealth::MonsterUse );

	if ( pev->speed )
		m_flGroundSpeed = pev->speed;
	else
		m_flGroundSpeed = 1000;
}

void CStealth :: Precache( void )
{
	PRECACHE_MODEL("models/stealth.mdl");

	PRECACHE_SOUND("apache/ap_whine1.wav");
	PRECACHE_SOUND("weapons/mortarhit.wav");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );

	m_iExplode	= PRECACHE_MODEL( "sprites/explode1.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/shrapnel.mdl" );
}

//=========================================================
// TraceAttack
//=========================================================
void CStealth :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	Vector vecOrigin = vecDir * 8.0 + ptr->vecEndPos;

	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );

	BreakModel( vecOrigin, vecDir.Normalize(), m_iBodyGibs, RANDOM_LONG(1,4) );
}

int CStealth :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType & (DMG_CLUB | DMG_POISON | DMG_RADIATION | DMG_NERVEGAS) )
		flDamage = 0.0;

	if ( bitsDamageType & DMG_BULLET )
		flDamage *= 0.4;

	return CBaseMonster::TakeDamage(  pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

LINK_ENTITY_TO_CLASS( monster_stealth, CStealth );

void CStealth :: MonsterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	ALERT( at_aiconsole, "Stealth: Use!\n" );
	if ( useType != ( USE_ON || USE_TOGGLE ) )
		return;

	if ( FStringNull(pev->target) )
		return;

	CBaseEntity *pDest;
	pDest = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	
	if ( !pDest )
		return;

	m_pGoalEnt = pDest;
	SetThink( &CStealth::StealthThink ); 
	pev->nextthink = gpGlobals->time;
}

#define STEALTH_MAX_ROLL	60
#define STEALTH_ROLLRATE	15

#define STEALTH_MAX_YAW		45
#define STEALTH_YAWRATE		10

void CStealth :: StealthThink( void )
{
//	ALERT( at_aiconsole, "Stealth: Think!\n" );
	
	if ( m_pGoalEnt == NULL )
	{
		ALERT( at_aiconsole, "Stealth: Goal entity is NULL!\n" );
		pev->nextthink = gpGlobals->time + 5.0;
		return;
	}

	if ( (m_pGoalEnt->pev->origin - pev->origin).Length() < 64 )
	{
		if ( m_pGoalEnt->GetNextTarget() != NULL )
		{
			m_pGoalEnt = m_pGoalEnt->GetNextTarget();
		}
		else
		{
			ALERT( at_aiconsole, "Stealth: Reached a dead end!\n" );

			STOP_SOUND( ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav" );
			m_pGoalEnt = NULL;

			SetThink( NULL );
			UTIL_Remove(this);
			return;
		}
	}

	Vector vecMove, vecAngles;
	float flSide, flInterval = 1.0;

	vecMove = (m_pGoalEnt->pev->origin - pev->origin).Normalize( );
	pev->velocity = vecMove * m_flGroundSpeed * flInterval;

	flSide = DotProduct( vecMove.Make2D(), gpGlobals->v_right.Make2D() );
	vecAngles = UTIL_VecToAngles( vecMove );

	UTIL_DrawBeam( pev->origin, pev->velocity.Normalize(), 4, 96, 32 );
	UTIL_DrawBeam( pev->origin, gpGlobals->v_right, 4, 32, 96 );

//	ALERT (at_aiconsole, "%f\n", pev->angles.z );
	ALERT (at_aiconsole, "%f\n", flSide );

	// roll

	/*if ( flSide > 0.95 )
	{
		if ( pev->angles.z != 0 )
		{
			ALERT (at_aiconsole, "Stealth: undoing roll!\n" );
			pev->avelocity.z += STEALTH_ROLLRATE * -(flSide/flSide);
		}
		else StopRolling();
	} 
	else */if ( flSide < 0 )
	{
		ALERT ( at_aiconsole, "Stealth: roll left\n" );
		if ( pev->angles.z > -STEALTH_MAX_ROLL )
			pev->avelocity.z -= STEALTH_ROLLRATE - ( STEALTH_ROLLRATE * flSide );
		else
			StopRolling();
	}
	else if ( flSide > 0 )
	{
		ALERT ( at_aiconsole, "Stealth: roll right\n" );
		if ( pev->angles.z < STEALTH_MAX_ROLL )
			pev->avelocity.z += STEALTH_ROLLRATE - ( STEALTH_ROLLRATE * flSide );
		else
			StopRolling();
	}

	// lazy way to get the jet turn, it's shown for a few seconds anyways
	pev->ideal_yaw = vecAngles.y;
	ChangeYaw(140);

	if (m_iSoundState == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav", 0.5, 0.4, 0, 90 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CStealth :: StopRolling( void )
{
	ALERT( at_aiconsole, "Stealth: Stopped rolling!\n");
	pev->avelocity.z = 0;
}

void CStealth :: BreakModel( Vector vecSrc, Vector vecVel, int iModel, int iAmount )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSrc.x );
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 128 );

			// velocity
			WRITE_COORD( vecVel.x ); 
			WRITE_COORD( vecVel.y );
			WRITE_COORD( vecVel.z );

			// randomization
			WRITE_BYTE( 30 ); 

			// Model
			WRITE_SHORT( iModel );	//model id#

			// # of shards
			WRITE_BYTE( iAmount );

			// duration
			WRITE_BYTE( 200 );// 10.0 seconds

			// flags

			WRITE_BYTE( BREAK_METAL | FTENT_SMOKETRAIL );
		MESSAGE_END();
}

void CStealth :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->movetype = MOVETYPE_TOSS;

	SetThink( &CStealth::DyingThink );
	SetTouch( &CStealth::CrashTouch );

//	BreakModel( pev->origin, m_iBodyGibs, 32 );
	pev->effects = 0;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
}

void CStealth :: DyingThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z + 64 );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 30 ); // scale * 10
			WRITE_BYTE( 160 ); // brightness
		MESSAGE_END();

	pev->avelocity.x += 4;
	pev->avelocity.z += 12;
//	pev->velocity = pev->velocity + gpGlobals->v_forward * m_flGroundSpeed/2;
}

void CStealth::CrashTouch( CBaseEntity *pOther )
{
	// only crash if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP) 
	{
		SetTouch( NULL );
		SetThink( NULL );
		GibMonster();
	}
}

void CStealth::GibMonster( void )
{
	STOP_SOUND( ENT(pev), CHAN_STATIC, "apache/ap_whine1.wav" );

	Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 256 );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 120 ); // scale * 10
			WRITE_BYTE( 255 ); // brightness
		MESSAGE_END();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z);
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z + 2000 ); // reach damage radius over .2 seconds
			WRITE_SHORT( m_iSpriteTexture );
			WRITE_BYTE( 0 ); // startframe
			WRITE_BYTE( 0 ); // framerate
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 32 );  // width
			WRITE_BYTE( 0 );   // noise
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 192 );   // r, g, b
			WRITE_BYTE( 128 ); // brightness
			WRITE_BYTE( 0 );		// speed
		MESSAGE_END();

	EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

	RadiusDamage( pev->origin, pev, pev, 300, CLASS_NONE, DMG_BLAST );

	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	vecSpot.z += 64;

	BreakModel( vecSpot, Vector(0, 0, 200),m_iBodyGibs, 64 );
	
	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();

	SetThink( &CStealth::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;

}