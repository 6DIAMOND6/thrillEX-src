/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "../hud.h"
#include "../cl_util.h"
#include "event_api.h"

extern "C"
{
	// HLDM
	void EV_FireGlock1( struct event_args_s *args  );
	void EV_FireGlock2( struct event_args_s *args  );
	void EV_FireShotGunSingle( struct event_args_s *args  );
	void EV_FireShotGunDouble( struct event_args_s *args  );
	void EV_FireMP5( struct event_args_s *args  );
	void EV_FireMP52( struct event_args_s *args  );
	void EV_FirePython( struct event_args_s *args  );
	void EV_FireGauss( struct event_args_s *args  );
	void EV_SpinGauss( struct event_args_s *args  );
	void EV_Crowbar( struct event_args_s *args );
	void EV_FireCrossbow( struct event_args_s *args );
	void EV_FireCrossbow2( struct event_args_s *args );
	void EV_FireRpg( struct event_args_s *args );
	void EV_EgonFire( struct event_args_s *args );
	void EV_EgonStop( struct event_args_s *args );
	void EV_EgonSwitch( struct event_args_s *args );
	void EV_HornetGunFire( struct event_args_s *args );
	void EV_TripmineFire( struct event_args_s *args );
	void EV_SnarkFire( struct event_args_s *args );
	void EV_ChubFire( struct event_args_s *args );
	void EV_LadderClimb( struct event_args_s *args );
	void EV_TrainPitchAdjust( struct event_args_s *args );
}

static int gSparkRamp[9] = { 0xfe, 0xfd, 0xfc, 0x6f, 0x6e, 0x6d, 0x6c, 0x67, 0x60 };

/*
===============
R_Blob2ParticlesCallback

A callback that imitates Alpha's engine pt_blob2 behavior
===============
*/
void R_Blob2ParticlesCallback(struct particle_s *particle, float frametime)
{
	float flGravity = frametime * gEngfuncs.hudGetServerGravityValue() * 0.05;
	float time2 = frametime * 10.0;
	float dvel = frametime * 4.0;

	// update position.
	VectorMA( particle->org, frametime, particle->vel, particle->org);

	if ( gEngfuncs.PM_PointContents( particle->org, NULL ) != CONTENTS_EMPTY ) 
	{
		particle->die = gEngfuncs.GetClientTime();
		return;
	}

	particle->ramp += time2;

	if( particle->ramp < 9.0 ) 
		particle->color = gSparkRamp[(int)particle->ramp];
		
	gEngfuncs.pEfxAPI->R_GetPackedColor( &particle->packedColor, particle->color );

	// Move particles down
	particle->vel[2] -= flGravity * 4.0;
}

void R_ParticleSparks( Vector pos )
{
	int			i, j;
	particle_t	*p;

	for ( i = 0 ; i < 15; i++)
	{
		p = gEngfuncs.pEfxAPI->R_AllocParticle( R_Blob2ParticlesCallback );
		if ( !p ) return;

		for( j = 0; j < 3; j++ )
		{
			p->org[j] = pos[j];
			p->vel[j] = gEngfuncs.pfnRandomFloat(-16.0, 15.0);
		}

		p->vel[2] = gEngfuncs.pfnRandomFloat(0.0, 63.0);

		p->color = 254;

		// This is for software to setup color correctly
		gEngfuncs.pEfxAPI->R_GetPackedColor( &p->packedColor, p->color );

		p->ramp = 0.0;
		p->type = pt_clientcustom;
		p->die = gEngfuncs.GetClientTime() + 3.0; // die in 3 seconds
	}
}

void EV_Sparks( event_args_t *args )
{
	Vector		org, vecVel;

	VectorCopy( args->origin, org );

	R_ParticleSparks( org );
}

void EV_HornetTrailThink ( struct tempent_s *ent, float frametime, float currenttime )
{
	if (currenttime < ent->entity.baseline.fuser1)
		return;

//	this will emit trails from grenades and other stuff sometimes
//	gEngfuncs.pEfxAPI->R_RocketTrail (ent->entity.prevstate.origin, ent->entity.origin, 6);

	// prevent unwanted trails
	if ( ent->entity.origin == ent->entity.attachment[0] )
	{
		if (ent->entity.baseline.fuser2 != -1) 
		{
			if (ent->entity.baseline.fuser2 < gEngfuncs.GetClientTime()) 
			{
				ent->die = gEngfuncs.GetClientTime();
			}
		}
		else ent->entity.baseline.fuser2 = gEngfuncs.GetClientTime() + 0.1;
	}
	else
	{
		ent->entity.baseline.fuser2 = -1;
		VectorCopy(ent->entity.origin, ent->entity.attachment[0]);
	}

	ent->entity.baseline.fuser1 = gEngfuncs.GetClientTime() + 0.01;
}

void EV_HornetTrail ( struct event_args_s *args )
{
	TEMPENTITY* pTrail = NULL;
	int iEntIndex = args->iparam1;

	pTrail = gEngfuncs.pEfxAPI->CL_TempEntAllocNoModel(args->origin);

	if (pTrail == NULL)
		return;

	pTrail->entity.baseline.fuser2 = -1;

	pTrail->flags |= (FTENT_PLYRATTACHMENT | FTENT_COLLIDEKILL | FTENT_CLIENTCUSTOM | FTENT_COLLIDEWORLD | FTENT_HORNETTRAIL);

	pTrail->callback = &EV_HornetTrailThink;
	pTrail->clientIndex = iEntIndex;

	pTrail->die = gEngfuncs.GetClientTime() + 8.0;
	pTrail->entity.baseline.fuser1 = gEngfuncs.GetClientTime();

}

#define SHARD_VELOCITY_RANGE	192.0
#define	SHARD_LIFE_TIME			1.5
#define	SHARD_SIZE_MIN			0.7
#define	SHARD_SIZE_MAX			1.0

void EV_Shards ( struct event_args_s *args )
{
	TEMPENTITY *pShard;
	Vector vecOrigin, vecVel;
	float flSize;
	int cFlag, iModel, iCount, iMax, i, j;

	VectorCopy( args->origin, vecOrigin );
//	VectorCopy( args->angles, vecMaxs );	// this was a hack to get the model size

	flSize = args->fparam1;


	iModel = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/shard.spr" );

	cFlag = FTENT_COLLIDEWORLD | FTENT_SLOWGRAVITY;

	iMax = 2;

	iMax += (int)flSize / 16;

//	gEngfuncs.Con_Printf( "%f, %i\n", flSize, iMax );

	iCount = gEngfuncs.pfnRandomLong(iMax/2, iMax);

	for ( i = 0; i < iCount; i++ )
	{
		for( j = 0; j < 3; j++ )
		{
			vecVel[j] = gEngfuncs.pfnRandomFloat( -SHARD_VELOCITY_RANGE, SHARD_VELOCITY_RANGE );
		}

		pShard = gEngfuncs.pEfxAPI->R_TempSprite( vecOrigin, vecVel, gEngfuncs.pfnRandomFloat( SHARD_SIZE_MIN, SHARD_SIZE_MAX ), iModel, kRenderTransAdd, kRenderFxNoDissipation, 255.0, SHARD_LIFE_TIME, cFlag );
		
		if (!pShard)
			return;

		pShard->entity.curstate.frame = gEngfuncs.pfnRandomLong(0,5);
	}
}

void EV_Spark ( struct event_args_s *args  );

#define SHRAPNEL_VELOCITY_RANGE		256

void ThrowShrapnel ( Vector vecOrigin )
{
	TEMPENTITY* pShrapnel = NULL;
	Vector vecVel;
	int iModel, iCount, i, j;

	iModel = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shrapnel.mdl" );
	iCount = gEngfuncs.pfnRandomLong(6, 12);

	for ( i = 0; i < iCount; i++ )
	{
		for( j = 0; j < 3; j++ )
		{
			vecVel[j] = gEngfuncs.pfnRandomFloat( -SHRAPNEL_VELOCITY_RANGE, SHRAPNEL_VELOCITY_RANGE );
		}

		pShrapnel = gEngfuncs.pEfxAPI->R_TempModel( vecOrigin, vecVel, vec3_origin, 1.0, iModel, 0 );
		
		if ( pShrapnel == NULL )
			continue;

		pShrapnel->flags |= FTENT_SMOKETRAIL;
	}
}

#define EXPLOSIONS_SPRITE_ONLY			0
#define EXPLOSIONS_SHRAPNEL_ONLY		1
#define EXPLOSIONS_EVERYTHING			2


void EV_Explode ( struct event_args_s *args  )
{
	cvar_t *explosions = NULL;
	Vector vecOrigin;
	float flDmg, flScale;
	int iModel, iScale, iType; 

	explosions = gEngfuncs.pfnGetCvarPointer( "cl_explosions" );

	if ( explosions )
	{
		iType = (int)explosions->value;
	}
	else iType = EXPLOSIONS_EVERYTHING;

	VectorCopy( args->origin, vecOrigin );
	flDmg = args->fparam1;
	flScale = args->fparam2;
	iScale = args->iparam1;
	
	if ( iScale > 0 )
		flScale = (float)iScale * 0.1;	// size is set directly
	else if ( flDmg > 0 )
		flScale = 0.5 + flDmg / 100;	// size is dependent on damage amount

	if ( iType == EXPLOSIONS_SHRAPNEL_ONLY )
		iModel = NULL;
	else
		iModel = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/explode1.spr" );

	gEngfuncs.pEfxAPI->R_Explosion( vecOrigin, iModel, flScale, 15.0, 0 );

	if ( iType > EXPLOSIONS_SPRITE_ONLY )
		ThrowShrapnel(vecOrigin);
}

#define BUBBLE_VELOCITY_RANGE		200.0
#define	BUBBLE_LIFE_TIME			1.5
#define	BUBBLE_SIZE_MIN				0.3
#define	BUBBLE_SIZE_MAX				0.6

void EV_BubbleThink ( struct tempent_s *ent, float frametime, float currenttime )
{
	vec3_t org;
	
	if (currenttime < ent->entity.baseline.fuser1)
		return;

	org = ent->entity.prevstate.origin;

	if ( gEngfuncs.PM_PointContents( org, NULL ) != CONTENTS_WATER ) 
	{
		// do not travel outside of water
		ent->die = gEngfuncs.GetClientTime();
		return;
	}

	ent->entity.baseline.origin[0] *= .9;
	ent->entity.baseline.origin[1] *= .9;

	if (ent->entity.baseline.origin[2] < 16.0)
		ent->entity.baseline.origin[2] += 16.0;

	ent->entity.baseline.fuser1 = gEngfuncs.GetClientTime() + 0.01;
}

void EV_BubbleExplode ( struct event_args_s *args  )
{
	TEMPENTITY* pBubble = NULL;
	Vector vecOrigin, vecVel;
	int iModel, iCount, i, j;

	VectorCopy( args->origin, vecOrigin );
	if ( args->iparam1 )
		iCount = args->iparam1;
	else
		iCount = 8;

	iModel = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/bubble.spr" );	

	for ( i = 0; i < iCount; i++ )
	{
		for( j = 0; j < 3; j++ )
		{
			vecVel[j] = gEngfuncs.pfnRandomFloat( -BUBBLE_VELOCITY_RANGE, BUBBLE_VELOCITY_RANGE );
		}

		pBubble = gEngfuncs.pEfxAPI->CL_TempEntAlloc( vecOrigin, gEngfuncs.hudGetModelByIndex(iModel) );
		
		if (!pBubble)
			return;

		pBubble->callback	= &EV_BubbleThink;
		pBubble->flags		= FTENT_COLLIDEWORLD | FTENT_COLLIDEKILL | FTENT_CLIENTCUSTOM;
		pBubble->die		= gEngfuncs.GetClientTime() + BUBBLE_LIFE_TIME + gEngfuncs.pfnRandomFloat(0.0, 1.0);

		pBubble->entity.baseline.fuser1 = gEngfuncs.GetClientTime();
		pBubble->entity.baseline.origin = vecVel;
		pBubble->entity.curstate.scale = gEngfuncs.pfnRandomFloat( BUBBLE_SIZE_MIN, BUBBLE_SIZE_MAX );
	}
}

void R_SmokeParticleCallback(struct particle_s *particle, float frametime)
{
	VectorMA( particle->org, frametime, particle->vel, particle->org);

	if ( gEngfuncs.PM_PointContents( particle->org, NULL ) != CONTENTS_EMPTY ) 
	{
		particle->die = gEngfuncs.GetClientTime();	// dont go through BSP
		return;
	}

	particle->vel[2] += 1;
}

#define SMOKE_VELOCITY_RANGE	8.0

void EV_Smoke ( struct event_args_s *args  )
{
	particle_t *p;
	Vector vecOrigin;
	int iCount, i, j;

	VectorCopy( args->origin, vecOrigin );
	if ( args->iparam1 )
		iCount = args->iparam1;
	else
		iCount = 12;

	for ( i = 0; i < iCount; i++ )
	{
		p = gEngfuncs.pEfxAPI->R_AllocParticle( R_SmokeParticleCallback );

		if (!p)
			return;
		
		for ( j = 0; j < 3; j++ )
			p->org[j] = vecOrigin[j] + gEngfuncs.pfnRandomFloat(-1.0, 1.0);

		p->vel[0] = gEngfuncs.pfnRandomFloat(-SMOKE_VELOCITY_RANGE, SMOKE_VELOCITY_RANGE);
		p->vel[1] = gEngfuncs.pfnRandomFloat(-SMOKE_VELOCITY_RANGE, SMOKE_VELOCITY_RANGE);
		p->vel[2] = SMOKE_VELOCITY_RANGE + gEngfuncs.pfnRandomFloat(0.0, SMOKE_VELOCITY_RANGE);

		p->color = gEngfuncs.pfnRandomLong(1, 5);

		p->die = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(2.0, 4.0);
	}
}

/*
======================
Game_HookEvents

Associate script file name with callback functions.  Callback's must be extern "C" so
 the engine doesn't get confused about name mangling stuff.  Note that the format is
 always the same.  Of course, a clever mod team could actually embed parameters, behavior
 into the actual .sc files and create a .sc file parser and hook their functionality through
 that.. i.e., a scripting system.

That was what we were going to do, but we ran out of time...oh well.
======================
*/
void Game_HookEvents( void )
{
	gEngfuncs.pfnHookEvent( "events/glock1.sc",					EV_FireGlock1 );
	gEngfuncs.pfnHookEvent( "events/glock2.sc",					EV_FireGlock2 );
	gEngfuncs.pfnHookEvent( "events/shotgun1.sc",				EV_FireShotGunSingle );
	gEngfuncs.pfnHookEvent( "events/shotgun2.sc",				EV_FireShotGunDouble );
	gEngfuncs.pfnHookEvent( "events/mp5.sc",					EV_FireMP5 );
	gEngfuncs.pfnHookEvent( "events/mp52.sc",					EV_FireMP52 );
	gEngfuncs.pfnHookEvent( "events/python.sc",					EV_FirePython );
	gEngfuncs.pfnHookEvent( "events/gauss.sc",					EV_FireGauss );
	gEngfuncs.pfnHookEvent( "events/gaussspin.sc",				EV_SpinGauss );
	gEngfuncs.pfnHookEvent( "events/train.sc",					EV_TrainPitchAdjust );
	gEngfuncs.pfnHookEvent( "events/crowbar.sc",				EV_Crowbar );
	gEngfuncs.pfnHookEvent( "events/crossbow1.sc",				EV_FireCrossbow );
	gEngfuncs.pfnHookEvent( "events/crossbow2.sc",				EV_FireCrossbow2 );
	gEngfuncs.pfnHookEvent( "events/rpg.sc",					EV_FireRpg );
	gEngfuncs.pfnHookEvent( "events/egon_fire.sc",				EV_EgonFire );
	gEngfuncs.pfnHookEvent( "events/egon_stop.sc",				EV_EgonStop );
	gEngfuncs.pfnHookEvent( "events/egon_switch.sc",			EV_EgonSwitch );
	gEngfuncs.pfnHookEvent( "events/firehornet.sc",				EV_HornetGunFire );
	gEngfuncs.pfnHookEvent( "events/tripfire.sc",				EV_TripmineFire );
	gEngfuncs.pfnHookEvent( "events/snarkfire.sc",				EV_SnarkFire );
	gEngfuncs.pfnHookEvent( "events/chubfire.sc",				EV_ChubFire );
	gEngfuncs.pfnHookEvent(	"events/sparks.sc",					EV_Sparks);
	gEngfuncs.pfnHookEvent(	"events/hornet_trail.sc",			EV_HornetTrail);
	gEngfuncs.pfnHookEvent(	"events/shards.sc",					EV_Shards);
	gEngfuncs.pfnHookEvent(	"events/spark.sc",					EV_Spark);
	gEngfuncs.pfnHookEvent(	"events/explode.sc",				EV_Explode);
	gEngfuncs.pfnHookEvent(	"events/bubbles.sc",				EV_BubbleExplode);
	gEngfuncs.pfnHookEvent(	"events/smoke.sc",					EV_Smoke);
	//gEngfuncs.pfnHookEvent( "events/ladder.sc",					EV_LadderClimb);
}

