#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>
#include "./calculatingPart.h"

float unNegativizer(float in)
{
	if(in<0)
		in=0;
	return in;
}

int main()
{
	sf::RenderWindow window(sf::VideoMode(700,700),"daGame",sf::Style::Titlebar|sf::Style::Close);
	phys::init(&window);
	sf::View view;
	view.setCenter(0,0);
	view.setViewport(sf::FloatRect(0,0,1,1));
	view.zoom(2);
	sf::View viewRocket;
	sf::View viewRocket2;
	sf::View viewRocketBorders;
	sf::View viewRocket2Borders;
	viewRocket.setViewport(sf::FloatRect(0,0,0.2,0.2));
	viewRocket2.setViewport(sf::FloatRect(0.8,0,0.2,0.2));
	viewRocketBorders.setViewport(sf::FloatRect(0,0,0.205,0.21));
	viewRocket2Borders.setViewport(sf::FloatRect(0.795,0,0.205,0.21));
	viewRocketBorders.zoom(0.1);
	viewRocket2Borders.zoom(0.1);
	viewRocket.zoom(0.1);
	viewRocket2.zoom(0.1);
	viewRocketBorders.setCenter(0,0);
	viewRocket2Borders.setCenter(0,0);
	sf::View lifebarView1;
	sf::View lifebarView2;
	lifebarView1.setViewport(sf::FloatRect(0.205,0,0.295,0.2));
	lifebarView2.setViewport(sf::FloatRect(0.5,0,0.295,0.2));
	lifebarView1.setCenter(0,0);
	lifebarView2.setCenter(0,0);
	lifebarView1.zoom(0.1);
	lifebarView2.zoom(0.1);
	lifebarView2.rotate(180);
	window.setView(view);
	float timestep=1.0/60.0;
    int velocityIterations = 6;
    int positionIterations = 2;
    run=true;
    bool flag=true;
	while(window.isOpen())
	{
		sf::Event event;
		while(window.pollEvent(event))
		{
			if(event.type==sf::Event::Closed)
				window.close();
			else if(event.type==sf::Event::KeyPressed)
			{
				if(event.key.code==sf::Keyboard::Escape)
					window.close();
			}
		}
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)&&sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			startgame(&window);
		if(!run)
			continue;
		window.clear();
		window.setView(view);
		keyboard::trigger_all();
		phys::world->Step(timestep,velocityIterations,positionIterations);
		phys::world->ClearForces();
		phys::tick();
		{
			sf::RectangleShape rect;
			rect.setSize(sf::Vector2f(1000,1000));
			rect.setOrigin(500,500);
			rect.setPosition(0,0);
			rect.setFillColor(sf::Color(177,177,177,255));
			window.setView(viewRocketBorders);
			window.draw(rect);
			window.setView(viewRocket2Borders);
			window.draw(rect);
			rect.setFillColor(sf::Color::Black);
			viewRocket.setCenter(phys::ourRocket->body->GetPosition().x*METER_IN_PIXELS,phys::ourRocket->body->GetPosition().y*METER_IN_PIXELS);
			rect.setPosition(phys::ourRocket->body->GetPosition().x*METER_IN_PIXELS,phys::ourRocket->body->GetPosition().y*METER_IN_PIXELS);
			window.setView(viewRocket);
			window.draw(rect);
			phys::simplyDraw();
			viewRocket2.setCenter(phys::ourRocket2->body->GetPosition().x*METER_IN_PIXELS,phys::ourRocket2->body->GetPosition().y*METER_IN_PIXELS);
			rect.setPosition(phys::ourRocket2->body->GetPosition().x*METER_IN_PIXELS,phys::ourRocket2->body->GetPosition().y*METER_IN_PIXELS);
			window.setView(viewRocket2);
			window.draw(rect);
			phys::simplyDraw();
			window.setView(lifebarView1);
			sf::RectangleShape supRect;
			supRect.setFillColor(sf::Color(64,64,64,255));
			supRect.setOutlineThickness(2);
			supRect.setOutlineColor(sf::Color::White);
			supRect.setScale(sf::Vector2f(0.2,0.295));
			supRect.setSize(sf::Vector2f(20*METER_IN_PIXELS,4*METER_IN_PIXELS));
			supRect.setOrigin(sf::Vector2f(10*METER_IN_PIXELS,supRect.getSize().y/2));
			supRect.setPosition(0,-2*METER_IN_PIXELS);
			rect.setFillColor(sf::Color::Red);
			rect.setScale(sf::Vector2f(0.2,0.295));
			rect.setSize(sf::Vector2f(unNegativizer(20*METER_IN_PIXELS*(phys::ourRocket->getLife()*1.0/ROCKET_INITIAL_LIFE)),4*METER_IN_PIXELS));
			rect.setOrigin(sf::Vector2f(10*METER_IN_PIXELS,rect.getSize().y/2));
			rect.setPosition(0,-2*METER_IN_PIXELS);
			window.draw(supRect);
			window.draw(rect);
			window.setView(lifebarView2);
			rect.setSize(sf::Vector2f(unNegativizer(20*METER_IN_PIXELS*(phys::ourRocket2->getLife()*1.0/ROCKET_INITIAL_LIFE)),4*METER_IN_PIXELS));
			rect.setOrigin(sf::Vector2f(10*METER_IN_PIXELS,supRect.getSize().y/2));
			rect.setPosition(0,2*METER_IN_PIXELS);
			supRect.setPosition(0,2*METER_IN_PIXELS);
			window.draw(supRect);
			window.draw(rect);

		}
		//std::cout<<"here\n";
		//here drawing code and physics's time advance
		if(phys::gameEnded)
		{
			window.setView(view);
			window.draw(phys::text);
		}
		phys::worldObjects.remove_if(phys::predicate);
		for(b2Body* i : phys::bodiesToDestroy)
		{
			delete (phys::object*)i->GetUserData();
			phys::world->DestroyBody(i);
		}
		phys::bodiesToDestroy.clear();
		phys::traces.clear();
		window.display();
		phys::ourRocket->compas=false;
		phys::ourRocket2->compas=false;
		if(flag)
			run=false,flag=false;
	}
}
