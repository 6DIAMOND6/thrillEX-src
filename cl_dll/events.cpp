//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"

void Game_HookEvents( void );

extern "C"
{
	void EV_Sparks( struct event_args_s *args  );
	void EV_HornetTrail ( struct event_args_s *args );
	void EV_Shards ( struct event_args_s *args );
	void EV_Spark ( struct event_args_s *args  );
	void EV_Explode ( struct event_args_s *args  );
	void EV_BubbleExplode ( struct event_args_s *args  );
	void EV_Smoke ( struct event_args_s *args  );
}

/*
===================
EV_HookEvents

See if game specific code wants to hook any events.
===================
*/
void EV_HookEvents( void )
{
	Game_HookEvents();
}

