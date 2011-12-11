#include "game.h"
#include "util/bitmap.h"
#include "util/debug.h"
#include "util/input/input-map.h"
#include "util/input/input-source.h"
#include "util/input/input-manager.h"
#include "util/input/keyboard.h"
#include "util/events.h"
#include <math.h>
#include <vector>

using std::vector;

namespace Asteroids{

/* hacks */
const int GFX_X = 640;
const int GFX_Y = 480;

enum GameInput{
    Quit
};

class Star{
public:
    Star(int x, int y, double brightness):
    x(x),
    y(y),
    brightness(brightness){
    }

    void draw(const Graphics::Bitmap & work) const {
        int c = 255 * brightness;
        work.putPixel(x, y, Graphics::makeColor(c, c, c));
    }

    int x;
    int y;
    double brightness;
};

class StarField{
public:
    StarField(){
        for (int i = 0; i < 1000; i++){
            stars.push_back(makeStar());
        }
    }

    void draw(const Graphics::Bitmap & work) const {
        for (vector<Star>::const_iterator it = stars.begin(); it != stars.end(); it++){
            const Star & star = *it;
            star.draw(work);
        }
    }

    Star makeStar(){
        return Star(Util::rnd(GFX_X), Util::rnd(GFX_Y), Util::rnd(100) / 100.0);
    }

    vector<Star> stars;
};

enum AsteroidSize{
    Small,
    Medium,
    Large
};

class SpriteManager{
public:
    SpriteManager(){
        sprite = Util::ReferenceCount<Graphics::Bitmap>(new Graphics::Bitmap(30, 30));
        sprite->clearToMask();
        sprite->circleFill(sprite->getWidth() / 2, sprite->getHeight() / 2, sprite->getWidth() / 2, Graphics::makeColor(200, 0, 0));
    }
    
    Util::ReferenceCount<Graphics::Bitmap> getPlayer() const {
        return sprite;
    }

    Util::ReferenceCount<Graphics::Bitmap> getAsteroidSprite(AsteroidSize size, int tick) const {
        return sprite;
    }

protected:
    Util::ReferenceCount<Graphics::Bitmap> sprite;
};

class World;

class Asteroid{
public:
    Asteroid(int x, int y, int angle, double speed, AsteroidSize size):
    x(x), y(y),
    angle(angle),
    speed(speed),
    size(size),
    ticker(0),
    radius(15){
        switch (size){
            case Large: life = 10; break;
            case Medium: life = 5; break;
            case Small: life = 1; break;
        }
    }

    void draw(const SpriteManager & manager, const Graphics::Bitmap & work){
        manager.getAsteroidSprite(size, ticker)->drawCenter((int) x, (int) y, work);
    }

    /* took a shot, lose some life. return true if dead. */
    bool hit(){
        life -= 1;
        return life <= 0;
    }

    void createMore(World & world);
    
    bool touches(double x, double y, int radius){
        return Util::distance(this->x, this->y, x, y) < radius + this->radius;
    }

    void logic(){
        ticker += 1;
        x += cos(Util::radians(angle)) * speed;
        y -= sin(Util::radians(angle)) * speed;
        if (x < 0){
            x = GFX_X;
        }
        if (x > GFX_X){
            x = 0;
        }
        if (y < 0){
            y = GFX_Y;
        }
        if (y > GFX_Y){
            y = 0;
        }
    }

protected:
    double x, y;
    int angle;
    double speed;
    AsteroidSize size;
    int ticker;
    int radius;
    int life;
};

class Bullet{
public:
    Bullet(double x, double y, int angle, double speed):
    x(x), y(y),
    angle(angle),
    speed(speed),
    radius(3){
    }

    void logic(){
        move();
    }

    void move(){
        x += cos(Util::radians(angle)) * speed;
        y -= sin(Util::radians(angle)) * speed;
    }

    int getRadius(){
        return radius;
    }

    double getX(){
        return x;
    }

    double getY(){
        return y;
    }

    void draw(const SpriteManager & manager, const Graphics::Bitmap & work){
        work.circleFill((int) x, (int) y, radius, Graphics::makeColor(0, 255, 0));
    }

protected:
    double x, y;
    int angle;
    double speed;
    int radius;
};

static const double gravity = 0.02;
class Player{
public:
    enum Keys{
        Thrust,
        TurnLeft,
        TurnRight,
        Shoot
    };

    Player(int x, int y):
    turnSpeed(3),
    x(x), y(y),
    angle(0), speed(0),
    addShot(false),
    shotCounter(0){
        input.set(Keyboard::Key_UP, Thrust);
        input.set(Keyboard::Key_LEFT, TurnLeft);
        input.set(Keyboard::Key_RIGHT, TurnRight);
        input.set(Keyboard::Key_SPACE, Shoot);
    }

    struct Hold{
        Hold():
        thrust(false),
        left(false),
        right(false),
        shoot(false){
        }

        bool thrust;
        bool left;
        bool right;
        bool shoot;
    };

    Hold hold;
    InputMap<Keys> input;
    InputSource source;
    const int turnSpeed;

    double getX(){
        return x;
    }

    double getY(){
        return y;
    }

    void doInput(){
        class Handler: public InputHandler<Keys> {
        public:
            Handler(Hold & hold):
            hold(hold){
            }

            Hold & hold;

            void release(const Keys & input, Keyboard::unicode_t unicode){
                switch (input){
                    case Thrust: {
                        hold.thrust = false;
                        break;
                    }
                    case TurnLeft: {
                        hold.left = false;
                        break;
                    }
                    case TurnRight: {
                        hold.right = false;
                        break;
                    }
                    case Shoot: {
                        hold.shoot = false;
                        break;
                    }
                }
            }

            void press(const Keys & input, Keyboard::unicode_t unicode){
                switch (input){
                    case Thrust: {
                        hold.thrust = true;
                        break;
                    }
                    case TurnLeft: {
                        hold.left = true;
                        break;
                    }
                    case TurnRight: {
                        hold.right = true;
                        break;
                    }
                    case Shoot: {
                        hold.shoot = true;
                        break;
                    }
                }
            }
        };

        Handler handler(hold);
        InputManager::handleEvents(input, source, handler);

        if (hold.thrust){
            increaseSpeed();
        }

        if (hold.left){
            turnLeft();
        }

        if (hold.right){
            turnRight();
        }
        
        if (hold.shoot){
            shoot();
        } else {
            resetShoot();
        }
    }

    void shoot(){
        if (shotCounter == 0){
            addShot = true;
            shotCounter = 10;
        }
    }

    void resetShoot(){
        addShot = false;
        shotCounter = 0;
    }

    void increaseSpeed(){
        speed += 0.2;
        if (speed > 2){
            speed = 2;
        }
    }

    void turnLeft(){
        angle += turnSpeed;
    }

    void turnRight(){
        angle -= turnSpeed;
    }

    void logic(World & world){
        doInput();
        speed -= gravity;
        if (speed < 0){
            speed = 0;
        }
        x += cos(Util::radians(angle)) * speed;
        y -= sin(Util::radians(angle)) * speed;

        if (shotCounter > 0){
            shotCounter -= 1;
        }

        if (addShot){
            addShot = false;
            createBullet(world);
        }

        if (x < 0){
            x = GFX_X;
        }
        if (x > GFX_X){
            x = 0;
        }
        if (y < 0){
            y = GFX_Y;
        }
        if (y > GFX_Y){
            y = 0;
        }
    }

    void draw(const SpriteManager & manager, const Graphics::Bitmap & work){
        manager.getPlayer()->drawCenter((int) x, (int) y, work);
    }

    void createBullet(World & world);

protected:
    double x, y;
    int angle;
    double speed;

    bool addShot;
    int shotCounter;
};

class World{
public:
    World():
    player(GFX_X / 2, GFX_Y / 2){
        for (int i = 0; i < 10; i++){
            asteroids.push_back(makeAsteroid(Large));
        }
    }

    StarField stars;
    SpriteManager manager;
    vector<Util::ReferenceCount<Asteroid> > asteroids;
    Player player;
    vector<Util::ReferenceCount<Bullet> > bullets;

    bool nearPlayer(int x, int y){
        return Util::distance(player.getX(), player.getY(), x, y) < 100;
    }

    void addAsteroid(const Util::ReferenceCount<Asteroid> & asteroid){
        asteroids.push_back(asteroid);
    }

    Util::ReferenceCount<Asteroid> makeAsteroid(AsteroidSize size){
        int x = Util::rnd(GFX_X);
        int y = Util::rnd(GFX_Y);
        while (nearPlayer(x, y)){
            x = Util::rnd(GFX_X);
            y = Util::rnd(GFX_Y);
        }
        return makeAsteroid(x, y, size);
    }

    Util::ReferenceCount<Asteroid> makeAsteroid(double x, double y, AsteroidSize size){
        return Util::ReferenceCount<Asteroid>(new Asteroid(x, y, Util::rnd(360), Util::rnd(10) / 7.0 + 0.5, size));
    }

    void logic(){
        for (vector<Util::ReferenceCount<Asteroid> >::iterator it = asteroids.begin(); it != asteroids.end(); it++){
            Util::ReferenceCount<Asteroid> asteroid = *it;
            asteroid->logic();
        }
        for (vector<Util::ReferenceCount<Bullet> >::iterator it = bullets.begin(); it != bullets.end(); it++){
            Util::ReferenceCount<Bullet> bullet = *it;
            bullet->logic();
        }
        player.logic(*this);

        enforceConstraints();
        doCollisions();
    }

    Util::ReferenceCount<Asteroid> findAsteroid(double x, double y, int radius){
        for (vector<Util::ReferenceCount<Asteroid> >::iterator it = asteroids.begin(); it != asteroids.end(); it++){
            Util::ReferenceCount<Asteroid> asteroid = *it;
            if (asteroid->touches(x, y, radius)){
                return asteroid;
            }
        }

        return Util::ReferenceCount<Asteroid>(NULL);
    }

    void removeAsteroid(const Util::ReferenceCount<Asteroid> & asteroid){
        for (vector<Util::ReferenceCount<Asteroid> >::iterator it = asteroids.begin(); it != asteroids.end(); /**/){
            if (*it == asteroid){
                it = asteroids.erase(it);
            } else {
                it++;
            }
        }
    }

    void doCollisions(){
        for (vector<Util::ReferenceCount<Bullet> >::iterator it = bullets.begin(); it != bullets.end(); /**/ ){
            Util::ReferenceCount<Bullet> bullet = *it;
            Util::ReferenceCount<Asteroid> asteroid = findAsteroid(bullet->getX(), bullet->getY(), bullet->getRadius());
            if (asteroid != NULL){
                if (asteroid->hit()){
                    removeAsteroid(asteroid);
                    asteroid->createMore(*this);
                }

                it = bullets.erase(it);
            } else {
                it++;
            }
        }
    }

    void enforceConstraints(){
        for (vector<Util::ReferenceCount<Bullet> >::iterator it = bullets.begin(); it != bullets.end(); /**/){
            Util::ReferenceCount<Bullet> bullet = *it;
            if (outside(bullet->getX(), bullet->getY())){
                it = bullets.erase(it);
            } else {
                it++;
            }
        }
    }

    bool outside(double x, double y){
        return x < 0 || y < 0 ||
               x > GFX_X || y > GFX_Y;
    }

    void draw(const Graphics::Bitmap & work){
        stars.draw(work);
        for (vector<Util::ReferenceCount<Asteroid> >::iterator it = asteroids.begin(); it != asteroids.end(); it++){
            Util::ReferenceCount<Asteroid> asteroid = *it;
            asteroid->draw(manager, work);
        }
        for (vector<Util::ReferenceCount<Bullet> >::iterator it = bullets.begin(); it != bullets.end(); it++){
            Util::ReferenceCount<Bullet> bullet = *it;
            bullet->draw(manager, work);
        }
        player.draw(manager, work);
    }

    void addBullet(double x, double y, int angle, double speed){
        bullets.push_back(Util::ReferenceCount<Bullet>(new Bullet(x, y, angle, speed)));
    }
};

void Asteroid::createMore(World & world){
    switch (size){
        case Large: {
            int many = Util::rnd(4) + 1;
            for (int i = 0; i < many; i++){
                world.addAsteroid(world.makeAsteroid(x, y, Medium));
            }
            break;
        }
        case Medium: {
            int many = Util::rnd(4) + 1;
            for (int i = 0; i < many; i++){
                world.addAsteroid(world.makeAsteroid(x, y, Small));
            }
            break;
        }
        case Small: {
            break;
        }
    }
}

void Player::createBullet(World & world){
    world.addBullet(x, y, angle, speed + 1);
}

void run(){
    Global::debug(0) << "Asteroids!" << std::endl;

    class Game: public Util::Logic, public Util::Draw {
    public:
        Game():
        quit(false){
            input.set(Keyboard::Key_ESC, Quit);
        }

        World world;
        InputMap<GameInput> input;
        InputSource source;
        
        void run(){
            world.logic();
            handleInput();
        }

        void handleInput(){
            class Handler: public InputHandler<GameInput> {
            public:
                Handler(Game & game):
                game(game){
                }

                Game & game;

                void release(const GameInput & input, Keyboard::unicode_t unicode){
                }

                void press(const GameInput & input, Keyboard::unicode_t unicode){
                    switch (input){
                        case Quit: {
                            game.quit = true;
                            break;
                        }
                    }
                }
            };

            Handler handler(*this);
            InputManager::handleEvents(input, source, handler);
        }

        double ticks(double system){
            return system;
        }

        bool done(){
            return quit;
        }

        void draw(const Graphics::Bitmap & work){
            work.clear();
            world.draw(work);
            work.BlitToScreen();
        }

    protected:
        bool quit;
    };

    Keyboard::pushRepeatState(false);
    Game game;
    Util::standardLoop(game, game);
    Keyboard::popRepeatState();
}

}
