#include	"cbase.h"
class CCineBlood : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT BloodStart ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT BloodGush ( void );
};