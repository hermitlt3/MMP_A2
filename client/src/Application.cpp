#include <iostream>
#include <string>
#include <fstream>

#include "Application.h"
#include "ship.h"
#include "Globals.h"

#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "Bitstream.h"
#include "GetTime.h"

#include "hge.h"

#include "config.h"
#include "MyMsgIDs.h"




float GetAbsoluteMag( float num )
{
	if ( num < 0 )
	{
		return -num;
	}

	return num;
}

/** 
* Constuctor
*
* Creates an instance of the graphics engine and network engine
*/

Application::Application() : 
	hge_(hgeCreate(HGE_VERSION)),
	rakpeer_(RakNetworkFactory::GetRakPeerInterface()),
	timer_( 0 )
{
}

/**
* Destructor
*
* Does nothing in particular apart from calling Shutdown
*/

Application::~Application() throw()
{
	Shutdown();
	rakpeer_->Shutdown(100);
	RakNetworkFactory::DestroyRakPeerInterface(rakpeer_);
}

/**
* Initialises the graphics system
* It should also initialise the network system
*/

bool Application::Init()
{
	std::ifstream inData;	
	std::string serverip;
    float init_pos_x, init_pos_y;

	inData.open("serverip.txt");

	inData >> serverip;

	srand( RakNet::GetTime() );

	hge_->System_SetState(HGE_FRAMEFUNC, Application::Loop);
	hge_->System_SetState(HGE_WINDOWED, true);
	hge_->System_SetState(HGE_USESOUND, false);
	hge_->System_SetState(HGE_TITLE, "Movement");
	hge_->System_SetState(HGE_LOGFILE, "movement.log");
	hge_->System_SetState(HGE_DONTSUSPEND, true);

	if(hge_->System_Initiate()) 
	{
        init_pos_x = (float)(rand( ) % 500 + 100);
        init_pos_y = (float)(rand( ) % 400 + 100);
        ships_.push_back( new Ship( rand() % 4 + 1, init_pos_x, init_pos_y ) );
		ships_.at(0)->SetName("My Ship");
		if (rakpeer_->Startup(1,30,&SocketDescriptor(), 1))
		{
			rakpeer_->SetOccasionalPing(true);
            return rakpeer_->Connect( serverip.c_str( ), DFL_PORTNUMBER, 0, 0 );
		}
	}
	return false;
}

/**
* Update cycle
*
* Checks for keypresses:
*   - Esc - Quits the game
*   - Left - Rotates ship left
*   - Right - Rotates ship right
*   - Up - Accelerates the ship
*   - Down - Deccelerates the ship
*
* Also calls Update() on all the ships in the universe
*/
bool Application::Update()
{
	if (hge_->Input_GetKeyState(HGEK_ESCAPE))
		return true;

	float timedelta = hge_->Timer_GetDelta();

	ships_.at(0)->SetAngularVelocity( 0.0f );

	if (hge_->Input_GetKeyState(HGEK_LEFT))
	{
		ships_.at(0)->SetAngularVelocity( ships_.at(0)->GetAngularVelocity() - DEFAULT_ANGULAR_VELOCITY );
	}

	if (hge_->Input_GetKeyState(HGEK_RIGHT))
	{
		ships_.at(0)->SetAngularVelocity( ships_.at(0)->GetAngularVelocity() + DEFAULT_ANGULAR_VELOCITY );
	}

	if (hge_->Input_GetKeyState(HGEK_UP))
	{
		ships_.at(0)->Accelerate(DEFAULT_ACCELERATION, timedelta);
	}

	if (hge_->Input_GetKeyState(HGEK_DOWN))
	{
		ships_.at(0)->Accelerate(-DEFAULT_ACCELERATION, timedelta);
	}

	for (ShipList::iterator ship = ships_.begin();
		ship != ships_.end(); ship++)
	{
		(*ship)->Update(timedelta);

		//collisions
		if( (*ship) == ships_.at(0) )
			checkCollisions( (*ship) );
	}


	if (Packet* packet = rakpeer_->Receive())
	{
		RakNet::BitStream bs(packet->data, packet->length, false);
		
		unsigned char msgid = 0;
		RakNetTime timestamp = 0;

		bs.Read(msgid);

		if (msgid == ID_TIMESTAMP)
		{
			bs.Read(timestamp);
			bs.Read(msgid);
		}

		switch(msgid)
		{
		case ID_CONNECTION_REQUEST_ACCEPTED:
			std::cout << "Connected to Server" << std::endl;
			break;

		case ID_NO_FREE_INCOMING_CONNECTIONS:
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			std::cout << "Lost Connection to Server" << std::endl;
			rakpeer_->DeallocatePacket(packet);
			return true;

		case ID_WELCOME:
			{
				unsigned int shipcount, id;
				float x_, y_;
				int type_;
				std::string temp;
				char chartemp[5];

				bs.Read(id);
				ships_.at(0)->setID( id );	
				bs.Read(shipcount);

				for (unsigned int i = 0; i < shipcount; ++ i)
				{
					bs.Read(id);
					bs.Read(x_);
					bs.Read(y_);
					bs.Read(type_);
					std::cout << "New Ship pos" << x_ << " " << y_ << std::endl;
					Ship* ship = new Ship(type_, x_, y_ ); 
					temp = "Ship ";
					temp += _itoa_s(id, chartemp, 10);
					ship->SetName(temp.c_str());
					ship->setID( id );
					ships_.push_back(ship);
				}

				SendInitialPosition();
			}
			break;

		case ID_NEWSHIP:
			{
				unsigned int id;
				bs.Read(id);

				if( id == ships_.at(0)->GetID() )
				{
					// if it is me
					break;
				}
				else
				{
					float x_, y_;
					int type_;
					std::string temp;
					char chartemp[5];

					bs.Read( x_ );
					bs.Read( y_ );
					bs.Read( type_ );
					std::cout << "New Ship pos" << x_ << " " << y_ << std::endl;
					Ship* ship = new Ship(type_, x_, y_);
					temp = "Ship "; 
					temp += _itoa_s(id, chartemp, 10);
					ship->SetName(temp.c_str());
					ship->setID( id );
					ships_.push_back(ship);
				}

			}
			break;

		case ID_LOSTSHIP:
			{
				unsigned int shipid;
				bs.Read(shipid);
				for (ShipList::iterator itr = ships_.begin(); itr != ships_.end(); ++itr)
				{
					if ((*itr)->GetID() == shipid)
					{
						delete *itr;
						ships_.erase(itr);
						break;
					}
				}
			}
			break;

		case ID_INITIALPOS:
			break;

		case ID_MOVEMENT:
			{
				unsigned int shipid;
				float temp;
				float x,y,w;
				float velocity_x, velocity_y, angular_velocity;
				bs.Read(shipid);
				for (ShipList::iterator itr = ships_.begin(); itr != ships_.end(); ++itr)
				{
					if ((*itr)->GetID() == shipid)
					{
						// Lab 13 Task 2 : Interpolation
#ifdef INTERPOLATEMOVEMENT
						// Step 8 : Instead of updating to SetLocation(), use SetServerLocation()
						// do bitstreams read for x, y, w
						bs.Read(x);
						bs.Read(y);
						bs.Read(w);
						// call SetServerLocation() for this ship
						(*itr)->SetServerLocation(x, y, w);
						// do a bitstream read to float temp
						bs.Read(temp);
						// call SetServerVelocityX()
						(*itr)->SetServerVelocityX(x);
						// do a bitstream read to float temp
						bs.Read(temp);
						// call SetServerVelocityX()
						(*itr)->SetServerVelocityY(y);
						// do a bitstream read to float temp
						bs.Read(temp);
						// call SetAngularVelocity()
						(*itr)->SetAngularVelocity(w);
						// call DoInterpolateUpdate()
						(*itr)->DoInterpolateUpdate();

#else
						bs.Read(x);
						bs.Read(y);
						bs.Read(w);
						(*itr)->setLocation( x, y, w ); 

						// Lab 13 Task 1 : Read Extrapolation Data velocity x, velocity y & angular velocity
	#ifdef EXTRAPOLATEMOVEMENT
						bs.Read(velocity_x);
						bs.Read(velocity_y);
						bs.Read(angular_velocity);
						(*itr)->SetVelocityX(velocity_x);
						(*itr)->SetVelocityY(velocity_y);
						(*itr)->SetAngularVelocity(angular_velocity);
	#endif
#endif

						break;
					}
				}
			}
			break;

		case ID_COLLIDE:
			{
				unsigned int shipid;
				float x, y;
				bs.Read(shipid);
				
				if( shipid == ships_.at(0)->GetID() )
				{
					std::cout << "collided with someone!" << std::endl;
					bs.Read(x);
					bs.Read(y);
					ships_.at(0)->SetX( x );
					ships_.at(0)->SetY( y );
					bs.Read(x);
					bs.Read(y);
					ships_.at(0)->SetVelocityX( x );
					ships_.at(0)->SetVelocityY( y );
					// Lab 13 Task 3 : Collision update for Interpolation
#ifdef INTERPOLATEMOVEMENT
					// Step 12 : Read and update SetServerVelocityX() and SetServerVelocityY()
					bs.Read(x);
					bs.Read(y);
					ships_.at(0)->SetServerVelocityX(x);
					ships_.at(0)->SetServerVelocityY(y);
#endif	
				}
			}
			break;

		default:
			std::cout << "Unhandled Message Identifier: " << (int)msgid << std::endl;

		}
		rakpeer_->DeallocatePacket(packet);
	}

	if (RakNet::GetTime() - timer_ > 1000)
	{
		timer_ = RakNet::GetTime();
		RakNet::BitStream bs2;
		unsigned char msgid = ID_MOVEMENT;
		bs2.Write(msgid);

		// Lab 13 Task 2 : Interpolation
#ifdef INTERPOLATEMOVEMENT
		// step 9 : Instead of sending x,y,w ..... , send the server version instead
		bs2.Write(ships_.at(0)->GetID());
		// do bitstream write for GetServerX()
		bs2.Write(ships_.at(0)->GetServerX());
		// do bitstream write for GetServerY()
		bs2.Write(ships_.at(0)->GetServerY());
		// do bitstream write for GetServerW()
		bs2.Write(ships_.at(0)->GetServerW());
		// do bitstream write for GetServerVelocityX()
		bs2.Write(ships_.at(0)->GetServerVelocityX());
		// do bitstream write for GetServerVelocityY()
		bs2.Write(ships_.at(0)->GetServerVelocityY());
		// do bitstream write for GetAngularVelocity()
		bs2.Write(ships_.at(0)->GetAngularVelocity());
#else
		bs2.Write(ships_.at(0)->GetID());
		bs2.Write(ships_.at(0)->GetX());
		bs2.Write(ships_.at(0)->GetY());
		bs2.Write(ships_.at(0)->GetW());
		// Lab 13 Task 1 : Add Extrapolation Data velocity x, velocity y & angular velocity
	#ifdef EXTRAPOLATEMOVEMENT
		bs2.Write(ships_.at(0)->GetVelocityX());
		bs2.Write(ships_.at(0)->GetVelocityY());
		bs2.Write(ships_.at(0)->GetAngularVelocity());
	
	#endif
#endif

		rakpeer_->Send(&bs2, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	}

	return false;
}


/**
* Render Cycle
*
* Clear the screen and render all the ships
*/
void Application::Render()
{
	hge_->Gfx_BeginScene();
	hge_->Gfx_Clear(0);

	ShipList::iterator itr;
	for (itr = ships_.begin(); itr != ships_.end(); itr++)
	{
		(*itr)->Render();
	}

	hge_->Gfx_EndScene();
}


/** 
* Main game loop
*
* Processes user input events
* Supposed to process network events
* Renders the ships
*
* This is a static function that is called by the graphics
* engine every frame, hence the need to loop through the
* global namespace to find itself.
*/
bool Application::Loop()
{
	Global::application->Render();
	return Global::application->Update();
}

/**
* Shuts down the graphics and network system
*/

void Application::Shutdown()
{
	hge_->System_Shutdown();
	hge_->Release();
}

/** 
* Kick starts the everything, called from main.
*/
void Application::Start()
{
	if (Init())
	{
		hge_->System_Start();
	}
}

bool Application::SendInitialPosition()
{
	RakNet::BitStream bs;
	unsigned char msgid = ID_INITIALPOS;
	bs.Write(msgid);
	bs.Write(ships_.at(0)->GetX());
	bs.Write(ships_.at(0)->GetY());
	bs.Write(ships_.at(0)->GetType());

	std::cout << "Sending pos" << ships_.at(0)->GetX() << " " << ships_.at(0)->GetY() << std::endl;

	rakpeer_->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

	return true;
}

bool Application::checkCollisions(Ship* ship)
{
	for (std::vector<Ship*>::iterator thisship = ships_.begin();
		thisship != ships_.end(); thisship++)
	{
		if( (*thisship) == ship ) continue;	//skip if it is the same ship

		if( ship->HasCollided( (*thisship) ) )
		{
			if( (*thisship)->CanCollide( RakNet::GetTime() ) &&  ship->CanCollide( RakNet::GetTime() ) )
			{
				std::cout << "collide!" << std::endl;

			// Lab 13 Task 3 : Collision update for Interpolation
#ifdef INTERPOLATEMOVEMENT
			// step 10 : Besides updating SetVelocityY() and SetVelocityX(), you need to update SetServerVelocityY() and SetServerVelocityX() as well
				if (GetAbsoluteMag(ship->GetServerVelocityY()) > GetAbsoluteMag((*thisship)->GetServerVelocityY()))
				{
					// this transfers vel to thisship
					(*thisship)->SetVelocityY( (*thisship)->GetVelocityY() + ship->GetVelocityY()/3 );
					ship->SetVelocityY( - ship->GetVelocityY() );
					
					(*thisship)->SetServerVelocityY((*thisship)->GetVelocityY());
					ship->SetServerVelocityY(ship->GetVelocityY());
				}
				else
				{
					ship->SetVelocityY(ship->GetVelocityY() + (*thisship)->GetVelocityY() / 3);
					(*thisship)->SetVelocityY(-(*thisship)->GetVelocityY() / 2);
					
					ship->SetServerVelocityY(ship->GetVelocityY());
					(*thisship)->SetServerVelocityY((*thisship)->GetVelocityY());
				}

				if (GetAbsoluteMag(ship->GetServerVelocityX()) > GetAbsoluteMag((*thisship)->GetServerVelocityX()))
				{
					// this transfers vel to thisship
					(*thisship)->SetVelocityX((*thisship)->GetVelocityX() + ship->GetVelocityX() / 3);
					ship->SetVelocityX(-ship->GetVelocityX());
					
					(*thisship)->SetServerVelocityX((*thisship)->GetVelocityX());
					ship->SetServerVelocityX(ship->GetVelocityX());
				}
				else
				{
					// ship transfers vel to asteroid
					ship->SetVelocityX(ship->GetVelocityX() + (*thisship)->GetVelocityX() / 3);
					(*thisship)->SetVelocityX(-(*thisship)->GetVelocityX() / 2);
					
					ship->SetServerVelocityX(ship->GetVelocityX());
					(*thisship)->SetServerVelocityX((*thisship)->GetVelocityX());
				}

				/*if (GetAbsoluteMag(ship->GetServerVelocityY()) > GetAbsoluteMag((*thisship)->GetServerVelocityY()))
				{
					// this transfers vel to thisship
					(*thisship)->SetServerVelocityY((*thisship)->GetServerVelocityY() + ship->GetServerVelocityY() / 3);
					ship->SetServerVelocityY(-ship->GetServerVelocityY());
				}
				else
				{
					ship->SetServerVelocityY(ship->GetServerVelocityY() + (*thisship)->GetServerVelocityY() / 3);
					(*thisship)->SetServerVelocityY(-(*thisship)->GetServerVelocityY() / 2);
				}

				if (GetAbsoluteMag(ship->GetServerVelocityX()) > GetAbsoluteMag((*thisship)->GetServerVelocityX()))
				{
					// this transfers vel to thisship
					(*thisship)->SetServerVelocityX((*thisship)->GetServerVelocityX() + ship->GetServerVelocityX() / 3);
					ship->SetServerVelocityX(-ship->GetServerVelocityX());
				}
				else
				{
					// ship transfers vel to asteroid
					ship->SetServerVelocityX(ship->GetServerVelocityX() + (*thisship)->GetServerVelocityX() / 3);
					(*thisship)->SetServerVelocityX(-(*thisship)->GetServerVelocityX() / 2);
				}*/
				ship->SetPreviousLocation();
		
#else
			if( GetAbsoluteMag( ship->GetVelocityY() ) > GetAbsoluteMag( (*thisship)->GetVelocityY() ) )
			{
				// this transfers vel to thisship
				(*thisship)->SetVelocityY( (*thisship)->GetVelocityY() + ship->GetVelocityY()/3 );
				ship->SetVelocityY( - ship->GetVelocityY() );
			}
			else
			{
				ship->SetVelocityY( ship->GetVelocityY() + (*thisship)->GetVelocityY()/3 ); 
				(*thisship)->SetVelocityY( -(*thisship)->GetVelocityY()/2 );
			}
			
			if( GetAbsoluteMag( ship->GetVelocityX() ) > GetAbsoluteMag( (*thisship)->GetVelocityX() ) )
			{
				// this transfers vel to thisship
				(*thisship)->SetVelocityX( (*thisship)->GetVelocityX() + ship->GetVelocityX()/3 );
				ship->SetVelocityX( - ship->GetVelocityX() );
			}
			else
			{
				// ship transfers vel to asteroid
				ship->SetVelocityX( ship->GetVelocityX() + (*thisship)->GetVelocityX()/3 ); 
				(*thisship)->SetVelocityX( -(*thisship)->GetVelocityX()/2 );
			}


//				ship->SetVelocityY( -ship->GetVelocityY() );
//				ship->SetVelocityX( -ship->GetVelocityX() );
			
				ship->SetPreviousLocation();
#endif
				SendCollision( (*thisship) );

				return true;
			}
				
		}

	}
	
	return false;
}

void Application::SendCollision( Ship* ship )
{
	RakNet::BitStream bs;
	unsigned char msgid = ID_COLLIDE;
	bs.Write( msgid );
	bs.Write( ship->GetID() );
	bs.Write( ship->GetX() );
	bs.Write( ship->GetY() );
	bs.Write( ship->GetVelocityX() );
	bs.Write( ship->GetVelocityY() );
	// Lab 13 Task 3 : Collision update for Interpolation
#ifdef INTERPOLATEMOVEMENT
	// step 11 : Send the ServerVelocityX and ServerVelocityY
	bs.Write(ship->GetServerVelocityX());
	bs.Write(ship->GetServerVelocityY());
#endif

	rakpeer_->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

}