#ifndef _paintown_tab_menu_h
#define _paintown_tab_menu_h

#include "menu.h"

class Font;
class Bitmap;
class MenuOption;
class Token;
class MenuAnimation;

class TabMenu : public Menu
{
    public:
	/*! ctor dtor */
	TabMenu();
	virtual ~TabMenu();
	
	/*! load */
	void load(const std::string &filename) throw (LoadException);
	
	/*! load */
	void load(Token *token) throw (LoadException);
	
	/*! do logic, draw whatever */
	void run();
    private:
	
	std::string name;
	
	std::vector <Menu *> menus;
	    
};
#endif
