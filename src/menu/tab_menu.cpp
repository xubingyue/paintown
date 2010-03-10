#include "util/bitmap.h"
#include "menu/tab_menu.h"
#include "menu/menu_global.h"
#include "menu/menu_option.h"
#include "util/funcs.h"
#include "util/sound.h"
#include "util/font.h"
#include "util/token.h"
#include "util/tokenreader.h"
#include "util/file-system.h"
#include "resource.h"
#include "globals.h"
#include "init.h"
#include "configuration.h"
#include "music.h"

#include "menu/optionfactory.h"
#include "menu/actionfactory.h"
#include "menu/menu_global.h"
#include "menu/menu_animation.h"

#include "input/input-manager.h"
#include "input/input-map.h"

#include <queue>
#include <map>

using namespace std;
using namespace Gui;

static int FONT_W = 16;
static int FONT_H = 16;
static int TEXT_SPACING_W = 10;
static int TEXT_SPACING_H = 5;

ColorBuffer::ColorBuffer(int color1, int color2):
index(0),
forward(true){
    Util::blend_palette(colors,maxColors,color1,color2);
}

ColorBuffer::~ColorBuffer(){
}

int ColorBuffer::update(){
    // Going to color1 from color2
    if (forward){
        if (index<maxColors-1){
            index++;
        } else {
            forward=!forward;
        }
    } else {
        // Going to color2 from color1
        if (index>0){
            index--;
        } else {
            forward=!forward;
        }
    }

    return colors[index];
}

void ColorBuffer::reset(){
    index = 0;
}

MenuBox::MenuBox(int w, int h):
fontColor(Bitmap::makeColor(255,255,255)),
running(false){
    position.radius=15;
    snapPosition.position.width = w;
    snapPosition.position.height = h;
}

MenuBox::~MenuBox(){
}

bool MenuBox::checkVisible(const RectArea &area){
    return (snapPosition.position.x < area.x + area.width
	    && snapPosition.position.x + snapPosition.position.width > area.x
	    && snapPosition.position.y < area.y + area.height
	    && snapPosition.position.y + snapPosition.position.height > area.y);
}

void MenuBox::setColors (const RectArea &info, const int fontColor){
    position.body = info.body;
    position.bodyAlpha = info.bodyAlpha;
    position.border = info.border;
    position.borderAlpha = info.borderAlpha;
    this->fontColor = fontColor;
}

void MenuBox::setColors (const int bodyColor, const int borderColor, const int fontColor){
    position.body = bodyColor;
    position.border = borderColor;
    this->fontColor = fontColor;
}

TabMenu::TabMenu():
fontColor(Bitmap::makeColor(150,150,150)),
selectedFontColor(Bitmap::makeColor(0,255,255)),
runningFontColor(Bitmap::makeColor(255,255,0)),
runningInfo(""),
location(0),
targetOffset(0),
totalOffset(0),
totalLines(1){
}

void TabMenu::load(Token *token) throw (LoadException){
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
                //*tok >> backboard.position.x >> backboard.position.y >> backboard.position.width >> backboard.position.height;
                *tok >> contextMenu.position.x >> contextMenu.position.y >> contextMenu.position.width >> contextMenu.position.height;
            } else if ( *tok == "position-body" ) {
                // This handles the body color of the menu box
                int r,g,b;
                /*
                *tok >> r >> g >> b >> backboard.position.bodyAlpha;
                backboard.position.body = Bitmap::makeColor(r,g,b);
                */
                *tok >> r >> g >> b >> contextMenu.position.bodyAlpha;
                contextMenu.position.body = Bitmap::makeColor(r,g,b);
            } else if ( *tok == "position-border" ) {
                // This handles the border color of the menu box
                int r,g,b;
                /*
                *tok >> r >> g >> b >> backboard.position.borderAlpha;
                backboard.position.border = Bitmap::makeColor(r,g,b);
                */
                *tok >> r >> g >> b >> contextMenu.position.borderAlpha;
                contextMenu.position.border = Bitmap::makeColor(r,g,b);
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
            } else if ( *tok == "runningtab-body" ) {
                // This handles the body color of the menu box
                int r,g,b;
                *tok >> r >> g >> b >> selectedTabInfo.bodyAlpha;
                runningTabInfo.body = Bitmap::makeColor(r,g,b);
            } else if ( *tok == "runningtab-border" ) {
                // This handles the border color of the menu box
                int r,g,b;
                *tok >> r >> g >> b >> selectedTabInfo.borderAlpha;
                runningTabInfo.border = Bitmap::makeColor(r,g,b);
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
            } else if ( *tok == "runningfont-color" ) {
                // This handles the font color of the menu box
                int r,g,b;
                *tok >> r >> g >> b;
                runningFontColor = Bitmap::makeColor(r,g,b);
            } else if( *tok == "anim" ) {
                MenuAnimation *animation = new MenuAnimation(tok);
                if (animation->getLocation() == 0){
                    backgroundAnimations.push_back(animation);
                } else if (animation->getLocation() == 1){
                    foregroundAnimations.push_back(animation);
                }
            } else if (*tok == "menu"){
                //MenuBox *menu = new MenuBox(backboard.position.width, backboard.position.height);
                MenuBox *menu = new MenuBox(contextMenu.position.width, contextMenu.position.height);
                if (menu){
                    // To avoid issues
                    menu->menu.setAsOption(true);
                    if (tok->numTokens() == 1){
                        std::string temp;
                        *tok >> temp;
                        menu->menu.load(Filesystem::find(Filesystem::RelativePath(temp)));
                    } else {
                        menu->menu.load(tok);
                    }

                    /* if the menu didn't have any options then we shouldn't include
                     * it. if we try to render a menu without options we will
                     * crash later on.
                     */
                    if (menu->menu.hasSomeOptions()){
                        tabs.push_back(menu);
                        const Font & vFont = Font::getFont(getFont(), FONT_W, FONT_H);
                        // set info on the box itself
                        menu->position.width = vFont.textLength(menu->menu.getName().c_str()) + TEXT_SPACING_W;
                        menu->position.height = vFont.getHeight() + TEXT_SPACING_H;
                    } else {
                        delete menu;
                    }
                } else {
                    throw LoadException("Problem reading menu");
                }
            } else if( *tok == "menuinfo" ){
                *tok >> menuInfo;
            } else if( *tok == "menuinfo-position" ){
                *tok >> menuInfoLocation.x >> menuInfoLocation.y;
            } else if( *tok == "runninginfo" ){
                *tok >> runningInfo;
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

    if (!menuInfo.empty()){
        if (! menuInfoLocation.x || ! menuInfoLocation.y){
            throw LoadException("The position for the menu info box in \"" + getName() + "\" must be set since there menuinfo is set!"); 
        }
    }

    // Set totalLines
    calculateTabLines();
}

/* FIXME: this method is a duplicate of Menu::load */
void TabMenu::load(const Filesystem::AbsolutePath & filename) throw (LoadException){
    // Must check for initial token, menu
    try{
        TokenReader tr(filename.path());
        Token * token = tr.readToken();
        load(token);
    } catch (const TokenException & e){
        throw LoadException(e.getReason());
    }
}

/* c++ isn't smart enough for me to put the enum inside a function, run(),
 * so I have to put it in the global scope wrapped with a namespace.
 */
namespace Tab{
    enum Input{
        Left,
        Right,
        Select,
        Exit,
    };
}

static bool closeFloat(double a, double b){
    const double epsilon = 0.001;
    return fabs(a-b) < epsilon;
}

void TabMenu::run(){
    //bool endMenu = false;
    bool done = false;

    if ( tabs.empty() ){
        return;
    }

    double runCounter = 0;
    Global::speed_counter = 0;
    int scrollCounter = 0;

    InputMap<Tab::Input> input;
    input.set(Keyboard::Key_H, 0, true, Tab::Left);
    input.set(Keyboard::Key_L, 0, true, Tab::Right);
    input.set(Keyboard::Key_LEFT, 0, true, Tab::Left);
    input.set(Keyboard::Key_RIGHT, 0, true, Tab::Right);
    input.set(Keyboard::Key_ENTER, 0, true, Tab::Select);
    input.set(Keyboard::Key_SPACE, 0, true, Tab::Select);
    input.set(Keyboard::Key_ESC, 0, true, Tab::Exit);
    input.set(Joystick::Left, 0, true, Tab::Left);
    input.set(Joystick::Right, 0, true, Tab::Right);
    input.set(Joystick::Button1, 0, true, Tab::Select);
    input.set(Joystick::Button2, 0, true, Tab::Exit);

    // Color effects
    ColorBuffer fontBuffer(selectedFontColor,runningFontColor);
    ColorBuffer borderBuffer(selectedTabInfo.border,runningTabInfo.border);
    ColorBuffer backgroundBuffer(selectedTabInfo.body,runningTabInfo.body);

    currentTab = tabs.begin();
    location = 0;
    targetOffset = 0;
    totalOffset = 0;
    // Set select color
    for (std::vector<MenuBox *>::iterator i = tabs.begin(); i != tabs.end(); ++i){
        if (i == currentTab){
            (*i)->setColors(selectedTabInfo,selectedFontColor);
        } else {
            (*i)->setColors(tabInfo,fontColor);
        }
    }

    // Reset fade stuff
    //resetFadeInfo();

    // Reset animations
    for (std::vector<MenuAnimation *>::iterator i = backgroundAnimations.begin(); i != backgroundAnimations.end(); ++i){
        (*i)->reset();
    }
    for (std::vector<MenuAnimation *>::iterator i = foregroundAnimations.begin(); i != foregroundAnimations.end(); ++i){
        (*i)->reset();
    }

    while (!done){

        bool draw = false;
        /*
        //const char vi_up = 'k';
        //const char vi_down = 'j';
        const char vi_left = 'h';
        const char vi_right = 'l';
        */

        InputManager::poll();

        if ( Global::speed_counter > 0 ){
            draw = true;
            runCounter += Global::speed_counter * Global::LOGIC_MULTIPLIER;
            while ( runCounter >= 1.0 ){
                runCounter -= 1;
                InputMap<Tab::Input>::Output inputState = InputManager::getMap(input);
                // Keys
                if (!(*currentTab)->running){
                    if (inputState[Tab::Left]){
                        MenuGlobals::playSelectSound();
                        // Reset color
                        (*currentTab)->setColors(tabInfo,fontColor);
                        if (currentTab > tabs.begin()){
                            currentTab--;
                            location--;
                            //targetOffset+=backboard.position.width;
                            targetOffset+=contextMenu.position.width;
                        } else {
                            currentTab = tabs.end()-1;
                            location=tabs.size()-1;
                            //targetOffset = (location*backboard.position.width) * -1;
                            targetOffset = (location*contextMenu.position.width) * -1;
                        }
                        (*currentTab)->setColors(selectedTabInfo,selectedFontColor);
                    }

                    if (inputState[Tab::Right]){
                        MenuGlobals::playSelectSound();
                        // Reset color
                        (*currentTab)->setColors(tabInfo,fontColor);
                        if (currentTab < tabs.begin()+tabs.size()-1){
                            currentTab++;
                            location++;
                            //targetOffset-=backboard.position.width;
                            targetOffset-=contextMenu.position.width;
                        } else {
                            currentTab = tabs.begin();
                            location = 0;
                            targetOffset = 0;
                        }
                        (*currentTab)->setColors(selectedTabInfo,selectedFontColor);
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
                    if (inputState[Tab::Select]){
                        /* im not sure why we have to wait for select to
                         * be released here. all the other menus seem
                         * to work just fine without waiting.
                         * anyway, no real harm comes from waiting so just wait.
                         */
                        InputManager::waitForRelease(input, Tab::Select);
                        // Run menu
                        (*currentTab)->running = true;
                        backgroundBuffer.reset();
                        borderBuffer.reset();
                        fontBuffer.reset();
                    }

                    if (inputState[Tab::Exit]){
                        /* is there a reason to set done = true ? */
                        done = true;
                        InputManager::waitForRelease(input, Tab::Exit);
                        throw ReturnException();
                    }
                } else {
                    try{
                        (*currentTab)->menu.act(done, false);
                        (*currentTab)->setColors(backgroundBuffer.update(),borderBuffer.update(),fontBuffer.update());
                    } catch (const ReturnException & re){
                        (*currentTab)->running = false;
                        (*currentTab)->menu.selectedOption->setState(MenuOption::Selected);
                        (*currentTab)->setColors(selectedTabInfo, selectedFontColor);
                    }
                }

                /*
                if (inputState[Exit]){
                    if (!(*currentTab)->running){
                        done = true;
                        InputManager::waitForRelease(input, Exit);
                        throw ReturnException();
                    } else {
                        (*currentTab)->running = false;
                        (*currentTab)->setColors(selectedTabInfo,selectedFontColor);
                    }
                }
                */

                // Animations
                for (std::vector<MenuAnimation *>::iterator i = backgroundAnimations.begin(); i != backgroundAnimations.end(); ++i){
                    (*i)->act();
                }
                for (std::vector<MenuAnimation *>::iterator i = foregroundAnimations.begin(); i != foregroundAnimations.end(); ++i){
                    (*i)->act();
                }

                // Lets do some logic for the box with text
                //updateFadeInfo();

                if (scrollCounter == 0 && !closeFloat(totalOffset, targetOffset)){
                    totalOffset = (totalOffset + targetOffset) / 2;
                    /* not sure if this is stricly necessary */
                    if (fabs(targetOffset - totalOffset) < 5){
                        totalOffset = targetOffset;
                    }
                }
                /* higher values of % X slow down scrolling */
                scrollCounter = (scrollCounter + 1) % 5;

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
            //drawTextBoard(work);

            // Menus
            if (currentDrawState == NoFade){
                drawMenus(work);
            }

            // Draw menu info text
            if (!(*currentTab)->running){
                drawInfoBox(menuInfo, menuInfoLocation, work);
            } else {
                drawInfoBox(runningInfo, menuInfoLocation, work);
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
            InputManager::poll();
        }
    }
}

void TabMenu::drawMenus(Bitmap *bmp){
    Gui::RectArea & backboard = contextMenu.position;
    const double incrementx = backboard.width;
    double startx = backboard.x + totalOffset;

    // Drawing menus
    for (std::vector<MenuBox *>::iterator i = tabs.begin(); i != tabs.end(); ++i){
        MenuBox *tab = *i;
        tab->snapPosition.position.x = (int) startx;
        tab->snapPosition.position.y = backboard.y;
        if (tab->checkVisible(backboard)){
            /* Set clipping rectangle need to know why text isn't clipping */
            int x1 = backboard.x+(backboard.radius/2);
            int y1 = backboard.y+(backboard.radius/2);
            int x2 = (backboard.x+backboard.width)-(backboard.radius/2);
            int y2 = (backboard.y+backboard.height)-(backboard.radius/2);
            bmp->setClipRect(x1, y1, x2, y2);
            //tab->menu.drawText(tab->snapPosition,bmp);
            bmp->setClipRect(0,0,bmp->getWidth(),bmp->getHeight());
        }
        startx += incrementx;
    }
    const Font & vFont = Font::getFont(getFont(), FONT_W, FONT_H);
    int tabstartx = backboard.x;
    int tabstarty = backboard.y - ((vFont.getHeight() + TEXT_SPACING_H) * totalLines);
    // Now draw tabs, has to be seperate from above since we need this to overlay the snaps
    for (std::vector<MenuBox *>::iterator i = tabs.begin(); i != tabs.end(); ++i){
        MenuBox *tab = *i;
        const int tabWidth = tab->position.width;
        if ((tabstartx + tabWidth) > (backboard.x + backboard.width)){
            tabstartx = backboard.x;
            tabstarty += tab->position.height;
        }
        tab->position.x = tabstartx;
        tab->position.y = tabstarty;
        tab->render(*bmp);
        // Draw text
        vFont.printf(tabstartx + ((tabWidth/2)-(vFont.textLength(tab->menu.getName().c_str())/2)), tabstarty, tab->fontColor, *bmp, tab->menu.getName(), 0 );
        tabstartx += tab->position.width;
    }
}

// Calculate the amount of lines per tabs
void TabMenu::calculateTabLines(){
    int tabstartx = contextMenu.position.x;//backboard.position.x;
    for (std::vector<MenuBox *>::iterator i = tabs.begin(); i != tabs.end(); ++i){
        MenuBox *tab = *i;
        const int tabWidth = tab->position.width;
        if ((tabstartx + tabWidth) > (contextMenu.position.x + contextMenu.position.width)){//(backboard.position.x + backboard.position.width)){
            tabstartx = contextMenu.position.x;//backboard.position.x;
            totalLines++;
        }
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
