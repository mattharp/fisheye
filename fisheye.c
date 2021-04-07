#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "stdio.h"
#include "math.h"


Uint32 get_pixel(SDL_Surface *surface, int x, int y);
void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
void fish_eye(SDL_Surface *surface, double w, double h);
int init( int width, int height );
void close();
SDL_Surface* load_surface( char* path );

SDL_Window* gWindow = NULL;
// surface contained by the window
SDL_Surface* gScreenSurface = NULL;
// surface of loaded image
SDL_Surface* gImgSurface = NULL;


Uint32 get_pixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    // address of pixel
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp)
    {
        case 1:
            return *p;
            break;

        case 2:
            return *(Uint16 *)p;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
                break;

            case 4:
                return *(Uint32 *)p;
                break;

            default:
                return 0;
          }
}

void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel);
    *target_pixel = pixel;
}

void fish_eye(SDL_Surface *surface, double w, double h)
{
    SDL_Surface *tmpSurface;
    tmpSurface = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
    SDL_FillRect(tmpSurface, NULL, SDL_MapRGB(tmpSurface->format, 0, 0, 0));

    //SDL_LockSurface(tmpSurface);

    for (int y=0;y<h;y++)
    {                                
        // normalize y coordinate to -1 ... 1
        double ny = ((2*y)/h)-1;                        
        // pre calculate ny*ny
        double ny2 = ny*ny;                                
        // for each column
        for (int x=0;x<w;x++)
        {                            
            // normalize x coordinate to -1 ... 1
            double nx = ((2*x)/w)-1;                    
            // pre calculate nx*nx
            double nx2 = nx*nx;
            // calculate distance from center (0,0)
            double r = sqrt(nx2+ny2);                
            // discard pixels outside circle
            if (0.0<=r&&r<=1.0)
            {                            
                double nr = sqrt(1.0-r*r);            
                // new distance is between 0 ... 1
                nr = (r + (1.0-nr)) / 2.0;
                // discard radius greater than 1.0
                if (nr<=1.0)
                {
                    // calculate the angle for polar coordinates
                    double theta = atan2(ny,nx);         
                    // calculate new x position with new distance in same angle
                    double nxn = nr*cos(theta);        
                    // calculate new y position with new distance in same angle
                    double nyn = nr*sin(theta);        
                    // map from -1 ... 1 to image coordinates
                    int x2 = (int)(((nxn+1)*w)/2.0);        
                    // map from -1 ... 1 to image coordinates
                    int y2 = (int)(((nyn+1)*h)/2.0);        
                    // find (x2,y2) position from source pixels
                    int srcpos = (int)(y2*w+x2);            
                    // make sure that position stays within arrays
                    if (srcpos>=0 & srcpos < w*h)
                    {
                        // get new pixel (x2,y2) and put it to target at (x,y)
                        set_pixel(tmpSurface, x, y, get_pixel(surface, x2, y2));
                    }
                }
            }
        }
    }

    //SDL_UnlockSurface(tmpSurface);

    SDL_BlitSurface( tmpSurface, NULL, surface, NULL );
    SDL_FreeSurface( tmpSurface );
} 


int init(int width, int height)
{
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "SDL Error: %s\n", SDL_GetError() );
        return 1;
    }
    else
    {
        gWindow = SDL_CreateWindow( "Fish Eye Lens", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN );
        if( gWindow == NULL )
        {
            printf( "SDL Error: %s\n", SDL_GetError() );
            return 1;
        }
        else
        {
            gScreenSurface = SDL_GetWindowSurface( gWindow );
        }
    }

    return 0;
}

void close()
{
    // free loaded image
    SDL_FreeSurface( gImgSurface );
    gImgSurface = NULL;

    // destroy window
    SDL_DestroyWindow( gWindow );
    gWindow = NULL;

    // quit SDL subsystems
    SDL_Quit();
}

SDL_Surface* load_surface( char* path )
{
    SDL_Surface* loadedSurface = IMG_Load( path );
    return loadedSurface;
}

int main( int argc, char* args[] )
{
    char* path;

    if( argc == 2 )
    {
        path = args[1];
    }
    else
    {
        printf( "Please supply an image path\n" );
        return 1;
    }

    gImgSurface = load_surface( path );
    if( gImgSurface == NULL )
    {
        printf( "Could not load image\n" );
        return 1;
    }

    // start SDL and create window
    if( init( gImgSurface->w, gImgSurface->h ) != 0 )
    {
        printf( "Failed to initialize!\n" );
        return 1;
    }

    // apply fish eye effect to surface
    fish_eye(gImgSurface, gImgSurface->w, gImgSurface->h);
            
    int running = 1;
    SDL_Event e;

    while( running )
    {
        while( SDL_PollEvent( &e ) != 0 )
        {
            if( e.type == SDL_QUIT )
            {
                running = 0;
            }
        }

        SDL_BlitSurface( gImgSurface, NULL, gScreenSurface, NULL );
        SDL_UpdateWindowSurface( gWindow );
    }

    close();

    return 0;
}
