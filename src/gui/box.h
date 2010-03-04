#ifndef _paintown_gui_box_h
#define _paintown_gui_box_h

#include <string>

#include "gui/widget.h"

namespace Gui{

class Box : public Widget{
	public:
		Box();
		Box( const Box & b );
		virtual ~Box();
		
		// copy
		Box &operator=( const Box &);
		
		// Logic
		virtual void act();
		
		// Render
		virtual void render(const Bitmap &);
		
		// Do a message Dialog centered on the given bitmap
		static void msgDialog(const Bitmap & bmp, const std::string & message, int radius = 0);
	
	protected:
};

}

#endif
