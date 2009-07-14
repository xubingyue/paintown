#include "util/bitmap.h"
#include "menu/tab_menu.h"
#include "menu/menu_global.h"
#include "menu/menu_option.h"
#include "util/funcs.h"
#include "util/sound.h"
#include "util/font.h"
#include "util/token.h"
#include "util/tokenreader.h"
#include "resource.h"
#include "globals.h"
#include "init.h"
#include "configuration.h"
#include "music.h"

#include "menu/optionfactory.h"
#include "menu/actionfactory.h"
#include "menu/menu_global.h"
#include "menu/menu_animation.h"
#include "gui/keyinput_manager.h"

#include <queue>
#include <map>

using namespace std;

static int FONT_W = 16;
static int FONT_H = 16;
static int TEXT_SPACING_W = 10;
static int TEXT_SPACING_H = 5;

static void setColors (MenuBox *menu, const RectArea &info, const int fontColor){
    menu->position.body = info.body;
    menu->position.bodyAlpha = info.bodyAlpha;
    menu->position.border = info.border;
    menu->position.borderAlpha = info.borderAlpha;
    menu->fontColor = fontColor;
}

MenuBox::MenuBox(int w, int h):
snap(new Bitmap(w,h)),
fontColor(Bitmap::makeColor(255,255,255)){
    position.radius=15;
}

MenuBox::~MenuBox(){
    if (snap){
	delete snap;
    }
}

void MenuBox::updateSnapshot(){
    menu.drawMenuSnap(snap);
}

TabMenu::TabMenu():
location(0),
targetOffset(0),
totalOffset(0),
scrollSpeed(8.5){
}

void TabMenu::load(Token *token)throw( LoadException ){
    if ( *token != "tabmenu" )
	throw LoadException("Not a tabbed menu");
    else if ( ! token->hasTokens() )
	return;
    
    while ( token->hasTokens() ){
	try{
	    Token * tok;
	    *token >> tok;
	    if ( *tok == "name" ){
		// Set menu name
		std::string temp;
		*tok >> temp;
		setName(temp);
	    } else if ( *tok == "position" ) {
		// This handles the placement of the menu list and surrounding box
		*tok >> backboard.position.x >> backboard.position.y >> backboard.position.width >> backboard.position.height;
	    } else if ( *tok == "position-body" ) {
		// This handles the body color of the menu box
		int r,g,b;
		*tok >> r >> g >> b >> backboard.position.bodyAlpha;
		backboard.position.body = Bitmap::makeColor(r,g,b);
	    } else if ( *tok == "position-border" ) {
		// This handles the border color of the menu box
		int r,g,b;
		*tok >> r >> g >> b >> backboard.position.borderAlpha;
		backboard.position.border = Bitmap::makeColor(r,g,b);
	    } else if ( *tok == "tab-body" ) {
		// This handles the body color of the menu box
		int r,g,b;
		*tok >> r >> g >> b >> tabInfo.bodyAlpha;
		tabInfo.body = Bitmap::makeColor(r,g,b);
	    } else if ( *tok == "tab-border" ) {
		// This handles the border color of the menu box
		int r,g,b;
		*tok >> r >> g >> b >> tabInfo.borderAlpha;
		tabInfo.border = Bitmap::makeColor(r,g,b);
	    } else if ( *tok == "selectedtab-body" ) {
		// This handles the body color of the menu box
		int r,g,b;
		*tok >> r >> g >> b >> selectedTabInfo.bodyAlpha;
		selectedTabInfo.body = Bitmap::makeColor(r,g,b);
	    } else if ( *tok == "selectedtab-border" ) {
		// This handles the border color of the menu box
		int r,g,b;
		*tok >> r >> g >> b >> selectedTabInfo.borderAlpha;
		selectedTabInfo.border = Bitmap::makeColor(r,g,b);
	    } else if ( *tok == "font-color" ) {
		// This handles the font color of the menu box
		int r,g,b;
		*tok >> r >> g >> b;
		fontColor = Bitmap::makeColor(r,g,b);
	    } else if ( *tok == "selectedfont-color" ) {
		// This handles the font color of the menu box
		int r,g,b;
		*tok >> r >> g >> b;
		selectedFontColor = Bitmap::makeColor(r,g,b);
	    } else if( *tok == "anim" ) {
		MenuAnimation *animation = new MenuAnimation(tok);
		if (animation->getLocation() == 0){
		    backgroundAnimations.push_back(animation);
		} else if (animation->getLocation() == 1){
		    foregroundAnimations.push_back(animation);
		}
	    } else if (*tok == "menu"){
		MenuBox *menu = new MenuBox(backboard.position.width, backboard.position.height);
		if (menu){
		    // To avoid issues
		    menu->menu.setAsOption(true);
		    if (tok->numTokens() == 1){
			std::string temp;
			*tok >> temp;
			menu->menu.load(Util::getDataPath() + temp);
		    } else {
			menu->menu.load(tok);
		    }
		    tabs.push_back(menu);
		    const Font & vFont = Font::getFont(getFont(), FONT_W, FONT_H);
		    // set info on the box itself
		    menu->position.width = vFont.textLength(menu->menu.getName().c_str()) + TEXT_SPACING_W;
		    menu->position.height = vFont.getHeight() + TEXT_SPACING_H;
		} else {
		    throw LoadException("Problem reading menu");
		}
	    } else {
		Global::debug( 3 ) <<"Unhandled menu attribute: "<<endl;
		if (Global::getDebug() >= 3){
		    tok->print(" ");
		}
	    }
	} catch ( const TokenException & ex ) {
	    // delete current;
	    string m( "Menu parse error: " );
	    m += ex.getReason();
	    throw LoadException( m );
	} catch ( const LoadException & ex ) {
	    // delete current;
	    throw ex;
	}
    }
	
    if ( getName().empty() ){
	    throw LoadException("No name set, the menu should have a name!");
    }
}

void TabMenu::load(const std::string &filename) throw (LoadException){
    // Must check for initial token, menu
    try{
        TokenReader tr( filename );
        Token * token = tr.readToken();
        load(token);
    } catch (const TokenException & e){
        throw LoadException(e.getReason());
    }
}

void TabMenu::run(){
    //bool endMenu = false;
    bool done = false;

    if ( tabs.empty() ){
        return;
    }

    double runCounter = 0;
    Global::speed_counter = 0;
    Global::second_counter = 0;
    
    currentTab = tabs.begin();
    location = targetOffset = totalOffset = 0;
    // Set select color
    for (std::vector<MenuBox *>::iterator i = tabs.begin(); i != tabs.end(); ++i){
	if (i == currentTab){
	    setColors((*i),selectedTabInfo,selectedFontColor);
	} else {
	    setColors((*i),tabInfo,fontColor);
	}
    }
    
     // Reset fade stuff
    resetFadeInfo();
    
    // Reset animations
    for (std::vector<MenuAnimation *>::iterator i = backgroundAnimations.begin(); i != backgroundAnimations.end(); ++i){
	(*i)->reset();
    }
    for (std::vector<MenuAnimation *>::iterator i = foregroundAnimations.begin(); i != foregroundAnimations.end(); ++i){
	(*i)->reset();
    }
    while (!done){
	bool runMenu = false;
	while (!done && !runMenu){

	    bool draw = false;
	    //const char vi_up = 'k';
	    //const char vi_down = 'j';
	    const char vi_left = 'h';
	    const char vi_right = 'l';

	    keyInputManager::update();

	    if ( Global::speed_counter > 0 ){
		draw = true;
		runCounter += Global::speed_counter * Global::LOGIC_MULTIPLIER;
		while ( runCounter >= 1.0 ){
		    runCounter -= 1;
		    // Keys
	    
		    if (keyInputManager::keyState(keys::LEFT, true) ||
			keyInputManager::keyState(vi_left, true)){
			MenuGlobals::playSelectSound();
			// Reset color
			setColors((*currentTab),tabInfo,fontColor);
			if (currentTab > tabs.begin()){
                            currentTab--;
			    location--;
			    targetOffset+=backboard.position.width;
                        } else {
			    currentTab = tabs.end()-1;
			    location=tabs.size()-1;
			    targetOffset = (location*backboard.position.width) * -1;
                        }
			setColors((*currentTab),selectedTabInfo,selectedFontColor);
		    }

		    if ( keyInputManager::keyState(keys::RIGHT, true )||
			    keyInputManager::keyState(vi_right, true )){
			MenuGlobals::playSelectSound();
			// Reset color
			setColors((*currentTab),tabInfo,fontColor);
			if (currentTab < tabs.begin()+tabs.size()-1){
                            currentTab++;
			    location++;
			    targetOffset-=backboard.position.width;
                        } else {
                            currentTab = tabs.begin();
			    location= targetOffset = 0;
                        }
			setColors((*currentTab),selectedTabInfo,selectedFontColor);
		    }
		    /*
		    if (keyInputManager::keyState(keys::DOWN, true) ||
			keyInputManager::keyState(vi_down, true)){
			MenuGlobals::playSelectSound();
		    }

		    if ( keyInputManager::keyState(keys::UP, true )||
			    keyInputManager::keyState(vi_up, true )){
			MenuGlobals::playSelectSound();
		    }
    */
		    if ( keyInputManager::keyState(keys::ENTER, true ) ){
			// Run menu
			runMenu = true;
		    }
		    if (keyInputManager::keyState(keys::ESC, true )){
			done = true;
		    }
		    
		    // Animations
		    for (std::vector<MenuAnimation *>::iterator i = backgroundAnimations.begin(); i != backgroundAnimations.end(); ++i){
			(*i)->act();
		    }
		    for (std::vector<MenuAnimation *>::iterator i = foregroundAnimations.begin(); i != foregroundAnimations.end(); ++i){
			(*i)->act();
		    }
		    
		    // Lets do some logic for the box with text
		    updateFadeInfo();
		    
		    // Update offset
		    if (totalOffset > targetOffset){
			totalOffset-=scrollSpeed;
			if (totalOffset < targetOffset){
			    totalOffset = targetOffset;
			}
		    }
		    else if (totalOffset < targetOffset){
			totalOffset+=scrollSpeed;
			if (totalOffset > targetOffset){
			    totalOffset = targetOffset;
			}
		    }
		}

		Global::speed_counter = 0;
	    }

	    if ( draw ){
		// Draw
		drawBackground(work);
		
		// Do background animations
		for (std::vector<MenuAnimation *>::iterator i = backgroundAnimations.begin(); i != backgroundAnimations.end(); ++i){
		    (*i)->draw(work);
		}
		
		// Draw text board
		drawTextBoard(work);
		
		// Menus
		if (currentDrawState == NoFade){
		    drawSnapshots(work);
		}
		
		// Draw foreground animations
		for (std::vector<MenuAnimation *>::iterator i = foregroundAnimations.begin(); i != foregroundAnimations.end(); ++i){
		    (*i)->draw(work);
		}
		
		// Finally render to screen
		work->BlitToScreen();
	    }

	    while ( Global::speed_counter < 1 ){
		Util::rest( 1 );
		keyInputManager::update();
	    }
	}
	// Check runmenu
	if(runMenu){
	    // Run menu
	    (*currentTab)->menu.run();
	    throw ReturnException();
	}
    }
}

void TabMenu::drawSnapshots(Bitmap *bmp){
    const double incrementx = backboard.position.width;
    double startx = backboard.position.x + totalOffset;
    
    // Drawing snapshots
    for (std::vector<MenuBox *>::iterator i = tabs.begin(); i != tabs.end(); ++i){
	MenuBox *tab = *i;
	tab->updateSnapshot();
	/* Set clipping rectangle */
        int x1 = backboard.position.x+(backboard.position.radius/2);
        int y1 = backboard.position.y+(backboard.position.radius/2);
        int x2 = (backboard.position.x+backboard.position.width)-(backboard.position.radius/2);
        int y2 = (backboard.position.y+backboard.position.height)-(backboard.position.radius/2);
	bmp->setClipRect(x1, y1, x2, y2);
	tab->snap->Blit(startx,backboard.position.y, *bmp);
	bmp->setClipRect(0,0,bmp->getWidth(),bmp->getHeight());
	startx += incrementx;
    }
    const Font & vFont = Font::getFont(getFont(), FONT_W, FONT_H);
    int tabstartx = backboard.position.x;
    int tabstarty = backboard.position.y - (vFont.getHeight() + TEXT_SPACING_H);
    // Now draw tabs, has to be seperate from above since we need this to overlay the snaps
    for (std::vector<MenuBox *>::iterator i = tabs.begin(); i != tabs.end(); ++i){
	MenuBox *tab = *i;
	const int tabWidth = tab->position.width;
	if ((tabstartx + tabWidth) > (backboard.position.x + backboard.position.width)){
	    tabstartx = backboard.position.x;
	    tabstarty += tab->position.height;
	}
	tab->position.x = tabstartx;
	tab->position.y = tabstarty;
	tab->render(bmp);
	// Draw text
	vFont.printf(tabstartx + ((tabWidth/2)-(vFont.textLength(tab->menu.getName().c_str())/2)), tabstarty, tab->fontColor, *bmp, tab->menu.getName(), 0 );
	tabstartx+=tab->position.width;
    }
}

TabMenu::~TabMenu(){
    // Rid menus
    for (std::vector<MenuBox *>::iterator i = tabs.begin(); i != tabs.end(); ++i){
	if (*i){
	    delete *i;
	}
    }
}
