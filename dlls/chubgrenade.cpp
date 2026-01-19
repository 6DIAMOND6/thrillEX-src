// UNDONE: croak on idle or secondary attack?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"
#include	"game.h"
#include	"weapons.h"
#include	"nodes.h"
#include	"player.h"
#include	"soundent.h"
#include	"gamerules.h"

#define		CHUBGRENADE_THROW_DELAY		1.0
#define		CHUBGRENADE_THROW_RANGE		48

enum chub_e {
	CHUB_IDLE1 = 0,
	CHUB_FIDGETLICK,
	CHUB_FIDGETCROAK,
	CHUB_DOWN,
	CHUB_UP,
	CHUB_THROW
};

LINK_ENTITY_TO_CLASS( weapon_chub, CChubGrenade );

void CChubGrenade :: Spawn ( void )
{
	Precache( );
	m_iId = WEAPON_CHUB;
	SET_MODEL(ENT(pev), "models/chumtoad.mdl");
	pev->effects = EF_NODRAW;
	FallInit();

	m_iDefaultAmmo = CHUB_DEFAULT_GIVE;
}

void CChubGrenade :: Precache ( void )
{
	UTIL_PrecacheOther("monster_chumtoad");
	PRECACHE_MODEL("models/v_chub.mdl");
	m_usChubFire = PRECACHE_EVENT ( 1, "events/chubfire.sc" );
}


int CChubGrenade::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Chubtoads";
	p->iMaxAmmo1 = CHUB_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_CHUB;
	p->iWeight = CHUB_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

int CChubGrenade::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		m_pPlayer->SetSuitUpdate("!HEV_SQUEEK", FALSE, SUIT_NEXT_IN_30MIN);
		return TRUE;
	}
	return FALSE;
}

BOOL CChubGrenade :: Deploy( )
{
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	return DefaultDeploy( "models/v_chub.mdl", "models/p_squeak.mdl", CHUB_UP, "chub" );	
}

void CChubGrenade :: Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.1;
	m_flAttackDelay = NULL;

	if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_CHUB);
		SetThink( &CChubGrenade::DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	SendWeaponAnim( CHUB_DOWN );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

BOOL CChubGrenade :: TraceChub( void )
{
#ifndef CLIENT_DLL
	TraceResult tr;
	Vector trace_origin;

	trace_origin = m_pPlayer->EyePosition();

	// pull out of the player bounding box
	trace_origin = trace_origin + gpGlobals->v_forward * VEC_HUMAN_HULL_MAX.x;	

	if ( m_pPlayer->pev->flags & FL_DUCKING )
	{
		trace_origin = trace_origin - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
	}

	// FIXME: magic number
	UTIL_TraceLine( trace_origin + gpGlobals->v_forward * 24, trace_origin + gpGlobals->v_forward * 64, dont_ignore_monsters, NULL, &tr );

	if ( tr.fAllSolid || tr.fStartSolid || tr.flFraction <= 0.25 )
		return FALSE;
	
	UTIL_TraceHull(tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, head_hull, NULL, &tr);

	if ( tr.pHit )
		return FALSE;

#endif
	
	return TRUE;
}

void CChubGrenade :: Throw()
{
#ifndef CLIENT_DLL
	edict_t			*pChub;
	entvars_t		*pevChub;
	Vector			vecSrc;

	pChub = CREATE_NAMED_ENTITY( MAKE_STRING("monster_chumtoad") );
	if ( !FNullEnt(pChub) )
	{
		pevChub = VARS( pChub );
		pevChub->origin = m_pPlayer->EyePosition() + gpGlobals->v_forward * CHUBGRENADE_THROW_RANGE;
		pevChub->angles = m_pPlayer->pev->angles;
		DispatchSpawn( ENT( pevChub ) );

		pevChub->flags &= ~FL_ONGROUND;
		pevChub->movetype = MOVETYPE_TOSS; // SERECKY JAN-17-26: Added this here to make the CHUB landing not as glitchy...
		pevChub->velocity = gpGlobals->v_forward * 500 + m_pPlayer->pev->velocity;
		pevChub->velocity.z += gpGlobals->v_up.z * 300;
		pevChub->angles.x = 0;
		pevChub->angles.z = 0;
	}


	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	m_fChubJustThrown = 1;
#endif
}

void CChubGrenade :: PrimaryAttack()
{
	if ( !(m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]) )
		return;

	ALERT(at_console, "chub\n");
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	int flags;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// SERECKY JAN-17-26: Moved this here to let the CHUB play animations with cl_lw on...
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usChubFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

#ifndef CLIENT_DLL
	if ( TraceChub() )
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flAttackDelay = gpGlobals->time + CHUBGRENADE_THROW_DELAY;
		m_flNextPrimaryAttack = GetNextAttackDelay(3.0);
	}
#endif


}

void CChubGrenade :: WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() || m_flAttackDelay )
		return;

	if (m_fChubJustThrown)
	{
		m_fChubJustThrown = 0;

		if ( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim( CHUB_UP );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if (flRand <= 0.75)
	{
		iAnim = CHUB_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = CHUB_FIDGETLICK;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0 / 16.0;
	}
	else
	{
		iAnim = CHUB_FIDGETCROAK;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 16.0;
	}
	SendWeaponAnim( iAnim );
}

void CChubGrenade::ItemPostFrame( void )
{
	if ( m_flAttackDelay )
	{
		if ( m_flAttackDelay <= gpGlobals->time )
		{
			m_flAttackDelay = NULL;

			if ( TraceChub() )
				Throw();

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}