#include "client.h"
#include "menu.h"
#include "game.h"
#include "battle.h"

#if defined(WIN32)
#include "DynDialogs.h"
#endif

SDL_Surface *screen;
SDL_Surface *tempmenusurf;
SDL_Surface* tiles[256]; // holds all the tiles loaded in from the tile library
SDL_Surface* cTiles[256]; // holds tiles that never change, eg. players
SDL_Surface* mTiles[256]; // holds tiles that never change, eg. players

TTF_Font *systemfont;

int mapX, mapY, layers, tileset, extradatalength; // map data
int* extradata;
int map[3][256][256];

//BYTE enemyID;
char aname[24][20];

int gamestate=0;
Mix_Music *music;   /* SDL_Mixer stuff here */

struct playerz player;

/* load_ConstantTiles - this function loads the tilesets that dont ever change into an array */
int load_ConstantTiles()
{
    int i,j;
    char buffer[128];
    SDL_Surface * tempsurf;
 
    for(i=0;i<256;i++)
     {
        sprintf(buffer,"tiles/0/%d.png",i);
        tempsurf=IMG_Load(buffer);
        if ( tempsurf ==NULL ) {
            tempsurf=IMG_Load("notex.png");
            if ( tempsurf ==NULL )
            {
                for(j=0;j<i-1;j++)
                   SDL_FreeSurface(cTiles[j]);
                return 1;
            }else{
                  SDL_SetColorKey(tempsurf, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(tempsurf->format,0xFF,0x00,0xFF));
                  cTiles[i] = SDL_DisplayFormat(tempsurf);
                  SDL_FreeSurface(tempsurf);
            }
        }else{
              SDL_SetColorKey(tempsurf, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(tempsurf->format,0xFF,0x00,0xFF));
              cTiles[i] = SDL_DisplayFormat(tempsurf);
              SDL_FreeSurface(tempsurf);
        }
    }
    return 0;
}

int load_MonsterTiles()
{
    int i,j;
    char buffer[128];
    SDL_Surface * tempsurf;
 
    for(i=0;i<256;i++)
     {
        sprintf(buffer,"tiles/m/%d.png",i);
        tempsurf=IMG_Load(buffer);
        if ( tempsurf ==NULL ) {
            tempsurf=IMG_Load("notex.png");
            if ( tempsurf ==NULL )
            {
                for(j=0;j<i-1;j++)
                   SDL_FreeSurface(mTiles[j]);
                return 1;
            }else{
                  SDL_SetColorKey(tempsurf, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(tempsurf->format,0xFF,0x00,0xFF));
                  mTiles[i] = SDL_DisplayFormat(tempsurf);
                  SDL_FreeSurface(tempsurf);
            }
        }else{
              SDL_SetColorKey(tempsurf, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(tempsurf->format,0xFF,0x00,0xFF));
              mTiles[i] = SDL_DisplayFormat(tempsurf);
              SDL_FreeSurface(tempsurf);
        }
    }
    return 0;
}

int load_Tiles(int tileset)
{
    int i,j;
    char buffer[128];
    SDL_Surface * tempsurf;
 
    for(i=0;i<256;i++)
     {
        sprintf(buffer,"tiles/%d/%d.png",tileset,i);
        tempsurf=IMG_Load(buffer);
        if ( tempsurf ==NULL ) {
            tempsurf=IMG_Load("notex.png");
            if ( tempsurf ==NULL )
            {
                for(j=0;j<256;j++)
                   SDL_FreeSurface(cTiles[j]);
                for(j=0;j<i-1;j++)
                   SDL_FreeSurface(tiles[j]);
                return 1;
            }else{
                  SDL_SetColorKey(tempsurf, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(tempsurf->format,0xFF,0x00,0xFF));
                  tiles[i] = SDL_DisplayFormat(tempsurf);
                  SDL_FreeSurface(tempsurf);
            }
        }else{
              SDL_SetColorKey(tempsurf, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(tempsurf->format,0xFF,0x00,0xFF));
              tiles[i] = SDL_DisplayFormat(tempsurf);
              SDL_FreeSurface(tempsurf);
        }
    }
    return 0;
}

void free_tiles()
{
    for(int i=0;i<256;i++)
    {
       if( tiles[i] != NULL)
           SDL_FreeSurface(tiles[i]);
    }    
}


void load_Map(int area)
{
     // load map Data (tile#)
     int q;
     FILE *fp;
     char str[20];
     sprintf(str, "maps/%d.lvl",area);
     fp = fopen( str,"r");
     if (fp == NULL)
     {
//        char *msg;
        printf ("could not load the mapfile %d",area);
//        MessageBox (0, msg, "Error", MB_ICONHAND); 
//        free (msg);
        exit (1);
     }
     else
     { 
         mapX = (int)fgetc(fp)+ (int)fgetc(fp)*256; 
         mapY = (int)fgetc(fp)+ (int)fgetc(fp)*256 ;
         layers = (int)fgetc(fp);
	     tileset = (int)fgetc(fp);
	     free_tiles();
         load_Tiles(tileset);
	     extradatalength = (int)fgetc(fp);
	     for(int i = 0; i < extradatalength; i++)
            extradata[i] = (int)fgetc(fp);
	     for( int y = 0; y <= mapY && y < 256; y++)
            for( int x = 0; x <= mapX && x < 256; x++)
            {
                 for(q=0;q<layers;q++)
                     map[q][x][y] = (int)fgetc(fp);
            }
     }
}

int main (int argc, char *argv[])
{
    char ack;
    char username[32],password[32],initbuf[110]; // couple of buffers
	int read,i; // other info
    int done=0;

   	IPaddress serverIP;    // networking information
	TCPsocket tcpsock=NULL;
	SDLNet_SocketSet set=NULL;
    FILE *fp;

    int audio_rate = 44100;              // audio stuff
    Uint16 audio_format = AUDIO_S16;
    int audio_channels = 2;
    int audio_buffers = 4096;

    gamestate=0;

	//We must first initialize the SDL video component, and check for success
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);  //When this program exits, SDL_Quit must be called

    // BEGIN SETUP OF NETWORK CONNECTIONS
    if (SDLNet_Init() ==-1){
		printf("Unable to initialize SDL_Net: %s\n", SDLNet_GetError());
		return 2;
	}
    // this is where the old login info was.

    screen = SDL_SetVideoMode(SCREEN_X, SCREEN_Y, 16, SDL_HWSURFACE | SDL_DOUBLEBUF );//| SDL_FULLSCREEN);
	if (screen == NULL) {
		printf("Unable to set video mode: %s\n", SDL_GetError());
		return 1;
	}

    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
      printf("Unable to open audio!\n");
      exit(1);
    }
    Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);

    if (TTF_Init()==-1) { printf("TTF_Init: %s\n", TTF_GetError()); exit(2); }
    systemfont=TTF_OpenFont("coure.fon", 10); if(!systemfont) { printf("TTF_OpenFont: %s\n", TTF_GetError()); }

    SDL_EnableKeyRepeat(100, 100);

    load_ConstantTiles();
    load_MonsterTiles();

     fp = fopen("ability.txt","r");
     if (fp == NULL)
     {
        printf ("could not load the ability names\n");
        exit (1);
     }else{
           for(i=0;i<24;i++)
             fscanf(fp,"%s\n",aname[i]);
     }
    
    while (!done)
    {
          if(gamestate==0)
          {
              inittitle();
               while (!done && gamestate==0)
               {
                    done=handletitle();
                    drawtitle();
               }
               destroytitle();
          }else if (gamestate==1)
          {
              initmenu();
               while (!done && gamestate==1)
               {
                done=handlemenu();
                drawmenu();
               }
               destroymenu();
          }else if (gamestate==2){
              initcredits();
               while (!done && gamestate==2)
               {
                done=handlecredits();
                drawcredits();
               }
               destroycredits();
          }else if (gamestate==3){
                
                #if defined(WIN32)
                   read=InputBox(NULL,"Enter the name or IP address of the server (draconis.game-server.cc if unsure)","Server address",username,31);
                 //  strcpy(username,"draconis.game-server.cc");
                  // strcpy(username,"localhost");
                   //strcpy(username,"130.184.85.192");
                   read=InputBox(NULL,"Enter your screen name.","Draconis",player.name,31);
                   read=InputBox(NULL,"Enter your password.","Draconis",password,31);
                #else
	               strncpy(username,argv[1],31);
	               strncpy(player.name,argv[2],31);
	               strncpy(password,argv[3],31);
                #endif
 
                if(username==NULL || SDLNet_ResolveHost(&serverIP,username,5009)==-1) {
                   printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
                   exit(1);
                }
                tcpsock=SDLNet_TCP_Open(&serverIP);
                if(!tcpsock) {
                   printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
                   exit(2);
                }
                set = SDLNet_AllocSocketSet(1);
                if (!set) {
                    printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
                    exit(1); //most of the time this is a major error, but do what you want.
                }
                if (SDLNet_TCP_AddSocket(set,tcpsock)==-1)
                {
                    printf("SDLNet_TCP_AddSocket: %s\n", SDLNet_GetError());
                    exit(1); //most of the time this is a major error, but do what you want.
                }
                SDLNet_TCP_Recv(tcpsock, &ack, 1);
                if (ack!='K')
                   done=1;
                else
                {
                   read=SDLNet_TCP_Send(tcpsock, player.name, 32);
                   read=SDLNet_TCP_Send(tcpsock, password, 32);
                   SDLNet_TCP_Recv(tcpsock, &initbuf, 110);
                   printf("My init player data is\n");
                   for(read=0;read<50;read++)
                     printf("%d\n",initbuf[read]);
                   printf("end of player data\n");
                   if (initbuf[0]=='F')            // invalid character
                      //gamestate=1;
                      done=1;
                   else if (initbuf[0]=='I')       // initial data: load map and tiles
                   {
               	      player.area =initbuf[1];
                	  player.matX =initbuf[2];
                	  player.matY =initbuf[3];
                	  player.hp =initbuf[4];
                	  player.hpm =initbuf[5];
                	  player.mp =initbuf[6];
                	  player.mpm =initbuf[7];
                	  player.startImage =initbuf[8];
                      player.actualImage=player.startImage;
                	  player.speed =initbuf[9];
                	  player.xp =initbuf[10]*256+initbuf[11];
                	  player.gold =initbuf[12]*256+initbuf[13];
                	  player.level =initbuf[14];
                	  player.str =initbuf[15];
                	  player.dex =initbuf[16];
                	  player.con =initbuf[17];
                	  player.iq =initbuf[18];
                	  player.wis =initbuf[19];
                	  player.cha =initbuf[20];
                	  player.fort =initbuf[21];
                	  player.reflex =initbuf[22];
                	  player.will =initbuf[23];
                	  player.def =initbuf[24];
                	  player.mdef =initbuf[25];
                	  for(int i=0;i<24;i++)
                	    player.abilities[i]=initbuf[26+i];
                	  for(int i=0;i<60;i++)
                	    player.items[i]=initbuf[50+i];
                      load_Map(player.area);
                    //  load_Tiles(tileset);
                      gamestate=10;
                   }else
                      done=1;
                }
                printf("Attempted connect, received %c from server.\n",ack);
                }else if (gamestate==9){ //MENU
                      initstartscreen();
                      printf("Succeeded at initstartscreen.\n");
    
                      while (!done && gamestate==9)
                      {
                            done=handlestartscreen();
                            drawstartscreen(player);
                            do_net_mapupdate(set,tcpsock); // get info
                      }
                      destroystartscreen();
                      printf("Succeeded at destroystartscreen.\n");
                }else if (gamestate==10){ //MAIN
                      initmapscreen();
                      printf("Succeeded at initmapscreen.\n");
                      while (!done && gamestate==10)
                      {
                            done=handlemapscreen(tcpsock);
                            drawmapscreen();
                            do_net_mapupdate(set,tcpsock); // get info
                      }
                      destroymapscreen();
                      printf("Succeeded at destroymapscreen.\n");
                }else if (gamestate==11){
			          printf("I'd set up the battle here.");
			          gamestate=12;
                }else if (gamestate==12){ //BATTLE
                      initbattlescreen();
                      printf("Succeeded at initbattlescreen.\n");
                      while (!done && gamestate==12)
                      {
                            done=handlebattlescreen(tcpsock);
                            drawbattlescreen();
                            do_net_battleupdate(set,tcpsock); // get info
                      }
                      destroybattlescreen();
                      printf("Succeeded at destroybattlescreen.\n");
                }else{
                done=1;           /* should not reach this point. */
          }
    }

    Mix_HaltMusic();
    Mix_FreeMusic(music);
    Mix_CloseAudio();

    return 0;
}
