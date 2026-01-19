#ifndef TRI_H
#define TRI_H

#include "particleman.h"

extern IParticleMan *g_pParticleMan;

typedef struct
{
	// scissor test
	int		scissor_x;
	int		scissor_y;
	int		scissor_width;
	int		scissor_height;
	qboolean		scissor_test;

	wrect_t		rcCrosshair;
	int			*rgbaCrosshair;
} tri_draw_t;




#endif //TRI_H