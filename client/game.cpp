/* game.c - game code goes here too. */
#include "../common.h"
#include "client.h"
#include "game.h"

extern SDL_Surface *screen;
extern int gamestate;
extern Mix_Music *music;   /* SDL_Mixer stuff here */
extern SDL_Surface* tiles[256]; // holds all the tiles loaded in from the tile library
extern SDL_Surface* cTiles[256]; // holds tiles that never change, eg. players
extern SDL_Surface* mTiles[256]; // holds tiles that never change, eg. players
extern int map[3][256][256];
extern TTF_Font *systemfont;
extern struct playerz player;
extern BYTE eimg;

int chatbox=0, startmenu=0;;
char outgoingmessage[80]={0};

/* Client-specific structures */
struct OtherPlayers
{
       BYTE actualImage;
       BYTE matX;
       BYTE matY;
       char moveX;
       char moveY;
} otherplayers[MAX_CLIENTS];

struct Monsters
{
       BYTE actualImage;
       BYTE matX;
       BYTE matY;
       char moveX;
       char moveY;
} monsters[MAX_MONSTERS_IN_AREA];

SDL_Surface *txt_name=NULL,*txt_chat[MAXCHAT]={NULL},*txt_hp=NULL,*txt_mp=NULL,*txt_matX=NULL,*txt_matY=NULL,*txt_selDisp=NULL, *mapmenu=NULL;
SDL_Surface *txt_chatfull[MAXFULLCHAT]={NULL};
int xdir,ydir,xoff,yoff,tempX,tempY;  // vars used for smooth scrolling.
extern int mapX, mapY;
char meMoveX=0, meMoveY=0;
unsigned char hilited=255;
int scopeX, scopeY, camX = 0, camY = 0;
int mapnumber,othersInArea=0;

char chatwin[MAXCHAT][20]={0};
char chatfull[MAXFULLCHAT][80]={0};
SDL_Rect tilerect;
SDL_Rect menurect;
SDL_Color textcolor;

double ticksLastUpdate=0;

inline void tile(int x, int y, int tex)
{
     tilerect.x=x;
     tilerect.y=y;
     SDL_BlitSurface(tiles[tex],NULL,screen,&tilerect);
}

inline void cTile(int x, int y, int tex)
{
     tilerect.x=x;
     tilerect.y=y;
     SDL_BlitSurface(cTiles[tex],NULL,screen,&tilerect);
}

inline void mTile(int x, int y, int tex)
{
     tilerect.x=x;
     tilerect.y=y;
     SDL_BlitSurface(mTiles[tex],NULL,screen,&tilerect);
}

int updateSelDisp(char *text)
{
    if (txt_selDisp!=NULL)
        SDL_FreeSurface(txt_selDisp);
    txt_selDisp=TTF_RenderText_Solid(systemfont,text,textcolor);

	return 0;
}

int statusUpdate()
{
    char statusmsg[512];

    if (txt_name!=NULL)
        SDL_FreeSurface(txt_name);
    txt_name=TTF_RenderText_Solid(systemfont,player.name,textcolor);

    sprintf(statusmsg,"HP: %d / %d",player.hp,player.hpm);
    if (txt_hp!=NULL)
        SDL_FreeSurface(txt_hp);
    txt_hp=TTF_RenderText_Solid(systemfont,statusmsg,textcolor);

    sprintf(statusmsg,"MP: %d / %d",player.mp,player.mpm);
    if (txt_mp!=NULL)
        SDL_FreeSurface(txt_mp);
    txt_mp=TTF_RenderText_Solid(systemfont,statusmsg,textcolor);
    
    
    sprintf(statusmsg,"XP: %d",player.xp);
    if (txt_matX!=NULL)
        SDL_FreeSurface(txt_matX);
    txt_matX=TTF_RenderText_Solid(systemfont,statusmsg,textcolor);
   
    sprintf(statusmsg,"Gold: %d",player.gold);
    if (txt_matY!=NULL)
        SDL_FreeSurface(txt_matY);
    txt_matY=TTF_RenderText_Solid(systemfont,statusmsg,textcolor);
    
	return 0;
}

int addToChat(char* message)
{
    int i;

    for(i=1;i<MAXCHAT;i++)
       strcpy(chatwin[i-1],chatwin[i]);
    strncpy(chatwin[MAXCHAT-1],message,19);

    for(i=0;i<MAXCHAT;i++)
    {
        if (txt_chat[i]!=NULL)
            SDL_FreeSurface(txt_chat[i]);
        txt_chat[i]=TTF_RenderText_Solid(systemfont,chatwin[i],textcolor);
    }

    for(i=1;i<MAXFULLCHAT;i++)
       strcpy(chatfull[i-1],chatfull[i]);
    strncpy(chatfull[MAXFULLCHAT-1],message,79);

    for(i=0;i<MAXFULLCHAT;i++)
    {
        if (txt_chatfull[i]!=NULL)
            SDL_FreeSurface(txt_chatfull[i]);
        txt_chatfull[i]=TTF_RenderText_Solid(systemfont,chatfull[i],textcolor);
    }

	return 0;
}

void initmapscreen()
{
     SDL_Surface *temp;
	tilerect.w=32;
	tilerect.h=32;
	menurect.x=SCREEN_Y;
	menurect.y=0;
    menurect.w=SCREEN_X-SCREEN_Y;
  	menurect.h=SCREEN_Y;	
	
  player.x = 224; 
  player.y = 216;
  scopeX = 9;
  scopeY = 9;
  xoff = 0;//(player.matX-7)*-32;
  yoff = 0;//(player.matY-7)*-32; 
  chatbox=0;

  textcolor.r=255;
  textcolor.g=255;
  textcolor.b=255;
  
  statusUpdate();
  updateSelDisp("Nothing selected.");

  /* This is the music to play. */
  music = Mix_LoadMUS("sound/2.mid");
  Mix_PlayMusic(music, -1);

  temp=IMG_Load("tiles/ui/mapmenu.png");
  mapmenu=SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
}

void destroymapscreen()
{
    Mix_HaltMusic();
    Mix_FreeMusic(music);
    SDL_FreeSurface(mapmenu);
}


void drawmapscreen()
{
    Uint32 color;
    
    double ticksDiff;

    color = SDL_MapRGB (screen->format, 0, 0, 0); // make a black background
    SDL_FillRect (screen, NULL, color);
    
    ticksDiff=SDL_GetTicks()-ticksLastUpdate;
    xoff=(int)((double)meMoveX*ticksDiff/7.8125); // smooth screen scrolling
    yoff=(int)((double)meMoveY*ticksDiff/7.8125);
    
    drawMap(0);
    drawMap(1);

    for(int i = 0; i < MAX_CLIENTS; i++)
            if (otherplayers[i].actualImage!=0)
                    cTile((int)((otherplayers[i].matX+7-player.matX)*32-xoff+(otherplayers[i].moveX*ticksDiff/7.8125)),(int)((otherplayers[i].matY+7-player.matY)*32-yoff+(otherplayers[i].moveY*ticksDiff/7.8125)-8),otherplayers[i].actualImage);

    for(int i = 0; i < MAX_MONSTERS_IN_AREA; i++)
            if (monsters[i].actualImage!=0)
                    mTile((int)((monsters[i].matX+7-player.matX)*32-xoff+(monsters[i].moveX*ticksDiff/7.8125)),(int)((monsters[i].matY+7-player.matY)*32-yoff+(monsters[i].moveY*ticksDiff/7.8125)-8),monsters[i].actualImage);
    
    cTile(player.x,player.y,player.actualImage);
    
    drawMap(2);

    if (hilited!=255)
      if (otherplayers[hilited].actualImage!=0)
        cTile((int)((otherplayers[hilited].matX+7-player.matX)*32-xoff+(otherplayers[hilited].moveX*ticksDiff/7.8125)),(int)((otherplayers[hilited].matY+7-player.matY)*32-yoff+(otherplayers[hilited].moveY*ticksDiff/7.8125)),245);
      else
      {
        hilited=255;
        updateSelDisp("Nothing selected.");
      }

    drawmapmenu();

    if (chatbox==1)
       drawChatbox();

    SDL_Flip(screen); // Make sure everything is displayed on screen
    SDL_Delay(1); // Don't run too fast 
}

void drawChatbox()
{
     int i;
     SDL_Rect fillable;
     SDL_Surface *tempsurfq;

     fillable.x=8;
     fillable.y=8;
     fillable.w=464;
     fillable.h=464;
     SDL_FillRect(screen,&fillable,SDL_MapRGB(screen->format,192,192,192));
     fillable.x=12;
     fillable.y=12;
     fillable.w=456;
     fillable.h=456;
     SDL_FillRect(screen,&fillable,SDL_MapRGB(screen->format,128,128,128));
     fillable.x=16;
     fillable.y=448;
     fillable.w=448;
     fillable.h=16;
     SDL_FillRect(screen,&fillable,SDL_MapRGB(screen->format,192,192,192));
     fillable.x=16;
     fillable.y=16;
     fillable.w=448;
     fillable.h=428;
     SDL_FillRect(screen,&fillable,SDL_MapRGB(screen->format,64,64,64));

    for(i=0;i<MAXFULLCHAT;i++)
    {
        if(txt_chatfull[i]!=NULL)
        {
           fillable.x=18;
           fillable.y=18+(15*i);
           fillable.w=txt_chatfull[i]->w;
           fillable.h=txt_chatfull[i]->h;
           SDL_BlitSurface(txt_chatfull[i],NULL,screen,&fillable);
        }
    }
    
    if (outgoingmessage[0]!='\0')
    {
        tempsurfq =TTF_RenderText_Solid(systemfont,outgoingmessage,textcolor);
        fillable.x=18;
        fillable.y=450;
        fillable.w=tempsurfq->w;
        fillable.h=tempsurfq->h;
        SDL_BlitSurface(tempsurfq,NULL,screen,&fillable);
        SDL_FreeSurface(tempsurfq);
    }

}

void drawMap(int layer)
{
     int a,b,sXS,sXE,sYS,sYE;
   
     if (player.matX-scopeX<0)
        sXS=-player.matX;
     else
         sXS=-scopeX;
     if (player.matX+scopeX>=mapX)
        sXE=mapX - player.matX;
     else
        sXE=scopeX;
     
     if (player.matY-scopeY<0)
        sYS=-player.matY;
     else
         sYS=-scopeY;
     if (player.matY+scopeY>=mapY)
        sYE=mapY - player.matY;
     else
        sYE=scopeY;

     for(a = sXS;a<=sXE;a++)  // draw tiles
         for(b = sYS; b <=sYE; b++)
                   if(map[layer][player.matX+a][player.matY+b] > 0)// && player.matX+b>=0 && player.matY+a>=0 & player.matX+b<mapX && player.matY+a<mapY)
                        tile((scopeX+a-2)*32-xoff,(scopeY+b-2)*32-yoff,map[layer][player.matX+a][player.matY+b]);
}

int drawmapmenu()
{
    SDL_Rect textrect;
    int i;
    
    textrect.x=480;
    textrect.y=0;
    textrect.w=160;
    textrect.h=480;
    SDL_BlitSurface(mapmenu,NULL,screen,&textrect);
    
    cTile(500,20,player.startImage);
    
    textrect.x=540;
    textrect.y=30;
    textrect.w=txt_name->w;
    textrect.h=txt_name->h;
    SDL_BlitSurface(txt_name,NULL,screen,&textrect);

    textrect.x=490;
    textrect.y=70;
    textrect.w=txt_hp->w;
    textrect.h=txt_hp->h;
    SDL_BlitSurface(txt_hp,NULL,screen,&textrect);

    textrect.x=490;
    textrect.y=90;
    textrect.w=txt_mp->w;
    textrect.h=txt_mp->h;
    SDL_BlitSurface(txt_mp,NULL,screen,&textrect);

    textrect.x=490;
    textrect.y=110;
    textrect.w=txt_matX->w;
    textrect.h=txt_matX->h;
    SDL_BlitSurface(txt_matX,NULL,screen,&textrect);

    textrect.x=490;
    textrect.y=130;
    textrect.w=txt_matY->w;
    textrect.h=txt_matY->h;
    SDL_BlitSurface(txt_matY,NULL,screen,&textrect);

    textrect.x=490;
    textrect.y=150;
    textrect.w=txt_selDisp->w;
    textrect.h=txt_selDisp->h;
    SDL_BlitSurface(txt_selDisp,NULL,screen,&textrect);

    for(i=0;i<MAXCHAT;i++)
    {
        if(txt_chat[i]!=NULL)
        {
           textrect.x=484;
           textrect.y=252+(15*i);
           textrect.w=txt_chat[i]->w;
           textrect.h=txt_chat[i]->h;
           SDL_BlitSurface(txt_chat[i],NULL,screen,&textrect);
        }
    }
	return 0;
}

int handlemapscreen(TCPsocket tcpsock)
{
    BYTE data[2];
    char chatmsg[257];
    int retval=0,x,y;
    SDL_Event event;
   
        /* Check for events */
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_KEYDOWN:
              {
                 if(chatbox != 1)           
                    if(event.key.keysym.sym == SDLK_RETURN)
                    { 
                            startmenu = 1;
                            gamestate=9;        
                    }             
                 if (event.key.keysym.sym == SDLK_UP)
                 {
                    if(player.matY-1 >= 0) 
                    {
                          data[0] = 'A';
                          data[1] = 1;
                          SDLNet_TCP_Send(tcpsock, data, 2);
                     }
                 }               
                 else if (event.key.keysym.sym == SDLK_RIGHT)
                 {
                    if(player.matX+1 <= mapX) // faster to check this after the fact its been established whether a key has been pressed
                    {         
                          data[0] = 'A';
                          data[1] = 2;
                          SDLNet_TCP_Send(tcpsock, data, 2);
                    }
                 }
                 else if(event.key.keysym.sym == SDLK_DOWN)
                 {
                    if(player.matY+1 <= mapY) // faster to check this after the fact its been established whether a key has been pressed
                    {   
                          data[0] = 'A';
                          data[1] = 3;
                          SDLNet_TCP_Send(tcpsock, data, 2);
                    }
                 }
                 else if(event.key.keysym.sym == SDLK_LEFT)
                 {
                    if(player.matX-1 >= 0) // faster to check this after the fact its been established whether a key has been pressed
                    { 
                          data[0] = 'A';
                          data[1] = 4;
                          SDLNet_TCP_Send(tcpsock, data, 2);
                    }
                 }else if (chatbox==1)
                 {
                       if (event.key.keysym.sym == SDLK_RETURN)
                       {
                         if (strlen(outgoingmessage)>0)
                         {
                            chatmsg[0]='C';
                            chatmsg[1]=strlen(outgoingmessage)-1;
                            for(x=0;x<strlen(outgoingmessage);x++)
                               chatmsg[x+2]=outgoingmessage[x];
                            SDLNet_TCP_Send(tcpsock,chatmsg,chatmsg[1]+3);
                            memset(outgoingmessage,'\0',80);
                            memset(chatmsg,'\0',257);
                         }
                       }else if (event.key.keysym.sym == SDLK_BACKSPACE)
                       {
                            outgoingmessage[strlen(outgoingmessage)-1]='\0';
                       }else if (strlen(outgoingmessage)<55 && (event.key.keysym.sym>31) && (event.key.keysym.sym < 127))
                       {
                          outgoingmessage[strlen(outgoingmessage)]=(char)(event.key.keysym.sym);// && 0x7F);
                       }
                 }
                 break;
            }
            case SDL_KEYUP:
              {
                 if(event.key.keysym.sym == SDLK_ESCAPE)
                 {
                    if (chatbox==1) chatbox=0; else {
                    data[0]='A';
                    data[1]=0xFF;
                    SDLNet_TCP_Send(tcpsock, data, 2);
                    gamestate=0;
                    }
                 }
                 break;
              }
            case SDL_MOUSEBUTTONUP:
            {
                 if (event.button.x<480)
                 {
                    x=(event.button.x/32)+player.matX-7;
                    y=(event.button.y/32)+player.matY-7;
                    hilited=255;
                    for(int i = 0; i < MAX_CLIENTS; i++)
                      if (otherplayers[i].actualImage!=0)
                         if (otherplayers[i].matX==x && otherplayers[i].matY==y)
                           hilited=i;
                    if (hilited!=255)
                    {
                      data[0]='Q';
                      data[1]=(unsigned char)hilited;
                      SDLNet_TCP_Send(tcpsock, data, 2);
                    }else{
                      updateSelDisp("Nothing selected.");
                    }
                 }else{
                       if (event.button.x>482 && event.button.x<559 && event.button.y>168 && event.button.y<208)
                          if (chatbox==0) chatbox=1; else { memset(outgoingmessage,'\0',80); chatbox=0; }
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

int do_net_mapupdate(SDLNet_SocketSet set,TCPsocket tcpsock)
{
    int otherinfo,tempincoming;
    BYTE incoming[2],flags;
    BYTE dataR[1500],data[2];
    
    int done=0,who,i,j;
    
    if(SDLNet_CheckSockets(set, 0))
    {                            
        if(SDLNet_SocketReady(tcpsock))
        {                                      
            SDLNet_TCP_Recv(tcpsock, incoming, 2);
            if (incoming[0]!='U' && incoming[0]!='C' && incoming[0]!='I' && incoming[0]!='R')
            {
               printf("Server error!  Disconnecting!");
               data[0]='A';
               data[1]=0xFF;
               SDLNet_TCP_Send(tcpsock, data, 2);
               done = 1;
            }else if (incoming[0]=='I')
            {
                  gamestate=11;
                  SDLNet_TCP_Recv(tcpsock, dataR, incoming[1]);
                  for(i=0;i<3;i++)
                    for(j=7;j>=0;j--)
                      if ((int)dataR[i] - (1 << j) >= 0)
                      {
                        player.abilities[8*i+j]=1;
                        dataR[i]=dataR[i]-(1 << j);
                      }else{
                        player.abilities[8*i+j]=0;
                      }
                      //player.abilities[8*i+j]=(dataR[i] && (BYTE)(0x01 << j)) > 0;
                  player.etype=dataR[3];
                  eimg=(player.etype*2)+1;
            }else if (incoming[0]=='R')
            {
                  SDLNet_TCP_Recv(tcpsock, dataR, incoming[1]);
                  dataR[incoming[1]+1]='\0';
                  updateSelDisp((char *)dataR);
            }else if (incoming[0]=='C')
            {
               SDLNet_TCP_Recv(tcpsock, dataR, incoming[1]);
               addToChat((char*)dataR);
            }else{
               othersInArea=0;
		for(who=0;who<MAX_CLIENTS;who++)
			otherplayers[who].actualImage=0;
		for(who=0;who<MAX_MONSTERS_IN_AREA;who++)
			monsters[who].actualImage=0;
               SDLNet_TCP_Recv(tcpsock, dataR, incoming[1]);
               int t = player.area; // Test to see if a new map needs to be loaded
               player.area=dataR[0];

               player.matX=player.matX+meMoveX;
               player.matY=player.matY+meMoveY;
               
               if(t != player.area)
		{
                 load_Map(player.area);
		 player.matX=dataR[1];
		 player.matY=dataR[2];
		 meMoveX=0;
		 meMoveY=0;
		}else{
               tempincoming = dataR[1];
               if (player.matX+1 == tempincoming)
                    meMoveX=1;
               else if (player.matX-1 == tempincoming)
                    meMoveX=-1;
               else
               {
                    meMoveX=0;
                    player.matX=tempincoming;
               }
               tempincoming=dataR[2];
               if (player.matY+1 == tempincoming)
                    meMoveY=1;
               else if (player.matY-1 == tempincoming)
                    meMoveY=-1;
               else
               {
                    meMoveY=0;
                    player.matY=tempincoming;
               }
		}
               player.actualImage=dataR[3];
               player.hp=dataR[4];
               player.mp=dataR[5];
               for(otherinfo=6;otherinfo<incoming[1];otherinfo=otherinfo+5)
               {
                 if (dataR[otherinfo+4] == 1)
                 {
                     othersInArea++;
                     who=dataR[otherinfo];
                     otherplayers[who].matX=otherplayers[who].matX+otherplayers[who].moveX;
                     otherplayers[who].matY=otherplayers[who].matY+otherplayers[who].moveY;
                     otherplayers[who].moveX=0;
                     otherplayers[who].moveY=0;
    
                     tempincoming = dataR[otherinfo+1];
                     if (otherplayers[who].matX == tempincoming-1)
                        otherplayers[who].moveX=1;
                     else if (otherplayers[who].matX == tempincoming+1)
                        otherplayers[who].moveX=-1;
                     else
                        otherplayers[who].matX=tempincoming;
                     tempincoming = dataR[otherinfo+2];
                     if (otherplayers[who].matY == tempincoming-1)
                        otherplayers[who].moveY=1;
                     else if (otherplayers[who].matY == tempincoming+1)
                        otherplayers[who].moveY=-1;
                     else
                        otherplayers[who].matY=tempincoming;
                     otherplayers[who].actualImage = dataR[otherinfo+3];
                     flags=dataR[otherinfo+4];
                 }else{
                     who=dataR[otherinfo];
                     monsters[who].matX=monsters[who].matX+monsters[who].moveX;
                     monsters[who].matY=monsters[who].matY+monsters[who].moveY;
                     monsters[who].moveX=0;
                     monsters[who].moveY=0;
                     tempincoming = dataR[otherinfo+1];
                     if (monsters[who].matX == tempincoming-1)
                        monsters[who].moveX=1;
                     else if (monsters[who].matX == tempincoming+1)
                        monsters[who].moveX=-1;
                     else
                        monsters[who].matX=tempincoming;
                     tempincoming = dataR[otherinfo+2];
                     if (monsters[who].matY == tempincoming-1)
                        monsters[who].moveY=1;
                     else if (monsters[who].matY == tempincoming+1)
                        monsters[who].moveY=-1;
                     else
                        monsters[who].matY=tempincoming;
                     monsters[who].actualImage = dataR[otherinfo+3];
                     flags=dataR[otherinfo+4];
                  }                       
               }
                 statusUpdate();
                 ticksLastUpdate=SDL_GetTicks();
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
