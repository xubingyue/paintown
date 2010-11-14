#ifdef HAVE_NETWORKING

#include "util/bitmap.h"
#include "client.h"
#include "input/keyboard.h"
#include "init.h"
#include "globals.h"
#include "util/funcs.h"
#include "util/font.h"
#include "util/file-system.h"
#include "level/utils.h"
#include "factory/font_render.h"
#include "factory/object_factory.h"
#include "factory/heart_factory.h"
#include "game/world.h"
#include "music.h"
#include "object/character.h"
#include "object/player.h"
#include "object/network_character.h"
#include "object/network_player.h"
#include "network_world_client.h"
#include "game/game.h"
#include "game/mod.h"
#include "exceptions/exception.h"
#include "loading.h"
#include "chat_client.h"
#include "network.h"
#include "configuration.h"
#include "util/system.h"
#include <string>
#include <sstream>

using namespace std;

namespace Network{

static bool uniqueId( const vector< Paintown::Object * > & objs, Paintown::Object::networkid_t id ){
    for ( vector< Paintown::Object * >::const_iterator it = objs.begin(); it != objs.end(); it++ ){
        Paintown::Object * o = *it;
        if ( o->getId() == id ){
            return false;
        }
    }
    return true;
}

/* send ok to server, get an ok back */
static void waitForServer(Socket socket){
    Message ok;
    ok << World::OK;
    ok.send( socket );
    Global::debug( 1 ) << "Sent ok " << endl;
    Message ok_ack(socket);
    int type;
    ok_ack >> type;
    if (type != World::OK){
        Global::debug(0) << "Did not receive ok from the server, instead received " << type << ". Badness may ensure" << endl;
    }
    Global::debug( 1 ) << "Received ok " << endl;
}

static void sendDummy(Socket socket){
    Message dummy;
    dummy.id = 0;
    dummy << World::IGNORE_MESSAGE;
    dummy.send( socket );
}

static void sendQuit(Socket socket){
    Message quit;
    quit.id = 0;
    quit << World::QUIT;
    quit.send(socket);
}

/* TODO: simplify this code */
static void playGame( Socket socket ){
    Util::Thread::Id loadingThread;
    try{
        /* TODO: get the info from the server */
        Level::LevelInfo info;
        int remap = 0;
        Filesystem::AbsolutePath playerPath = Paintown::Mod::getCurrentMod()->selectPlayer("Pick a player", info, remap);
        Paintown::Player * player = new Paintown::Player(playerPath);
        player->setMap(remap);
        ((Paintown::Player *) player)->ignoreLives();
        Filesystem::RelativePath path = Filesystem::cleanse(playerPath);
        // path.erase( 0, Util::getDataPath().length() );

        Loader::startLoading( &loadingThread );

        /* send the path of the chosen player */
        Message create;
        create << World::CREATE_CHARACTER;
        create.path = path.path();
        create.send( socket );

        /* get the id from the server */
        Message myid( socket );
        int type;
        int alliance;
        myid >> type;
        Paintown::Object::networkid_t client_id = (Paintown::Object::networkid_t) -1;
        if ( type == World::SET_ID ){
            myid >> client_id >> alliance;
            player->setId( client_id );
            player->setAlliance(alliance);
            Global::debug( 1 ) << "Client id is " << client_id << endl;
        } else {
            Global::debug( 0 ) << "Bogus message, expected SET_ID(" << World::SET_ID << ") got " << type << endl;
        }

        vector< Paintown::Object * > players;
        players.push_back( player );

        map<Paintown::Object::networkid_t, string> clientNames;

        bool done = false;
        int showHelp = 800;
        while ( ! done ){
            Message next( socket );
            int type;
            next >> type;
            switch ( type ){
                case World::CREATE_CHARACTER : {
                    Paintown::Object::networkid_t id;
                    int alliance;
                    next >> id >> alliance;
                    if ( uniqueId( players, id ) ){
                        Global::debug(1) << "Create a new network player id " << id << " alliance " << alliance << endl;
                        Paintown::Character * c = new Paintown::NetworkPlayer(Filesystem::find(Filesystem::RelativePath(next.path)), alliance);
                        c->setId( id );
                        ((Paintown::NetworkCharacter *)c)->alwaysShowName();
                        players.push_back( c );
                    }
                    break;
                }
                case World::CLIENT_INFO : {
                    Paintown::Object::networkid_t id;
                    next >> id;
                    string name = next.path;
                    clientNames[id] = name;
                    break;
                }
                case World::LOAD_LEVEL : {
                    Filesystem::AbsolutePath level = Filesystem::find(Filesystem::RelativePath(next.path));
                    NetworkWorldClient world(socket, players, level, client_id, clientNames);
                    Music::pause();
                    Music::fadeIn( 0.3 );
                    Music::loadSong(Filesystem::getFiles(Filesystem::find(Filesystem::RelativePath("music/")), "*" ) );
                    Music::play();

                    Global::info("Waiting for ok from server");
                    waitForServer(socket);

                    world.startMessageHandler();

                    Loader::stopLoading(loadingThread);
                    try{
                        vector<Paintown::Object*> xplayers;
                        bool forceQuit = ! Game::playLevel(world, xplayers, showHelp);
                        showHelp = 0;

                        ObjectFactory::destroy();
                        HeartFactory::destroy();

                        Loader::startLoading( &loadingThread );

                        if (forceQuit){
                            Global::debug(1, __FILE__) << "Force quit" << endl;
                            sendQuit(socket);
                            /* After quit is sent the socket will be closed
                             * by someone later on. The input handler in the client
                             * world will throw a network exception and then
                             * the thread will die.
                             */
                            done = true;
                        } else {
                            Global::debug(1) << "Stop running client world" << endl;
                            /* this dummy lets the server message handler
                             * stop running. its currently blocked waiting for
                             * a message to come through.
                             */
                            sendDummy(socket);
                            world.stopRunning();
                            Global::debug(1) << "Wait for server " << endl;
                            /* then wait for a barrier */
                            waitForServer(socket);
                        }
                    } catch ( const Exception::Return & e ){
                        /* do we need to close the socket here?
                         * when this function returns the socket will be
                         * close anyway.
                         */
                        Network::close( socket );
                    }
                    break;
                }
                /* thats all folks! */
                case World::GAME_OVER : {
                    done = true;
                    break;
                }
            }
        }

        for (vector<Paintown::Object*>::iterator it = players.begin(); it != players.end(); it++){
            delete *it;
        }
    } catch ( const LoadException & le ){
        Global::debug(0, "client") << "Load exception: " + le.getTrace() << endl;
    }

    Loader::stopLoading(loadingThread);
}

static void drawBox( const Bitmap & area, const Bitmap & copy, const string & str, const Font & font, bool hasFocus ){
	copy.Blit( area );
	Bitmap::transBlender( 0, 0, 0, 192 );
	area.drawingMode( Bitmap::MODE_TRANS );
	area.rectangleFill( 0, 0, area.getWidth(), area.getHeight(), Bitmap::makeColor( 0, 0, 0 ) );
	area.drawingMode( Bitmap::MODE_SOLID );
	int color = Bitmap::makeColor( 255, 255, 255 );
	if ( hasFocus ){
		color = Bitmap::makeColor( 255, 255, 0 );
	}
	area.rectangle( 0, 0, area.getWidth() - 1, area.getHeight() - 1, color );
	font.printf( 1, 0, Bitmap::makeColor( 255, 255, 255 ), area, str, 0 );
}

static char lowerCase( const char * x ){
	if ( x[0] >= 'A' && x[0] <= 'Z' ){
		return x[0] - 'A' + 'a';
	}
	return x[0];
}

static bool handleNameInput( string & str, const vector< int > & keys ){
	bool cy = false;
	for ( vector< int >::const_iterator it = keys.begin(); it != keys.end(); it++ ){
		const int & key = *it;
		if ( str.length() < 16 && Keyboard::isAlpha( key ) ){
			str += lowerCase( Keyboard::keyToName( key ) );
			cy = true;
		} else if ( key == Keyboard::Key_BACKSPACE ){
			if ( str != "" ){
				str = str.substr( 0, str.length() - 1 );
				cy = true;
			}
		}
	}
	return cy;
}

static bool handleHostInput( string & str, const vector< int > & keys ){
	bool cy = false;
	for ( vector< int >::const_iterator it = keys.begin(); it != keys.end(); it++ ){
		const int & key = *it;
		if ( Keyboard::isAlpha( key ) || key == Keyboard::Key_STOP ){
			str += lowerCase( Keyboard::keyToName( key ) );
			cy = true;
		} else if ( key == Keyboard::Key_BACKSPACE ){
			if ( str != "" ){
				str = str.substr( 0, str.length() - 1 );
				cy = true;
			}
		}
	}
	return cy;
}

static bool handlePortInput( string & str, const vector< int > & keys ){
	bool cy = false;
	for ( vector< int >::const_iterator it = keys.begin(); it != keys.end(); it++ ){
		const int & key = *it;
		if ( Keyboard::isNumber( key ) || key == Keyboard::Key_STOP ){
			str += Keyboard::keyToName( key );
			cy = true;
		} else if ( key == Keyboard::Key_BACKSPACE ){
			if ( str != "" ){
				str = str.substr( 0, str.length() - 1 );
			cy = true;
			}
		}
	}
	return cy;
}

static void popup( Bitmap & work, const Font & font, const string & message ){
	int length = font.textLength( message.c_str() ) + 20; 
	// Bitmap area( *Bitmap::Screen, GFX_X / 2 - length / 2, 220, length, font.getHeight() * 3 );
	Bitmap area( work, GFX_X / 2 - length / 2, 220, length, font.getHeight() * 3 );
	Bitmap::transBlender( 0, 0, 0, 128 );
	area.drawingMode( Bitmap::MODE_TRANS );
	area.rectangleFill( 0, 0, area.getWidth(), area.getHeight(), Bitmap::makeColor( 64, 0, 0 ) );
	area.drawingMode( Bitmap::MODE_SOLID );
	int color = Bitmap::makeColor( 255, 255, 255 );
	area.rectangle( 0, 0, area.getWidth() - 1, area.getHeight() - 1, color );
	font.printf( 10, area.getHeight() / 2, Bitmap::makeColor( 255, 255, 255 ), area, message, 0 );
        work.BlitToScreen();
}

static const char * getANumber(){
	switch( Util::rnd( 10 ) ){
		case 0 : return "0";
		case 1 : return "1";
		case 2 : return "2";
		case 3 : return "3";
		case 4 : return "4";
		case 5 : return "5";
		case 6 : return "6";
		case 7 : return "7";
		case 8 : return "8";
		case 9 : return "9";
		default : return "0";
	}
}


void networkClient(){
	Bitmap background(Global::titleScreen().path());
	Global::speed_counter = 0;
	Keyboard keyboard;
	keyboard.setAllDelay( 200 );
        
        const char * propertyLastClientName = "network:last-client-name";
        const char * propertyLastClientHost = "network:last-client-host";
        const char * propertyLastClientPort = "network:last-client-port";

	string name = Configuration::getStringProperty(propertyLastClientName, string("player") + getANumber() + getANumber());
	string host = Configuration::getStringProperty(propertyLastClientHost, "localhost");
	string port = Configuration::getStringProperty(propertyLastClientPort, "7887");

	enum Focus{
		NAME, HOST, PORT, CONNECT, BACK
	};
			
	const Font & font = Font::getFont(Global::DEFAULT_FONT, 20, 20 );

	Bitmap work( GFX_X, GFX_Y );
	
	Focus focus = NAME;

	bool done = false;
	bool draw = true;
	while ( ! done ){
		int think = Global::speed_counter;
		while ( think > 0 ){
			think -= 1;
			keyboard.poll();

			if ( keyboard[ Keyboard::Key_TAB ] || keyboard[ Keyboard::Key_DOWN ] ){
				draw = true;
				switch ( focus ){
					case NAME : focus = HOST; break;
					case HOST : focus = PORT; break;
					case PORT : focus = CONNECT; break;
					case CONNECT : focus = BACK; break;
					case BACK : focus = NAME; break;
					default : focus = HOST;
				}
			}

            if ( keyboard[ Keyboard::Key_UP ] ){
                draw = true;
                switch ( focus ){
                    case NAME : focus = BACK; break;
                    case HOST : focus = NAME; break;
					case PORT : focus = HOST; break;
					case CONNECT : focus = PORT; break;
					case BACK : focus = CONNECT; break;
                }
            }

			if ( keyboard[ Keyboard::Key_ESC ] ){
				throw Exception::Return(__FILE__, __LINE__);
			}

			if ( keyboard[ Keyboard::Key_ENTER ] ){
				switch ( focus ){
					case NAME :
					case HOST :
					case PORT : break;
					case CONNECT : {
						done = true;
						try{
                                                    Configuration::setStringProperty(propertyLastClientName, name);
                                                    Configuration::setStringProperty(propertyLastClientHost, host);
                                                    Configuration::getStringProperty(propertyLastClientPort, port);
							istringstream is( port );
							int porti;
							is >> porti;
							Network::Socket socket = Network::connect( host, porti );
							ChatClient chat( socket, name );
							keyboard.wait();
							chat.run();
							if ( chat.isFinished() ){
                                                            playGame( socket );
							}
							Network::close( socket );
						} catch ( const NetworkException & e ){
							popup( work, font, e.getMessage() );
							keyboard.wait();
							keyboard.readKey();
							/*
							Global::showTitleScreen();
							font.printf( 20, min_y - font.getHeight() * 3 - 1, Bitmap::makeColor( 255, 255, 255 ), *Bitmap::Screen, "Name", 0 );
							font.printf( 20, min_y - font.getHeight() - 1, Bitmap::makeColor( 255, 255, 255 ), *Bitmap::Screen, "Host", 0 );
							font.printf( 20, min_y + font.getHeight() * 2 - font.getHeight() - 1, Bitmap::makeColor( 255, 255, 255 ), *Bitmap::Screen, "Port", 0 );
							font.printf( 20, 20, Bitmap::makeColor( 255, 255, 255 ), *Bitmap::Screen, "Press TAB to cycle the next input", 0 );
							*/
							done = false;
							draw = true;
							think = 0;
						}
						break;
					}
					case BACK : done = true; break;
				}
			}

			vector< int > keys;
			keyboard.readKeys( keys );
			switch ( focus ){
				case HOST : {
					draw = draw || handleHostInput( host, keys );
					break;
				}
				case PORT : {
					draw = draw || handlePortInput( port, keys );
					break;
				}
				case NAME : {
					draw = draw || handleNameInput( name, keys );
					break;
				}
				default : {
					break;
				}
			}
			
			Global::speed_counter = 0;
		}

		if ( draw ){
			draw = false;

			background.Blit( work );

			const int inputBoxLength = font.textLength( "a" ) * 30;
			const int min_y = 140;

			font.printf( 20, min_y - font.getHeight() * 3 - 1, Bitmap::makeColor( 255, 255, 255 ), work, "Your name", 0 );
			Bitmap nameBox( work, 20, min_y - font.getHeight() * 2, inputBoxLength, font.getHeight() );
			Bitmap copyNameBox( nameBox.getWidth(), nameBox.getHeight() );
			nameBox.Blit( copyNameBox );

			font.printf( 20, min_y - font.getHeight() - 1, Bitmap::makeColor( 255, 255, 255 ), work, "Host (IP address or name)", 0 );
			Bitmap hostBox( work, 20, min_y, inputBoxLength, font.getHeight() );
			Bitmap copyHostBox( hostBox.getWidth(), hostBox.getHeight() );
			hostBox.Blit( copyHostBox );

			font.printf( 20, min_y + font.getHeight() * 2 - font.getHeight() - 1, Bitmap::makeColor( 255, 255, 255 ), work, "Network Host Port", 0 );
			Bitmap portBox( work, 20, min_y + font.getHeight() * 2, inputBoxLength, font.getHeight() );
			Bitmap copyPortBox( portBox.getWidth(), portBox.getHeight() );
			portBox.Blit( copyPortBox );

			font.printf( 20, 20, Bitmap::makeColor( 255, 255, 255 ), work, "Press TAB to cycle the next input", 0 );

			int focusColor = Bitmap::makeColor( 255, 255, 0 );
			int unFocusColor = Bitmap::makeColor( 255, 255, 255 );

			drawBox( nameBox, copyNameBox, name, font, focus == NAME );
			drawBox( hostBox, copyHostBox, host, font, focus == HOST );
			drawBox( portBox, copyPortBox, port, font, focus == PORT );
			font.printf( 20, min_y + font.getHeight() * 5, focus == CONNECT ? focusColor : unFocusColor, work, "Connect", 0 );
			font.printf( 20, min_y + font.getHeight() * 6 + 5, focus == BACK ? focusColor : unFocusColor, work, "Back", 0 );

			work.BlitToScreen();
		}

		while ( Global::speed_counter == 0 ){
			Util::rest( 1 );
			keyboard.poll();
		}
	}
}

}

#endif
