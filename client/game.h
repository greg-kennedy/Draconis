/* game.h - game header goes in here! */

#ifndef _GAME_H_
#define _GAME_H_

#include "../common.h"
/*#include "SDL/SDL.h"
#include "SDL/SDL_net.h"
#include "SDL/SDL_mixer.h"
#include "SDL/SDL_ttf.h"
#include <string.h> */

#define MAXCHAT 15
#define MAXFULLCHAT 28

inline void tile(int, int, int);
inline void cTile(int, int, int);
inline void mTile(int, int, int);
int drawmapmenu();
void drawMap(int layer);
int addToChat (char*);
int statusUpdate();

void initmapscreen();
void destroymapscreen();
void drawmapscreen();
int handlemapscreen(TCPsocket);
void drawChatbox();

int do_net_mapupdate(SDLNet_SocketSet,TCPsocket);

extern SDL_Surface *tempmenusurf;

#endif /* End of _GAME_H_ */
