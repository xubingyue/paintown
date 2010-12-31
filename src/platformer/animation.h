#ifndef platformer_animation_h
#define platformer_animation_h

#include <string>
#include <vector>
#include <map>

#include "util/load_exception.h"

/*! Platformers temporary animation class (replace with some other workaround tied into the system later) */

class Bitmap;
class Token;

namespace Platformer{
    
typedef std::map< int, Bitmap *> imageMap;

class Frame{
    public:
	Frame(const Token *token, imageMap &) throw (LoadException);
	Frame(Bitmap *);
	virtual ~Frame();
	virtual void draw(int x, int y, const Bitmap &);
	
	virtual inline void setTime(int time){
	    this->time = time;
	}
	
	virtual inline const int getTime() const {
	    return this->time;
	}
	
	virtual inline void setHorizontalFlip(bool flip){
	    this->horizontalFlip = flip;
	}
	
	virtual inline void setVerticalFlip(bool flip){
	    this->verticalFlip = flip;
	}
	
	virtual inline void setAlpha(int alpha){
	    this->alpha = alpha;
	}
    private:
	/*! Bitmap of this frame */
	Bitmap * bmp;
	/*! Duration to show this frame (a duration of -1 is forever) */
	int time;
	/*! Flip the bitmap horizontally */
	bool horizontalFlip;
	/*! Flip the bitmap Vertically */
	bool verticalFlip;
	/*! Alpha to draw this bitmap at */
	int alpha;
};
    
class Animation{
    public:
	Animation(const Token *) throw (LoadException);
	~Animation();
	
	virtual void act();
	virtual void draw(int x, int y, const Bitmap &);
	virtual void forwardFrame();
	virtual void backFrame();
	
	virtual inline const std::string & getName() const {
	    return this->name;
	}
	
	virtual inline void setFrame(int frame){
	    this->currentFrame = frame;
	}

	virtual inline void reset(){ 
	    this->currentFrame = 0;
	}
    private:
	/*! Name of Animation */
	std::string name;
	/*! Ticks of current frame (No increment if frame time is -1) */
	int ticks;
	/*! Current Frame */
	unsigned int currentFrame;
	/*! Loop point */
	unsigned int loop;
	/*! All frames */
	std::vector<Frame *> frames;
	/*! Image map */
	imageMap images;
};
}
#endif