/*This source code copyrighted by Lazy Foo' Productions (2004-2015)
and may not be redistributed without written permission.*/

//Using SDL, SDL_image, standard IO, and strings
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <string>

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 560;

//Game dimension constants
const int BOARD_TOP_OFFSET = 80;
const int BOARD_LEFT_OFFSET = 5;
const int GRID_HORIZONTAL_UNIT = 90;
const int GRID_VERTICAL_UNIT = 80;

const int TOTAL_BUTTONS = 7;
const int BUTTON_HEIGHT = GRID_VERTICAL_UNIT * 7;
const int BUTTON_WIDTH = GRID_HORIZONTAL_UNIT;

int playerTurn = 1;
int outcome = 0;
bool gameOver = false;

enum LButtonSprite
{
	BUTTON_SPRITE_EMPTY = 0,
	BUTTON_SPRITE_BLACK = 1,
	BUTTON_SPRITE_RED = 2,
	BUTTON_SPRITE_TOTAL = 3
};

enum direction
{
    NORTH = 0,
    EAST = 1,
    SOUTH = 2,
    WEST = 3,
    NORTHEAST = 4,
    SOUTHEAST = 5,
    SOUTHWEST = 6,
    NORTHWEST = 7
};
//Texture wrapper class
class LTexture
{
	public:
		//Initializes variables
		LTexture();

		//Deallocates memory
		~LTexture();

		//Loads image at specified path
		bool loadFromFile( std::string path );

        //Creates image from font string
        bool loadFromRenderedText( std::string textureText, SDL_Color textColor );

		//Deallocates texture
		void free();

		//Renders texture at given point
		void render( int x, int y, SDL_Rect* clip = NULL );

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

//The mouse button
class LButton
{
	public:
		//Initializes internal variables
		LButton();

		//Sets top left position
		void setPosition( int x, int y );

		//Handles mouse event
		void handleEvent( SDL_Event* e, int column );

		//Shows button sprite
		void render();

	private:
		//Top left position
		SDL_Point mPosition;

		//Currently used global sprite
		LButtonSprite mCurrentSprite;
};

class PlaySpace
{
    public:
        //Initializes internal variables
        PlaySpace();

        //Set the position
        void setPosition( int x, int y );

        //Set the status of the space
        void setStatus(int token);

        //Check the status of the space
        int checkStatus();

        //Shows play space sprite
        void render();

    private:
        //graphical position of the space
        SDL_Point position;

        //Status of the play space (empty, black, red)
        int status;

        //Sprite to display at play space position
        LButtonSprite playSpaceSprite;
};

class Grid
{
    public:
        //Initializes internal variables
        Grid();

        //Set active row
        void setRow(int r);

        //Set active column
        void setColumn(int c);

        //drop token into selected column
        bool dropToken(int player);

        //check if grid is full
        // Note: possibly replace with move counter?
        bool isFull();

        bool connectFour();

        //recursive function to check for a sequence of similarly marked tiles in a specified direction
        int checkSequence(int r, int c, int token, direction dir);

        //Render the state of the playing grid
        void render();

    private:
        //Current selected row/column
        int row;
        int col;

        // 7x6 game board array
        PlaySpace spaces[6][7];
};

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;

//Globally used font
TTF_Font *gFont = NULL;

//Scene sprites
SDL_Rect gSpriteClips[ 3 ];
LTexture gSpriteSheetTexture;
LTexture gBoardTexture;
LTexture gBlackWin;
LTexture gRedWin;
LTexture gDraw;

//Buttons objects
LButton gButtons[ TOTAL_BUTTONS ];

//Game Board
Grid connect4Grid;

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

bool LTexture::loadFromFile( std::string path )
{
	//Get rid of preexisting texture
	free();

	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load( path.c_str() );
	if( loadedSurface == NULL )
	{
		printf( "Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError() );
	}
	else
	{
		//Color key image
		SDL_SetColorKey( loadedSurface, SDL_TRUE, SDL_MapRGB( loadedSurface->format, 0, 0xFF, 0xFF ) );

		//Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( gRenderer, loadedSurface );
		if( newTexture == NULL )
		{
			printf( "Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}

	//Return success
	mTexture = newTexture;
	return mTexture != NULL;
}

bool LTexture::loadFromRenderedText( std::string textureText, SDL_Color textColor )
{
    //Get rid of preexisting texture
    free();

    //Render text surface
    SDL_Surface* textSurface = TTF_RenderText_Solid( gFont, textureText.c_str(), textColor );
    if( textSurface == NULL )
    {
        printf( "Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError() );
    }
    else
    {
        //Create texture from surface pixels
        mTexture = SDL_CreateTextureFromSurface( gRenderer, textSurface );
        if( mTexture == NULL )
        {
            printf( "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError() );
        }
        else
        {
            //Get image dimensions
            mWidth = textSurface->w;
            mHeight = textSurface->h;
        }

        //Get rid of old surface
        SDL_FreeSurface( textSurface );
    }

    //Return success
    return mTexture != NULL;
}

void LTexture::free()
{
	//Free texture if it exists
	if( mTexture != NULL )
	{
		SDL_DestroyTexture( mTexture );
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::render( int x, int y, SDL_Rect* clip )
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, mWidth, mHeight };

	//Set clip rendering dimensions
	if( clip != NULL )
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopy( gRenderer, mTexture, clip, &renderQuad );
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}

LButton::LButton()
{
	mPosition.x = 0;
	mPosition.y = 0;
	mCurrentSprite = BUTTON_SPRITE_EMPTY;
	playerTurn = 1;
}

void LButton::setPosition( int x, int y )
{
	mPosition.x = x;
	mPosition.y = y;
}

void LButton::handleEvent( SDL_Event* e , int columnNum)
{
	//If mouse event happened
	if( e->type == SDL_MOUSEMOTION || e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP )
	{
		//Get mouse position
		int x, y;
		SDL_GetMouseState( &x, &y );

		//Check if mouse is in button
		bool inside = true;

		//Mouse is left of the button
		if( x < mPosition.x + 1 )
		{
			inside = false;
		}
		//Mouse is right of the button
		else if( x > mPosition.x + BUTTON_WIDTH - 1 )
		{
			inside = false;
		}
		//Mouse above the button
		else if( y < mPosition.y )
		{
			inside = false;
		}
		//Mouse below the button
		else if( y > mPosition.y + BUTTON_HEIGHT )
		{
			inside = false;
		}

		//Mouse is outside button
		if( !inside )
		{
			mCurrentSprite = BUTTON_SPRITE_EMPTY;
		}
		//Mouse is inside button
		else
		{
			//Set mouse over sprite
			switch( e->type )
			{
				case SDL_MOUSEMOTION:
				    if (playerTurn == 1)
                        mCurrentSprite = BUTTON_SPRITE_BLACK;
				    else if (playerTurn == 2)
                        mCurrentSprite = BUTTON_SPRITE_RED;
				    connect4Grid.setColumn(columnNum);
				break;

				case SDL_MOUSEBUTTONDOWN:
				    if (connect4Grid.dropToken(playerTurn))
                    {
                        if (connect4Grid.isFull())
                        {
                            outcome = 3;
                            gameOver = true;
                        }

                        if (connect4Grid.connectFour())
                        {
                              outcome = playerTurn;
                              gameOver = true;
                        }

                        if (playerTurn == 1)
                        {
                            playerTurn = 2;
                            mCurrentSprite = BUTTON_SPRITE_RED;
                        }
                        else if (playerTurn == 2)
                        {
                            playerTurn = 1;
                            mCurrentSprite = BUTTON_SPRITE_BLACK;
                        }
                        render();
                    }
				break;

				case SDL_MOUSEBUTTONUP:
				break;
			}
		}
	}
}

void LButton::render()
{
	//Show current button sprite
	gSpriteSheetTexture.render( mPosition.x, mPosition.y, &gSpriteClips[ mCurrentSprite ] );
}

PlaySpace::PlaySpace()
{
	status = 0;
	playSpaceSprite = BUTTON_SPRITE_EMPTY;
}

void PlaySpace::setPosition(int x, int y)
{
	position.x = x;
	position.y = y;
}

void PlaySpace::setStatus(int token)
{
    status = token;
}

int PlaySpace::checkStatus()
{
    return status;
}

void PlaySpace::render()
{
    //playSpaceSprite = checkStatus();
    gSpriteSheetTexture.render( position.x, position.y, &gSpriteClips[ status ] );
}

Grid::Grid()
{
    row = 0;
    col = 0;

    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 7; j++)
        {
           spaces[i][j].setStatus(0);
           spaces[i][j].setPosition(BOARD_LEFT_OFFSET + GRID_HORIZONTAL_UNIT * j, BOARD_TOP_OFFSET + GRID_VERTICAL_UNIT * i);
        }
    }
}

void Grid::setRow(int r)
{
    row = r;
}

void Grid::setColumn(int c)
{
    col = c;
}

bool Grid::dropToken(int player)
{
    for (int i = 5; i >=0; i--)
    {
        if (spaces[i][col].checkStatus() == 0)
        {
            spaces[i][col].setStatus(player);
            setRow(i);
            return true;
        }
    }
    return false;
}

// Note: possibly replace with move counter?
bool Grid::isFull()
{
    bool result = true;
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 7; j++)
        {
            if (spaces[i][j].checkStatus() == 0)
                result = false;
        }
    }
    return result;
}

bool Grid::connectFour()
{
    bool result = false;
    if ( checkSequence(row, col, playerTurn, SOUTH) >= 4)
        result = true;

    if ( checkSequence(row, col, playerTurn, WEST) + checkSequence(row, col, playerTurn, EAST) - 1 >= 4)
        result = true;

    if ( checkSequence(row, col, playerTurn, NORTHWEST) + checkSequence(row, col, playerTurn, SOUTHEAST) - 1 >= 4)
        result = true;

    if ( checkSequence(row, col, playerTurn, SOUTHWEST) + checkSequence(row, col, playerTurn, NORTHEAST) -1 >= 4)
        result = true;

    return result;
}

int Grid::checkSequence(int r, int c, int token, direction dir)
{
    int rDif = 1;
    int cDif = 1;

    switch (dir)
    {
    case NORTH:
        rDif = -1;
        cDif = 0;
        break;

    case EAST:
        rDif = 0;
        cDif = 1;
        break;

    case SOUTH:
        rDif = 1;
        cDif = 0;
        break;

    case WEST:
        rDif = 0;
        cDif = -1;
        break;

    case NORTHEAST:
        rDif = -1;
        cDif = 1;
        break;

    case SOUTHEAST:
        rDif = 1;
        cDif = 1;
        break;

    case SOUTHWEST:
        rDif = 1;
        cDif = -1;
        break;

    case NORTHWEST:
        rDif = -1;
        cDif = -1;
        break;
    }

    if (r > 5 || c > 6 || r < 0 || c < 0)
    {
        return 0;
    }
    else if (spaces[r][c].checkStatus() == token)
    {
        return ( 1 + checkSequence( r + rDif, c + cDif, token, dir) );
    }
    else
        return 0;
}

void Grid::render()
{
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 7; j++)
        {
           spaces[i][j].render();
        }
    }
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
		{
			printf( "Warning: Linear texture filtering not enabled!" );
		}

		//Create window
		gWindow = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
		if( gWindow == NULL )
		{
			printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
			success = false;
		}
		else
		{
			//Create renderer for window
			gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED );
			if( gRenderer == NULL )
			{
				printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
				success = false;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if( !( IMG_Init( imgFlags ) & imgFlags ) )
				{
					printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
					success = false;
				}

				 //Initialize SDL_ttf
				if( TTF_Init() == -1 )
				{
					printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
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

	//Load board texture
	if( !gBoardTexture.loadFromFile( "Connect4Board.png" ) )
	{
		printf( "Failed to load sprite sheet texture!\n" );
		success = false;
	}

    //Open the font
    gFont = TTF_OpenFont( "04B_30__.ttf", 28 );
    if( gFont == NULL )
    {
        printf( "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError() );
        success = false;
    }
    else
    {
        //Render text
        SDL_Color textColor = { 0, 0, 0 };
        if( !gBlackWin.loadFromRenderedText( "Black Wins!!!", textColor ) )
        {
            printf( "Failed to render text texture!\n" );
            success = false;
        }

        textColor = { 255, 0, 0};
        if( !gRedWin.loadFromRenderedText( "Red Wins!!!", textColor ) )
        {
            printf( "Failed to render text texture!\n" );
            success = false;
        }

        textColor = { 0, 0, 255};
        if( !gDraw.loadFromRenderedText( "Draw...", textColor ) )
        {
            printf( "Failed to render text texture!\n" );
            success = false;
        }
    }

	//Load sprite sheet texture
	if( !gSpriteSheetTexture.loadFromFile( "Connect4Tokens.png" ) )
	{
		printf( "Failed to load sprite sheet texture!\n" );
		success = false;
	}
	else
	{
		//Set empty sprite
		gSpriteClips[ 0 ].x =   0;
		gSpriteClips[ 0 ].y =   0;
		gSpriteClips[ 0 ].w = 90;
		gSpriteClips[ 0 ].h = 80;

		//Set black sprite
		gSpriteClips[ 1 ].x = 90;
		gSpriteClips[ 1 ].y =   0;
		gSpriteClips[ 1 ].w = 90;
		gSpriteClips[ 1 ].h = 80;

		//Set red sprite
		gSpriteClips[ 2 ].x = 180;
		gSpriteClips[ 2 ].y =   0;
		gSpriteClips[ 2 ].w = 90;
		gSpriteClips[ 2 ].h = 80;

		//Set buttons in corners
		for( int i = 0; i < TOTAL_BUTTONS; i++ )
		{
			gButtons[ i ].setPosition( BOARD_LEFT_OFFSET + i * GRID_HORIZONTAL_UNIT, 0 );
		}
	}




	return success;
}

void close()
{
	//Free loaded image
	gSpriteSheetTexture.free();
	gBoardTexture.free();
	gBlackWin.free();
	gRedWin.free();
    gDraw.free();

    //Free global font
    TTF_CloseFont( gFont );
    gFont = NULL;

	//Destroy window
	SDL_DestroyRenderer( gRenderer );
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

int main( int argc, char* args[] )
{
	//Start up SDL and create window
	if( !init() )
	{
		printf( "Failed to initialize!\n" );
	}
	else
	{
		//Load media
		if( !loadMedia() )
		{
			printf( "Failed to load media!\n" );
		}
		else
		{
			//Main loop flag
			bool quit = false;

			//Event handler
			SDL_Event e;

			//While application is running
			while( !quit )
			{
                //Handle events on queue
                while( SDL_PollEvent( &e ) != 0 )
                {
                    //User requests quit
                    if( e.type == SDL_QUIT )
                    {
                        quit = true;
                    }

                    if ( !gameOver)
                    {
                        //Handle button events
                        for( int i = 0; i < TOTAL_BUTTONS; ++i )
                        {
                        gButtons[ i ].handleEvent( &e, i );
                        }
                    }
                }

                //Clear screen
                SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
                SDL_RenderClear( gRenderer );

                //Render Playspaces
                connect4Grid.render();

                //Render Board
                gBoardTexture.render( 0, GRID_VERTICAL_UNIT, NULL );

                if ( !gameOver)
                {
                    //Render buttons
                    for( int i = 0; i < TOTAL_BUTTONS; ++i )
                    {
                            gButtons[ i ].render();
                    }
                }
                else
                {
                    switch (outcome)
                    {
                        //Black wins
                    case 1:
                        gBlackWin.render( ( SCREEN_WIDTH - gBlackWin.getWidth() ) / 2, ( BOARD_TOP_OFFSET - gBlackWin.getHeight() ) / 2 );
                        break;

                        //Red Wins
                    case 2:
                        gRedWin.render( ( SCREEN_WIDTH - gRedWin.getWidth() ) / 2, ( BOARD_TOP_OFFSET - gRedWin.getHeight() ) / 2 );
                        break;

                        //Draw
                    case 3:
                        gDraw.render( ( SCREEN_WIDTH - gDraw.getWidth() ) / 2, ( BOARD_TOP_OFFSET - gDraw.getHeight() ) / 2 );
                        break;

                    default:
                        break;
                    }
                }

                //Update screen
                SDL_RenderPresent( gRenderer );
            }
		}
	}

	//Free resources and close SDL
	close();

	return 0;
}
