#include <allegro.h>

#include "bitmap.h"
#include "lit_bitmap.h"

LitBitmap::LitBitmap( const Bitmap & b ):
Bitmap( b ){
}

LitBitmap::LitBitmap():
Bitmap(){
}
	
LitBitmap::~LitBitmap(){
}

void LitBitmap::draw( const int x, const int y, const Bitmap & where ) const {
	::draw_sprite_ex( where.getBitmap(), getBitmap(), x, y, SPRITE_LIT );
}
	
void LitBitmap::drawHFlip( const int x, const int y, const Bitmap & where ) const {
	::draw_sprite_h_flip_ex( where.getBitmap(), getBitmap(), x, y, SPRITE_LIT );
}
