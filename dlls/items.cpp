/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

// UNDONE: make ammo inherit this

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"

extern int gmsgItemPickup;

class CWorldItem : public CBaseEntity
{
public:
	void	KeyValue(KeyValueData *pkvd ); 
	void	Spawn( void );
	int		m_iType;
};

LINK_ENTITY_TO_CLASS(world_items, CWorldItem);

void CWorldItem::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWorldItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	switch (m_iType) 
	{
	case 1:	// CROWBAR
		pEntity = CBaseEntity::Create( "weapon_crowbar", pev->origin, pev->angles );
		break;
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create( "item_battery", pev->origin, pev->angles );
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create( "item_security", pev->origin, pev->angles );
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", pev->origin, pev->angles );
		break;
	}

	if (!pEntity)
	{
		ALERT( at_console, "unable to create world_item %d\n", m_iType );
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}


void CItem::Spawn( void )
{
	if ( !FBitSet( pev->spawnflags, SF_AMMO_NOFALL ) ) 
	{ 
		pev->movetype = MOVETYPE_TOSS; 
	}
	else 
	{
		pev->movetype = MOVETYPE_NONE;
	}

	if ( FBitSet( pev->spawnflags, SF_AMMO_INDRAWER ) )
	{
		UTIL_SetSize(pev, Vector(-4, -4, 0), Vector(4, 4, 4));
		SetThink( &CItem::DrawerThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else 
	{
		UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	}

	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	SetTouch(&CItem::ItemTouch);

	if (!FBitSet( pev->spawnflags, SF_AMMO_NOFALL ))
	{
		if (DROP_TO_FLOOR(ENT(pev)) == 0)	// this engine function makes item fall even if it has NOFALL spawnflag set
		{
			ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z);
			UTIL_Remove( this );
			return;
		}
	}
}

void CItem::DrawerThink( void )
{
	CBaseEntity* pEntity = NULL;
	Vector vecStart, vecEnd;
	TraceResult tr;

	vecStart = pev->origin;
	vecStart.z += 2;

	pEntity = UTIL_FindEntityInSphere( pEntity, vecStart, 12.0 );

	if ( pEntity->IsPlayer() )
	{
	//	ALERT ( at_console, "sphere\n" );
		this->ItemTouch(pEntity);
		return;
	}

	vecEnd = pev->origin;
	vecEnd.z += 24;

	UTIL_TraceLine( pev->origin, vecEnd, dont_ignore_monsters, edict(), &tr);

	pEntity = CBaseEntity::Instance(tr.pHit);

	if ( pEntity->IsPlayer() )
	{
	//	ALERT ( at_console, "trace\n" );
		this->ItemTouch(pEntity);
		return;
	}
	
	pev->nextthink = gpGlobals->time + 0.1;
}

extern int gEvilImpulse101;

void CItem::ItemTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
	{
		return;
	}

	if ( FBitSet(pev->spawnflags, SF_AMMO_TRACECHECK) )
	{
		TraceResult tr;

		UTIL_TraceLine( Center(), pOther->Center(), dont_ignore_monsters, edict(), &tr );

		if ( tr.pHit != pOther->edict() )
			return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if ( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if (MyTouch( pPlayer ))
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );
		
		if ( pev->message )
		{
		UTIL_ShowMessage( STRING(pev->message), pOther );
//		CLIENT_PRINTF( ENT( pPlayer->pev ), print_center, STRING(pev->message) );	// quake-style message
		}
		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		{
			Respawn(); 
		}
		else
		{
			UTIL_Remove( this );
		}
	}
	else if (gEvilImpulse101)
	{
		UTIL_Remove( this );
	}

	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if ( pOwner != NULL )
	{
		pOwner->DeathNotice( pev );
	}
}

CBaseEntity* CItem::Respawn( void )
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, g_pGameRules->VecItemRespawnSpot( this ) );// blip to whereever you should respawn.

	SetThink ( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this ); 
	return this;
}

void CItem::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CItem::ItemTouch );
}

#define SF_SUIT_LONGLOGON		0x0001

class CItemSuit : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_suit.mdl");
		pev->netname = MAKE_STRING("H.E.V Suit");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_suit.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->weapons & (1<<WEAPON_SUIT) )
			return FALSE;

		if ( pev->spawnflags & SF_SUIT_LONGLOGON )
			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_AAx");

		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->netname) );
		MESSAGE_END();

		pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);



class CItemBattery : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_battery.mdl");
		pev->netname = MAKE_STRING("Battery");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_battery.mdl");
		PRECACHE_SOUND( "items/gunpickup2.wav" );
		PRECACHE_SOUND( "items/gunpickup1.wav" );
		PRECACHE_SOUND( "items/gunpickup3.wav" );
		PRECACHE_SOUND( "items/gunpickup4.wav" );

		PRECACHE_SOUND( "items/ammopickup1.wav" );
		PRECACHE_SOUND( "items/ammopickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) &&
			(pPlayer->pev->weapons & (1<<WEAPON_SUIT)))
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += gSkillData.batteryCapacity;
			pPlayer->pev->armorvalue = min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING(pev->netname) );
			MESSAGE_END();
			
			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)( (float)(pPlayer->pev->armorvalue * 100.0) * (1.0/MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;
		
			sprintf( szcharge,"!HEV_%1dP", pct );
			
			//EMIT_SOUND_SUIT(ENT(pev), szcharge);
			pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);

char *GetItemSuitSentence( char* chSentenceName, int iNumber )
{
	char string[32];

	sprintf(string, "%s%iP", chSentenceName, iNumber);

//	ALERT( at_console, "%s, %i\n", string, iNumber );

	return string;
}

class CItemAntidote : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_antidote.mdl");
		pev->netname = MAKE_STRING("Anti-Toxin Canister");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_antidote.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
	if ( pPlayer->m_rgItems[ITEM_ANTIDOTE] < ANTIDOTE_MAX_CARRY )
	{
	//	pPlayer->SetSuitUpdate("!HEV_ANTIDOTE", FALSE, SUIT_NEXT_IN_1MIN);

		pPlayer->SetSuitUpdate(GetItemSuitSentence( "!HEV_TOX", pPlayer->m_rgItems[ITEM_ANTIDOTE] ), FALSE, SUIT_REPEAT_OK);

		EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;

		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->netname) );
		MESSAGE_END();

		return TRUE;
	}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);


class CItemSecurity : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_security.mdl");
		pev->netname = MAKE_STRING("Security Card");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_security.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;

		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->netname) );
		MESSAGE_END();

		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);

class CItemLongJump : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_longjump.mdl");
		pev->netname = MAKE_STRING("Long-Jump Module");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_longjump.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->m_fLongJump )
		{
			return FALSE;
		}

		if ( ( pPlayer->pev->weapons & (1<<WEAPON_SUIT) ) )
		{
			pPlayer->m_fLongJump = TRUE;// player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING(pev->netname) );
			MESSAGE_END();

			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A1" );	// Play the longjump sound UNDONE: Kelly? correct sound?
			pPlayer->m_iLongJumpBattery = 5;
			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_longjump, CItemLongJump );

class CItemAdrenaline : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_adrenaline.mdl");
		pev->netname = MAKE_STRING("Adrenaline Canister");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_adrenaline.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->m_rgItems[ITEM_ADRENALINE] <= 0 )
		{
			pPlayer->SetSuitUpdate("!HEV_ADRENALINE", FALSE, SUIT_NEXT_IN_1MIN);
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
			pPlayer->m_rgItems[ITEM_ADRENALINE] += 1;

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING(pev->netname) );
			MESSAGE_END();

			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_adrenaline, CItemAdrenaline);

class CItemRadiation : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_rad.mdl");
		pev->netname = MAKE_STRING("Anti-Radiation Canister");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_rad.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->m_rgItems[ITEM_RADIATION] < RAD_MAX_CARRY )
		{
			pPlayer->SetSuitUpdate(GetItemSuitSentence( "!HEV_RAD", pPlayer->m_rgItems[ITEM_RADIATION] ), FALSE, SUIT_REPEAT_OK);

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
			pPlayer->m_rgItems[ITEM_RADIATION] += 1;

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING(pev->netname) );
			MESSAGE_END();

			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_rad, CItemRadiation);
LINK_ENTITY_TO_CLASS(item_radiation, CItemRadiation);

class CItemShield : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_shield.mdl");
		pev->netname = MAKE_STRING("Alien Shield");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_shield.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( !pPlayer->m_rgItems[ITEM_SHIELD] )
		{
			pPlayer->SetSuitUpdate("!HEV_A2", FALSE, SUIT_REPEAT_OK);
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );		
			pPlayer->m_rgItems[ITEM_SHIELD] = 1;

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING(pev->netname) );
			MESSAGE_END();
			
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_shield, CItemShield);