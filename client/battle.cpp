/* battle.c - game code goes here too. */
#include "../common.h"
#include "battle.h"

extern SDL_Surface *screen;
extern int gamestate;
extern Mix_Music *music;   /* SDL_Mixer stuff here */
extern SDL_Surface* cTiles[256]; // holds tiles that never change, eg. players
extern SDL_Surface* mTiles[256]; // holds tiles that never change, eg. players
extern TTF_Font *systemfont;
extern struct playerz player;
extern SDL_Rect tilerect;

struct enemyz {
       int monid;
       int moncharge;
       BYTE image;
/*       BYTE monspeed;
       BYTE monhp;
       BYTE monmp; */
} enemy;

extern char aname[20][20];

BYTE eimg, battlestatus, battleanim, atype,asel=0;

SDL_Surface *bg, *bmenu;
//SDL_Surface *spells[10]=NULL;

/* Client-specific structures */

/* Global defines */
SDL_Surface *btxt_name=NULL,*btxt_hp=NULL,*btxt_mp=NULL;
//SDL_Surface *btxt_monname=NULL,*btxt_monhp=NULL,*btxt_monmp=NULL; // might be useful for view stat spells.

SDL_Rect btilerect, bmenurect, chargerect;
SDL_Color btextcolor, bhilite;

double bticksLastUpdate=0;

inline void mTile(int x, int y, int tex)
{
     btilerect.x=x;
     btilerect.y=y;
     SDL_BlitSurface(mTiles[tex],NULL,screen,&btilerect);
}

inline void cTile(int x, int y, int tex)
{
     btilerect.x=x;
     btilerect.y=y;
     SDL_BlitSurface(cTiles[tex],NULL,screen,&btilerect);
}

void initbattlescreen()
{
	SDL_Surface *temp;

    tilerect.x=0;
    tilerect.y=0;
    tilerect.w=640;
    tilerect.h=240;
    
    chargerect.x=466;
    chargerect.y=365;
    chargerect.h=16;
 
    char areaimg[30];
	btilerect.w=32;
	btilerect.h=32;
    player.area = 1; //TEMP!!!!
    sprintf(areaimg,"tiles/a/%d.png",player.area);
    temp=IMG_Load(areaimg);
	if (temp!=NULL)
	{
		bg=SDL_DisplayFormat(temp);
		SDL_FreeSurface(temp);
	}else{
		bg=NULL;
	}  
    temp=IMG_Load("tiles/ui/battlemenu.png");
	if (temp!=NULL)
	{
		bmenu=SDL_DisplayFormat(temp);
		SDL_FreeSurface(temp);
	}else{
		bmenu=NULL;
	}

    player.charge=0;
  
    btextcolor.r=255;
    btextcolor.g=255;
    btextcolor.b=255;

    bhilite.r=255;
    bhilite.g=255;
    bhilite.b=0;
  
    bmenurect.x=0;
    bmenurect.y=320;
    bmenurect.w=640;
    bmenurect.h=240;
    battleStatusUpdate();
  
    enemy.image=eimg;
    // text
    
  /* This is the music to play. */
  music = Mix_LoadMUS("sound/1.mid");
  Mix_PlayMusic(music, -1);
}

void destroybattlescreen()
{
     SDL_FreeSurface (bg);
     SDL_FreeSurface (bmenu);
    Mix_HaltMusic();
    Mix_FreeMusic(music);
}

void drawbattlescreen()
{
    SDL_Rect textrect;
    double ticksDiff;
    SDL_Surface *temptxtsurf=NULL;
    int i,j;

    ticksDiff=SDL_GetTicks()-bticksLastUpdate;

    SDL_BlitSurface(bg,NULL,screen,&tilerect);
    SDL_BlitSurface(bmenu,NULL,screen,&bmenurect);
	
    if (battlestatus==0)
    	chargerect.w=(Uint16)((player.charge + (player.speed * (long)ticksDiff/250)) * 0.5);
   	else
    	chargerect.w=(Uint16)(player.charge*.5);
	if (chargerect.w>128) chargerect.w=128;
	SDL_FillRect(screen,&chargerect,SDL_MapRGB(screen->format,0xFF,0xFF,0));

    textrect.x=434;
    textrect.y=336;
    textrect.w=btxt_name->w;
    textrect.h=btxt_name->h;
    SDL_BlitSurface(btxt_name,NULL,screen,&textrect);

    textrect.x=434;
    textrect.y=390;
    textrect.w=btxt_hp->w;
    textrect.h=btxt_hp->h;
    SDL_BlitSurface(btxt_hp,NULL,screen,&textrect);

    textrect.x=434;
    textrect.y=410;
    textrect.w=btxt_mp->w;
    textrect.h=btxt_mp->h;
    SDL_BlitSurface(btxt_mp,NULL,screen,&textrect);

/*    textrect.x=20;
    textrect.y=340;
    textrect.w=btxt_monhp->w;
    textrect.h=btxt_monhp->h;
    SDL_BlitSurface(btxt_monhp,NULL,screen,&textrect);

    textrect.x=100;
    textrect.y=340;
    textrect.w=btxt_monmp->w;
    textrect.h=btxt_monmp->h;
    SDL_BlitSurface(btxt_monmp,NULL,screen,&textrect); */

    if (battlestatus==0) {
    	mTile(100,200,enemy.image);
	    cTile(400,200,player.actualImage);
    } else if (battlestatus==1) { // monster attacking
        if (battleanim<5)
    	   mTile((int)(100+(20*(double)((double)battleanim+(double)ticksDiff/250))),200,enemy.image);
        else if (battleanim>14)
    	   mTile((int)(100+(20*(20-(double)((double)battleanim+(double)ticksDiff/250)))),200,enemy.image);
	    else
    	   mTile(200,200,enemy.image);
	    cTile(400,200,player.actualImage);

        // spell and attack pictures go here

    } else if (battlestatus==2) { // player attacking
    	mTile(100,200,enemy.image);
        if (battleanim<5)
    	   cTile((int)(400-(20*(double)((double)battleanim+(double)ticksDiff/250))),200,player.actualImage);
        else if (battleanim>14)
    	   cTile((int)(400-(20*(20-(double)((double)battleanim+(double)ticksDiff/250)))),200,player.actualImage);
	    else
     	    cTile(300,200,player.actualImage);

        // spell and attack pictures go here - check atype 

    } else if (battlestatus==3) { // monster dead
    	mTile(100,200,254);
	    cTile(400,200,player.actualImage);
    } else if (battlestatus==4) { // player dead!
    	mTile(100,200,enemy.image);
	    mTile(400,200,254);
     }

     for(i=0;i<6;i++)
       for(j=0;j<4;j++)
       if (player.abilities[4*i+j]==1)
       {
         if (asel==4*i+j)
         {
             temptxtsurf=TTF_RenderText_Solid(systemfont,aname[4*i+j],bhilite);
         } else {
             temptxtsurf=TTF_RenderText_Solid(systemfont,aname[4*i+j],btextcolor);
         }
          textrect.x=20+100*j;
          textrect.y=340+20*i;
          textrect.w=temptxtsurf->w;
          textrect.h=temptxtsurf->h;
          SDL_BlitSurface(temptxtsurf,NULL,screen,&textrect);
           SDL_FreeSurface(temptxtsurf);
       }

    SDL_Flip(screen); // Make sure everything is displayed on screen
    SDL_Delay(1); // Don't run too fast 
}


int handlebattlescreen(TCPsocket tcpsock)
{
    unsigned char data[2];
    int retval=0;
    SDL_Event event;

        /* Check for events */
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_KEYUP:
              {
                 if(event.key.keysym.sym == SDLK_ESCAPE)
                 {
                    data[0]='A';
                    data[1]=0xFF;
                    SDLNet_TCP_Send(tcpsock, data, 2);
                    gamestate=0;   
                 }
                 break;
              }
            case SDL_MOUSEBUTTONUP:
            {
                 if (event.button.x>20 && event.button.x<420 && event.button.y>340 && event.button.y<460)
                 {
                    asel=(((int)event.button.x-20)/100) + (4 * (((int)event.button.y-340)/20));
                    if (player.abilities[asel]==0) asel=0;
                 }
                 break;
            }
            case SDL_QUIT:
            {
                data[0]='A';
                data[1]=0xFF;
                SDLNet_TCP_Send(tcpsock, data, 2);
                retval = 1;
                break;
            }
            default:
                break;
            }}
            data[0]=0;
            data[1]=0;
            return retval;
}

int battleStatusUpdate()
{
    char statusmsg[512];

//player text
    if (btxt_name!=NULL)
        SDL_FreeSurface(btxt_name);
    btxt_name=TTF_RenderText_Solid(systemfont,player.name,btextcolor);

    sprintf(statusmsg,"HP: %d / %d",player.hp,player.hpm);
    if (btxt_hp!=NULL)
        SDL_FreeSurface(btxt_hp);
    btxt_hp=TTF_RenderText_Solid(systemfont,statusmsg,btextcolor);

    sprintf(statusmsg,"MP: %d / %d",player.mp,player.mpm);
    if (btxt_mp!=NULL)
        SDL_FreeSurface(btxt_mp);
    btxt_mp=TTF_RenderText_Solid(systemfont,statusmsg,btextcolor);
   

// monster text

	return 0;
}

int do_net_battleupdate(SDLNet_SocketSet set,TCPsocket tcpsock)
{
    int otherinfo,tempincoming;
    unsigned char incoming[2],flags;
    unsigned char dataR[1500],data[2];
    
    int done=0,who;
    
    if(SDLNet_CheckSockets(set, 0))
    {                            
        if(SDLNet_SocketReady(tcpsock))
        {                                      
            SDLNet_TCP_Recv(tcpsock, incoming, 2);
            //printf("Got from server: data %d,%d\n",incoming[0],incoming[1]);
            if (incoming[0]!='B' && incoming[0]!='C' && incoming[0]!='D')
            {
               printf("Server error!  Disconnecting!");
               data[0]='A';
               data[1]=0xFF;
               SDLNet_TCP_Send(tcpsock, data, 2);
               done = 1;   
            }else if (incoming[0]=='C')
            {
               SDLNet_TCP_Recv(tcpsock, dataR, incoming[1]);
//               addToChat(dataR);
            }else if (incoming[0]=='D')   // back to the map!
            {
               SDLNet_TCP_Recv(tcpsock, dataR, incoming[1]);
               player.hp=dataR[0];
               player.hpm=dataR[1];
               player.mp=dataR[2];
               player.mpm=dataR[3];
               player.xp=dataR[4]*256+dataR[5];
               player.gold=dataR[6]*256+dataR[7];
               player.level=dataR[8];
               player.str=dataR[9];
               player.dex=dataR[10];
               player.con=dataR[11];
               player.iq=dataR[12];
               player.wis=dataR[13];
               player.cha=dataR[14];
               player.fort=dataR[15];
               player.reflex=dataR[16];
               player.will=dataR[17];
               player.def=dataR[18];
               player.mdef=dataR[19];
              // for(int i=0;i<24;i++)
              //    player.abilities[i]=dataR[20+i];
               player.area = 0;
               gamestate=10;
            }else{ // must = 'B'
                  SDLNet_TCP_Recv(tcpsock, dataR, incoming[1]);
                  enemy.image=dataR[0];
                  player.actualImage=dataR[1];
                  player.charge=dataR[2];
                  battlestatus=dataR[3];
                  battleanim=dataR[4];
                  player.hp= dataR[5];
                  player.mp= dataR[6];
                  atype= dataR[7];
//                  enemy.monhp= dataR[4];
//                  enemy.monmp= dataR[5];
//             printf("monster battle HP %d\n",dataR[4]);
                  
                  bticksLastUpdate=SDL_GetTicks();
                  battleStatusUpdate();
               data[0]='A';
               data[1]=asel+1;
               SDLNet_TCP_Send(tcpsock, data, 2);
           }
        }
        else
        {
            printf("I think some error occurred");
            done=1;
        }
    }
    return done;
}
