#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "SDL/SDL.h"
#include "SDL/SDL_net.h"
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_mixer.h"
#include "SDL/SDL_image.h"

#include "../common.h"

#include <string.h>

int load_MonsterTiles();
int load_ConstantTiles();
int load_tiles(int); /*
void init_gameword();
void free_tiles(); */
void load_Map(int);
/* void init_gameworld();*/


#endif /* End of _CLIENT_H_ */
