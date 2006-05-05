/* menu.h - menu, title and credits screens */

#ifndef _MENU_H_
#define _MENU_H_


//#include "common.h"
//#include "SDL/SDL.h"
//#include "SDL/SDL_mixer.h"
#include "client.h"

void inittitle();
void destroytitle();
void drawtitle();
int handletitle();

void initmenu();
void destroymenu();
void drawmenu();
int handlemenu();

void initcredits();
void destroycredits();
void drawcredits();
int handlecredits();

void initstartscreen();
void destroystartscreen();
void drawstartscreen(playerz);
int handlestartscreen();

void drawPlainText(char*,int,int);
char* getWpnName(int);
char* getAccName(int);
char* getArmName(int);
char* getHelName(int);
char* getShdName(int);
#endif /* End of _MENU_H_ */
