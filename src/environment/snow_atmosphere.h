#ifndef _paintown_snow_atmosphere_h
#define _paintown_snow_atmosphere_h

class Bitmap;

#include "atmosphere.h"

struct Flake{
	Flake(int x, int y, int type, int angle ):
		x(x),
		y(y),
		dx(x),
		dy(y),
                angle(angle),
		type(type),
		dir(0),
                spin(0){}
	int x, y;
	double dx, dy;
        int angle;
	int type;
	int dir;
        int spin;
};

class SnowAtmosphere: public Atmosphere {
public:

	SnowAtmosphere();	
	virtual ~SnowAtmosphere();

	virtual void draw( Bitmap * work );
	virtual void act();
	
protected:
	vector< Flake * > flakes;
};

#endif
