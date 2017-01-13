#ifndef _SHIP_H_
#define _SHIP_H_

#include <hge.h>
#include <hgerect.h>
#include <memory>
#include <string>

class hgeSprite;
class hgeFont;

//#define INTERPOLATEMOVEMENT 
#define EXTRAPOLATEMOVEMENT

/**
* The Ship class represents a single spaceship floating in space. It obeys
* 2D physics in terms of displacement, velocity and acceleration, as well
* as angular position and displacement. The size of the current art is
* 128*128 pixels
*/

class Ship
{
	HTEXTURE tex_; //!< Handle to the sprite's texture
	std::auto_ptr<hgeSprite> sprite_; //!< The sprite used to display the ship
	std::auto_ptr<hgeFont> font_;
	hgeRect collidebox;

	std::string mytext_;
	float x_; //!< The x-ordinate of the ship
	float y_; //!< The y-ordinate of the ship
	float w_; //!< The angular position of the ship
	float velocity_x_; //!< The resolved velocity of the ship along the x-axis
	float velocity_y_; //!< The resolved velocity of the ship along the y-axis

	float oldx, oldy;	// for reset back to previous location if collision detected

	// Lab 13 Task 2 : add for interpolation
#ifdef INTERPOLATEMOVEMENT
	// step 1 : add new variables
	
#endif

	unsigned int id;
	int type_;
	float angular_velocity;

	unsigned int collidetimer;
public:

	Ship(int type, float locx_, float locy_);
	~Ship();
	void Update(float timedelta);
	void Render();
	void Accelerate(float acceleration, float timedelta);
	void SetName(const char * text);
	
	hgeRect* GetBoundingBox();
	bool HasCollided( Ship *ship );

	float GetVelocityX() { return velocity_x_; }
	float GetVelocityY() { return velocity_y_; }

	void SetVelocityX( float velocity ) { velocity_x_ = velocity; }
	void SetVelocityY( float velocity ) { velocity_y_ = velocity; }

	float GetAngularVelocity() { return angular_velocity; }

	void SetAngularVelocity( float av ) { angular_velocity = av; }

	void SetPreviousLocation()
	{
		x_ = oldx;
		y_ = oldy;
	}

	unsigned int GetID() { return id; }

	void setID(unsigned int newid ) { id = newid; }

	void setLocation( float x, float y, float w ) 
	{ 
		x_ = x; 
		y_ = y; 
		w_ = w; 
	}

	void SetX( float x ) { x_ = x; }
	void SetY( float y ) { y_ = y; }

	float GetX() { return x_; }
	float GetY() { return y_; }
	float GetW() { return w_; }

	int GetType() { return type_; }

	bool CanCollide( unsigned int timer ) 
	{
		if( timer - collidetimer > 2000 )
		{
			collidetimer = timer;

			return true;
		}

		return false;
	}

	// Lab 13 Task 2 : Interpolation
#ifdef INTERPOLATEMOVEMENT
	// step 2: add new member functions here
	
#endif

};

#endif