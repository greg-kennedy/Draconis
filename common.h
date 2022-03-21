#ifndef COMMON_H_
#define COMMON_H_

// these must not exceed 51 (combined)
#define MAX_CLIENTS 8
#define MAX_MONSTERS_IN_AREA 43

#define SCREEN_X 640
#define SCREEN_Y 480

typedef unsigned char BYTE;

struct playerz {
	char name[32];
	BYTE hp, hpm;
	BYTE mp, mpm;
	BYTE level;
	unsigned short xp, gold;
	BYTE str, iq, dex, wis, con, cha;
	BYTE speed, fort, reflex, will, def, mdef;

	BYTE abilities[24];
	BYTE items[60];

	BYTE area, matX, matY, x, y;
	BYTE startImage, actualImage, etype, charge;
};

#endif
