#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include <cmath>
#include <vector> // Long Term Solution
#include <sstream> // Long Term Solution
#include <iostream> // Temporary Solution to a Long Term Problem
#include <string>
#include <stdio.h> // Debugging

struct coordinate
{
	int x;
	int y;
};

struct line
{
	coordinate start;
	coordinate end;
	SDL_Color color;
	char label;
};

const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 500;

//The window to render to
SDL_Window* gWindow = NULL;
//The window renderer
SDL_Renderer* gRenderer = NULL;
//Globally used fonts
TTF_Font *gLabelFont = NULL;
TTF_Font *gPointFont = NULL;

const line SUPPLY_CURVE = { {265, 85}, {590 , 410}, {0x00, 0xFF, 0xFF, 0xFF}, 'S' };
const line DEMAND_CURVE = { {265, 410}, {590 , 85}, {0x00, 0xFF, 0xFF, 0xFF}, 'D' };

const line RSUPPLY_CURVE = { {285, 85}, {610, 410}, {0x00, 0xFF, 0x00, 0xFF}, 'S' };
const line RDEMAND_CURVE = { {285, 410}, {610, 85}, {0x00, 0xFF, 0x00, 0xFF}, 'D' };
const line LSUPPLY_CURVE = { {245, 85}, {570, 410}, {0xFF, 0x00, 0x00, 0xFF}, 'S' };
const line LDEMAND_CURVE = { {245, 410}, {570, 85}, {0xFF, 0x00, 0x00, 0xFF}, 'D' };

//Texture wrapper class
class LTexture
{
public:
	//Initializes variables
	LTexture();

	//Deallocates memory
	~LTexture();

	//Loads image at specified path
	bool loadFromFile(std::string path);

#if defined(SDL_TTF_MAJOR_VERSION)
	//Creates image from font string
	bool loadFromRenderedText(std::string textureText, SDL_Color textColor, TTF_Font *font = gLabelFont);
#endif

	//Deallocates texture
	void free();

	//Set color modulation
	void setColor(Uint8 red, Uint8 green, Uint8 blue);

	//Set blending
	void setBlendMode(SDL_BlendMode blending);

	//Set alpha modulation
	void setAlpha(Uint8 alpha);

	//Renders texture at given point
	void render(int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

	//Gets image dimensions
	int getWidth();
	int getHeight();

private:
	//The actual hardware texture
	SDL_Texture* mTexture;

	//Image dimensions
	int mWidth;
	int mHeight;
};

line gCURVES[4] = { LSUPPLY_CURVE, RSUPPLY_CURVE, LDEMAND_CURVE, RDEMAND_CURVE };
bool gDrawCurves[4] = { false, false, false, false };

//Get Intersections Funciton Required
int gIntersections = 0;
std::stringstream gInters;

//The button sections
SDL_Rect gButtons[8] = 
{ 
	{650,  15, 150, 220},  //LSUPPLY
	{820,  15, 150, 220}, //RSUPPLY
	{650, 265, 150, 220}, //LDEMAND
	{820, 265, 150, 220}, //RDEMAND
	{15,  0, 150, 30},
	{15, 45, 150, 30},
	{15, 90, 150, 30},
	{15, 135, 35, 30}
};

std::string gProduct = "";
LTexture gXAxisTexture, gYAxisTexture, gGraphTitleTexture;

//Get Resource Functions
bool init();
bool loadMedia();
void close();

//TO DO: Make functions below
void drawEquilibriumPoint(coordinate point);

int getDistanceSquared(int oneX, int oneY, int twoX, int twoY);
coordinate getIntersection(line first, line second);
void buttonCollision(int x, int y);
void drawLine(line Line, int width = 0);

LTexture::LTexture()
{
	//Initialize
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::~LTexture()
{
	//Deallocate
	free();
}

bool LTexture::loadFromFile(std::string path)
{
	//Get rid of preexisting texture
	free();

	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		//Color key image
		SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0xFF, 0xFF, 0xFF)); //Color Key White

		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if (newTexture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	//Return success
	mTexture = newTexture;
	return mTexture != NULL;
}

#if defined(SDL_TTF_MAJOR_VERSION)
bool LTexture::loadFromRenderedText(std::string textureText, SDL_Color textColor, TTF_Font *font)
{
	//Get rid of preexisting texture
	free();

	//Render text surface
	SDL_Surface* textSurface = TTF_RenderText_Solid(font, textureText.c_str(), textColor);
	if (textSurface == NULL)
	{
		printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
	}
	else
	{
		//Create texture from surface pixels
		mTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
		if (mTexture == NULL)
		{
			printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = textSurface->w;
			mHeight = textSurface->h;
		}

		//Get rid of old surface
		SDL_FreeSurface(textSurface);
	}

	//Return success
	return mTexture != NULL;
}
#endif

void LTexture::free()
{
	//Free texture if it exists
	if (mTexture != NULL)
	{
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
	//Modulate texture rgb
	SDL_SetTextureColorMod(mTexture, red, green, blue);
}

void LTexture::setBlendMode(SDL_BlendMode blending)
{
	//Set blending function
	SDL_SetTextureBlendMode(mTexture, blending);
}

void LTexture::setAlpha(Uint8 alpha)
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod(mTexture, alpha);
}

void LTexture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, mWidth, mHeight };

	//Set clip rendering dimensions
	if (clip != NULL)
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx(gRenderer, mTexture, clip, &renderQuad, angle, center, flip);
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}

int getDistanceSquared(int oneX, int oneY, int twoX, int twoY)
{
	int x, y;
	x = twoX - oneX;
	y = twoY - oneY;
	return ((x * x) + (y * y));
}

void drawEquilibriumPoint(coordinate point)
{
	SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
	for (int i = 230; i <= point.x; i += 4)
	{
		SDL_RenderDrawPoint(gRenderer, i, point.y);
	}
	for (int j = 420; j >= point.y; j -= 4)
	{
		SDL_RenderDrawPoint(gRenderer, point.x, j);
	}
	//TO DO: DRAW 'label' AT ('point.x', 'point.y' - 5)
	//REQUIRES LTEXTURE CLASS AND gINTERS MATH
}

coordinate getIntersection(line first, line second)
{
	coordinate res;

	//ALGEBRA
	double ABm = (first.end.y - first.start.y) / (first.end.x - first.start.x);
	double ABb = (first.start.y - (ABm * first.start.x));

	double CDm = (second.end.y - second.start.y) / (second.end.x - second.start.x);
	double CDb = (second.start.y - (CDm * second.start.x));

	CDb += ABb * -1;
	CDm += ABm * -1;

	res.x = (CDb * -1) / CDm;
	res.y = ((res.x * ABm) + ABb);

	return res;
}

void buttonCollision(int x, int y) 
{
	//if the button is on the left side
	if (x <= WINDOW_WIDTH / 2)
	{
		for (int i = 4; i < 8; i++) // Check buttons 4 - 7
		{
			//if the mouse is between the x boundaries of the box
			if (x > gButtons[i].x && x < gButtons[i].x + gButtons[i].w)
			{
				//if the mouse is between the y boundaries of the box
				if (y > gButtons[i].y && y < gButtons[i].y + gButtons[i].h)
				{
					switch (i)
					{
					case 4:
						//CHANGE GRAPH TITLE : kinda irrelevant now
						break;
					case 5:
						//CHANGE X-AXIS TITLE : kinda irrelevant now
						break;
					case 6:
						//CHANGE Y-AXIS TITLE : kinda irrelevant now
						break;
					case 7:
						//APPLY CHANGES
						std::cout << "Product: ";
						std::cin >> gProduct;
						std::cout << std::endl;
						gYAxisTexture.loadFromRenderedText("Price of " + gProduct, { 0, 0, 0, 255 }, gLabelFont);
						gXAxisTexture.loadFromRenderedText("Quantity of " + gProduct + " Demanded", { 0, 0, 0, 255 }, gLabelFont);
						gGraphTitleTexture.loadFromRenderedText("Supply and Demand of " + gProduct, { 0, 0, 0, 255 }, gLabelFont);
						break;
					}
					break;
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < 4; i++) // Check buttons 0 - 3
		{
			//if the mouse is between the x boundaries of the box
			if (x > gButtons[i].x && x < gButtons[i].x + gButtons[i].w)
			{
				//if the mouse is between the y boundaries of the box
				if (y > gButtons[i].y && y < gButtons[i].y + gButtons[i].h)
				{
					gDrawCurves[i] = gDrawCurves[i] ? false : true;
					break;
				}
			}
		}
	}
}

void drawLine(line Line, int width)
{
	SDL_SetRenderDrawColor(gRenderer, Line.color.r, Line.color.g, Line.color.b, Line.color.a); //pick the color
	for (int i = 0; i <= width; i++)
	{
		SDL_RenderDrawLine(gRenderer, Line.start.x + i, Line.start.y, Line.end.x + i, Line.end.y); //Draw the line
	}
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}

		//Create window
		gWindow = SDL_CreateWindow("EconChartMaker",			//Window Title
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,	//Define where to place the window
			WINDOW_WIDTH, WINDOW_HEIGHT,						//Define the width and height of the window
			SDL_WINDOW_SHOWN);									//Creation Flags | Window is shown upon creation

		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
			if (gRenderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
					success = false;
				}

				//Initialize SDL_ttf
				if (TTF_Init() == -1)
				{
					printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
					success = false;
				}

				//Initialize SDL_mixer
				if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
				{
					printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
					success = false;
				}
			}
		}
	}

	return success;
}

bool loadMedia()
{
	//Loading success flag
	bool success = true;

	//Open the font
	gLabelFont = TTF_OpenFont("../Assets/comic.ttf", 20);
	if (gLabelFont == NULL)
	{
		printf("Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError());
		success = false;
	}

	//Open the font
	gPointFont = TTF_OpenFont("../Assets/comic.ttf", 14);
	if (gPointFont == NULL)
	{
		printf("Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError());
		success = false;
	}

	gXAxisTexture.loadFromRenderedText("Quantity Demanded of Product A", { 0, 0, 0, 255 }, gLabelFont);
	gYAxisTexture.loadFromRenderedText("Price of Product A", { 0, 0, 0, 255 }, gLabelFont);
	gGraphTitleTexture.loadFromRenderedText("Supply and Demand of Product A", { 0, 0, 0, 255 }, gLabelFont);

	return success;
}

void close()
{
	//Free global font
	TTF_CloseFont(gLabelFont);
	gLabelFont = NULL;
	TTF_CloseFont(gPointFont);
	gPointFont = NULL;

	//Destroy window	
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	Mix_Quit();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

int main(int argc, char* args[]) {

	//Start up SDL and create window
	if (!init())
	{
		printf("Failed to initialize!\n");
		return 0;
	}

	//Load media
	if (!loadMedia())
	{
		printf("Failed to load media!\n");
		return 0;
	}
	
	//Main loop flag
	bool quit = false;

	//Event handler
	SDL_Event e;

	//While application is running
	while (!quit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			if (e.type == SDL_MOUSEBUTTONDOWN) {
				//Get mouse position
				int x, y;
				SDL_GetMouseState(&x, &y);
				buttonCollision(x, y);
			}
		}

		//Clear screen
		SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(gRenderer);

		//TO DO: ADD THINGS TO RENDER HERE

		{//CONSTANT OBJECTS WHICH MUST ALWAYS BE DRAWN
			//DRAW BORDER 
			SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
			for (int i = 638; i <= 642; i++) {
				SDL_RenderDrawLine(gRenderer, i, 0, i, WINDOW_HEIGHT);
			}

			//DRAW BUTTONS
			for (int i = 0; i < sizeof(gButtons) / sizeof(SDL_Rect); i++) {
				SDL_RenderDrawRect(gRenderer, &gButtons[i]);
			}

			//DRAW X-AXIS
			for (int i = 225; i <= 230; i++) {
				SDL_RenderDrawLine(gRenderer, i, 75, i, 425);
			}
			//DRAW Y-AXIS
			for (int i = 420; i <= 425; i++) {
				SDL_RenderDrawLine(gRenderer, 225, i, 600, i);
			}

			//DRAW SUPPLY AND DEMAND LINES
			drawLine(SUPPLY_CURVE, 3);
			drawLine(DEMAND_CURVE, 3);

			drawEquilibriumPoint(getIntersection(SUPPLY_CURVE, DEMAND_CURVE));

			//DRAW LABELS
			gXAxisTexture.render(255, 460);
			gYAxisTexture.render(120 + (112 - gYAxisTexture.getWidth()), 220, (SDL_Rect *)0, 270);
			gGraphTitleTexture.render(265, 40);
		}
		
		//DRAW INCRESED/DECREASED SUPPLY/DEMAND CURVES
		for (int i = 0; i < 4; i++)
		{
			if (gDrawCurves[i])
			{
				//DRAW LINE
				drawLine(gCURVES[i], 3);
				//DRAW EQUILIBRIUM LINES - NEED TO ADD LABELS
				if (gCURVES[i].label == 'S')
				{
					drawEquilibriumPoint(getIntersection(gCURVES[i], DEMAND_CURVE));
				}
				else
				{
					drawEquilibriumPoint(getIntersection(SUPPLY_CURVE, gCURVES[i]));
				}
			}
		}

		//Update screen
		SDL_RenderPresent(gRenderer);
	}

	//Free resources and close SDL
	close();

	return 0;
}