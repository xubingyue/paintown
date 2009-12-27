#include "util/bitmap.h"
#include "game/adventure_world.h"
#include "network_world_client.h"
#include "network.h"
#include "level/scene.h"
#include "globals.h"
#include "script/script.h"
#include "init.h"
#include "util/font.h"
#include "factory/font_render.h"
#include "level/blockobject.h"
#include "util/funcs.h"
#include "util/file-system.h"
#include "level/cacher.h"
#include "object/object.h"
#include "object/player.h"
#include "factory/object_factory.h"
#include <pthread.h>
#include <string.h>
#include "util/system.h"
#include "cacher.h"
#include "input/input-manager.h"
#include <sstream>

#include "object/character.h"
#include "object/cat.h"
#include "object/item.h"

using namespace std;

/* use java-style OOP */
typedef AdventureWorld super;

static std::ostream & debug( int level ){
    return Global::debug(level, "network-world-client");
}

static void * handleMessages( void * arg ){
    NetworkWorldClient * world = (NetworkWorldClient *) arg;
    NLsocket socket = world->getServer();
    // pthread_mutex_t * lock = world->getLock();

    /* at 100 messages per second (which is more than normal)
     * this can count 1.36 years worth of messages.
     * a reasonable rate is probably 10 messages per second which
     * gives 13.6 years worth of messages.
     */
    unsigned int received = 0;

    try{
        while ( world->isRunning() ){
            received += 1;
            {
                ostringstream context;
                context << __FILE__ << " " << (System::currentMicroseconds() / 1000);
                Global::debug(2, context.str()) << "Receiving message " << received << endl;
            }
            Network::Message m( socket );
            // pthread_mutex_lock( lock );
            world->addIncomingMessage( m );

            {
                ostringstream context;
                context << __FILE__ << " " << (System::currentMicroseconds() / 1000);
                Global::debug(2, context.str()) << "Received path '" << m.path << "'" << endl;
            }
            // pthread_mutex_unlock( lock );
        }
    } catch (const Network::MessageEnd & end){
        debug(1) << "Closed connection with socket " << socket << endl;
    } catch (const Network::NetworkException & n){
        debug( 0 ) << "Network exception: " << n.getMessage() << endl;
    }

    debug( 1 ) << "Client input stopped" << endl;

    return NULL;
}

static void do_finish_chat_input(void * arg){
    NetworkWorldClient * world = (NetworkWorldClient *) arg;
    world->endChatLine();
}
	
NetworkWorldClient::NetworkWorldClient( Network::Socket server, const std::vector< Object * > & players, const string & path, Object::networkid_t id, const map<Object::networkid_t, string> & clientNames, int screen_size ) throw ( LoadException ):
super( players, path, new NetworkCacher(), screen_size ),
server( server ),
removeChatTimer(0),
world_finished( false ),
secondCounter(Global::second_counter),
id(id),
running(true),
currentPing(0),
enable_chat(false),
clientNames(clientNames),
pingCounter(0){
    objects.clear();
    pthread_mutex_init( &message_mutex, NULL );
    pthread_mutex_init( &running_mutex, NULL );

    input.set(Keyboard::Key_T, 0, false, Talk);

    chatInput.addHandle(Keyboard::Key_ENTER, do_finish_chat_input, this);
}

void NetworkWorldClient::startMessageHandler(){
    pthread_create( &message_thread, NULL, handleMessages, this );
}
	
NetworkWorldClient::~NetworkWorldClient(){
    debug( 1 ) << "Destroy client world" << endl;
    chatInput.disable();
    /*
       stopRunning();
       pthread_join( message_thread, NULL );
       */
}

void NetworkWorldClient::endChatLine(){
    string message = chatInput.getText();
    chatInput.disable();
    chatInput.clearInput();

    if (message != ""){
        Network::Message chat;
        chat.id = 0;
        chat << CHAT;
        chat << getId();
        chat << message;
        addMessage(chat);
        chatMessages.push_back("You: " + message);
        while (chatMessages.size() > 10){
            chatMessages.pop_front();
        }
    }
}
	
bool NetworkWorldClient::isRunning(){
    pthread_mutex_lock( &running_mutex );
    bool b = running;
    pthread_mutex_unlock( &running_mutex );
    return b;
}

void NetworkWorldClient::stopRunning(){
    pthread_mutex_lock( &running_mutex );
    running = false;
    pthread_mutex_unlock( &running_mutex );
    Network::Message finish;
    finish << World::FINISH;
    finish.id = 0;
    finish.send(getServer());
    debug(1) << "Sent finish, waiting for message thread to end." << endl;
    pthread_join( message_thread, NULL );
}
	
void NetworkWorldClient::addIncomingMessage( const Network::Message & message ){
    pthread_mutex_lock( &message_mutex );
    incoming.push_back( message );
    pthread_mutex_unlock( &message_mutex );
}
	
void NetworkWorldClient::getIncomingMessages(vector<Network::Message> & messages){
    // vector< Network::Message > m;
    pthread_mutex_lock( &message_mutex );
    messages = incoming;
    incoming.clear();
    pthread_mutex_unlock( &message_mutex );
    // return m;
}

static Network::Message pausedMessage(){
    Network::Message message;
    message.id = 0;
    message << World::PAUSE;

    return message;
}

static Network::Message unpausedMessage(){
    Network::Message message;
    message.id = 0;
    message << World::UNPAUSE;

    return message;
}

void NetworkWorldClient::changePause(){
    super::changePause();
    if (isPaused()){
        addMessage(pausedMessage());
    } else {
        addMessage(unpausedMessage());
    }
}

bool NetworkWorldClient::uniqueObject( Object::networkid_t id ){
    for ( vector< Object * >::iterator it = objects.begin(); it != objects.end(); it++ ){
        Object * o = *it;
        if ( o->getId() == id ){
            return false;
        }
    }
    return true;
}

void NetworkWorldClient::handleCreateCharacter( Network::Message & message ){
    int alliance;
    Object::networkid_t id;
    int map;
    string path = Filesystem::find(message.path);
    message >> alliance >> id >> map;
    if ( uniqueObject( id ) ){
        bool found = false;
        for ( vector< PlayerTracker >::iterator it = players.begin(); it != players.end(); it++ ){
            Character * character = (Character *) it->player;
            if ( character->getId() == id ){
                character->deathReset();
                addObject( character );
                found = true;
                break;
            }

        }
        if ( ! found ){
            /* TODO: set the block id (different from network id) */
            BlockObject block;
            int isPlayer;
            message >> isPlayer;
            if (isPlayer == World::IS_PLAYER){
                block.setType( ObjectFactory::NetworkPlayerType );
            } else {
                block.setType( ObjectFactory::NetworkCharacterType );
            }
            block.setMap( map );
            block.setPath( path );
            Character * character = (Character *) ObjectFactory::createObject( &block );
            if ( character == NULL ){
                debug( 0 ) << "Could not create character!" << endl;
                return;
            }
            debug( 1 ) << "Create '" << path << "' with id " << id << " alliance " << alliance << endl;
            /* this is the network id */
            character->setId( id );
            character->setAlliance( alliance );
            /* TODO: should these values be hard-coded? */
            character->setX( 200 );
            character->setY( 0 );
            character->setZ( 150 );
            addObject( character );
        }
    } else {
        debug( 1 ) << id << " is not unique" << endl;
    }
}

void NetworkWorldClient::handleCreateCat( Network::Message & message ){
    Object::networkid_t id;
    message >> id;
    if ( uniqueObject( id ) ){
        string path = Filesystem::find(message.path);
        BlockObject block;
        block.setType( ObjectFactory::CatType );
        block.setPath( path );
        /* TODO: should these values be hard-coded? */
        block.setCoords( 200, 150 );
        Cat * cat = (Cat *) ObjectFactory::createObject( &block );
        if ( cat == NULL ){
            debug( 0 ) << "Could not create cat" << endl;
            return;
        }

        cat->setY( 0 );
        cat->setId( (unsigned int) -1 );
        addObject( cat );
    }
}
	
bool NetworkWorldClient::finished() const {
    return world_finished;
}

Object * NetworkWorldClient::removeObject( Object::networkid_t id ){
    for ( vector< Object * >::iterator it = objects.begin(); it != objects.end(); ){
        Object * o = *it;
        if ( o->getId() == id ){
            it = objects.erase( it );
            return o;
        } else {
            it++;
        }
    }
    return NULL;
}

void NetworkWorldClient::handleCreateItem( Network::Message & message ){
    int id;
    message >> id;
    if ( uniqueObject( id ) ){
        int x, z;
        int value;
        message >> x >> z >> value;
        string path = Filesystem::find(message.path);
        BlockObject block;
        block.setType( ObjectFactory::ItemType );
        block.setPath( path );
        /* TODO: dont hard-code this */
        block.setStimulationType( "health" );
        block.setStimulationValue( value );
        block.setCoords( x, z );
        Item * item = (Item *) ObjectFactory::createObject( &block );
        if ( item == NULL ){
            debug( 0 ) << "Could not create item" << endl;
            return;
        }

        item->setY( 0 );
        item->setId( id );
        addObject( item );
    }
}

void NetworkWorldClient::handleCreateBang( Network::Message & message ){
    int x, y, z;
    message >> x >> y >> z;
    Object * addx = bang->copy();
    addx->setX( x );
    addx->setY( 0 );
    addx->setZ( y+addx->getHeight()/2 );
    addx->setHealth( 1 );
    addx->setId( (unsigned int) -1 );
    addObject( addx );
}

Object * NetworkWorldClient::findNetworkObject( Object::networkid_t id ){
    for ( vector< Object * >::iterator it = objects.begin(); it != objects.end(); it++ ){
        Object * o = *it;
        if ( o->getId() == id ){
            return o;
        }
    }
    return NULL;
}

/* TODO: this code is duplicated in game/adventure_world.cpp and network_world.cpp.
 * try to abstract it out at some point.
 */
void NetworkWorldClient::removePlayer(Object * player){
    for ( vector< PlayerTracker >::iterator it = players.begin(); it != players.end(); ){
        PlayerTracker & tracker = *it;
        if (tracker.player == player){
            void * handle = tracker.script;
            if (handle != NULL){
                Script::Engine::getEngine()->destroyObject(handle);
            }
            tracker.player->setScriptObject(NULL);

            it = players.erase(it);
        } else {
            it++;
        }
    }
}

void NetworkWorldClient::handlePing(Network::Message & message){
    unsigned int id;
    message >> id;
    if (pings.find(id) != pings.end()){
        uint64_t drift = (message.timestamp - pings[id]);

        /* exponential moving average
         * http://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average
         */
        /* tested values of alpha
         *   0.25 - ok
         *   0.3 - ok
         */
        double alpha = 0.3;
        /* divide by 2 because drift is the time from client->server->client.
         * we want just client->server.
         * this assumes the time between client->server and server->client
         * is the same, which is probably a reasonable assumption.
         */
        currentPing = alpha * drift / 2 + (1.0 - alpha) * currentPing;

        Global::debug(1) << "Ping " << id << ": " << drift << " average ping: " << currentPing << endl;

        /* dont fill up the ping table; save memory */
        pings.erase(id);
    } else {
        Global::debug(0) << "Unknown ping reply: " << id << endl;
    }
}
	
/* messages are defined in ../world.h */
void NetworkWorldClient::handleMessage( Network::Message & message ){
    Global::debug(2) << "Handle message for id " << message.id << endl;
	if ( message.id == 0 ){
		int type;
		message >> type;
                Global::debug(2) << "Message type " << type << endl;
		switch ( type ){
			case CREATE_CHARACTER : {
				handleCreateCharacter( message );
				break;
			}
			case CREATE_CAT : {
				handleCreateCat( message );	
				break;
			}
			case CREATE_BANG : {
				handleCreateBang( message );
				break;
			}
			case CREATE_ITEM : {
				handleCreateItem( message );
				break;
			}
			case NEXT_BLOCK : {
				int block;
				message >> block;
				scene->advanceBlocks( block );
				break;
			}
			case GRAB : {
                                Object::networkid_t grabbing;
                                Object::networkid_t grabbed;
				message >> grabbing;
				message >> grabbed;
				Character * c_grabbing = (Character *) findNetworkObject( grabbing );
				Character * c_grabbed = (Character *) findNetworkObject( grabbed );
				if ( c_grabbing != NULL && c_grabbed != NULL ){
					c_grabbed->grabbed( c_grabbing );
					c_grabbing->setLink( c_grabbed );
				}
				break;
			}
			case DELETE_OBJ : {
                                Object::networkid_t id;
				message >> id;
				Object * o = removeObject( id );

                                if (isPlayer(o)){
                                    removePlayer(o);
                                } else {
                                    if ( o != NULL ){
                                        delete o;
                                    }
                                }
				break;
			}
                        case CHAT : {
                            Object::networkid_t him;
                            message >> him;
                            string name = clientNames[him];
                            if (him == 0){
                                name = "Server";
                            }
                            chatMessages.push_back(name + ": " + message.path);
                            while (chatMessages.size() > 10){
                                chatMessages.pop_front();
                            }
                            break;
                        }
			case IGNORE_MESSAGE : {
				break;
			}
			case REMOVE : {
                                Object::networkid_t id;
				message >> id;
				removeObject( id );
				break;
			}
			case FINISH : {

				debug( 1 ) << "Received finish message" << endl;
				world_finished = true;
				break;
			}
			case NOTHING : {
				debug( 0 ) << "Invalid message. Data dump" << endl;
				for ( int i = 0; i < Network::DATA_SIZE; i++ ){
					debug( 0 ) << (int) message.data[ i ] << " ";
				}
				debug( 0 ) << endl;
				break;
			}
                        case PING_REPLY : {
                            handlePing(message);
                            break;
                        }
                        case PAUSE : {
                            this->pause();
                            break;
                        }
                        case UNPAUSE : {
                            this->unpause();
                            break;
                        }
		}
	} else {
		for ( vector< Object * >::iterator it = objects.begin(); it != objects.end(); it++ ){
			Object * o = *it;
                        /* message.id is a uint16_t and getId() is an unsigned long */
			if ( o->getId() == message.id ){
				o->interpretMessage( message );
			}
		}
	}
}

void NetworkWorldClient::addMessage( Network::Message m, Network::Socket from, Network::Socket to ){
    if (m.id == (uint32_t) -1){
        ostringstream out;
        out << "Message not properly formed: ";
        for ( int i = 0; i < Network::DATA_SIZE; i++ ){
            out << (int) m.data[ i ] << " ";
        }
        throw Network::NetworkException(out.str());
    }
    /* clients should only send updates about their own character to the server.
     * all other movement (like from enemies) should be generated by the server.
     */
    if ( m.id == id || m.id == 0 ){
        debug(2) << "Sending message to id " << m.id << endl;
        outgoing.push_back( m );
    }
}
	
void NetworkWorldClient::doScene( int min_x, int max_x ){
    scene->act(min_x, max_x, 0);
#if 0
    vector< Object * > objs;
    scene->act( min_x, max_x,  &objs );

    /* throw out everything the scene just made because the server is
     * going to tell us which objects/characters to make
     */
    for ( vector< Object * >::iterator it = objs.begin(); it != objs.end(); it++ ){
        delete *it;
    }
#endif
}

Network::Message NetworkWorldClient::pingMessage(unsigned int pingId){
    Network::Message message;
    message.id = 0;
    message << World::PING_REQUEST;
    message << pingId;
    pings[pingId] = System::currentMicroseconds();

    Global::debug(1) << "Sending ping request " << pingId << endl;

    /* pings won't fill up unless the server dies or something, so
     * as a fail-safe don't let the table get too big
     */
    if (pings.size() > 1000){
        Global::debug(0, __FILE__) << "Warning: ping table is full. Is the server or network dead?" << endl;
        pings.clear();
    }

    return message;
}

void NetworkWorldClient::sendMessage( const Network::Message & message, Network::Socket socket ){
	message.send( socket );
}

void NetworkWorldClient::sendMessages(const vector<Network::Message> & messages, Network::Socket socket){
    Network::sendAllMessages(messages, socket);
    /*
    int length = Network::totalSize(messages);
    uint8_t * data = new uint8_t[length];
    Network::dump(messages, data);
    Network::sendBytes(socket, data, length);
    delete[] data;
    */
}
	
void NetworkWorldClient::draw(Bitmap * work){
    super::draw(work);
    const Font & font = Font::getFont(Filesystem::find(Global::DEFAULT_FONT), 15, 15);
    FontRender * render = FontRender::getInstance();

    /* the coordinates are only right becuase I know that the screen is twice as
     * large as `work'. FontRender should be changed so that we don't have to know
     * its coordinate system, something like opengl where -1 is the top and 1
     * is the bottom.
     */
    render->addMessage(font, 1, work->getHeight() * 2 - font.getHeight() - 1, Bitmap::makeColor(255, 255, 255), -1, "Ping %d", (int) (currentPing / 1000));
    // font.printf(1, work->getHeight() - 11, Bitmap::makeColor( 255, 255, 255 ), *work, "Ping %d", 0, (int) (currentPing / 1000));

    const Font & font2 = Font::getFont(Filesystem::find(Global::DEFAULT_FONT), 18, 18);

    if (chatInput.isEnabled()){
        const int green = Bitmap::makeColor(0, 255, 0);
        render->addMessage(font2, 1, work->getHeight() * 2 - font2.getHeight() * 2 - 1, green, -1, string("Say: ") + chatInput.getText());
    }

    int y = work->getHeight() * 2 - 1 - font2.getHeight() * 3 - 1;
    for (deque<string>::reverse_iterator it = chatMessages.rbegin(); it != chatMessages.rend(); it++){
        string message = *it;
        render->addMessage(font2, 1, y, Bitmap::makeColor(255, 255, 255), -1, message);
        y -= font2.getHeight() + 1;
    }

}

void NetworkWorldClient::act(){
	
    if ( quake_time > 0 ){
        quake_time--;
    }

    if (removeChatTimer > 0){
        removeChatTimer -= 1;
        if (removeChatTimer == 0 && chatMessages.size() > 0){
            chatMessages.pop_front();
        }
    } else if (chatMessages.size() > 0){
        removeChatTimer = 175;
    }

    InputMap<Keys>::Output inputState = InputManager::getMap(input);
    if (inputState[Talk]){
        enable_chat = true;
    } else {
        if (enable_chat){
            chatInput.enable();
            enable_chat = false;
        }
    }

    chatInput.doInput();

    vector< Object * > added_effects;
    if (! isPaused()){

        for ( vector< Object * >::iterator it = objects.begin(); it != objects.end(); it++ ){
            Object * o = *it;
            o->act( &objects, this, &added_effects );
            if ( o->getZ() < getMinimumZ() ){
                o->setZ( getMinimumZ() );
            }
            if ( o->getZ() > getMaximumZ() ){
                o->setZ( getMaximumZ() );
            }
        }

        double lowest = 9999999;
        for ( vector< PlayerTracker >::iterator it = players.begin(); it != players.end(); it++ ){
            Object * player = it->player;
            double mx = player->getX() - screen_size / 2;
            if ( it->min_x < mx ){
                it->min_x++;
            }

            if ( it->min_x + screen_size >= scene->getLimit() ){
                it->min_x = scene->getLimit() - screen_size;
            }

            if ( it->min_x < lowest ){
                lowest = it->min_x;
            }

            /*
               if ( player->getX() < it->min_x ){
               player->setX( it->min_x );
               }
               */
            if ( player->getX() < 0 ){
                player->setX( 0 );
            }

            if ( player->getX() > scene->getLimit() ){
                player->setX( scene->getLimit() );
            }
            if ( player->getZ() < getMinimumZ() ){
                player->setZ( getMinimumZ() );
            }
            if ( player->getZ() > getMaximumZ() ){
                player->setZ( getMaximumZ() );
            }
        }

        doScene( 0, 0 );
    }

    vector< Network::Message > messages;
    getIncomingMessages(messages);
    // vector< Network::Message > messages = getIncomingMessages();
    for ( vector< Network::Message >::iterator it = messages.begin(); it != messages.end(); it++ ){
        handleMessage( *it );
    }

    for ( vector< Object * >::iterator it = objects.begin(); it != objects.end(); ){
        if ( (*it)->getHealth() <= 0 ){
            (*it)->died( added_effects );
            if ( ! isPlayer( *it ) ){
                delete *it;
            }
            it = objects.erase( it );
        } else ++it;
    }

    if (currentPing < 1 || abs((long)(Global::second_counter) - (long)secondCounter) > 2){
        pingCounter += 1;
        addMessage(pingMessage(pingCounter));
        secondCounter = Global::second_counter;
    }

    sendMessages(outgoing, getServer());
    outgoing.clear();

    for ( vector< Object * >::iterator it = added_effects.begin(); it != added_effects.end(); ){
        Object * o = *it;
        o->setId( (Object::networkid_t) -1 );
        it++;
    }
    objects.insert( objects.end(), added_effects.begin(), added_effects.end() );
}
