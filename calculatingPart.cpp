#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>
#include <list>
#include <math.h>
#include <vector>
#include <iostream>
#include <random>
#include <string>
#include <utility>

#define ROCKET_DENSITY 2
#define ROCKET_FRICTION 0.5
#define ROCKET_RESTITUTION 0.9
#define ROCKET_A_DUMPING 0.7
#define ROCKET_L_DUMPING 0.01
#define ROCKET_ACCEL_FORCE 5
#define ROCKET_ROTATION_SPEED 0.8
#define ROCKET_ANIMATION_FRAME_STEP 5
#define ROCKET_INITIAL_LIFE 200
#define ROCKET_ANIMATION_SPRITES 8
#define ROCKET_BOMBTHROW_COOLDOWN_IN_FRAMES 100
#define ROCKET_THRUST_PARTICLES_COUNT 1
#define ROCKET_LIFEBAR_WIDTH 2
#define ROCKET_LIFEBAR_HEIGHT 0.15
#define ROCKET_THERMOISOLATION_COEFFICIENT 10
#define ROCKET_TOO_COOL_HEIGHT 100
#define ROCKET_BACKWARD_THRUST_MODIFIER 0.3
#define PLANET_RADIUS_IN_METERS 15    //Yep in meters
#define SAFE_HEIGHT_FOR_SPAWN 3
#define PLANET_DENSITY 100
#define PLANET_INTENSIVITY 1
#define BOMB_DENSITY 10
#define BOMB_FRICTION 1
#define BOMB_RESTITUTION 1
#define BOMB_A_DUMPING 0
#define BOMB_L_DUMPING 0.0
#define BOMB_RADIUS 0.3
#define BOMB_EXPLOSION_RANGE 6
#define BOMB_EXPLOSION_EFFECT_AMOUNT 5
#define BOMB_EXPLOSION_SPREAD 0.6
#define BOMB_DAMAGE 1
#define BOMB_INITIAL_LIFE 5
#define EXPLOSION_LIFETIME_IN_FRAMES 20
#define EXPLOSION_RAYCAST_STEP 360
#define PARTICLE_LINEAR_DAMPING 0.01
#define PARTICLE_LIFETIME_IN_FRAMES 50
#define PARTICLE_AVERAGE_SIZE 3
#define PARTICLE_ANGULAR_SPEED_MOD 10
#define PARTICLE_SPEED_MOD 1.0/60
#define BOA_IN_PARROTS 38
#define METER_IN_PIXELS 16.0
#define GRAVITY_CONSTANT_G 0.0001
#define CANNON_POS b2Vec2(-10/METER_IN_PIXELS,-12/METER_IN_PIXELS)
#define CANNON_RANGE 30
#define CANNON_MAX_SPREAD M_PI/9
#define CANNON_TRACE_COLOR sf::Color::Yellow
#define CANNON_COOLDOWN 0

sf::Sprite* compasSprite;
void floatRound(float &toRound,float rounder) //cheap and dirty; only positive
{
	while(toRound-rounder>0)
		toRound-=rounder;
}

float rad_to_deg(float rad)
	{
		float deg = 180*(rad/M_PI);
		while(deg>360)
			deg-=360;
		while(deg<0)
			deg+=360;
		return deg;
	}
b2Vec2 rotate(b2Vec2 & vector,float angleRadians)
{
	b2Vec2 result;
	float aCos=cosf(angleRadians);
	float aSin=sinf(angleRadians);
	result.x=vector.x*aCos-vector.y*aSin;
	result.y=vector.x*aSin+vector.y*aCos;
	return result;
}
bool run=false;
bool started=false;
void startgame(sf::RenderWindow* window)
{
	if(started)
		return;
	started=true;
	run=true;
}
namespace misc
{
	unsigned int id = 0;
	unsigned int give_id()
	{
		return id++;
	}
}
namespace keyboard
{
	class event
	{
		protected:
		bool active;
		public:
			unsigned int id;
			sf::Keyboard::Key trigger_key;
			std::function<void(void)> itself;
			event(sf::Keyboard::Key a,std::function<void(void)> lamda)
			{
				itself=lamda;
				id=misc::give_id();
				trigger_key=a;
				active=false;
			}
			void setstate(bool isactive)
			{
				active=isactive;
			}
			bool getstate()
			{
				return active;
			}
			void trigger()
			{
				if(active)
					itself();
			}
	};
	std::vector<std::list<event>> binded(102);
	event * bind_event(sf::Keyboard::Key a,std::function<void(void)> lamda)
	{
		return &(*binded[a].emplace(binded[a].end(),a,lamda));
	}
	void trigger_all()
	{
		for(int i = sf::Keyboard::A;i<=sf::Keyboard::Pause;i++)
		{
			if(sf::Keyboard::isKeyPressed((sf::Keyboard::Key)i))
			{
				for(event j : binded[i])
				{
					j.trigger();
				}
			}
		}
	}
}

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(0, 1);

namespace phys
{
	std::list<std::pair<b2Vec2,b2Vec2> > traces;
	void endgame(int n);
	b2World * world;
	sf::RenderWindow* window;
	class object;
	std::list<object*> worldObjects;
	std::vector<sf::Sprite*> rocketSpriteSheet(ROCKET_ANIMATION_SPRITES);
//Here are classes used in our game

	class object
	{
	public:
		virtual bool diediedie()=0;
		virtual void damage()=0;
		virtual void draw(sf::RenderTarget * target = window)=0;
		virtual void falseDamage()=0;
		virtual void drawSpec(sf::RenderTarget * target = window)=0;
		b2Body* body;
	};

class MyContactListener : public b2ContactListener
{
public:

	void BeginContact(b2Contact* contact)
	{

	}
	void EndContact(b2Contact* contact)
	{

	} 
	void PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
	{

	}
	void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
	{
		b2Fixture* fixA=contact->GetFixtureA();
		b2Body* bodyA=fixA->GetBody();
		b2Fixture* fixB=contact->GetFixtureB();
		b2Body* bodyB=fixB->GetBody();
		object* objA=(object*)bodyA->GetUserData();
		object* objB=(object*)bodyB->GetUserData();
		//objA->falseDamage();
		//objB->falseDamage();
		for(float i=impulse->normalImpulses[0]+impulse->normalImpulses[1]-1;i>0;i--)
		{
			objA->damage();
			objB->damage();;
		}
	}

} ;
std::list<b2Body*> bodiesToDestroy;
	class coloredParticle
	{
		sf::Color color;
		b2Vec2 position;
		b2Vec2 speed;
		float linear_damping;
		float angle,angularSpeed;
		int lifetime;
		bool destruct;
		sf::RectangleShape rect;
		public:
			bool isExpired()
			{
				return destruct;
			}
			coloredParticle(b2Vec2 pos,b2Vec2 velocity,sf::Color clr): color(clr),position(pos),speed(velocity),linear_damping(PARTICLE_LINEAR_DAMPING),lifetime(PARTICLE_LIFETIME_IN_FRAMES),destruct(false)
			{
				rect.setFillColor(color);
				float size=PARTICLE_AVERAGE_SIZE*(0.5+dis(gen));
				rect.setSize(sf::Vector2f(size,size));
				rect.setOrigin(sf::Vector2f(size/2,size/2));
				angle=dis(gen)*360;
				angularSpeed=dis(gen)*PARTICLE_ANGULAR_SPEED_MOD;
				speed*=PARTICLE_SPEED_MOD;
			}
			void tick(sf::RenderTarget * target = window)
			{
				lifetime--;
				if(lifetime<=0)
					destruct=true;
				position+=speed;
				//speed-=b2Vec2(speed.x*linear_damping,speed.y*linear_damping);
				rect.setPosition(position.x*METER_IN_PIXELS,position.y*METER_IN_PIXELS);
				angle+=angularSpeed;
				floatRound(angle,360);
				rect.setRotation(angle);
				target->draw(rect);
			}
			void draw(sf::RenderTarget * target = window)
			{
				target->draw(rect);
			}
	};
class explosion
{
	sf::Color color;
	b2Vec2 position;
	int lifetime;
	bool destruct;
	sf::CircleShape circ;
	public:
		bool isExpired()
		{
			return destruct;
		}
		explosion(b2Vec2 pos,bool coldOne=false) : position(pos),lifetime(EXPLOSION_LIFETIME_IN_FRAMES),destruct(false)
		{
			color.r=(dis(gen)*0.5+0.5)*255;
			color.g=dis(gen)*128;
			color.b=0;
			color.a=192;
			if(coldOne)
				color.b=color.r,color.r=0,color.a=16;
			circ.setFillColor(color);
			float size=PARTICLE_AVERAGE_SIZE*10*(0.5+dis(gen));
			circ.setRadius(size);
			circ.setOrigin(sf::Vector2f(size,size));
			circ.setPosition(position.x*METER_IN_PIXELS,position.y*METER_IN_PIXELS);
		}
		void tick(sf::RenderTarget * target = window)
		{
			lifetime--;
			if(lifetime<=0)
				destruct=true;
			//speed-=b2Vec2(speed.x*linear_damping,speed.y*linear_damping);
			target->draw(circ);
		}
		void draw(sf::RenderTarget * target = window)
		{
			target->draw(circ);
		}
	};
	std::list<coloredParticle*> particles;
	std::list<explosion*> explosions;
	void emitParticle(b2Vec2 pos,b2Vec2 velocity,sf::Color clr)
	{
		coloredParticle* ourParticle=new coloredParticle(pos,velocity,clr);
		particles.push_back(ourParticle);
	}

	void emitExplosion(b2Vec2 pos,bool coldOne=false)
	{
		explosion* ourExplosion=new explosion(pos,coldOne);
		explosions.push_back(ourExplosion);
	}

	class planet : public object
	{
		b2BodyDef bodydef;
		b2FixtureDef fixtureDef;
		b2CircleShape shape;
		float intensivity;
		public:
			void damage()
			{
				//nothing
			}
			void falseDamage() {};
			bool diediedie()
			{
				return false;
				//MUAHAHAHAHA IT'S UNSTOPPABLE
			}
			void draw(sf::RenderTarget * target = window)
			{
				sf::CircleShape cShape;
				cShape.setPosition(0,0);
				cShape.setRadius(shape.m_radius*METER_IN_PIXELS);
				cShape.setFillColor(sf::Color(64,64,64,255));
				cShape.setOrigin(shape.m_radius*METER_IN_PIXELS,shape.m_radius*METER_IN_PIXELS);
				target->draw(cShape);
			}
			void drawSpec(sf::RenderTarget * target = window)
			{
				sf::CircleShape cShape;
				cShape.setPosition(0,0);
				cShape.setRadius(shape.m_radius*METER_IN_PIXELS);
				cShape.setFillColor(sf::Color(64,64,64,255));
				cShape.setOrigin(shape.m_radius*METER_IN_PIXELS,shape.m_radius*METER_IN_PIXELS);
				target->draw(cShape);
			}
			planet(float radius,float density,float nintensivity): intensivity(nintensivity)
			{
				shape.m_p=b2Vec2(0,0);
				shape.m_radius=radius;
				fixtureDef.shape=&shape;
				fixtureDef.density=density;
				fixtureDef.restitution=0.3;
				fixtureDef.friction=0.5;
				bodydef.type=b2_staticBody;
			}
			void init()
			{
				body=world->CreateBody(&bodydef);
				body->SetUserData(this);
				body->CreateFixture(&fixtureDef);
				worldObjects.push_back(this);
			}
	};

	sf::CircleShape bombSprite;
	b2FixtureDef bombFixtureDef;
	b2BodyDef bombBodyDef;
	b2CircleShape bombshape;
	void initBomb()
	{
		bombshape.m_radius=BOMB_RADIUS;
		bombFixtureDef.shape=&bombshape;
		bombFixtureDef.density=BOMB_DENSITY;
		bombFixtureDef.restitution=BOMB_RESTITUTION;
		bombFixtureDef.friction=BOMB_FRICTION;
		bombBodyDef.angularDamping=BOMB_A_DUMPING;
		bombBodyDef.linearDamping=BOMB_L_DUMPING;
		bombBodyDef.type=b2_dynamicBody;
		bombSprite.setRadius(BOMB_RADIUS*METER_IN_PIXELS);
		bombSprite.setOrigin(BOMB_RADIUS*METER_IN_PIXELS,BOMB_RADIUS*METER_IN_PIXELS);
		bombSprite.setFillColor(sf::Color::Blue);
	}
	b2Vec2 pointOfRayCastCallback;
	class raycastCallback : public b2RayCastCallback
	{
		float32 ReportFixture(	b2Fixture* fixture, const b2Vec2& point,const b2Vec2& normal, float32 fraction)
		{
			pointOfRayCastCallback=point;
			//for(int i=0;i<BOMB_DAMAGE;i++)
			((object*)(fixture->GetBody()->GetUserData()))->damage();
			((object*)(fixture->GetBody()->GetUserData()))->falseDamage();
			return fraction;
		}
	};

	class bomb : public object
	{
		b2Vec2 speed;
		bool destruct;
		int life;
		public:
			bomb(b2Vec2 pos, b2Vec2 nSpeed): destruct(false)
			{
				bombBodyDef.position=pos;
				speed=nSpeed;
				life=BOMB_INITIAL_LIFE;
			}
			void init()
			{
				body=world->CreateBody(&bombBodyDef);
				body->CreateFixture(&bombFixtureDef);
				body->SetLinearVelocity(speed);
				body->SetUserData(this);
				worldObjects.push_back(this);
			}
			void draw(sf::RenderTarget * target = window)
			{
				bombSprite.setPosition(body->GetPosition().x*METER_IN_PIXELS,body->GetPosition().y*METER_IN_PIXELS);
				target->draw(bombSprite);
			}
			void drawSpec(sf::RenderTarget * target = window)
			{
				bombSprite.setPosition(body->GetPosition().x*METER_IN_PIXELS,body->GetPosition().y*METER_IN_PIXELS);
				target->draw(bombSprite);
			}
			void damage() 
			{
				life--;
				if(life<0)
					falseDamage();
			};
			void falseDamage()
			{
				if(destruct)
					return;
				b2Vec2 p2(0,BOMB_EXPLOSION_RANGE);
				b2Vec2 p1(0,-0.5*BOMB_RADIUS);
				destruct=true;
				b2Vec2 bodyPos=body->GetPosition();
				bodiesToDestroy.push_back(body);
				for(float i = 0; i<EXPLOSION_RAYCAST_STEP;i++)
				{
					p1=rotate(p1,2*M_PI/EXPLOSION_RAYCAST_STEP);
					p2=rotate(p2,2*M_PI/EXPLOSION_RAYCAST_STEP);
					world->RayCast(new raycastCallback,bodyPos+p1,bodyPos+p2);
				}
				for(int i=0;i<BOMB_EXPLOSION_EFFECT_AMOUNT;i++)
				{
					emitExplosion(b2Vec2(body->GetPosition().x+2*((dis(gen)-0.5)*BOMB_EXPLOSION_SPREAD),body->GetPosition().y+2*(dis(gen)-0.5)*BOMB_EXPLOSION_SPREAD));
				}
			}
			bool diediedie()
			{
				return destruct;
			}
	};
	planet* ourPlanet;
	
	class rocket : public object
	{
		b2BodyDef bodyDef;
		void selfDestruct(bool tooCold = false)
		{
			if(destruct)
				return;
			destruct=true;
			//bodiesToDestroy.push_back(body);
			for(int i=0;i<BOMB_EXPLOSION_EFFECT_AMOUNT;i++)
				emitExplosion(b2Vec2(body->GetPosition().x+2*((dis(gen)-0.5)*BOMB_EXPLOSION_SPREAD),body->GetPosition().y+2*(dis(gen)-0.5)*BOMB_EXPLOSION_SPREAD),tooCold);
			endgame(playerNumber);
		}
		int life;
		int curFrame,curSprite;
		bool accelerating;
		bool destruct;
		int cooldown,heatCooldown,cannonCooldown;
		void falseDamage() {};
		std::function<void(sf::Color&)> thrustColorGenerator;
	public:
		bool compas;
		int playerNumber;
		int getLife()
		{
			return life;
		}
		bool diediedie()
		{
			return destruct;
		}
		rocket(int nLife,b2Vec2 coords, float angle,std::function<void(sf::Color&)> thrustColorGen=[](sf::Color& color){
			color.r=(dis(gen)*0.5+0.5)*255;
			color.g=dis(gen)*128;
			color.b=0;
			color.a=192;
		}) : life(nLife),curFrame(0),accelerating(false),destruct(false),cooldown(0),thrustColorGenerator(thrustColorGen)
		{
			bodyDef.type= b2_dynamicBody;
			bodyDef.position=coords;
			bodyDef.angle=angle;
			bodyDef.angularDamping=ROCKET_A_DUMPING;
			bodyDef.linearDamping=ROCKET_L_DUMPING;
			heatCooldown=0;
			cannonCooldown=0;
		}
		void damage()
		{
			life--;
			if(life<=0)
				selfDestruct();
		}
		void init(b2FixtureDef* fixtureDef)
		{
			bodyDef.userData=this;
			body=world->CreateBody(&bodyDef);
			body->CreateFixture(fixtureDef);
			worldObjects.push_back(this);
		}
		void accelerateLinear(bool backwards = false)
		{
			float angle=body->GetAngle();
			float modifier = 1;
			if(curSprite<0)
				modifier=0.2;
			b2Vec2 force = b2Vec2(0,-ROCKET_ACCEL_FORCE*modifier);
			if(backwards)
				force=b2Vec2(0,ROCKET_ACCEL_FORCE*ROCKET_BACKWARD_THRUST_MODIFIER);
			force=rotate(force,angle);
			body->ApplyForceToCenter(force,true);
			if(!backwards)
			{
				accelerating=true;
				thrust();	
			}
		}
		void accelerateAngular(bool ccw)
		{
			body->SetAngularVelocity(ROCKET_ROTATION_SPEED*(1-2*ccw));
		}
		void thrust()
		{
			b2Vec2 pos(0,1);
			b2Vec2 position=body->GetPosition()+(rotate(pos,(body->GetAngle())));
			b2Vec2 velocity=body->GetLinearVelocity()+(rotate(pos,(body->GetAngle()+M_PI*(0.5-dis(gen))*0.7)));
			sf::Color color;
			color.r=(dis(gen)*0.5+0.5)*255;
			color.g=dis(gen)*128;
			color.b=0;
			color.a=192;
			for(int i=0;i<ROCKET_THRUST_PARTICLES_COUNT;i++)
				emitParticle(position,velocity,color);
		}
		void draw(sf::RenderTarget * target = window)
		{
			//body->SetAngularVelocity(0);
			curFrame++;
			if((curFrame>ROCKET_ANIMATION_FRAME_STEP&&curSprite>=0)||(curSprite<0&&curFrame>ROCKET_ANIMATION_FRAME_STEP*2))
			{
				curSprite=(curSprite+1)%(ROCKET_ANIMATION_SPRITES-4);
				curFrame=0;
			}
			if(!accelerating)
				curSprite=-4,curFrame=ROCKET_ANIMATION_FRAME_STEP-1;
			accelerating=false;
			rocketSpriteSheet[curSprite+4]->setPosition(body->GetPosition().x*METER_IN_PIXELS,body->GetPosition().y*METER_IN_PIXELS);
			rocketSpriteSheet[curSprite+4]->setRotation(rad_to_deg(body->GetAngle()));
			target->draw(*rocketSpriteSheet[curSprite+4]);
			if(cooldown>0)
				cooldown--;
			if(heatCooldown>0)
				heatCooldown--;
			if(heatCooldown==0)
			{
				b2Vec2 pos = body->GetPosition();
				pos-=ourPlanet->body->GetPosition();
				if(pos.LengthSquared()>=ROCKET_TOO_COOL_HEIGHT*ROCKET_TOO_COOL_HEIGHT)
					life--,heatCooldown=ROCKET_THERMOISOLATION_COEFFICIENT;
			}
			if(cannonCooldown>0)
				cannonCooldown--;
			if(life<0)
				selfDestruct(true);
		}
		void drawSpec(sf::RenderTarget * target = window)
		{
			//body->SetAngularVelocity(0);
			rocketSpriteSheet[curSprite+4]->setPosition(body->GetPosition().x*METER_IN_PIXELS,body->GetPosition().y*METER_IN_PIXELS);
			rocketSpriteSheet[curSprite+4]->setRotation(rad_to_deg(body->GetAngle()));
			compasSprite->setPosition(body->GetPosition().x*METER_IN_PIXELS,body->GetPosition().y*METER_IN_PIXELS);
			compasSprite->setRotation(-rad_to_deg(std::atan2((body->GetPosition()-ourPlanet->body->GetPosition()).x,(body->GetPosition()-ourPlanet->body->GetPosition()).y)));
			target->draw(*rocketSpriteSheet[curSprite+4]);
			if(compas)
				target->draw(*compasSprite);
		}
		void throwBomb()
		{
			if(cooldown>0)
				return;
			b2Vec2 pos = b2Vec2(0,2);
			bomb* ourBomb = new bomb(body->GetPosition()+(rotate(pos,(body->GetAngle()))),body->GetLinearVelocity());
			ourBomb->init();
			cooldown=ROCKET_BOMBTHROW_COOLDOWN_IN_FRAMES;
		}
		void shoot()
		{
			if(cannonCooldown>0)
				return;
			cannonCooldown=CANNON_COOLDOWN;
			b2Vec2 p1=CANNON_POS;
			p1=rotate(p1,body->GetAngle());
			p1+=body->GetPosition();
			b2Vec2 p2=b2Vec2(0,-CANNON_RANGE);
			p2=rotate(p2,body->GetAngle()+(dis(gen)-0.5)*CANNON_MAX_SPREAD);
			pointOfRayCastCallback=p1+p2;
			world->RayCast(new raycastCallback,p1,p1+p2);
			traces.push_back(std::make_pair(p1,pointOfRayCastCallback));
			if(pointOfRayCastCallback.x!=(p1+p2).x&&pointOfRayCastCallback.y!=(p1+p2).y)
			{
				sf::Color color;
				color.r=(dis(gen)*0.5+0.5)*255;
				color.g=dis(gen)*128;
				color.b=0;
				color.a=192;
				emitParticle(pointOfRayCastCallback,b2Vec2(0,0), color);
			}
		}
	};
	rocket* ourRocket;
	rocket* ourRocket2;
	void init(sf::RenderWindow * rWindow)
	{
		initBomb();
		world = new b2World(b2Vec2(0,0));
		world->SetContactListener(new MyContactListener());
		window=rWindow;
		//here we define our rocket's properties
		b2FixtureDef rocketF;
		rocketF.density=ROCKET_DENSITY;
		rocketF.friction=ROCKET_FRICTION;
		rocketF.restitution=ROCKET_RESTITUTION;
		b2PolygonShape shape;
		b2Vec2 arr[8] ={b2Vec2(-3/METER_IN_PIXELS,-16/METER_IN_PIXELS),b2Vec2(-11/METER_IN_PIXELS,-12/METER_IN_PIXELS),b2Vec2(-11/METER_IN_PIXELS,14/METER_IN_PIXELS),b2Vec2(-6/METER_IN_PIXELS,18/METER_IN_PIXELS),b2Vec2(7/METER_IN_PIXELS,18/METER_IN_PIXELS),b2Vec2(11/METER_IN_PIXELS,14/METER_IN_PIXELS),b2Vec2(6/METER_IN_PIXELS,-13/METER_IN_PIXELS),b2Vec2(3/METER_IN_PIXELS,-16/METER_IN_PIXELS)}; 
		shape.Set(arr, 8);
		rocketF.shape = &shape;
		//here we make rocket and spawn it in  of world
		ourRocket = new rocket(ROCKET_INITIAL_LIFE,b2Vec2(0,-PLANET_RADIUS_IN_METERS-SAFE_HEIGHT_FOR_SPAWN),0);
		ourRocket->init(&rocketF);
		ourRocket2 = new rocket(ROCKET_INITIAL_LIFE,b2Vec2(0,PLANET_RADIUS_IN_METERS+SAFE_HEIGHT_FOR_SPAWN),M_PI,[](sf::Color& color)
			{
				float clr =(dis(gen)*0.25+0.75)*255; 
				color.r=0;
				color.g=0;
				color.b=clr;
				color.a=192;
			});
		ourRocket2->init(&rocketF);
		ourRocket->playerNumber=1;
		ourRocket2->playerNumber=2;
		//here we fill universe with rocket's sprites
		//yep,like in declaring shape of rocket, this is big amount of code, but in my previous projects I
		// implemented universal loader of bodies(events from it too),but it too complex for this small game
		// and costs less memory to do
		/*
		sf::Texture* universalTexturePointer = new sf::Texture();
		universalTexturePointer->loadFromFile("./firstAttempt0.png");
		sf::Sprite* universalSpritePointer = new sf::Sprite();
		universalSpritePointer->setTexture(*universalTexturePointer);
		universalSpritePointer->setOrigin(16,16);
		rocketSpriteSheet[0]=universalSpritePointer;
		universalTexturePointer = new sf::Texture();
		universalTexturePointer->loadFromFile("./firstAttempt1.png");
		universalSpritePointer = new sf::Sprite();
		universalSpritePointer->setTexture(*universalTexturePointer);
		universalSpritePointer->setOrigin(16,16);
		rocketSpriteSheet[1]=universalSpritePointer;
		rocketSpriteSheet[3]=universalSpritePointer;
		universalTexturePointer = new sf::Texture();
		universalTexturePointer->loadFromFile("./firstAttempt2.png");
		universalSpritePointer = new sf::Sprite();
		universalSpritePointer->setTexture(*universalTexturePointer);
		universalSpritePointer->setOrigin(16,16);
		rocketSpriteSheet[2]=universalSpritePointer;
		rocketSpriteSheet[4]=universalSpritePointer;
		universalTexturePointer = new sf::Texture();
		universalTexturePointer->loadFromFile("./firstAttempt3.png");
		universalSpritePointer = new sf::Sprite();
		universalSpritePointer->setTexture(*universalTexturePointer);
		universalSpritePointer->setOrigin(16,16);
		rocketSpriteSheet[5]=universalSpritePointer;
		universalTexturePointer = new sf::Texture();
		universalTexturePointer->loadFromFile("./firstAttempt4.png");
		universalSpritePointer = new sf::Sprite();
		universalSpritePointer->setTexture(*universalTexturePointer);
		universalSpritePointer->setOrigin(16,16);
		rocketSpriteSheet[6]=universalSpritePointer;
		universalTexturePointer = new sf::Texture();
		universalTexturePointer->loadFromFile("./firstAttempt5.png");
		universalSpritePointer = new sf::Sprite();
		universalSpritePointer->setTexture(*universalTexturePointer);
		universalSpritePointer->setOrigin(16,16);
		rocketSpriteSheet[7]=universalSpritePointer;
		universalTexturePointer=new sf::Texture();
		universalTexturePointer->loadFromFile("./compas.png");
		universalSpritePointer=new sf::Sprite();
		universalSpritePointer->setTexture(*universalTexturePointer);
		universalSpritePointer->setOrigin(16,16);
		universalSpritePointer->scale(sf::Vector2f(2,2));
		compasSprite=universalSpritePointer;
		*/
		//Now we load single texture for performance purpose
		sf::Texture* universalTexturePointer = new sf::Texture;
		universalTexturePointer->loadFromFile("joined.png");
		sf::Sprite* universalSpritePointer;
		for(int i=0;i<ROCKET_ANIMATION_SPRITES;i++)
		{
			int iM=i;
			if(i>=3)
				iM-=2;
			universalSpritePointer = new sf::Sprite;
			universalSpritePointer->setTexture(*universalTexturePointer);
			universalSpritePointer->setTextureRect(sf::IntRect(iM*32,0,32,48));
			universalSpritePointer->setOrigin(16,16);
			rocketSpriteSheet[i]=universalSpritePointer;
		}
		universalSpritePointer=new sf::Sprite();
		universalSpritePointer->setTexture(*universalTexturePointer);
		universalSpritePointer->setTextureRect(sf::IntRect(0,48,32,32));
		universalSpritePointer->setOrigin(16,16);
		universalSpritePointer->scale(sf::Vector2f(2,2));
		compasSprite=universalSpritePointer;
		//done
		//and now we use keyboard events system(from my previous projects)
		//why? because we CAN
		keyboard::bind_event(sf::Keyboard::Up,[](){ourRocket->accelerateLinear();})->setstate(true);
		keyboard::bind_event(sf::Keyboard::Down,[](){ourRocket->accelerateLinear(true);})->setstate(true);
		keyboard::bind_event(sf::Keyboard::Left, [](){ourRocket->accelerateAngular(true);})->setstate(true);
		keyboard::bind_event(sf::Keyboard::Right,[](){ourRocket->accelerateAngular(false);})->setstate(true);
		//and now we bind also to wasd
		//why? because it more comfortable than arrow keys
		keyboard::bind_event(sf::Keyboard::W,[](){ourRocket2->accelerateLinear();})->setstate(true);
		keyboard::bind_event(sf::Keyboard::S,[](){ourRocket2->accelerateLinear(true);})->setstate(true);
		keyboard::bind_event(sf::Keyboard::A, [](){ourRocket2->accelerateAngular(true);})->setstate(true);
		keyboard::bind_event(sf::Keyboard::D,[](){ourRocket2->accelerateAngular(false);})->setstate(true);
		//Bombs
		keyboard::bind_event(sf::Keyboard::F,[](){ourRocket2->throwBomb();})->setstate(true);
		keyboard::bind_event(sf::Keyboard::K,[](){ourRocket->throwBomb();})->setstate(true);
		//Gatlings
		keyboard::bind_event(sf::Keyboard::L,[](){ourRocket->shoot();})->setstate(true);
		keyboard::bind_event(sf::Keyboard::G,[](){ourRocket2->shoot();})->setstate(true);
		//Compas
		keyboard::bind_event(sf::Keyboard::H,[](){ourRocket2->compas=true;})->setstate(true);
		keyboard::bind_event(sf::Keyboard::SemiColon,[](){ourRocket->compas=true;})->setstate(true);
		//now we make planet
		ourPlanet = new planet(PLANET_RADIUS_IN_METERS,PLANET_DENSITY,PLANET_INTENSIVITY);
		ourPlanet->init();
	}
	bool predicate(object* value)
	{
		return value->diediedie();
	}
	bool particlePredicate(coloredParticle* value)
	{
		return value->isExpired();
	}
	bool explosionPredicate(explosion* value)
	{
		return value->isExpired();
	}
	bool fw=false,sw=false;
	void tick()
	{
		for(auto i : worldObjects)
		{
			i->draw();
			b2Vec2 vectorToPlanet=(i->body->GetPosition()-ourPlanet->body->GetPosition());
			float distanceSquared=vectorToPlanet.LengthSquared();
			vectorToPlanet.Normalize();
			if(distanceSquared==0)
				continue;
			if(distanceSquared>500*500/(METER_IN_PIXELS*METER_IN_PIXELS))
				i->body->ApplyForceToCenter(-1*(4*GRAVITY_CONSTANT_G*i->body->GetMass()*PLANET_DENSITY*pow(ourPlanet->body->GetFixtureList()->GetShape()->m_radius,3)*4*M_PI/3/distanceSquared)*vectorToPlanet,true);	
			if(distanceSquared>800*800/(METER_IN_PIXELS*METER_IN_PIXELS))
				i->body->ApplyForceToCenter(-1*(8*GRAVITY_CONSTANT_G*i->body->GetMass()*PLANET_DENSITY*pow(ourPlanet->body->GetFixtureList()->GetShape()->m_radius,3)*4*M_PI/3/distanceSquared)*vectorToPlanet,true);	
			if(distanceSquared>1600*1600/(METER_IN_PIXELS*METER_IN_PIXELS))
				i->body->ApplyForceToCenter(-1*(16*GRAVITY_CONSTANT_G*i->body->GetMass()*PLANET_DENSITY*pow(ourPlanet->body->GetFixtureList()->GetShape()->m_radius,3)*4*M_PI/3/distanceSquared)*vectorToPlanet,true);	
			i->body->ApplyForceToCenter(-1*(GRAVITY_CONSTANT_G*i->body->GetMass()*PLANET_DENSITY*pow(ourPlanet->body->GetFixtureList()->GetShape()->m_radius,3)*4*M_PI/3/distanceSquared)*vectorToPlanet,true);
		}
		for(auto i : particles)
		{
			i->tick();
		}
		particles.remove_if(particlePredicate);
		for(auto i : explosions)
			i->tick();
		explosions.remove_if(explosionPredicate);
		if(sw||fw)
		{
			run=false;
			if(fw xor sw)
				std::cout<<"Player "<<(fw?1:2)<<" wins!\n";
			else
				std::cout<<"Draw,motherfuckers!\n";
		}
		sf::VertexArray vertArr;
		vertArr.setPrimitiveType(sf::Lines);
		sf::Vertex vertex;
		vertex.color=CANNON_TRACE_COLOR;
		for(std::pair<b2Vec2,b2Vec2> i:traces)
		{
			vertArr.clear();
			vertex.position=sf::Vector2f(i.first.x*METER_IN_PIXELS,i.first.y*METER_IN_PIXELS);
			vertArr.append(vertex);
			vertex.position=sf::Vector2f(i.second.x*METER_IN_PIXELS,i.second.y*METER_IN_PIXELS);
			vertArr.append(vertex);
			window->draw(vertArr);
		}
	}
	std::string textText="Player ";
	sf::Text text;
	bool gameEnded=false;
	void endgame(int n)
	{
		if(n==1)
			sw=true;
		if(n==2)
			fw=true;
		//gameEnded=true;
		//run=false;
		//std::cout<<"Player "<<3-n<<" wins!\n";
		/*textText+=(3-n);
		textText+=" wins!\n";
		text.setString(textText);
		text.setColor(sf::Color::Magenta);
		text.setPosition(0,0);
		text.setCharacterSize(40);
		window->draw(text);*/
	}
	void simplyDraw()
	{
		for(auto i: worldObjects)
			i->drawSpec();
		for(auto i: particles)
			i->draw();
		for(auto i: explosions)
			i->draw();
		sf::VertexArray vertArr;
		vertArr.setPrimitiveType(sf::Lines);
		sf::Vertex vertex;
		vertex.color=CANNON_TRACE_COLOR;
		for(std::pair<b2Vec2,b2Vec2> i:traces)
		{
			vertArr.clear();
			vertex.position=sf::Vector2f(i.first.x*METER_IN_PIXELS,i.first.y*METER_IN_PIXELS);
			vertArr.append(vertex);
			vertex.position=sf::Vector2f(i.second.x*METER_IN_PIXELS,i.second.y*METER_IN_PIXELS);
			vertArr.append(vertex);
			window->draw(vertArr);
		}
	}
	void deInit()
	{

	}
}
