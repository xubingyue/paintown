#ifndef _paintown_python_h
#define _paintown_python_h

#ifdef HAVE_PYTHON

#include <Python.h>
#include "script/script.h"
#include <string>

class World;

class PythonEngine: public Script::Engine {
public:
    PythonEngine(const std::string & path);

    virtual void init();
    virtual void shutdown();
    virtual void createWorld(const World & world);
    virtual void destroyWorld(const World & world);
    virtual void * createCharacter(void * character);
    virtual void * createPlayer(void * player);
    virtual void destroyObject(void * handle);
    virtual void objectTick(void * handle);
    virtual void objectTakeDamage(void * who, void * handle, int damage);
    virtual void objectCollided(void * me, void * him);
    virtual void characterAttacked(void * me, void * him);
    virtual void tick();

    virtual ~PythonEngine();
protected:
    std::string path;
    std::string module;
    PyObject * user;
};

namespace PythonModule{
    class AutoObject{
    public:
        AutoObject(PyObject*);
        virtual ~AutoObject();

        PyObject * getObject();

    private:
        PyObject * object;
    };
}

#endif

#endif
