#ifndef SPIT_H
#define SPIT_H

class CSpit : public CBaseAnimating
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );
};
#endif