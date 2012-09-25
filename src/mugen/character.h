#ifndef paintown_mugen_character_h
#define paintown_mugen_character_h

#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include "exception.h"

// #include "util/network/network.h"
#include "util/pointer.h"
#include "util/input/input-map.h"
#include "animation.h"
#include "util.h"
#include "compiler.h"
#include "util/bitmap.h"
#include "object.h"
#include "common.h"
#include "sprite.h"

namespace Ast{
    class KeyList;
    class Key;
    class Value;
    class Section;
}

/*
namespace Mugen{
namespace Compiler{
    class Value;
}
}
*/

namespace PaintownUtil = Util;

namespace Graphics{
class Bitmap;
}
class MugenItemContent;

namespace Mugen{

class Sound;
class Sprite;
class Animation;
extern PaintownUtil::Parameter<Filesystem::RelativePath> stateFileParameter;

class Behavior;
class Stage;

struct Constant{
    enum ConstantType{
        None,
        Double,
        ListOfDouble,
    };

    Constant():
    type(None){
    }

    Constant(double value):
    type(Double),
    double_(value){
    }

    Constant(std::vector<double> doubles):
    type(ListOfDouble),
    doubles(doubles){
    }

    ConstantType type;

    double double_;
    std::vector<double> doubles;
};

namespace StateType{
    extern std::string Stand;
    extern std::string Crouch;
    extern std::string Air;
    extern std::string LyingDown;
}

namespace Move{
    extern std::string Attack;
    extern std::string Idle;
    extern std::string Hit;
}


struct WinGame{
    /* TODO: add an explanation for each win type that describes how to
     * achieve that state.
     */
    enum WinType{
        Normal,
        Special,
        Hyper,
        NormalThrow,
        Cheese,
        TimeOver,
        Suicide,
        Teammate,
        /* Overlayed */
        /* is this needed now that the `perfect' bool exists? */
        Perfect,
    };

    WinGame():
    type(Normal),
    perfect(false){
    }

    WinType type;
    bool perfect;

};

namespace PhysicalAttack{
    extern std::string Normal;
    extern std::string Throw;
    extern std::string Projectile;
}

class Character;

struct HitState{
    HitState():
        shakeTime(0),
        hitTime(-1),
        hits(0),
        slideTime(0),
        returnControlTime(0),
        recoverTime(0),
        yAcceleration(0),
        yVelocity(0),
        xVelocity(0),
        guarded(false),
        damage(0),
        chainId(-1),
        spritePriority(0),
        moveContact(0){
        }

    void update(Mugen::Stage & stage, const Character & guy, bool inAir, const HitDefinition & hit);
    int shakeTime;
    int hitTime;
    
    /* FIXME: handle hits somehow. corresponds to hitcount
     * hitcount: Returns the number of hits taken by the player in current combo. (int)
     */
    int hits;
    int slideTime;
    int returnControlTime;
    int recoverTime;
    double yAcceleration;
    double yVelocity;
    double xVelocity;
    AttackType::Animation animationType;
    AttackType::Ground airType;
    AttackType::Ground groundType;
    AttackType::Ground hitType;
    bool guarded;
    int damage;
    int chainId;
    int spritePriority;

    struct Fall{
        Fall():
            fall(false),
            recover(true),
            recoverTime(0),
            xVelocity(0),
            yVelocity(0),
            changeXVelocity(false),
            damage(0){
            }

        struct Shake{
            int time;
        } envShake;

        bool fall;
        bool recover;
        int recoverTime;
        double xVelocity;
        double yVelocity;
        bool changeXVelocity;
        double damage;
    } fall;

    int moveContact;
};

struct RecordingInformation{
    std::ofstream out;
    /* last set of commands */
    std::vector<std::string> commands;
    /* last count of ticks for the current set of commands */
    int ticks;

    ~RecordingInformation(){
        out.close();
    }
};

struct HitOverride{
    HitOverride():
        time(0){
        }

    int time;
    HitAttributes attributes;
    int state;
    bool forceAir;
};

struct AfterImage{
    AfterImage():
        currentTime(0),
        timegap(0),
        framegap(0),
        lifetime(0),
        length(0),
        translucent(Default){
        }

    struct Image{
        Image():
            sprite(false),
        life(0){
        }

        Image(Frame sprite, Effects effects, int life, int x, int y, bool show):
            sprite(sprite),
            extra(-1),
            effects(effects),
            life(life),
            x(x),
            y(y),
            show(show){
            }

        Frame sprite;
        // Bitmap cache;
        unsigned int extra;
        Effects effects;
        int life;
        int x;
        int y;
        bool show;
    };

    /* RGB is a macro on windows */
    struct RGBx{
        RGBx():
            red(0),
            green(0),
            blue(0){
            }

        double red, green, blue;
    };

    /* count ticks */
    int currentTime;
    int timegap;
    int framegap;

    /* time left for the afterimage effects will be added */
    int lifetime;

    /* maximum number of afterimage frames */
    unsigned int length;
    TransType translucent;

    int paletteColor;
    bool invertColor;

    RGBx bright;
    RGBx contrast;
    RGBx postBright;
    RGBx add;
    RGBx multiply;

    std::deque<Image> frames;
};

struct PaletteEffects{
    PaletteEffects():
        time(0),
        addRed(0),
        addGreen(0),
        addBlue(0),
        multiplyRed(0),
        multiplyGreen(0),
        multiplyBlue(0),
        sinRed(0),
        sinGreen(0),
        sinBlue(0),
        period(0),
        invert(0),
        color(0),
        counter(0){
        }

    PaletteEffects(const PaletteEffects & copy):
        time(copy.time),
        addRed(copy.addRed),
        addGreen(copy.addGreen),
        addBlue(copy.addBlue),
        multiplyRed(copy.multiplyRed),
        multiplyGreen(copy.multiplyGreen),
        multiplyBlue(copy.multiplyBlue),
        sinRed(copy.sinRed),
        sinGreen(copy.sinGreen),
        sinBlue(copy.sinBlue),
        period(copy.period),
        invert(copy.invert),
        color(copy.color),
        counter(copy.counter){
        }

    int time;
    int addRed;
    int addGreen;
    int addBlue;
    int multiplyRed;
    int multiplyGreen;
    int multiplyBlue;
    int sinRed;
    int sinGreen;
    int sinBlue;
    int period;
    int invert;
    int color;
    unsigned int counter;
};


class StateController;

/* comes from a StateDef */
class State{
public:
    State(int id);

    enum Type{
        Standing,
        Crouching,
        Air,
        LyingDown,
        Unchanged,
    };

    virtual inline void setType(Type t){
        type = t;
    }

    virtual inline void setAnimation(Compiler::Value * animation){
        this->animation = animation;
    }

    virtual inline int getState() const {
        return id;
    }

    virtual State * deepCopy() const;

    virtual inline void setControl(Compiler::Value * control){
        changeControl = true;
        this->control = control;
    }

    virtual void setJuggle(Compiler::Value * juggle);

    virtual void setVelocity(Compiler::Value * x, Compiler::Value * y);
    virtual void setPhysics(Physics::Type p);
    virtual void setPower(Compiler::Value * power);

    virtual inline Compiler::Value * getPower() const {
        return powerAdd;
    }

    virtual inline bool powerChanged() const {
        return changePower;
    }

    virtual void setMoveType(const std::string & type);
    virtual void setLayer(int layer);
    virtual void setSpritePriority(Compiler::Value * priority);
                    
    virtual inline void setHitDefPersist(bool what){
        hitDefPersist = what;
    }
    
    virtual inline bool doesHitDefPersist() const {
        return hitDefPersist;
    }

    virtual inline const std::vector<StateController*> & getControllers() const {
        return controllers;
    }

    virtual void addController(StateController * controller);
    virtual void addControllerFront(StateController * controller);

    virtual void transitionTo(const Mugen::Stage & stage, Character & who);

    virtual ~State();

protected:
    int id;
    Type type;
    Compiler::Value * animation;
    bool changeControl;
    Compiler::Value * control;
    std::vector<StateController*> controllers;
    bool changeVelocity;
    Compiler::Value * velocity_x, * velocity_y;
    bool changePhysics;
    Physics::Type physics;
    bool changePower;
    Compiler::Value * powerAdd;
    std::string moveType;
    Compiler::Value * juggle;
    bool hitDefPersist;
    int layer;
    bool changeLayer;

    Compiler::Value * spritePriority;
    bool changeSpritePriority;
};

class Command;

class Character: public Object {
public:
	// Location at dataPath() + "mugen/chars/"
	Character(const Filesystem::AbsolutePath & s, int alliance);
	// Character(const char * location );
	Character(const Filesystem::AbsolutePath & s, const int x, const int y, int alliance);
	Character(const Character &copy);

	virtual ~Character();

        enum Specials{
            Intro,
            Invisible,
            RoundNotOver,
            NoBarDisplay,
            NoBG,
            NoFG,
            NoStandGuard,
            NoCrouchGuard,
            NoAirGuard,
            NoAutoTurn,
            NoJuggleCheck,
            NoKOSnd,
            NoKOSlow,
            NoShadow,
            GlobalNoShadow,
            NoMusic,
            NoWalk,
            TimerFreeze,
            UnGuardable
        };
	
	// Convert to paintown character or whatever
	// Do code
	
	virtual void load(int useAct = 1);
	
	virtual void renderSprite(const int x, const int y, const unsigned int group, const unsigned int image, Graphics::Bitmap *bmp, const int flip=1, const double scalex = 1, const double scaley = 1);
			   
	// Change palettes
	virtual void nextPalette();
	virtual void priorPalette();
	
	virtual inline const std::string getName() const {
            return getStateData().name;
        }

        virtual const Filesystem::AbsolutePath & getLocation() const {
            return getStateData().location;
        }

        virtual inline const std::string & getAuthor() const {
            return getStateData().author;
        }
	
	virtual inline const std::string getDisplayName() const {
            return getStateData().displayName;
        }
	
	virtual inline unsigned int getCurrentPalette() const {
            return getStateData().currentPalette;
        }

        virtual inline const std::map<int, PaintownUtil::ReferenceCount<Animation> > & getAnimations() const {
            return getStateData().animations;
        }

        virtual PaintownUtil::ReferenceCount<Animation> getAnimation(int id) const;

        /*
        virtual inline void setInput(const InputMap<Mugen::Keys> & inputLeft, const InputMap<Mugen::Keys> & inputRight){
            this->inputLeft = inputLeft;
            this->inputRight = inputRight;
        }
        */
	
	virtual inline const Mugen::SoundMap & getSounds() const {
            return getStateData().sounds;
        }

	virtual inline const Mugen::SoundMap * getCommonSounds() const {
            return getStateData().commonSounds;
        }

        virtual inline PaintownUtil::ReferenceCount<Mugen::Sprite> getSprite(int group, int image){
            return getStateData().sprites[group][image];
        }

        virtual const PaintownUtil::ReferenceCount<Mugen::Sprite> getCurrentFrame() const;
        PaintownUtil::ReferenceCount<Animation> getCurrentAnimation() const;

        virtual void drawReflection(Graphics::Bitmap * work, int rel_x, int rel_y, int intensity);
            
        virtual void startInput(const Stage & stage);

    /*! This all the inherited members */
    virtual void act(std::vector<Mugen::Character*>*, Stage*, std::vector<Mugen::Character*>*);                       
    virtual void draw(Graphics::Bitmap*, int cameraX, int cameraY);
    virtual const std::string getAttackName();
    virtual int getDamage() const;
    virtual double getForceX() const;
    virtual double getForceY() const;
    virtual int getWidth() const;
    virtual int getBackWidth() const;

    virtual bool isOwnPalette() const;
    virtual void setOwnPalette(bool what);

    virtual void drawMugenShade(Graphics::Bitmap * work, int rel_x, int intensity, Graphics::Color color, double scale, int fademid, int fadehigh);

    virtual double getMaxHealth() const {
        return getStateData().max_health;
    }

    virtual void setMaxHealth(double health);
    virtual void setHealth(double health);
    virtual double getHealth() const;

    virtual void reverseFacing();

    virtual void doMovement(const std::vector<Character*> & objects, Stage & stage);

    /* absolute X coordinate of the back of the character */
    virtual int getBackX() const;
    /* same thing for the front */
    virtual int getFrontX() const;
    
    virtual bool isAttacking() const;

    /* record moves */
    virtual void startRecording(int filename);
    virtual void stopRecording();

    virtual void changeState(Mugen::Stage & stage, int state);

    virtual void delayChangeState(Mugen::Stage & stage, int stateNumber);

    /* change back to states in the players own cns file */
    virtual void changeOwnState(Mugen::Stage & stage, int state, const std::vector<std::string> & inputs);
    
    virtual void setAnimation(int animation, int element = 0);
    
    /* true if enemy can hit this */
    virtual bool canBeHit(Character * enemy);
    
    virtual int getAnimation() const;
    
    virtual inline int getCurrentState() const {
        return getStateData().currentState;
    }

    virtual PaintownUtil::ReferenceCount<State> getState(int id) const;
    virtual void setState(int id, PaintownUtil::ReferenceCount<State> what);

        virtual inline std::string getStateType() const {
            return getStateData().stateType;
        }

        virtual inline void setStateType(const std::string & str){
            getStateData().stateType = str;
        }

        virtual inline void setXVelocity(double x){
            getStateData().velocity_x = x;
        }

        virtual inline double getXVelocity() const {
            return getStateData().velocity_x;
        }
        
        virtual inline void setYVelocity(double y){
            getStateData().velocity_y = y;
        }
        
        virtual inline double getYVelocity() const {
            return getStateData().velocity_y;
        }

        virtual inline double getWalkBackX() const {
            return getStateData().walkback;
        }
        
        virtual inline double getWalkForwardX() const {
            return getStateData().walkfwd;
        }

        virtual inline void setWalkBackX(double x){
            getStateData().walkback = x;
        }
        
        virtual inline void setWalkForwardX(double x){
            getStateData().walkfwd = x;
        }
        
        virtual inline double getRunBackX() const {
            return getStateData().runbackx;
        }

        virtual inline void setRunBackX(double x){
            getStateData().runbackx = x;
        }

        virtual inline void setRunBackY(double x){
            getStateData().runbacky = x;
        }

        virtual inline double getRunBackY() const {
            return getStateData().runbacky;
        }
        
        virtual inline double getRunForwardX() const {
            return getStateData().runforwardx;
        }

        virtual inline void setRunForwardX(double x){
            getStateData().runforwardx = x;
        }
        
        virtual inline double getRunForwardY() const {
            return getStateData().runforwardy;
        }
        
        virtual inline void setRunForwardY(double x){
            getStateData().runforwardy = x;
        }

        virtual inline double getNeutralJumpingX() const {
            return getStateData().jumpneux;
        }

        virtual inline void setNeutralJumpingX(double x){
            getStateData().jumpneux = x;
        }
        
        virtual inline double getNeutralJumpingY() const {
            return getStateData().jumpneuy;
        }
        
        virtual inline void setNeutralJumpingY(double x){
            getStateData().jumpneuy = x;
        }

        virtual inline double getYPosition() const {
            return getY();
        }
        
        virtual inline double getPower() const {
            return getStateData().power;
        }

        virtual inline void setPower(double p){
            getStateData().power = p;
        }

        virtual void addPower(double d);

        virtual inline bool hasControl() const {
            return getStateData().has_control;
        }

        virtual inline void setControl(bool b){
            getStateData().has_control = b;
        }

        virtual inline void setJumpBack(double x){
            getStateData().jumpback = x;
        }

        virtual inline double getJumpBack() const {
            return getStateData().jumpback;
        }

        virtual inline void setJumpForward(double x){
            getStateData().jumpfwd = x;
        }

        virtual inline double getJumpForward() const {
            return getStateData().jumpfwd;
        }
        
        virtual inline void setRunJumpBack(double x){
            getStateData().runjumpback = x;
        }

        virtual inline int getRunJumpBack() const {
            return getStateData().runjumpback;
        }

        virtual inline void setRunJumpForward(double x){
            getStateData().runjumpfwd = x;
        }

        virtual inline double getRunJumpForward() const {
            return getStateData().runjumpfwd;
        }

        virtual inline void setAirJumpNeutralX(double x){
            getStateData().airjumpneux = x;
        }

        virtual inline double getAirJumpNeutralX() const {
            return getStateData().airjumpneux;
        }
        
        virtual inline void setAirJumpNeutralY(double y){
            getStateData().airjumpneuy = y;
        }

        virtual inline double getAirJumpNeutralY() const {
            return getStateData().airjumpneuy;
        }

        virtual inline void setAirJumpBack(double x){
            getStateData().airjumpback = x;
        }

        virtual inline double getAirJumpBack() const {
            return getStateData().airjumpback;
        }

        virtual inline void setAirJumpForward(double x){
            getStateData().airjumpfwd = x;
        }

        virtual inline double getAirJumpForward() const {
            return getStateData().airjumpfwd;
        }

        virtual int getStateTime() const;

        virtual inline int getPreviousState() const {
            return getStateData().previousState;
        }

        virtual inline const std::string & getMoveType() const {
            return getStateData().moveType;
        }

        virtual double getXScale() const;
        virtual double getYScale() const;

        virtual inline void setMoveType(const std::string & str){
            getStateData().moveType = str;
        }

        virtual void destroyed(Stage & stage);

        /* True if the character is asserting the intro flag */
        virtual bool isAssertIntro();

        virtual void resetStateTime();

        virtual void setVariable(int index, const RuntimeValue & value);
        virtual void setFloatVariable(int index, const RuntimeValue & value);
        virtual void setSystemVariable(int index, const RuntimeValue & value);
        virtual RuntimeValue getVariable(int index) const;
        virtual RuntimeValue getFloatVariable(int index) const;
        virtual RuntimeValue getSystemVariable(int index) const;

        virtual inline Physics::Type getCurrentPhysics() const {
            return getStateData().currentPhysics;
        }

        virtual void setCurrentPhysics(Physics::Type p){
            getStateData().currentPhysics = p;
        }

        virtual void setGravity(double n){
            getStateData().gravity = n;
        }

        virtual double getGravity() const {
            return getStateData().gravity;
        }

        virtual int getDefense() const {
            return getStateData().defense;
        }

        virtual void setDefense(int x){
            getStateData().defense = x;
        }
	
        virtual int getFallDefenseUp() const {
            return getStateData().fallDefenseUp;
        }

        virtual void setFallDefenseUp(int x){
            getStateData().fallDefenseUp = x;
        }

        virtual int getAttack() const {
            return getStateData().attack;
        }

        virtual void setLieDownTime(int x){
            getStateData().lieDownTime = x;
        }

        virtual int getLieDownTime() const {
            return getStateData().lieDownTime;
        }

        virtual double getCrouchingFriction() const {
            return getStateData().crouchFriction;
        }

        virtual void setCrouchingFriction(double n){
            getStateData().crouchFriction = n;
        }

        virtual double getCrouchingFrictionThreshold() const {
            return getStateData().crouchFrictionThreshold;
        }

        virtual void setCrouchingFrictionThreshold(double n){
            getStateData().crouchFrictionThreshold = n;
        }
                            
        virtual void setJumpChangeAnimationThreshold(double n){
            getStateData().jumpChangeAnimationThreshold = n;
        }

        virtual double getJumpChangeAnimationThreshold() const {
            return getStateData().jumpChangeAnimationThreshold;
        }

        virtual void setAirGetHitGroundLevel(double n){
            getStateData().airGetHitGroundLevel = n;
        }

        virtual double getAirGetHitGroundLevel() const {
            return getStateData().airGetHitGroundLevel;
        }

        virtual void setStandingFriction(double n){
            getStateData().standFriction = n;
        }

        virtual double getStandingFriction() const {
            return getStateData().standFriction;
        }

        virtual void setStandingFrictionThreshold(double n){
            getStateData().standFrictionThreshold = n;
        }

        virtual double getStandingFrictionThreshold() const {
            return getStateData().standFrictionThreshold;
        }

        virtual double getGroundFriction() const;

        /* returns the point plus the characters current X position but taking into
         * account the direction the character is facing. that is if the character
         * is facing right then its x + getX(), if left then getX() - x
         */
        virtual double forwardPoint(double x) const;
            
        virtual bool hasAnimation(int index) const;

        virtual void enableDebug(){
            getStateData().debug = true;
        }

        virtual void disableDebug(){
            getStateData().debug = false;
        }

        virtual void toggleDebug(){
            getStateData().debug = ! getStateData().debug;
        }

        virtual HitDefinition & getHit(){
            return getStateData().hit;
        }
        
        virtual const HitDefinition & getHit() const {
            return getStateData().hit;
        }
        
        void enableHit();
        void disableHit();
    
        virtual const Character * getRoot() const;

        /* binds this character to 'bound' meaning this character's position
         * will be the same as 'bound' and adjusted by the facing and offsets
         */
        virtual void bindTo(const Character * bound, int time, int facing, double offsetX, double offsetY);
        
        std::map<int, std::vector<Character *> > & getTargets();
        const std::map<int, std::vector<Character*> > & getTargets() const;

        virtual inline int getHeight() const {
            return getStateData().height;
        }

        virtual inline void setHeight(int h){
            getStateData().height = h;
        }

        /* `this' hit `enemy' */
        void didHit(Character * enemy, Mugen::Stage & stage);
        
        /* `this' object hit `enemy' but the enemy guarded it (so not really a hit) */
        void didHitGuarded(Character * enemy, Mugen::Stage & stage);

        /* `enemy' hit `this' with hitdef `hit' */
        void wasHit(Mugen::Stage & stage, Character * enemy, const HitDefinition & hit);

        /* `this' character guarded `enemy' */
        void guarded(Mugen::Stage & stage, Object * enemy, const HitDefinition & hit);

        /* `enemy' reversed us */
        void wasReversed(Mugen::Stage & stage, Character * enemy, const ReversalData & data);

        /* we reversed the enemy */
        void didReverse(Mugen::Stage & stage, Character * enemy, const ReversalData & data);

        /* true if the player is holding back */
        bool isBlocking(const HitDefinition & hit);
        /* true if the player was attacked and blocked it */
        bool isGuarding() const;

        /* Resets the movehit flag to 0. That is, after executing MoveHitReset,
         * the triggers MoveContact, MoveGuarded, and MoveHit will all return
         * 0.
         */
        void resetHitFlag();

        virtual const HitState & getHitState() const {
            return getStateData().hitState;
        }

        virtual HitState & getHitState(){
            return getStateData().hitState;
        }

        const std::vector<Area> getAttackBoxes() const;
        const std::vector<Area> getDefenseBoxes() const;

        /* paused from an attack */
        virtual bool isPaused() const;
        /* time left in the hitpause */
        virtual int pauseTime() const;

        /* prevent character from being hit, like after a KO */
        virtual void setUnhurtable();
	
        /* if kill is true then the damage can reduce health below 1, otherwise
         * health cannot go below 1
         * if defense is true then the defenseMultiplier is taken into account
         */
        virtual void takeDamage(Stage & world, Mugen::Object * obj, double amount, bool kill, bool defense);
        /* passes true for kill and defense */
        virtual void takeDamage(Stage & world, Mugen::Object * obj, int amount);
        virtual void takeDamage(Stage & world, Object * obj, double amount, double forceX, double forceY);
        /* lose some life */
        virtual void hurt(double amount);

        /* character can be hit */
        virtual void setHurtable();

        bool canTurn() const;
        void doTurn(Mugen::Stage & stage);

        /* recover after falling */
        virtual bool canRecover() const;

        virtual PaintownUtil::ReferenceCount<Mugen::Sound> getSound(int group, int item) const;
        virtual PaintownUtil::ReferenceCount<Mugen::Sound> getCommonSound(int group, int item) const;

        virtual inline void setJugglePoints(int x){
            getStateData().airjuggle = x;
        }

        virtual inline int getJugglePoints() const {
            return getStateData().airjuggle;
        }

        virtual void resetJugglePoints();

        virtual inline void setCurrentJuggle(int j){
            getStateData().currentJuggle = j;
        }

        virtual inline int getCurrentJuggle() const {
            return getStateData().currentJuggle;
        }

        virtual inline void setCommonSounds(const Mugen::SoundMap * sounds){
            getStateData().commonSounds = sounds;
        }

        virtual inline void setExtraJumps(int a){
            getStateData().airjumpnum = a;
        }

        virtual inline int getExtraJumps() const {
            return getStateData().airjumpnum;
        }

        virtual inline double getAirJumpHeight() const {
            return getStateData().airjumpheight;
        }

        virtual inline void setAirJumpHeight(double f){
            getStateData().airjumpheight = f;
        }

        /* number of times the character has been hit */
        virtual unsigned int getWasHitCount() const;

        virtual int getCurrentCombo() const;

        virtual inline int getHitCount() const {
            return getStateData().hitCount;
        }

        virtual inline const std::vector<WinGame> & getWins() const {
            return getStateData().wins;
        }

        virtual inline void clearWins(){
            getStateData().wins.clear();
        }

        virtual void addWin(WinGame win);

        virtual inline int getMatchWins() const {
            return getStateData().matchWins;
        }

        virtual void addMatchWin();

        virtual void resetPlayer();

        virtual inline void setBehavior(Behavior * b){
            getStateData().behavior = b;
        }

        virtual inline Behavior * getBehavior(){
            return this->getStateData().behavior;
        }
	
        virtual int getIntPersistIndex() const {
            return getStateData().intpersistindex;
        }

        virtual void setIntPersistIndex(int index){
            getStateData().intpersistindex = index;
        }

	virtual int getFloatPersistIndex() const {
            return getStateData().floatpersistindex;
        }

        virtual void setFloatPersistIndex(int index){
            getStateData().floatpersistindex = index;
        }

        /* Called when the current round ended */
        virtual void roundEnd(Mugen::Stage & stage);

        virtual inline void setDefaultSpark(const ResourceEffect & effect){
            getStateData().spark = effect;
        }

        virtual inline void setDefaultGuardSpark(const ResourceEffect & effect){
            getStateData().guardSpark = effect;
        }

        virtual inline ResourceEffect getDefaultSpark() const {
            return getStateData().spark;
        }

        virtual inline ResourceEffect getDefaultGuardSpark() const {
            return getStateData().guardSpark;
        }
	
        virtual bool getKoEcho() const {
            return getStateData().koecho;
        }

        virtual void setKoEcho(bool x){
            getStateData().koecho = x;
        }

        virtual inline void setRegeneration(bool r){
            this->getStateData().regenerateHealth = r;
        }
        
        virtual inline int getAttackDistance() const {
	    return this->getStateData().attackdist;
	}
    
        /* let go of a bound character (BindToRoot / BindToTarget */
        virtual void unbind(Character * who);

        virtual void setDrawOffset(double x, double y);

        virtual void setAfterImageTime(int time);

        virtual void updateAngleEffect(double angle);
        virtual double getAngleEffect() const;
        virtual void drawAngleEffect(double angle, bool setAngle, double scaleX, double scaleY);

        virtual void assertSpecial(Specials special);

        virtual void setWidthOverride(int edgeFront, int edgeBack, int playerFront, int playerBack);
        virtual void setHitByOverride(int slot, int time, bool standing, bool crouching, bool aerial, const std::vector<AttackType::Attribute> & attributes);
        virtual void setHitOverride(int slot, const HitAttributes & attribute, int state, int time, bool air);

        virtual void setDefenseMultiplier(double defense);
        virtual void setAttackMultiplier(double attack);

        virtual void doFreeze();
	
        virtual void moveX(double x, bool force = false);
        virtual void moveY(double y, bool force = false);

        virtual void setSpritePriority(int priority);
        virtual int getSpritePriority() const;
        
        virtual void setForeignAnimation(PaintownUtil::ReferenceCount<Animation> animation, int number);

        virtual bool isHelper() const;

        /* increase combo count */
        virtual void addCombo(int combo);

        virtual void disablePushCheck();
        virtual bool isPushable();

        virtual void setReversalActive();
        virtual void setReversalInactive();
        virtual bool isReversalActive();

        /* Takes into account isReversalActive and the attack attributes of who
         * versus the reversal.attrs
         */
        virtual bool canReverse(Character * who);
        ReversalData & getReversal();

        virtual void setTransOverride(TransType type, int alphaFrom, int alphaTo);

        virtual Point getMidPosition() const;
        virtual Point getHeadPosition() const;

        /* bind the enemy to the target id. used for target redirection
         * and BindToTarget
         */
        virtual void setTargetId(int id, Character * enemy);

        /* get a target for a given id */
        virtual Character * getTargetId(int id) const;
    
        virtual bool withinGuardDistance(const Mugen::Character * enemy) const;

        /* For testing only. Will run through all the state controllers and call the triggers
         * on them, but won't change states or anything.
         */
        virtual void testStates(Mugen::Stage & stage, const std::vector<std::string> & active, int state);
    
        /* Is bound by a TargetBind or whatever */
        virtual bool isBound() const;
    
        /* True if the hitflags match our current state */
        virtual bool compatibleHitFlag(const HitDefinition::HitFlags & flags);
                                
        virtual double getAirHitRecoverMultiplierX() const;
        virtual double getAirHitRecoverMultiplierY() const;

        virtual void setAirHitRecoverMultiplierX(double x);
        virtual void setAirHitRecoverMultiplierY(double y);

        virtual double getAirHitGroundRecoverX() const;
        virtual double getAirHitGroundRecoverY() const;

        virtual void setAirHitGroundRecoverX(double x);
        virtual void setAirHitGroundRecoverY(double y);

        virtual double getAirHitRecoverAddX() const;
        virtual double getAirHitRecoverAddY() const;

        virtual void setAirHitRecoverAddX(double x);
        virtual void setAirHitRecoverAddY(double y);


protected:
    void initialize();

    virtual inline void setCurrentState(int state){
        this->getStateData().currentState = state;
    }

    void checkStateControllers();
    
    virtual void loadSelectData();

    virtual void loadCmdFile(const Filesystem::RelativePath & path);
    virtual void loadCnsFile(const Filesystem::RelativePath & path);
    virtual void loadStateFile(const Filesystem::AbsolutePath & base, const std::string & path);

    virtual void addCommand(Command * command);

    virtual void setConstant(std::string name, const std::vector<double> & values);
    virtual void setConstant(std::string name, double value);

    virtual std::vector<std::string> doInput(const Mugen::Stage & stage);
    virtual bool doStates(Mugen::Stage & stage, const std::vector<std::string> & active, int state);

    void resetJump(Mugen::Stage & stage, const std::vector<std::string> & inputs);
    void doubleJump(Mugen::Stage & stage, const std::vector<std::string> & inputs);
    void stopGuarding(Mugen::Stage & stage, const std::vector<std::string> & inputs);

    void maybeTurn(const std::vector<Character*> & objects, Stage & stage);

    /*
    internalCommand_t resetJump;
    internalCommand_t doubleJump;
    internalCommand_t stopGuarding;
    */

    virtual void fixAssumptions();
    virtual StateController * parseState(Ast::Section * section);
    virtual PaintownUtil::ReferenceCount<State> parseStateDefinition(Ast::Section * section, const Filesystem::AbsolutePath & path, std::map<int, PaintownUtil::ReferenceCount<State> > & states);

    virtual void useCharacterData(const Character * who);

    // InputMap<Mugen::Keys> & getInput();

public:
    virtual void setAfterImage(int time, int length, int timegap, int framegap, TransType effects, int paletteColor, bool invertColor, const AfterImage::RGBx & bright, const AfterImage::RGBx & contrast, const AfterImage::RGBx & postBright, const AfterImage::RGBx & add, const AfterImage::RGBx & multiply);

    void drawAfterImage(const AfterImage & afterImage, const AfterImage::Image & frame, int index, int x, int y, const Graphics::Bitmap & work);
    void processAfterImages();

    virtual void setPaletteEffects(int time, int addRed, int addGreen, int addBlue, int multiplyRed, int multiplyGreen, int multiplyBlue, int sinRed, int sinGreen, int sinBlue, int period, int invert, int color);
    Graphics::Bitmap::Filter * getPaletteEffects(unsigned int time);

protected:

    PaintownUtil::ReferenceCount<Animation> replaceSprites(const PaintownUtil::ReferenceCount<Animation> & animation);

    virtual void recordCommands(const std::vector<std::string> & commands);

protected:

    struct StateData{
        StateData();
        StateData(const StateData & copy);

        /* Location is the directory passed in ctor
           This is where the def is loaded and all the relevant files
           are loaded from
           */
        Filesystem::AbsolutePath location;

        Filesystem::AbsolutePath baseDir;

        /* These items are taken from character.def file */

        /* Base definitions */

        // Name of Character
        std::string name;
        // Name of Character to Display why there is two is beyond me, retards
        std::string displayName;
        // Version date (unused)
        std::string versionDate;
        // Version that works with mugen (this isn't mugen)
        std::string mugenVersion;
        // Author 
        std::string author;
        // Palette defaults
        std::vector< unsigned int> palDefaults;
        unsigned int currentPalette;

        /* Relevant files */

        // Command set file
        Filesystem::RelativePath cmdFile;
        // Constants
        std::string constantsFile;
        /*
        // States
        std::string stateFile;
        // Common States
        std::string commonStateFile;
        // Other state files? I can't find documentation on this, in the meantime we'll wing it
        std::string stFile[12];
        */
        // Sprites
        std::string sffFile;
        // animations
        std::string airFile;
        // Sounds
        std::string sndFile;
        // Palettes max 12
        std::map<int, std::string> palFile;

        // Arcade mode ( I don't think we will be using this anytime soon )
        std::string introFile;
        std::string endingFile;

        /* Now on to the nitty gritty */

        /* Player Data and constants comes from cns file */

        /* Section [Data] */

        // Life
        int life;
        // Attack
        int attack;
        // Defence
        int defense;
        // How much to bring up defese on fall
        int fallDefenseUp;
        // How long to lie down when fall
        int lieDownTime;
        /* starting air juggle points */
        int airjuggle;

        /* number of juggle points left */
        int juggleRemaining;

        /* number of juggle points the current move will take */
        int currentJuggle;

        // Default Hit Spark Number for hitdefs ???
        ResourceEffect spark;
        // Default guard spark number
        ResourceEffect guardSpark;
        // Echo on KO (I guess is for the death sound)
        bool koecho;
        // Volume offset on characters sounds
        int volumeoffset;
        // Maybe used in VS mode later
        /* According to the definition: 
           Variables with this index and above will not have their values
           reset to 0 between rounds or matches. There are 60 int variables,
           indexed from 0 to 59, and 40 float variables, indexed from 0 to 39.
           If omitted, then it defaults to 60 and 40 for integer and float
           variables repectively, meaning that none are persistent, i.e. all
           are reset. If you want your variables to persist between matches,
           you need to override state 5900 from common1.cns.
           */
        int intpersistindex;
        int floatpersistindex;

        /* Section [Size] */

        // Horizontal Scaling Factor
        double xscale;
        //Vertical scaling factor.
        double yscale;
        //      ;Player width (back, ground)
        int groundback;
        //   ;Player width (front, ground)
        int groundfront;
        //      ;Player width (back, air)
        int airback;
        //     ;Player width (front, air)
        int airfront;
        //  = 60          ;Height of player (for opponent to jump over)
        int height;
        // = 160    ;Default attack distance
        int attackdist;
        //  = 90 ;Default attack distance for projectiles
        int projattackdist;
        //  = 0     ;Set to 1 to scale projectiles too
        bool projdoscale;
        // = -5, -90   ;Approximate position of head
        Mugen::Point headPosition;
        //  = -5, -60    ;Approximate position of midsection
        Mugen::Point midPosition;
        //  = 0     ;Number of pixels to vertically offset the shadow
        int shadowoffset;
        // = 0,0    ;Player drawing offset in pixels (x, y)
        Mugen::Point drawOffset;

        /* Section [Velocity] */

        //   = 2.4      ;Walk forward
        double walkfwd;
        // = -2.2     ;Walk backward
        double walkback;
        //  = 4.6, 0    ;Run forward (x, y)
        double runforwardx;
        double runforwardy;
        // = -4.5,-3.8 ;Hop backward (x, y)
        double runbackx;
        double runbacky;
        // = 0,-8.4    ;Neutral jumping velocity (x, y)
        double jumpneux;
        double jumpneuy;
        // = -2.55    ;Jump back Speed (x, y)
        double jumpback;
        // = 2.5       ;Jump forward Speed (x, y)
        double jumpfwd;
        // = -2.55,-8.1 ;Running jump speeds (opt)
        double runjumpback;
        // = 4,-8.1      ;.
        double runjumpfwd;
        // = 0,-8.1      ;.
        double airjumpneux;
        double airjumpneuy;
        // Air jump speeds (opt)
        double airjumpback;
        double airjumpfwd;

        double power;

        /* Movement */

        //  = 1      ;Number of air jumps allowed (opt)
        int airjumpnum;
        //  = 35  ;Minimum distance from ground before you can air jump (opt)
        double airjumpheight;
        // = .44         ;Vertical acceleration
        double yaccel;
        //  = .85  ;Friction coefficient when standing
        // double standfriction;
        //  = .82 ;Friction coefficient when crouching
        double crouchFriction;

        /* TODO: use this variable for something
         * Mugen 1.0
         */
        double crouchFrictionThreshold;

        double velocity_air_gethit_airrecover_mul_x;
        double velocity_air_gethit_airrecover_mul_y;

        /* Sprites */
        Mugen::SpriteMap sprites;
        // Bitmaps of those sprites
        // std::map< unsigned int, std::map< unsigned int, Graphics::Bitmap * > > bitmaps;

        /* Animation Lists stored by action number, ie [Begin Action 500] */
        std::map< int, PaintownUtil::ReferenceCount<Animation> > animations;

        /* Sounds */
        Mugen::SoundMap sounds;
        /* sounds from the stage */
        const Mugen::SoundMap * commonSounds;

        /* Commands, Triggers or whatever else we come up with */
        std::map<std::string, Constant> constants;

        std::vector<Command *> commands;

        std::map<int, PaintownUtil::ReferenceCount<State> > states;

        int currentState;
        int previousState;
        int currentAnimation;

        // Debug state
        bool debug;

        /*
           InputMap<Mugen::Keys> inputLeft;
           InputMap<Mugen::Keys> inputRight;
           */

        double velocity_x, velocity_y;

        bool has_control;

        /* how much time the player has been in the current state */
        int stateTime;

        /* dont delete these in the destructor, the state controller will do that */
        std::map<int, RuntimeValue> variables;
        std::map<int, RuntimeValue> floatVariables;
        std::map<int, RuntimeValue> systemVariables;
        Physics::Type currentPhysics;

        /* yaccel */
        double gravity;

        /* stand.friction */
        double standFriction;

        /* TODO: use this variable for something.
         * Mugen 1.0 variable
         */ 
        double standFrictionThreshold;

        /* S (stand), C (crouch), A (air), L (lying down) */
        std::string stateType;
        std::string moveType;

        HitDefinition hit;

        HitState hitState;
        // unsigned int lastTicket;

        int combo;
        // int nextCombo;

        int hitCount;

        std::vector<WinGame> wins;

        int matchWins;

        Behavior * behavior;

        PaintownUtil::ReferenceCount<Animation> foreignAnimation;
        int foreignAnimationNumber;

        /* true if the player is holding the back button */
        bool blocking;

        //! regenerate health?
        bool regenerateHealth;
        bool regenerating;
        int regenerateTime;
        int regenerateHealthDifference;

        /* used to communicate the need to guard in the engine */
        // bool needToGuard;

        /* true if the player is currently guarding an attack */
        bool guarding;

        AfterImage afterImage;

        struct WidthOverride{
            WidthOverride():
                enabled(false),
                edgeFront(0),
                edgeBack(0),
                playerFront(0),
                playerBack(0){
                }

            bool enabled;
            int edgeFront, edgeBack;
            int playerFront, playerBack;
        } widthOverride;

        struct HitByOverride{
            HitByOverride():
                time(0){
                }

            bool standing;
            bool crouching;
            bool aerial;
            int time;
            std::vector<AttackType::Attribute> attributes;
        } hitByOverride[2];

        /* reduces damage taken */
        double defenseMultiplier;
        /* increase attack damage */
        double attackMultiplier;

        /* if frozen then the player won't move */
        bool frozen;

        ReversalData reversal;
        bool reversalActive;

        struct TransOverride{
            TransOverride():
                enabled(false){
                }

            bool enabled;
            TransType type;
            int alphaSource;
            int alphaDestination;
        } transOverride;

        PaintownUtil::ReferenceCount<RecordingInformation> record;

        // PushPlayer only one tick
        int pushPlayer;
        
        struct SpecialStuff{
            SpecialStuff():
                invisible(false),
                intro(false){
                }

            void reset(){
                invisible = false;
                intro = false;
            }

            bool invisible;
            bool intro;
        } special;

        PaletteEffects paletteEffects;

        double max_health;
        double health;

        /* keeps track of binds to other characters. Used for BindToRoot and
         * BindToTarget
         */
        struct Bind{
            Bind():
                bound(NULL),
                time(0),
                facing(0),
                offsetX(0),
                offsetY(0){
                }

            Bind(const Bind & you):
                bound(you.bound),
                time(you.time),
                facing(you.facing),
                offsetX(you.offsetX),
                offsetY(you.offsetY){
                }

            const Character * bound;
            int time;
            int facing;
            double offsetX;
            double offsetY;
        };

        Bind bind;

        std::map<int, std::vector<Character *> > targets;

        int spritePriority;

        /* number of times this character has been hit in total */
        unsigned int wasHitCounter;

        /* TODO: what is this for?
         * Mugen 1.0
         */
        double jumpChangeAnimationThreshold;

        /* TODO: what is this for?
         * Mugen 1.0
         */
        double airGetHitGroundLevel;

        struct CharacterData {
            CharacterData():
                who(NULL),
                enabled(false){
                }

            const Character * who;
            bool enabled;
        } characterData;

        bool ownPalette;
        /* keep a count of the number of times changeState was called to prevent
         * an infinite recursion.
         */
        int maxChangeStates;

        double drawAngle;
        struct DrawAngleEffect{
            DrawAngleEffect():
                enabled(false),
                angle(0),
                scaleX(0),
                scaleY(0){
                }

            bool enabled;
            double angle;
            double scaleX;
            double scaleY;
        } drawAngleData;

        /* Current set of commands, updated in act() */
        std::vector<std::string> active;
        std::map<int, HitOverride> hitOverrides;

        double velocity_air_gethit_groundrecover_x;
        double velocity_air_gethit_groundrecover_y;
        
        double velocity_air_gethit_recover_add_x;
        double velocity_air_gethit_recover_add_y;
    };

    StateData stateData;

public:
    const StateData & getStateData() const {
        return stateData;
    }

    StateData & getStateData(){
        return stateData;
    }
};

}

#endif
