#ifndef mugen_font_h
#define mugen_font_h

#include <fstream>
#include <string>
#include <map>

// Extend the font interface already made for paintown
#include "util/font.h"

class Bitmap;

enum FontType{
    Fixed = 0,
    Variable
};

struct FontLocation{
    int startx;
    int width;
};

class MugenFont : public Font{
public:
    MugenFont( const std::string & file );
    MugenFont( const char * file );
    MugenFont( const MugenFont &copy );
    virtual ~MugenFont();
    
    MugenFont & operator=( const MugenFont &copy );
    
    // Implement Font stuff
    virtual void setSize( const int x, const int y );
    virtual int getSizeX() const;
    virtual int getSizeY() const;
    
    virtual int textLength( const char * text ) const;

    virtual int getHeight( const std::string & str ) const;
    virtual int getHeight() const;

    virtual void printf( int x, int y, int color, const Bitmap & work, const std::string & str, int marker, ... ) const;
    virtual void printf( int x, int y, int xSize, int ySize, int color, const Bitmap & work, const std::string & str, int marker, ... ) const;
    
    virtual void render( int x, int y, int position, int bank, const Bitmap & work, const std::string & str );
    
    virtual void changeBank(int bank);
    
    inline int getCurrentBank() { return currentBank; };
    inline int getTotalBanks() { return colors; };

protected:
    unsigned char * findBank(int bank);
    
protected:
    // File
    std::ifstream ifile;
    std::string myfile;
    
    // defaults from mugen font file
    FontType type;
    int width;
    int height;
    int spacingx;
    int spacingy;
    int colors;
    int offsetx;
    int offsety;
    Bitmap * bmp;
    unsigned char *pcx;
    unsigned char palette[768];
    unsigned long pcxsize;
    // mapping positions of font in bitmap
    std::map< char, FontLocation> positions;
    
    int currentBank;
    
    void load();
};

#endif
