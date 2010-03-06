#ifndef _paintown_gui_context_box_h
#define _paintown_gui_context_box_h

#include <string>
#include <vector>

#include "gui/widget.h"
#include "gui/box.h"

namespace Gui{
    
class ContextItem{
    public:
	ContextItem();
	virtual ~ContextItem();
	
	virtual const std::string & getText()=0;
};

class ContextBox : public Widget{
    public:
        ContextBox();
        ContextBox(const ContextBox &);
        virtual ~ContextBox();
        
        //! copy
        ContextBox &operator=(const ContextBox &);
        //! Logic
        virtual void act();
        //! Render
        virtual void render(const Bitmap &);
        //! Next
        virtual void next();
        //! Previous
        virtual void previous();
	//! open context box
	virtual void open();
	//! Close context box
	virtual void close();
        //! Set context list
        virtual inline void setList(std::vector<ContextItem *> & list){
            this->context = &list;
            this->current = 0;
        }
        //! Set current font
	virtual inline void setFont(const std::string & font, int width, int height){
	    this->font = font;
	    this->fontWidth = width;
	    this->fontHeight = height;
	}
        //! Get current index
        virtual inline unsigned int getCurrentIndex(){
            return this->current;
        }
        //! Is active?
	virtual inline bool isActive(){
	    return (this->fadeState != NotActive);
	}
    private:
	
	void doFade();
	
	void drawText(const Bitmap &);
	
	enum FadeState{
	    NotActive,
	    FadeInBox,
	    FadeInText,
	    Active,
	    FadeOutText,
	    FadeOutBox,
	};
        //! Current index
        unsigned int current;
	
	//! Current fade state
	FadeState fadeState;

        //! Context list
        std::vector<ContextItem *> * context;
	
	//! Current font
	std::string font;
	int fontWidth;
	int fontHeight;
	
	//! Fade speed
	int fadeSpeed;
	
	//! Fade Aplha
	int fadeAlpha;
	
	//! Board
	Box board;
};

}

#endif
