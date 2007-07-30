#include "projectile.h"
#include "object_attack.h"
#include "util/token.h"
#include "util/token_exception.h"
#include "globals.h"
#include "animation.h"
#include <iostream>

using namespace std;

/* the alliance must be set by someone else at some point */
Projectile::Projectile( Token * token ) throw( LoadException ):
ObjectAttack( ALLIANCE_NONE ),
main( NULL ),
death( NULL ),
dx( 0 ),
dy( 0 ),
life( 0 ){

	if ( *token != "projectile" ){
		throw LoadException( "Token does not start with 'projectile'. Instead it starts with " + token->getName() );
	}

	Token * current;
	while ( token->hasTokens() ){
		try{
			*token >> current;
			if ( *current == "anim" ){
				Animation * animation = new Animation( current, NULL );
				if ( animation->getName() == "main" ){
					main = animation;
				} else if ( animation->getName() == "death" ){
					death = animation;
				} else {
					cout << "Unknown animation for projectile: " + animation->getName() + ". Must be either 'main' or 'death'." << endl;
					delete animation;
				}
			}
		} catch ( const TokenException & e ){
			throw LoadException( "Could not load projectile because " + e.getReason() );
		}
	}

	if ( main == NULL ){
		throw LoadException( "No 'main' animation given" );
	}
}
	
Projectile::Projectile( const Projectile * const projectile ):
ObjectAttack( projectile->getAlliance() ),
main( new Animation( *projectile->main, NULL ) ),
death( NULL ),
dx( projectile->getDX() ),
dy( projectile->getDY() ),
life( projectile->getLife() ){
	if ( projectile->death != NULL ){
		death = new Animation( *projectile->death, NULL );
	}
}
	
Projectile::~Projectile(){
	if ( main ){
		delete main;
	}
	if ( death ){
		delete death;
	}
}
	
const int Projectile::getHealth() const {
	return 1;
}

void Projectile::act( vector< Object * > * others, World * world, vector< Object * > * add ){
	moveX( getDX() );
	moveY( getDY() );
	if ( main->Act() ){
		main->reset();
	}
}

void Projectile::draw( Bitmap * work, int rel_x ){
	if ( getFacing() == Object::FACING_RIGHT ){
		main->Draw( getRX() - rel_x, getRY(), work );
	} else {
		main->DrawFlipped( getRX() - rel_x, getRY(), work ); 
	}
}

void Projectile::grabbed( Object * obj ){
}

void Projectile::unGrab(){
}

Object * Projectile::copy(){
	return new Projectile( this );
}

const std::string & Projectile::getAttackName(){
	return name;
}

bool Projectile::isAttacking(){
	return true;
}

bool Projectile::collision( ObjectAttack * obj ){
	return false;
}

int Projectile::getDamage() const {
	return 0;
}

bool Projectile::isCollidable( Object * obj ){
	return true;
}

bool Projectile::isGettable(){
	return false;
}

const int Projectile::getWidth() const {
	return 0;
}

const int Projectile::getHeight() const {
	return 0;
}

void Projectile::getAttackCoords( int & x, int & y){
}

const double Projectile::minZDistance() const {
	return 0;
}

void Projectile::attacked( Object * something, vector< Object * > & objects ){
}
