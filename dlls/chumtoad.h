class CChub : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	int  IRelationship( CBaseEntity *pTarget );
	void IdleSound ( void );
	virtual int	ObjectCaps( void ) 
	{ 
		int flags = CBaseToggle :: ObjectCaps() & (~FCAP_ACROSS_TRANSITION); 
		return flags | FCAP_IMPULSE_USE;
	}

	void RecalculateWaterlevel( void );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	Schedule_t  *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void EXPORT ChubUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void GibMonster( void );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void SetYawSpeed( void );

	BOOL FCanCheckAttacks ( void ) { return FALSE; } // chubby is passive
	BOOL IsBarnaclePrey ( void ) { return TRUE; }

	void EXPORT MonsterThink ( void );
	void EXPORT WaterDeathThink ( void );
	void Swim( void );

	CUSTOM_SCHEDULES;

	float	m_flBlink;
	BOOL	m_fPlayingDead;
	float	m_flPlayDeadTime;
	float	m_flNextCroak; // don't save
	BOOL	m_fInWater;
};