#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "spit.h"

LINK_ENTITY_TO_CLASS( spit, CSpit );

void CSpit:: Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/spit.mdl");

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	ResetSequenceInfo();
}

void CSpit::Animate( void )
{
	// Adjust projectile's angles according to current velocity
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.x = -pev->angles.x;

	// Animate the spit
	StudioFrameAdvance();

}

void CSpit::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CSpit *pSpit = GetClassPtr( (CSpit *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles( vecVelocity );

	pSpit->SetThink ( &CSpit::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CSpit :: Touch ( CBaseEntity *pOther )
{
	SetThink ( &CSpit::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}