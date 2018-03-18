// CrazyTanks.cpp: определяет точку входа для консольного приложения.
//
#include "stdafx.h"
#include <iostream>
#include <cstdlib>
#include <windows.h>
#include <time.h>
#include <fstream>
#include <conio.h>
#include <vector>
#include <list>
#include <string>

#include <thread>
#include <mutex>

using namespace std;

const int
	HEIGHT	= 20, 
	WIDTH	= 60;
const char
	WALL		= '#',
	BORDER		= '#',
	FIELD		= ' ',
	TANKS		= '%',
	TANK		= '@',
	WHIZBANGS	= '+',
	WHIZBANG	= '*';

enum Direction { UP, LEFT, RIGHT, DOWN, STOP };


class Time {
public:
	int hour;
	int minute;
	int second;
	Time( int hour, int minute, int second ) : hour( hour ), minute( minute ), second( second ) {}
	void clock( int seconds ) {
		hour = seconds / 3600;
		minute = ( seconds - ( hour * 3600 ) ) / 60;
		second = seconds - ( hour * 3600 ) - ( minute * 60 );
	}
};
class Point {
public:

	int x;
	int y;
	Direction dir;
	Point() {
		dir = STOP;
	}

	Point( int height, int width ) : x( width ), y( height ), dir( STOP ) {
	}
	bool operator == ( const Point &point ) {
		return ( x == point.x && y == point.y );
	}
	bool isVailid() {
		return x > 0 && y > 0;
	}
	bool hasIn( list<Point> &list ) const {
		for ( auto item : list )
		{
			if ( this->y == item.y && this->x == item.x ) {
				return true;
			}
		}
		return false;
	}
	bool isBorder( Point height, Point width ) {
		return ( x == height.y || y == width.y || x == height.x || y == width.x );
	}
	Point moveCursor()
	{
		COORD coord;
		coord.X = x;
		coord.Y = y;
		SetConsoleCursorPosition( GetStdHandle( STD_OUTPUT_HANDLE ), coord );
		return Point( y, x );
	}
	char getChar()
	{
		char c = '\0';
		CONSOLE_SCREEN_BUFFER_INFO con;
		HANDLE hcon = GetStdHandle( STD_OUTPUT_HANDLE );
		moveCursor();
		if ( hcon != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo( hcon, &con ) )
		{
			DWORD read = 0;
			if ( !ReadConsoleOutputCharacterA( hcon, &c, 1, con.dwCursorPosition, &read ) || read != 1 ) {
				c = '\0';
			}

		}
		return c;
	}
	Point setChar( char sym ) {
		moveCursor();
		cout << sym;
		return Point( y, x );
	}
	void randPoint( Point height, Point width ) {
		x = rand() % ( width.x - 1 );
		y = rand() % ( height.x - 1 );
		while ( !( isVailid() && !isBorder( height, width ) && x > width.y && y > height.y ) )
		{
			x = rand() % ( width.x - 1 );
			y = rand() % ( height.x - 1 );
		}

	}
	Point move( Direction dir ) {
		switch ( dir )
		{
		case UP:
			return Point( y - 1, x );
		case LEFT:
			return Point( y, x - 1 );
		case RIGHT:
			return Point( y, x + 1 );
		case DOWN:
			return Point( y + 1, x );
		}
		return Point( y, x );
	}


};
class Map {


public:
	const int COUNT_POINT = 30;
	const int MAX_LENGTH = 5;
	list<Point> walls;
	Map( Point height, Point width ) {
		_height = height;
		_width = width;
	}
	void build() {

		srand( time( NULL ) );

		for ( int i = 0; i < COUNT_POINT; i++ )
		{
			auto points = Point( 0, 0 );

			points.randPoint( _height, _width );

			walls.push_back( points );

			createPath( points );
		}
	}

private:
	Point _height;
	Point _width;
	void createPath( Point points )
	{
		int lengthWall = rand() % MAX_LENGTH;
		for ( int k = 0; k < lengthWall; k++ )
		{
			int direction = rand() % 3, point;
			switch ( direction )
			{
			case 0:
				point = points.y - 1;
				if ( point <= _height.y ) {
					--k;
					continue;
				}
				else {
					points.y = point;
				}
				break;
			case 1:
				point = points.x - 1;
				if ( point <= _width.y ) {
					--k;
					continue;
				}
				else {
					points.x = point;
				}
				break;
			case 2:
				point = points.x + 1;
				if ( point >= _height.x ) {
					--k;
					continue;
				}
				else {
					points.x = point;
				}
				break;
			case 3:
				point = points.y + 1;
				if ( points.y >= _width.x ) {
					--k;
					continue;
				}
				else {
					points.y = point;
				}
				break;
			}
			if ( !points.hasIn( walls ) ) {
				walls.push_back( points );
			}
			else {
				continue;
			}


		}
	}
};



bool gameOver;
int score = 0, health = 3, countTank = 5;
Time timeg = Time( 0, 0, 0 );
clock_t startGame, restartGame;

Point
polygone_h = Point( 5, ( HEIGHT - 5 ) ),
polygone_w = Point( 0, WIDTH ),
boss = Point( HEIGHT / 2 + 3, WIDTH / 2 );

vector<Point>
tanks = vector<Point>( 5 ),
whizbangs = vector<Point>( 50 ),
whizbang = vector<Point>( 50 );

void draw();
void logic( mutex &mtx );
void events( mutex &mtx );
void shoot( mutex &mtx );
void tick( time_t begin );

bool canMoveObj( Point &points, Direction dir );
Point moveObj( Point &points, Direction dir, char obj );
void moveShoot( vector<Point> &shoots, char type );
bool isDie( Point &points, char type );

int main()
{
	_COORD coord;
	coord.X = WIDTH;
	coord.Y = HEIGHT;

	_SMALL_RECT Rect;
	Rect.Top = 0;
	Rect.Left = 0;
	Rect.Bottom = coord.Y + 1;
	Rect.Right = coord.X + 1;

	HANDLE Handle = GetStdHandle( STD_OUTPUT_HANDLE );      // get handle
	SetConsoleScreenBufferSize( Handle, coord );            // set buffer size
	SetConsoleWindowInfo( Handle, TRUE, &Rect );            // set window size

	//draw map and tanks
	draw();
	//create thread for: moving tanks, moving main tank and for bullets
	mutex mtx;
	thread t1( logic, ref( mtx ) );
	thread t2( events, ref( mtx ) );
	thread t3( shoot, ref( mtx ) );
	t1.detach();
	t3.detach();
	t2.join();
	//clean all and print result
	system( "cls" );
	Point( HEIGHT / 2 , WIDTH / 2 - 10 ).moveCursor();
	if ( score == 50 ) {
		cout << "Victory !!!";
	}
	else {
		cout << "Game over :( ";
	}
	Point( HEIGHT / 2 + 1, WIDTH / 2 - 15 ).moveCursor();
	cout << "Press escape to close ...";
	
	while ( _getch() != 27 ) {
	}
	return 0;
}
void drawBorder( Point height, Point width ) {
	for ( int i = height.y; i < height.x; i++ )
	{
		for ( int k = width.y; k < width.x; k++ ) {
			if ( i == 0 || i == height.x - 1 ) {
				Point( i, k ).setChar( BORDER );
			}
			else
				if ( k == 0 && i != 0 || k == width.x - 1 && i != height.x - 1 ) {
					Point( i, k ).setChar( BORDER );
				}
				else {
					Point( i, k ).setChar( FIELD );
				}
		}
	}
}
void draw() {
	system( "cls" );
	Map map = Map(
		polygone_h,
		polygone_w
	);
	//draw border of header
	drawBorder(
		Point( 0, 5 ),
		Point( 0, WIDTH )
	);
	//draw main border
	drawBorder( polygone_h, polygone_w );

	//build map
	map.build();
	for ( Point points : map.walls )
	{
		points.setChar( WALL );
	}
	// fill head
	auto points = Point( 2, 2 );
	points.moveCursor();
	cout << "health: " << health;
	points.x = WIDTH / 2 - 10;
	points.moveCursor();
	cout << "score: " << score;
	points.x = WIDTH - 25;
	points.moveCursor();
	cout << "time: " << timeg.hour << "  h. " << timeg.minute << "  m. " << timeg.second << "  s. ";
	// draw tanks
	for ( int i = 0; i < countTank; i++ )
	{
		Point tank = Point( 0, 0 );
		do
		{
			tank.randPoint( polygone_h, polygone_w );

			if ( tank.getChar() == FIELD
				&& tank.move( UP ).getChar() == FIELD
				&& tank.move( DOWN ).getChar() == FIELD
				&& tank.move( LEFT ).getChar() == FIELD
				&& tank.move( RIGHT ).getChar() == FIELD
				) {

				break;
			}

		} while ( true );
		tanks[i] = ( Point( tank ) );
		tank.setChar( TANKS );
	}
	//draw boss
	while ( !canMoveObj( boss, STOP ) )
	{
		boss = boss.move( ( Direction ) ( rand() % 4 ) );
	}
	boss.setChar( TANK );
	//set whizbang
	for ( int i = 0; i < whizbangs.size(); i++ )
	{
		if ( i < whizbang.size() ) {
			whizbang[i] = Point( 0, 0 );
		}
		whizbangs[i] = Point( 0, 0 );
	}
}
void logic( mutex &mtx )
{
	bool goon = true;
	while (!gameOver )
	{
		mtx.lock();
		//time game
		tick( startGame );
		int dir = 4;
		for ( int i = 0; i < tanks.size(); i++ )
		{
			dir = rand() % 4;
			//who kill skip
			if ( tanks[i] == Point( 0, 0 ) )
			{
				continue;
			}
			//move tanks 
			if ( canMoveObj( tanks[i], static_cast<Direction> ( dir ) ) ) {
				tanks[i] = moveObj( tanks[i], static_cast<Direction> ( dir ), TANKS );
				//recharge whizbang
				for ( int k = 0; k < whizbangs.size(); k++ ) {
					if ( whizbangs[k] == Point( 0, 0 ) && tanks[i].move( static_cast<Direction> ( dir ) ).getChar() == FIELD ) {
						whizbangs[k] = tanks[i].move( static_cast<Direction> ( dir ) );
						whizbangs[k].dir = static_cast<Direction> ( dir );
						break;
					}
				}
			}
		}
		mtx.unlock();
		Sleep( 330 );
	}

}
void events( mutex &mtx ) {
	char ev = '\0';
	while ( !gameOver )
	{
		mtx.lock();
		//events
		/*
		 *	moving and shoot of boss
		 *  
		*/
		if ( _kbhit() != 0 ) {
			ev = _getch();
			switch ( ev )
			{
			case 27:
				gameOver = true;
				break;
			case 72:
				if ( canMoveObj( boss, UP ) ) {
					boss = moveObj( boss, UP, TANK );
					boss.dir = UP;
				}
				break;
			case 80:
				if ( canMoveObj( boss, DOWN ) ) {
					boss = moveObj( boss, DOWN, TANK );
					boss.dir = DOWN;
				}
				break;
			case 75:
				if ( canMoveObj( boss, LEFT ) ) {
					boss = moveObj( boss, LEFT, TANK );
					boss.dir = LEFT;
				}
				break;
			case 77:
				if ( canMoveObj( boss, RIGHT ) ) {
					boss = moveObj( boss, RIGHT, TANK );
					boss.dir = RIGHT;
				}
				break;
			case 32:
				//recharge whizbang
				for ( int k = 0; k < whizbang.size(); k++ ) {
					if ( whizbang[k] == Point( 0, 0 ) && boss.move( boss.dir ).getChar() == FIELD ) {
						whizbang[k] = boss.move( boss.dir );
						whizbang[k].dir = boss.dir;
						break;
					}
				}
				break;

			}
		}
		if ( countTank == 0  || health == 0) {
			gameOver = true;
		}
		mtx.unlock();
	}
}
void shoot( mutex &mtx ) {
	while ( !gameOver )
	{
		mtx.lock();

		moveShoot( whizbang, WHIZBANG );
		moveShoot( whizbangs, WHIZBANGS );
		mtx.unlock();
		Sleep( 10 );
	}
}
void tick( time_t begin ) {

	auto points = Point( 2, 0 );
	auto last = Time( 0, 0, 0 );
	clock_t now = clock();
	last.clock( ( ( now - startGame) / CLOCKS_PER_SEC ) );
	if ( last.hour != timeg.hour || last.minute == 60 ) {
		points.x = WIDTH - 13;
		timeg.hour = timeg.hour + 1;
		timeg.hour = 0;
		if ( timeg.minute == 0 )
		{
			points.moveCursor();
			cout << "  ";
		}
		points.x = WIDTH - 19;
		points.moveCursor();
		cout << timeg.hour;
	}
	else if ( last.minute != timeg.minute || last.second == 60 ) {
		points.x = WIDTH - 7;
		timeg.minute = timeg.minute + 1;
		timeg.second = 0;
		if ( timeg.second == 0 )
		{
			points.moveCursor();
			cout << "  ";
		}
		points.x = WIDTH - 13;
		points.moveCursor();
		cout << timeg.minute;
	}
	else if ( last.second != timeg.second ) {
		points.x = WIDTH - 7;
		timeg.second = timeg.second + 1;
		points.moveCursor();
		cout << timeg.second;

	}
}
bool canMoveObj( Point &points, Direction dir ) {
	return( points.move( static_cast<Direction> ( dir ) ).getChar() == FIELD && points.x < WIDTH && points.y < ( HEIGHT - 4 ) && points.y >  4 );
}
Point moveObj( Point &points, Direction dir, char obj ) {
	points.move( STOP ).setChar( FIELD );
	points = points.move( static_cast<Direction> ( dir ) ).setChar( obj );
	return points;
}
void moveShoot( vector<Point> &shoots, char type ) {
	for ( int i = 0; i < shoots.size(); i++ ) {

		if ( !( shoots[i] == Point( 0, 0 ) ) )
		{
			auto dir = shoots[i].dir;
			if ( dir == STOP ) {
				continue;
			}
			if ( canMoveObj( shoots[i], shoots[i].dir ) )
			{
				shoots[i] = moveObj( shoots[i], shoots[i].dir, type );
				shoots[i].dir = dir;
				if ( shoots[i].move( shoots[i].dir ).getChar() == TANKS  && type == WHIZBANG )
				{
					for ( size_t e = 0; e < tanks.size(); e++ )
					{
						if ( tanks[e] == shoots[i].move( shoots[i].dir ) ) {
							tanks[e].setChar( FIELD );
							tanks[e] = Point( 0, 0 );
							score += 10;
							countTank -= 1;
							Point( 2, WIDTH / 2 - 3 ).moveCursor();
							cout << score;
						}
					}
				}

				if ( shoots[i].move( shoots[i].dir ).getChar() == TANK && type == WHIZBANGS )
				{
					health -= 1;
					Point( 2, 10 ).moveCursor();
					cout << health;
				}
			}
			else {
				shoots[i].setChar( FIELD );
				shoots[i] = Point( 0, 0 );
			}
		}
	}
}
bool isDie( Point &points, char type ) {
	return (
		points.move( UP ).getChar() == type ||
		points.move( DOWN ).getChar() == type ||
		points.move( LEFT ).getChar() == type ||
		points.move( RIGHT ).getChar() == type
		);
}
