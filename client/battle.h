/* battle.h - battle header goes in here! */

#ifndef _BATTLE_H_
#define _BATTLE_H_

//#include "common.h"
#include "SDL/SDL.h"
#include "SDL/SDL_net.h"
#include "SDL/SDL_mixer.h"
#include "SDL/SDL_ttf.h"
#include <string.h>

#include "client.h"

/*inline void mtile(int, int, int);
inline void ctile(int, int, int);*/
/*int drawbattlemenu();
int addToChat (char*);
int statusUpdate();*/

//inline void tile(int, int, int);
inline void cTile(int, int, int);
inline void mTile(int, int, int);

void initbattlescreen();
void destroybattlescreen();
void drawbattlescreen();
int battleStatusUpdate();
int handlebattlescreen(TCPsocket);

int do_net_battleupdate(SDLNet_SocketSet,TCPsocket);

#endif /* End of _BATTLE_H_ */
