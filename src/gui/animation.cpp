#include "gui/animation.h"

#include <vector>
#include <math.h>
#include <sstream>
#include "util/token.h"
#include "util/bitmap.h"
#include "globals.h"
#include "util/funcs.h"
#include "util/file-system.h"

using namespace std;
using namespace Gui;

// Temporary solution
static void renderSprite(const Bitmap & bmp, const int x, const int y, const int alpha, const bool hflip, const bool vflip, const Bitmap & work){
    if (alpha != 255){
	    Bitmap::transBlender( 0, 0, 0, alpha );
	    if (hflip && !vflip){
	        bmp.drawTransHFlip(x,y, work);
	    } else if (!hflip && vflip){
	        bmp.drawTransVFlip(x,y, work);
	    } else if (hflip && vflip){
	        bmp.drawTransHVFlip(x,y, work);
	    } else if (!hflip && !vflip){
	        bmp.drawTrans(x,y, work);
	    }
    }
    else {
	    if (hflip && !vflip){
	        bmp.drawHFlip(x,y, work);
	    } else if (!hflip && vflip){
	        bmp.drawVFlip(x,y, work);
	    } else if (hflip && vflip){
	        bmp.drawHVFlip(x,y, work);
	    } else if (!hflip && !vflip){
	        bmp.draw(x,y, work);
	    }
    }
}

AnimationPoint::AnimationPoint():x(0),y(0){}
AnimationArea::AnimationArea():x1(0),y1(0),x2(0),y2(0){}

Frame::Frame(Token *the_token, imageMap &images) throw (LoadException):
bmp(0),
time(0),
horizontalFlip(false),
verticalFlip(false),
alpha(255){
    if ( *the_token != "frame" ){
        throw LoadException(__FILE__, __LINE__, "Not an frame");
    }
    Token tok(*the_token);
    /* The usual setup of an animation frame is
    // use image -1 to not draw anything, it can be used to get a blinking effect
    (frame (image NUM) (alpha NUM) (offset x y) (hflip 0|1) (vflip 0|1) (time NUM))
    */
    while ( tok.hasTokens() ){
        try{
            Token * token;
            tok >> token;
            if (*token == "image"){
                // get the number
                int num;
                *token >> num;
                // now assign the bitmap
                bmp = images[num];
            } else if (*token == "alpha"){
                // get alpha
                *token >> alpha;
            } else if (*token == "offset"){
                // Get the offset location it defaults to 0,0
                *token >> offset.x >> offset.y;
            } else if (*token == "hflip"){
                // horizontal flip
                *token >> horizontalFlip;
            } else if (*token == "vflip"){
                // horizontal flip
                *token >> verticalFlip;
            } else if (*token == "time"){
                // time to display
                *token >> time;
            } else {
                Global::debug( 3 ) << "Unhandled menu attribute: "<<endl;
                if (Global::getDebug() >= 3){
                    token->print(" ");
                }
            }
        } catch ( const TokenException & ex ) {
            throw LoadException(__FILE__, __LINE__, ex, "Menu animation parse error");
        } catch ( const LoadException & ex ) {
            throw ex;
        }
    }
}

Frame::~Frame(){
}

void Frame::act(const double xvel, const double yvel){
    scrollOffset.x +=xvel;
    scrollOffset.y +=yvel;
    if (scrollOffset.x >=bmp->getWidth()){
	    scrollOffset.x = 0;
    } else if (scrollOffset.x <= -(bmp->getWidth())){
	    scrollOffset.x = 0;
    }
    if (scrollOffset.y >=bmp->getHeight()){
	    scrollOffset.y = 0;
    } else if (scrollOffset.y <= -(bmp->getHeight())){
	    scrollOffset.y = 0;
    }
}

static bool closeFloat(double a, double b){
    const double epsilon = 0.001;
    return fabs(a-b) < epsilon;
}

void Frame::draw(const int xaxis, const int yaxis, const Bitmap & work){
    if (!bmp){
        return;
    }

    if (!closeFloat(scrollOffset.x, 0) || !closeFloat(scrollOffset.y, 0)){

        // Lets do some scrolling
        Bitmap temp = Bitmap::temporaryBitmap(bmp->getWidth(), bmp->getHeight());
        AnimationPoint loc;
        if (scrollOffset.x < 0){
            loc.x = scrollOffset.x + bmp->getWidth();
        } else if (scrollOffset.x > 0){
            loc.x = scrollOffset.x - bmp->getWidth();
        }
        if (scrollOffset.y < 0){
            loc.y = scrollOffset.y + bmp->getHeight();
        } else if (scrollOffset.y > 0){
            loc.y = scrollOffset.y - bmp->getHeight();
        }
        bmp->Blit((int) scrollOffset.x, (int) scrollOffset.y, temp);
        bmp->Blit((int) scrollOffset.x, (int) loc.y, temp);
        bmp->Blit((int) loc.x, (int) scrollOffset.y, temp);
        bmp->Blit((int) loc.x, (int) loc.y, temp);

        renderSprite(temp, (int)(xaxis+offset.x), (int)(yaxis+offset.y), alpha, horizontalFlip, verticalFlip, work);

    } else {
        renderSprite(*bmp, (int)(xaxis+offset.x), (int)(yaxis+offset.y), alpha, horizontalFlip, verticalFlip, work);
    }
}

Animation::Animation(Token *the_token) throw (LoadException):
id(0),
depth(Background0),
ticks(0),
currentFrame(0),
loop(0),
allowReset(true){
    images[-1] = 0;
    std::string basedir = "";
    if ( *the_token != "anim" ){
        throw LoadException(__FILE__, __LINE__, "Not an animation");
    }
    /* The usual setup of an animation is
	The images must be listed prior to listing any frames, basedir can be used to set the directory where the images are located
	loop will begin at the subsequent frame listed after loop
	axis is the location in which the drawing must be placed
	location *old* - used to render in background or foreground (0 == background [default]| 1 == foreground)
    depth - used to render in background or foreground space (depth background 0) | (depth foreground 1)
	reset - used to allow resetting of animation (0 == no | 1 == yes [default])
	velocity - used to get a wrapping scroll effect while animating
	(anim (id NUM) 
	      (location NUM)
          (depth background|foreground NUM)
	      (basedir LOCATION)
	      (image NUM FILE) 
	      (velocity x y)
	      (axis x y) 
	      (frame "Read comments above in constructor") 
	      (loop)
	      (reset NUM)
	      (window x1 y1 x2 y2))
    */
    Token tok(*the_token);
    while ( tok.hasTokens() ){
        try{
            Token * token;
            tok >> token;
            if (*token == "id"){
                // get the id
                *token >> id;
            } else if (*token == "location"){
                // translate location to depth
                int location = 0;
                *token >> location;
                if (location == 0){
                    depth = Background0;
                } else if (location == 1){
                    depth = Foreground0;
                }
            } else if (*token == "depth"){
                // get the depth
                std::string name;
                int level;
                *token >> name >> level;
                if (name == "background"){
                    switch (level){
                        case 0: depth = Background0;break;
                        case 1: depth = Background1;break;
                        default: depth = Background0;break;
                    }
                } else if (name == "foreground"){
                    switch (level){
                        case 0: depth = Foreground0;break;
                        case 1: depth = Foreground1;break;
                        default: depth = Foreground0;break;
                    }
                }
            } else if (*token == "basedir"){
                // set the base directory for loading images
                *token >> basedir;
            } else if (*token == "image"){
                // add bitmaps by number to the map
                int number;
                std::string temp;
                *token >> number >> temp;
                Bitmap *bmp = new Bitmap(Filesystem::find(Filesystem::RelativePath(basedir + temp)).path());
                if (bmp->getError()){
                    delete bmp;
                } else {
                    images[number] = bmp;
                }
            } else if (*token == "axis"){
                // Get the axis location it defaults to 0,0
                *token >> axis.x >> axis.y;
            } else if (*token == "window"){
                // time to display
                *token >> window.x1 >> window.y1 >> window.x2 >> window.y2;
            } else if (*token == "velocity"){
                // This allows the animation to get a wrapping scroll action going on
                *token >> velocity.x >> velocity.y;
            } else if (*token == "frame"){
                // new frame
                Frame *frame = new Frame(token,images);
                frames.push_back(frame);
            } else if (*token == "loop"){
                // start loop here
                loop = frames.size();
            } else if (*token == "reset"){
                // Allow reset of animation
                *token >> allowReset;
            } else {
                Global::debug( 3 ) << "Unhandled menu attribute: "<<endl;
                if (Global::getDebug() >= 3){
                    token->print(" ");
                }
            }
        } catch ( const TokenException & ex ) {
            throw LoadException(__FILE__, __LINE__, ex, "Menu animation parse error");
        } catch ( const LoadException & ex ) {
            throw ex;
        }
    }
    if (loop >= frames.size()){
        ostringstream out;
        out << "Loop location is larger than the number of frames. Loop: " << loop << " Frames: " << frames.size();
        throw LoadException(__FILE__, __LINE__, out.str());
    }
}

Animation::~Animation(){
    for (std::vector<Frame *>::iterator i = frames.begin(); i != frames.end(); ++i){
	    if (*i){
	        delete *i;
	    }
    }
    for (imageMap::iterator i = images.begin(); i != images.end(); ++i){
	    if (i->second){
	        delete i->second;
	    }
    }
}
void Animation::act(){
    // Used for scrolling
    for (std::vector<Frame *>::iterator i = frames.begin(); i != frames.end(); ++i){
	    (*i)->act(velocity.x, velocity.y);
    }
    if( frames[currentFrame]->time != -1 ){
	    ticks++;
	    if(ticks >= frames[currentFrame]->time){
		    ticks = 0;
		    forwardFrame();
	    }
    }
}
void Animation::draw(const Bitmap & work){
    /* should use sub-bitmaps here */
     // Set clip from the axis default is 0,0,bitmap width, bitmap height
    work.setClipRect(window.x1,window.y1,work.getWidth() + window.x2,work.getHeight() + window.y2);
    frames[currentFrame]->draw((int) axis.x, (int) axis.y,work);
    work.setClipRect(0,0,work.getWidth(),work.getHeight());
}

void Animation::forwardFrame(){
    if (currentFrame < frames.size() -1){
        currentFrame++;
    } else {
        currentFrame = loop;
    }
}
void Animation::backFrame(){
    if (currentFrame > loop){
	    currentFrame--;
    } else {
	    currentFrame = frames.size() - 1;
    }
}


