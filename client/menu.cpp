/* menu.c - menu, title and credits screens */

#include "menu.h"

extern SDL_Surface *tempmenusurf;
       
extern SDL_Surface *screen;
extern TTF_Font *systemfont;
extern SDL_Color textcolor;
extern int gamestate;
       int menuoverlayD, startmenumode = 1;
       

extern Mix_Music *music;   /* SDL_Mixer stuff here */


void drawPlainText(char* text, int x, int y)
{
       SDL_Rect textrect;
       SDL_Surface *txt_s=NULL;
       txt_s=TTF_RenderText_Solid(systemfont,text,textcolor);
       textrect.x=x;
       textrect.y=y;
       textrect.w=txt_s->w;
       textrect.h=txt_s->h;
       SDL_BlitSurface(txt_s,NULL,screen,&textrect);   
}

void inittitle()
{
     SDL_Surface *surf=NULL;
     surf=IMG_Load("tiles/ui/title.png");
     tempmenusurf = SDL_DisplayFormat(surf);
     SDL_FreeSurface(surf);
     menuoverlayD = SDL_GetTicks();
     /* This is the music to play. */
	 music = Mix_LoadMUS("sound/1.mid");
     Mix_PlayMusic(music, -1);
}

void destroytitle()
{
	 SDL_FreeSurface(tempmenusurf);
}

void drawtitle()
{
	SDL_BlitSurface(tempmenusurf,NULL,screen,NULL);
    /* Make sure everything is displayed on screen */
    int temp = SDL_GetTicks();
    if(temp - menuoverlayD > 10000) // if wait to long go to credits
        gamestate = 2;
    SDL_Flip (screen);
    /* Don't run too fast */
    SDL_Delay (1);
}

int handletitle()
{
    SDL_Event event;
    int retval=0;

    /* Check for events */
    while (SDL_PollEvent (&event))
    {
        switch (event.type)
        {
        case SDL_KEYUP:
             gamestate=1;
            break;
        case SDL_MOUSEBUTTONUP:
             gamestate=1;
             break;
        case SDL_QUIT:
            retval = 1;
            break;
        default:
            break;
        }
    }
    return retval;
}

void initmenu()
{
     SDL_Surface *surf=NULL;
     surf=IMG_Load("tiles/ui/menu.png");
     tempmenusurf = SDL_DisplayFormat(surf);
     SDL_FreeSurface(surf);

}

void destroymenu()
{
	 SDL_FreeSurface(tempmenusurf);
     Mix_HaltMusic();
     Mix_FreeMusic(music);
}

void drawmenu()
{
	SDL_BlitSurface(tempmenusurf,NULL,screen,NULL);
    /* Make sure everything is displayed on screen */
    SDL_Flip (screen);
    /* Don't run too fast */
    SDL_Delay (1);
}

int handlemenu()
{
    SDL_Event event;
    int retval=0;

    /* Check for events */
    while (SDL_PollEvent (&event))
    {
        switch (event.type)
        {
        case SDL_KEYUP:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                retval=1;
            break;
        case SDL_MOUSEBUTTONUP:
             if (event.button.y>140 && event.button.y<170)
                gamestate=3;
             else if (event.button.y>275 && event.button.y<305)
                gamestate=2;
             else if (event.button.y>340 && event.button.y<370)
                retval=1;
             break;
        case SDL_QUIT:
            retval = 1;
            break;
        default:
            break;
        }
    }
    return retval;
}

void initcredits()
{
     SDL_Surface *surf=NULL;
     surf=IMG_Load("tiles/ui/credits.png");
     tempmenusurf = SDL_DisplayFormat(surf);
     SDL_FreeSurface(surf);

}

void destroycredits()
{
	 SDL_FreeSurface(tempmenusurf);
}

void drawcredits()
{
	SDL_BlitSurface(tempmenusurf,NULL,screen,NULL);
    /* Make sure everything is displayed on screen */
    SDL_Flip (screen);
    /* Don't run too fast */
    SDL_Delay (1);
}

int handlecredits()
{
    SDL_Event event;
    int retval=0;

    /* Check for events */
    while (SDL_PollEvent (&event))
    {
        switch (event.type)
        {
        case SDL_KEYUP:
            gamestate=1;
            break;
        case SDL_MOUSEBUTTONUP:
             gamestate=1;
             break;
        case SDL_QUIT:
            retval = 1;
            break;
        default:
            break;
        }
    }
    return retval;
}



void initstartscreen()
{
     SDL_Surface *surf=NULL;
     surf=IMG_Load("tiles/ui/start.png");
     tempmenusurf = SDL_DisplayFormat(surf);
     SDL_FreeSurface(surf);

}

void destroystartscreen()
{
	 SDL_FreeSurface(tempmenusurf);
     Mix_HaltMusic();
     Mix_FreeMusic(music);
}

void drawstartscreen(playerz play)
{
    char status[30];
           
	SDL_BlitSurface(tempmenusurf,NULL,screen,NULL);
    
    if(startmenumode == 1){
       sprintf(status,"STATS");
       drawPlainText(status,184,30);
       sprintf(status,"NAME %s",play.name);
       drawPlainText(status,184,50);                
       sprintf(status,"HP %d / %d",play.hp,play.hpm);
       drawPlainText(status,184,70);
       sprintf(status,"MP %d / %d",play.mp,play.mpm);
       drawPlainText(status,184,90);
       sprintf(status,"LVL  %d",play.level);
       drawPlainText(status,184,120);      
       sprintf(status,"XP   %d",play.xp);
       drawPlainText(status,250,120);
       sprintf(status,"GOLD %d",play.gold);
       drawPlainText(status,184,140);
       sprintf(status,"STR %d",play.str);
       drawPlainText(status,184,170);  
       sprintf(status,"INT %d",play.iq);
       drawPlainText(status,250,170);    
       sprintf(status,"DEX %d",play.dex);
       drawPlainText(status,184,190);  
       sprintf(status,"WIS %d",play.wis);
       drawPlainText(status,250,190);
       sprintf(status,"CON %d",play.con);
       drawPlainText(status,184,210);
       sprintf(status,"CHA %d",play.cha);
       drawPlainText(status,250,210);
       
    }else if(startmenumode == 2){
       sprintf(status,"EQUIPMENT");
       drawPlainText(status,184,30);
       sprintf(status,"WEAPON");
       drawPlainText(status,184,50);
       for(int i=0; i<5; i++){  
          //sprintf(status,"%d: %s",i,getWpnName(play.weapon[i]));
          sprintf(status,"%d: ",i); //TEMP
          drawPlainText(status,184,70+20*i+1);}    
       sprintf(status,"SHIELD");
       drawPlainText(status,184,170);  
       sprintf(status,"HELMET");
       drawPlainText(status,360,170);  
       for(int i=0; i<5; i++){
          //sprintf(status,"%d: %s",i,getArmName(play.armor[i]));
          sprintf(status,"%d: ",i); //TEMP
          drawPlainText(status,184,190+20*i+1);}    
       for(int i=0; i<5; i++){
          //sprintf(status,"%d: %s",i,getShdName(play.shield[i]));
          sprintf(status,"%d: ",i); //TEMP
          drawPlainText(status,360,190+20*i+1);}
       sprintf(status,"HELMET");
       drawPlainText(status,184,300);  
       sprintf(status,"ACCESSORY");
       drawPlainText(status,360,300);    
       for(int i=0; i<5; i++){
          //sprintf(status,"%d: %s",i,getHelName(play.helmet[i]));
          sprintf(status,"%d: ",i+1); //TEMP
          drawPlainText(status,184,320+20*i);}    
       for(int i=0; i<5; i++){
          //sprintf(status,"%d: %s",i,getAccName(play.acces[i]));
          sprintf(status,"%d: ",i+1); //TEMP
          drawPlainText(status,360,320+20*i);}          
    }else if(startmenumode == 3){
       sprintf(status,"ABILITIES");
       drawPlainText(status,184,30);
       for(int i=0; i<12; i++){     
         sprintf(status,"%d: %d",i+1,play.abilities[i]);
         drawPlainText(status,184,50+i*20); } 
       for(int i=12; i<24; i++){     
         sprintf(status,"%d: %d",i+1,play.abilities[i]);
         drawPlainText(status,320,50+(i-12)*20); }      
    }else if(startmenumode == 4){
       sprintf(status,"ITEMS");
       drawPlainText(status,184,30); 
       for(int i=0; i<20; i++){     
         sprintf(status,"%d: %d",i+1,play.items[i]);
         drawPlainText(status,184,50+i*20); } 
       for(int i=20; i<40; i++){     
         sprintf(status,"%d: %d",i+1,play.items[i]);
         drawPlainText(status,334,50+(i-20)*20); }   
       for(int i=40; i<60; i++){     
         sprintf(status,"%d: %d",i+1,play.items[i]);
         drawPlainText(status,494,50+(i-40)*20); } 
    }

    /* Make sure everything is displayed on screen */
    SDL_Flip (screen);
    /* Don't run too fast */
    SDL_Delay (1);
    
}

int handlestartscreen()
{
    SDL_Event event;
    int retval=0;

    /* Check for events */
    while (SDL_PollEvent (&event))
    {
        switch (event.type)
        {
        case SDL_KEYUP:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                retval=1;
            break;
        case SDL_MOUSEBUTTONUP:
             if (event.button.x>30 && event.button.x<110 &&event.button.y>25 && event.button.y<50){
                startmenumode=1;
             }else  if (event.button.x>30 && event.button.x<110 &&event.button.y>64 && event.button.y<84){
                startmenumode=2;
             }else  if (event.button.x>30 && event.button.x<110 &&event.button.y>98 && event.button.y<118){
                startmenumode=3;
             }else  if (event.button.x>30 && event.button.x<110 &&event.button.y>128 && event.button.y<158){
                startmenumode=4;
             }else  if (event.button.x>30 && event.button.x<110 &&event.button.y>168 && event.button.y<188){
                startmenumode=1;
                gamestate=10;
             }
             
             break;
        case SDL_QUIT:
            retval = 1;
            break;
        default:
            break;
        }
    }
    return retval;
}

char* getWpnName(int i)
{
     char* s = NULL;
     sprintf(s,"none");
     if(i == 1) 
       sprintf(s,"wood stick");
     else  if(i == 2) 
       sprintf(s,"wood club");
     return s;     
}

char* getArmName(int i)
{
     char* s = NULL;
     sprintf(s,"none");
     if(i == 1) 
       sprintf(s,"cloth");
     else  if(i == 2) 
       sprintf(s,"leather");
     return s;     
}

char* getHelName(int i)
{
     char* s = NULL;
     sprintf(s,"none");
     if(i == 1) 
       sprintf(s,"cloth");
     else  if(i == 2) 
       sprintf(s,"leather");
     return s;     
}

char* getShdName(int i)
{
     char* s = NULL;
     sprintf(s,"none");
     if(i == 1) 
       sprintf(s,"wood");
     else  if(i == 2) 
       sprintf(s,"aluminum");
     return s;     
}

char* getAccName(int i)
{
     char* s = NULL;
     sprintf(s,"none");
     if(i == 1) 
       sprintf(s,"power glove (+2)");
     else  if(i == 2) 
       sprintf(s,"rogue boots (+2)");
     return s;     
}
