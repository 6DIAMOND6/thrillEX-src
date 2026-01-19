#define	CROSSBOW_BOLT_STANDARD				0
#define	CROSSBOW_BOLT_POISONED				1
#define	CROSSBOW_BOLT_EXPLOSIVE				2
#define	CROSSBOW_BOLT_SPEAR					3
#define	CROSSBOW_BOLT_BLOATER				4

class CCrossbowBolt : public CBaseEntity
{
public:
#ifndef CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT BubbleThink( void );
	void IgniteTrail( void );
	void EXPORT BoltTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );

private:
	int m_iTrail;
	int m_iBoltType;
public:
	static CCrossbowBolt *BoltCreate( int iBoltType );
};
