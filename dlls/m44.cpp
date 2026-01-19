#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"

class CM44 : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void ) { return CLASS_NONE; }
	int	BloodColor( void ) { return DONT_BLEED; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	BOOL IsBarnaclePrey( void ) { return TRUE; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );
};

LINK_ENTITY_TO_CLASS( monster_m44, CM44 );

void CM44 :: Spawn( void )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/m44.mdl");

	UTIL_SetSize( pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_STEP;
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_YES;

	pev->health			= 200;

	MonsterInit();
}

void CM44 :: Precache( void )
{
	PRECACHE_MODEL( "models/m44.mdl" );
}

int CM44 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	flDamage = 0;

	return CBaseEntity::TakeDamage(  pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CM44 :: GibMonster( void )
{			
}

void CM44 :: Killed( entvars_t *pevAttacker, int iGib )
{
	UTIL_Remove(this);
}