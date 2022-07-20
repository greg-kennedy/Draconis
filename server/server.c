#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include <sys/time.h>

#include "../common.h"

#define PORT "5009"   // port we're listening on

#define NUM_AREA 4	// how many areas we have maps for

enum client_state {
	none,
	await_username,
	await_password,
	active,

	battle,
	battle_mon_attack,
	battle_player_attack,
	battle_mon_dead,
	battle_player_dead
};

struct client_connection {
	enum client_state state;
	int fd;

	unsigned char action;
	unsigned char state_detail;

	char password[32];
	struct playerz player;
} client[MAX_CLIENTS] = {{0}};

// map and area-related objects
struct trigger {
	unsigned char type;
	unsigned char x, y;
	unsigned char param[5];
};

struct monster {
	unsigned char state, brain, x, y, type, hp;
};

struct area {
	unsigned int w, h;
	unsigned char * collision;

	unsigned char flags;

	unsigned int trigger_count;
	struct trigger * triggers;

	struct monster monsters[MAX_MONSTERS_IN_AREA];
} areas[NUM_AREA] = {{0}};

// get sockaddr, IPv4 or IPv6:
void * get_in_addr(struct sockaddr * sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr);

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// Check if a move is valid or not.
//  Returns a 2-byte block.
//  First byte is a collision type:
//    Returns 0 if the space is empty,
//    1 if it is blocked by a wall or OOB,
//    2 if it is occupied by a player,
//    3 if it is occupied by a monster.
//    4 if there's a trigger in this spot
//  Second byte is an ID for types 2, 3 and 4
static const unsigned char * const check_move(const int targA, const int targX, const int targY)
{
	static unsigned char ret[2] = { '\0' };
	const struct area * const a = &areas[targA];

	// no moving out of bounds
	if (targX < 0 || targX >= a->w ||
		targY < 0 || targY >= a->h ||
		// blocked area
		a->collision[targY * a->w + targX]) {
		ret[0] = 0x01;
		return ret;
	}

	// collision with a player
	for (int i = 0; i < MAX_CLIENTS; i ++) {
		if (client[i].state == active &&
			targA == client[i].player.area &&
			targX == client[i].player.matX && targY == client[i].player.matY) {
			ret[0] = 0x02;
			ret[1] = i;
			return ret;
		}
	}

	// collision with a monster
	if (a->flags) {
		// monsters only exist if flags set
		for (int i = 0; i < MAX_MONSTERS_IN_AREA; i ++) {
			if (a->monsters[i].state &&
				targX == a->monsters[i].x && targY == a->monsters[i].y) {
				// monsters in battle are impassable
				if (a->monsters[i].state == 2)
					ret[0] = 0x01;
				else
					ret[0] = 0x03;

				ret[1] = i;
				return ret;
			}
		}
	}

	// collision with a trigger
	for (int i = 0; i < a->trigger_count; i ++) {
		if (targX == a->triggers[i].x &&
			targY == a->triggers[i].y) {
			ret[0] = 0x04;
			ret[1] = i;
			return ret;
		}
	}

	// free square
	ret[0] = 0;
	return ret;
}

int main(void)
{
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	int fdmax;        // maximum file descriptor number
	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	unsigned char buf[259];    // buffer for client data
	int nbytes;
	char remoteIP[INET6_ADDRSTRLEN];
	int yes = 1;      // for setsockopt() SO_REUSEADDR, below
	int i, j, rv;
	struct addrinfo hints, *ai, *p;
	signal(SIGPIPE, SIG_IGN);
	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}

	for (p = ai; p != NULL; p = p->ai_next) {
		// get the pointer to the address itself,
		// different fields in IPv4 and IPv6:
		void * addr;
		char * ipver;
		char ipstr[INET6_ADDRSTRLEN];

		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in * ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		} else { // IPv6
			struct sockaddr_in6 * ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}

		// convert the IP to a string and print it:
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		printf("  %s: %s\n", ipver, ipstr);
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (listener < 0)
			continue;

		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this

	// listen
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(3);
	}

	// add the listener to the master set
	FD_SET(listener, &master);
	// keep track of the biggest file descriptor
	fdmax = listener; // so far, it's this one

	// load maps
	for (int i = 0; i < NUM_AREA; i ++) {
		FILE * fp;
		char str[20];
		sprintf(str, "maps/%d.clp", i);
		fp = fopen(str, "rb");

		if (fp == NULL) {
			perror("fopen");
			continue;
		}

		areas[i].w = 1 + fgetc(fp) + fgetc(fp) * 256;
		areas[i].h = 1 + fgetc(fp) + fgetc(fp) * 256;
		areas[i].flags = fgetc(fp);
		areas[i].trigger_count = fgetc(fp);

		if (areas[i].trigger_count) {
			areas[i].triggers = malloc(areas[i].trigger_count * sizeof(struct trigger));

			for (int j = 0; j < areas[i].trigger_count; j ++) {
				areas[i].triggers[j].type = fgetc(fp);
				areas[i].triggers[j].x = fgetc(fp);
				areas[i].triggers[j].y = fgetc(fp);
				fread(areas[i].triggers[j].param, 1, 5, fp);
			}
		} else
			areas[i].triggers = NULL;

		printf("MAP %d: %u by %u, flags=%u, trigger count %u\n", i, areas[i].w, areas[i].h, areas[i].flags, areas[i].trigger_count);
		areas[i].collision = malloc(areas[i].h * areas[i].w);
		int read_bytes = fread(areas[i].collision, 1, areas[i].h * areas[i].w, fp);

		if (read_bytes != areas[i].h * areas[i].w) {
			printf("read_bytes %d != size %d\n", read_bytes, areas[i].h * areas[i].w);
			perror("fread");
		}

		fclose(fp);
	}

	// game timers
	struct timeval tv_now, tv_next;
	gettimeofday(&tv_now, NULL);
	//printf("The time is %d.%06d sec\n", tv_now.tv_sec, tv_now.tv_usec);
	tv_next.tv_sec = tv_now.tv_sec;
	tv_next.tv_usec = tv_now.tv_usec + 250000;

	if (tv_next.tv_usec > 999999) {
		tv_next.tv_sec ++;
		tv_next.tv_usec -= 1000000;
	}

	//printf("Next time is %d.%06d sec\n", tv_next.tv_sec, tv_next.tv_usec);
	// main loop
	unsigned int frame = 0;

	for (;;) {
		// set timer for remaining duration between now and next tick
		struct timeval tv;

		if (tv_now.tv_usec > tv_next.tv_usec) {
			tv.tv_sec = tv_next.tv_sec - tv_now.tv_sec - 1;
			tv.tv_usec = tv_next.tv_usec + 1000000 - tv_now.tv_usec;
		} else {
			tv.tv_sec = tv_next.tv_sec - tv_now.tv_sec;
			tv.tv_usec = tv_next.tv_usec - tv_now.tv_usec;
		}

		//printf("Sleeping %d.%06d sec\n", tv.tv_sec, tv.tv_usec);
		read_fds = master; // copy it

		if (select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1) {
			perror("select");
			exit(4);
		}

		// run through the existing connections looking for data to read
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // we got one!!
				if (i == listener) {
					// handle new connections
					addrlen = sizeof remoteaddr;
					newfd = accept(listener,
							(struct sockaddr *)&remoteaddr,
							&addrlen);

					if (newfd == -1)
						perror("accept");
					else {
						printf("selectserver: new connection from %s on "
							"socket %d\n",
							inet_ntop(remoteaddr.ss_family,
								get_in_addr((struct sockaddr *)&remoteaddr),
								remoteIP, INET6_ADDRSTRLEN),
							newfd);

						// set up the client connection
						for (j = 0; j < MAX_CLIENTS; j ++) {
							if (client[j].state == none) {
								// found an empty slot we can set up
								client[j].state = await_username;
								client[j].fd = newfd;
								FD_SET(newfd, &master); // add to master set

								if (newfd > fdmax)      // keep track of the max
									fdmax = newfd;

								// acknowledge a connection by sending 'K'
								if (send(newfd, "K", 1, 0) == -1)
									perror("send");

								break;
							}
						}

						if (j == MAX_CLIENTS) {
							// failed to find empty slot
							close(newfd);
						}
					}
				} else {
					// handle data from a client
					//  find their game state first based on the fd
					int id;

					for (id = 0; id < MAX_CLIENTS; id ++) {
						if (client[id].fd == i) break;
					}

					if (id == MAX_CLIENTS) {
						// failed to match socket descriptor to any known clients
						//  that's weird right?
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
					} else {
						unsigned int expect;

						if (client[id].state == await_username || client[id].state == await_password)
							expect = 32;
						else
							expect = 2;

						if ((nbytes = recv(i, buf, expect, 0)) <= 0) {
							// got error or connection closed by client
							if (nbytes == 0) {
								// connection closed
								printf("selectserver: socket %d hung up\n", i);
							} else
								perror("recv");

							client[id].state = none;
							client[id].fd = 0;
							close(i); // bye!
							FD_CLR(i, &master); // remove from master set
						} else {
							// we got some data from a client
							if (client[id].state == await_username) {
								// got username, awaiting password
								strncpy(client[id].player.name, buf, 31);
								client[id].player.name[31] = '\0';
								printf("Client %d (socket %d) sent username '%s'\n", id, i, client[id].player.name);
								// verify user not already logged in
								client[id].state = await_password;

								for (j = 0; j < MAX_CLIENTS; j++) {
									if (id != j &&
										client[j].state != none &&
										client[j].state != await_username &&
										strcmp(client[id].player.name, client[j].player.name) == 0) {
										// already here, kick em out
										printf("Client %d (socket %d): username '%s' already in game (client %d, socket %d)\n", id, i, client[id].player.name, j, client[j].fd);

										if (send(newfd, "F", 1, 0) == -1)
											perror("send");

										client[id].state = none;
										client[id].fd = 0;
										close(i); // bye!
										FD_CLR(i, &master); // remove from master set
										break;
									}
								}
							} else if (client[id].state == await_password) {
								// got password, try login
								strncpy(client[id].password, buf, 31);
								client[id].password[31] = '\0';
								printf("Client %d (socket %d, username '%s') sent password '%s'\n", id, i, client[id].player.name, "****");
								// open file
								sprintf(buf, "players/%s.txt", client[id].player.name);
								FILE * fp = fopen(buf, "r");

								if (! fp) {
									perror("fopen");
									printf("Client %d (socket %d): username '%s' does not exist\n", id, i, client[id].player.name);

									if (send(newfd, "F", 1, 0) == -1)
										perror("send");

									client[id].state = none;
									client[id].fd = 0;
									close(i); // bye!
									FD_CLR(i, &master); // remove from master set
								} else {
									fscanf(fp, "%s", buf);

									if (strncmp(client[id].password, buf, 31) != 0) {
										printf("Client %d (socket %d, username '%s') invalid password\n", id, i, client[id].player.name);

										// invalid password
										if (send(newfd, "F", 1, 0) == -1)
											perror("send");

										client[id].state = none;
										client[id].fd = 0;
										close(i); // bye!
										FD_CLR(i, &master); // remove from master set
									} else {
										printf("Client %d (socket %d, username '%s') successfully logged in\n", id, i, client[id].player.name);
										// login OK, read rest of character data
										fscanf(fp, "%hhu", &client[id].player.area);
										fscanf(fp, "%hhu", &client[id].player.matX);
										fscanf(fp, "%hhu", &client[id].player.matY);
										fscanf(fp, "%hhu", &client[id].player.hp);
										fscanf(fp, "%hhu", &client[id].player.hpm);
										fscanf(fp, "%hhu", &client[id].player.mp);
										fscanf(fp, "%hhu", &client[id].player.mpm);
										fscanf(fp, "%hhu", &client[id].player.startImage);
										client[id].player.actualImage = client[id].player.startImage;
										fscanf(fp, "%hhu", &client[id].player.speed);
										fscanf(fp, "%hu", &client[id].player.xp);
										fscanf(fp, "%hu", &client[id].player.gold);
										fscanf(fp, "%hhu", &client[id].player.level);
										fscanf(fp, "%hhu", &client[id].player.str);
										fscanf(fp, "%hhu", &client[id].player.dex);
										fscanf(fp, "%hhu", &client[id].player.con);
										fscanf(fp, "%hhu", &client[id].player.iq);
										fscanf(fp, "%hhu", &client[id].player.wis);
										fscanf(fp, "%hhu", &client[id].player.cha);
										fscanf(fp, "%hhu", &client[id].player.fort);
										fscanf(fp, "%hhu", &client[id].player.reflex);
										fscanf(fp, "%hhu", &client[id].player.will);
										fscanf(fp, "%hhu", &client[id].player.def);
										fscanf(fp, "%hhu", &client[id].player.mdef);

										for (j = 0; j < 24; j++)
											fscanf(fp, "%hhu", &client[id].player.abilities[j]);

										for (j = 0; j < 60; j++)
											fscanf(fp, "%hhu", &client[id].player.items[j]);

										//build "init" packet
										buf[0] = 'I';
										buf[1] = client[id].player.area;
										buf[2] = client[id].player.matX;
										buf[3] = client[id].player.matY;
										buf[4] = client[id].player.hp;
										buf[5] = client[id].player.hpm;
										buf[6] = client[id].player.mp;
										buf[7] = client[id].player.mpm;
										buf[8] = client[id].player.startImage;
										buf[9] = client[id].player.speed;
										buf[10] = client[id].player.xp / 256;
										buf[11] = client[id].player.xp % 256;
										buf[12] = client[id].player.gold / 256;
										buf[13] = client[id].player.gold % 256;
										buf[14] = client[id].player.level;
										buf[15] = client[id].player.str;
										buf[16] = client[id].player.dex;
										buf[17] = client[id].player.con;
										buf[18] = client[id].player.iq;
										buf[19] = client[id].player.wis;
										buf[20] = client[id].player.cha;
										buf[21] = client[id].player.fort;
										buf[22] = client[id].player.reflex;
										buf[23] = client[id].player.will;
										buf[24] = client[id].player.def;
										buf[25] = client[id].player.mdef;

										for (j = 0; j < 24; j++)
											buf[26 + j] = client[id].player.abilities[j];

										for (j = 0; j < 60; j++)
											buf[50 + j] = client[id].player.items[j];

										// invalid password
										if (send(newfd, buf, 110, 0) == -1) {
											perror("send");
											client[id].state = none;
											client[id].fd = 0;
											close(i); // bye!
											FD_CLR(i, &master); // remove from master set
										} else
											client[id].state = active;
									}

									fclose(fp);
								}
							} else {
								if (buf[0] == 'A') {
									// action
									if (buf[1] == 0xFF) {
										printf("Client %d (socket %d, username '%s') requested disconnect\n", id, i, client[id].player.name);
										client[id].state = none;
										client[id].fd = 0;
										close(i); // bye!
										FD_CLR(i, &master); // remove from master set
									}

									// queue movement
									client[id].action = buf[1];
								} else if (buf[0] == 'Q') {
									// query
									j = buf[1];

									if (client[j].state == active) {
										snprintf(buf, 259, "R %s", client[j].player.name);
										buf[1] = strlen(buf) - 1;

										//printf("QUERY: Sending %u bytes, '%s'\n", buf[1]+2, buf);
										if (send(i, buf, buf[1] + 2, 0) == -1) {
											perror("send");
											client[id].state = none;
											client[id].fd = 0;
											close(i); // bye!
											FD_CLR(i, &master); // remove from master set
										}
									}
								} else if (buf[0] == 'C') {
									// chat - read rest of bytes
									char message[257] = {'\0'};

									if ((nbytes = recv(i, message, buf[1] + 1, 0)) <= 0) {
										// got error or connection closed by client
										if (nbytes == 0) {
											// connection closed
											printf("selectserver: socket %d hung up\n", i);
										} else
											perror("recv");

										client[id].state = none;
										client[id].fd = 0;
										close(i); // bye!
										FD_CLR(i, &master); // remove from master set
									} else {
										// relay chat to everyone
										printf("Client %d (socket %d, username '%s'): chat '%s' (%u bytes)\n", id, i, client[id].player.name, message, buf[1] + 1);
										// build chat message
										snprintf(buf, 259, "C %s> %s", client[id].player.name, message);
										buf[1] = strlen(buf) - 1;

										//printf("CHAT: Sending %u bytes, '%s'\n", buf[1]+2, buf);
										for (j = 0; j < MAX_CLIENTS; j ++) {
											if (client[j].state == active) {
												if (send(client[j].fd, buf, buf[1] + 2, 0) == -1) {
													perror("send");
													client[j].state = none;
													close(client[j].fd);
													FD_CLR(client[j].fd, &master); // remove from master set
													client[j].fd = 0;
												}
											}
										}
									}
								}
							}
						}
					}
				} // END handle data from client
			} // END got new incoming connection
		} // END looping through file descriptors

		// ADVANCE CLOCK and HANDLE GAME UPDATES
		gettimeofday(&tv_now, NULL);

		if (tv_now.tv_sec > tv_next.tv_sec ||
			(tv_now.tv_sec == tv_next.tv_sec && tv_now.tv_usec > tv_next.tv_usec)) {
			// set timer for next update
			tv_next.tv_usec += 250000;

			if (tv_next.tv_usec > 999999) {
				tv_next.tv_sec ++;
				tv_next.tv_usec -= 1000000;
			}

			// process any client actions
			for (int i = 0; i < MAX_CLIENTS; i ++) {
				if (client[i].state == active) {
					if (client[i].action) {
						// client wants to move
						//  record their starting and target coords
						int targX = client[i].player.matX;
						int targY = client[i].player.matY;
						int targImg;

						switch (client[i].action) {
						case 1:
							targY --;
							targImg = 6;
							break;

						case 2:
							targX ++;
							targImg = 4;
							break;

						case 3:
							targY ++;
							targImg = 0;
							break;

						case 4:
							targX --;
							targImg = 2;
							break;
						}

						const unsigned char * movable = check_move(client[i].player.area, targX, targY);

						// space seems free for movement
						if (movable[0] == 0) {
							client[i].player.matX = targX;
							client[i].player.matY = targY;
							client[i].player.actualImage = client[i].player.startImage + targImg;
						} else if (movable[0] == 0x03) {
							// walked into monster!
							//  send them the Battle Init
							// locally. etype is the ID number of the monster we hit
							client[i].player.etype = movable[1];
							struct monster * m = &areas[client[i].player.area].monsters[movable[1]];
							buf[0] = 'I';
							buf[1] = 0x04;
							// copy player abilitie (bitstream)
							buf[2] = buf[3] = buf[4] = 0;

							for (j = 0; j < 24; j ++) {
								if (client[i].player.abilities[j])
									buf[2 + (j / 8)] |= (1 << (j % 8));
							}

							// remotely. etype is the TYPE of monster we're fighting
							buf[5] = m->type;

							if (send(client[i].fd, buf, buf[1] + 2, 0) == -1) {
								perror("send");
								client[i].state = none;
								close(client[i].fd);
								FD_CLR(client[i].fd, &master); // remove from master set
								client[i].fd = 0;
							}

							// put the Client into Battle (both charge)
							client[i].state = battle;
							client[i].player.charge = 0;
							// put the Monster into Battle too, reset its initial charge (brain)
							m->state = 2;
							m->brain = 0;
							// give HP, how about 5 * eType + 1 (higher types = tougher enemy)
							m->hp = (1 + m->type);
						} else if (movable[0] == 0x04) {
							// trigger
							if (areas[client[i].player.area].triggers[movable[1]].type == 0x01) {
								// teleport
								client[i].player.matX = areas[client[i].player.area].triggers[movable[1]].param[1];
								client[i].player.matY = areas[client[i].player.area].triggers[movable[1]].param[2];
								client[i].player.area = areas[client[i].player.area].triggers[movable[1]].param[0];
							}
						}

						// clear any client action
						client[i].action = 0;
					}
				} else if (client[i].state >= battle) {
					// depending on battle state things might happen!
					struct monster * m = &areas[client[i].player.area].monsters[client[i].player.etype];

					switch (client[i].state) {
					case battle:

						// increment the player's charge meter
						if (client[i].player.charge + client[i].player.speed > 255) {
							// can make attack!
							client[i].state = battle_player_attack;
							client[i].state_detail = 0;
						} else
							// increment charge counter
							client[i].player.charge += client[i].player.speed;

						// increment the enemy's charge meter
						if (m->brain + (m->type * 10) > 255) {
							client[i].state = battle_mon_attack;
							client[i].state_detail = 0;
						} else
							m->brain += (m->type * 10);

						break;

					case battle_mon_attack:
						// monster is making an attack
						//  the state counter increases from 0 to 19
						client[i].state_detail ++;

						// at 20 damage is dealt
						if (client[i].state_detail == 20) {
							if (m->type >= client[i].player.hp) {
								client[i].player.hp = 0;
								client[i].state = battle_player_dead;
							} else {
								client[i].player.hp -= m->type;
								client[i].state = battle;
								m->brain = 0;
							}
						}

						break;

					case battle_player_attack:
						// player is attacking the monster
						client[i].state_detail ++;

						// at 20 damage is dealt
						if (client[i].state_detail == 20) {
							if (client[i].player.level >= m->hp) {
								m->hp = 0;
								client[i].state = battle_mon_dead;
							} else {
								m->hp -= client[i].player.level;
								client[i].state = battle;
								client[i].player.charge = 0;
							}
						}

						break;

					case battle_mon_dead:
						// monster dead, let's wait a moment
						client[i].state_detail ++;

						// dead monster
						if (client[i].state_detail == 40) {
							// ok well return player to active
							//  give some XP haha
							client[i].player.xp += m->type;
							client[i].state = active;
							// monster dead
							m->state = 0;
							m->brain = 0;
						}

						break;

					case battle_player_dead:
						// player died, give some time to process that
						client[i].state_detail ++;

						// dead player means the monster goes back to patrolling
						//  have to do something about the player being dead tho
						if (client[i].state_detail == 40) {
							m->state = 1;
							m->brain = 0;
							// condolences in chat
							snprintf(buf, 259, "C %s has been killed!", client[i].player.name);
							buf[1] = strlen(buf) - 1;

							//printf("CHAT: Sending %u bytes, '%s'\n", buf[1]+2, buf);
							for (j = 0; j < MAX_CLIENTS; j ++) {
								if (client[j].state >= active) {
									if (send(client[j].fd, buf, buf[1] + 2, 0) == -1) {
										perror("send");
										client[j].state = none;
										close(client[j].fd);
										FD_CLR(client[j].fd, &master); // remove from master set
										client[j].fd = 0;
									}
								}
							}

							// send player back to the Spawn Room
							client[i].player.area = client[i].player.matX = client[i].player.matY = 1;
							client[i].player.hp = client[i].player.hpm;
							client[i].player.mp = client[i].player.mpm;
							client[i].state = active;
						}

						break;
					}

					// a player in Active state needs a (D)one with Battle message
					if (client[i].state == active) {
						buf[0] = 'D';
						buf[1] = 20;
						buf[2] = client[i].player.hp;
						buf[3] = client[i].player.hpm;
						buf[4] = client[i].player.mp;
						buf[5] = client[i].player.mpm;
						buf[6] = client[i].player.xp / 256;
						buf[7] = client[i].player.xp % 256;
						buf[8] = client[i].player.gold / 256;
						buf[9] = client[i].player.gold % 256;
						buf[10] = client[i].player.level;
						buf[11] = client[i].player.str;
						buf[12] = client[i].player.dex;
						buf[13] = client[i].player.con;
						buf[14] = client[i].player.iq;
						buf[15] = client[i].player.wis;
						buf[16] = client[i].player.cha;
						buf[17] = client[i].player.fort;
						buf[18] = client[i].player.reflex;
						buf[19] = client[i].player.will;
						buf[20] = client[i].player.def;
						buf[21] = client[i].player.mdef;

						if (send(client[i].fd, buf, 22, 0) == -1) {
							perror("send");
							client[i].state = none;
							close(client[i].fd); // bye!
							FD_CLR(client[i].fd, &master); // remove from master set
							client[i].fd = 0;
						}

						client[i].action = 0;
					}
				}
			}

			// make monster moves etc
			//  no monsters in area 0
			for (int i = 1; i < NUM_AREA; i ++) {
				struct area * a = &areas[i];

				if (a->flags) {
					for (int j = 0; j < MAX_MONSTERS_IN_AREA; j ++) {
						struct monster * m = &a->monsters[j];

						if (m->state == 1) {
							// monster is alive
							// advance brain
							m->brain ++;

							if (m->brain > 7) {
								// and move in a random direction if so
								m->brain = 0;
								int targX = m->x;
								int targY = m->y;

								switch (rand() % 4) {
								case 0:
									targY --;
									break;

								case 1:
									targX ++;
									break;

								case 2:
									targY ++;
									break;

								case 3:
									targX --;
									break;
								}

								const unsigned char * movable = check_move(i, targX, targY);

								// space seems free for movement
								if (movable[0] == 0) {
									m->x = targX;
									m->y = targY;
								}
							}
						} else if (m->state == 2) {
							// monster is battling
							//  not really much to do here except wait I guess
						} else {
							// monster is dead
							//  spawn a new one?
							if (rand() % 256) {
								int targX = rand() % a->w;
								int targY = rand() % a->h;
								const unsigned char * spawnable = check_move(i, targX, targY);

								if (spawnable[0] == 0) {
									m->x = targX;
									m->y = targY;
									m->type = rand() % 3;
									m->brain = 0;
									m->state = 1;
								}
							}
						}
					}
				}
			}

			// send updates
			//  updates are 'U', length, some self-info, then
			//  some other-info
			for (int i = 0; i < MAX_CLIENTS; i ++) {
				if (client[i].state == active) {
					buf[0] = 'U';
					buf[1] = 6;
					buf[2] = client[i].player.area;
					buf[3] = client[i].player.matX;
					buf[4] = client[i].player.matY;
					buf[5] = client[i].player.actualImage + (frame % 2);
					buf[6] = client[i].player.hp;
					buf[7] = client[i].player.mp;

					for (int j = 0; j < MAX_CLIENTS; j ++) {
						// other players in the same area
						//  TODO: limit to players on same screen
						if (i != j && client[j].state == active && client[i].player.area == client[j].player.area) {
							buf[buf[1] + 2] = j;
							buf[buf[1] + 3] = client[j].player.matX;
							buf[buf[1] + 4] = client[j].player.matY;
							buf[buf[1] + 5] = client[j].player.actualImage + (frame % 2);
							buf[buf[1] + 6] = 1;	// player
							buf[1] += 5;
						}
					}

					if (areas[client[i].player.area].flags) {
						for (int j = 0; j < MAX_MONSTERS_IN_AREA; j ++) {
							// other monsters in the same area
							if (areas[client[i].player.area].monsters[j].state) {
								buf[buf[1] + 2] = j;
								buf[buf[1] + 3] = areas[client[i].player.area].monsters[j].x;
								buf[buf[1] + 4] = areas[client[i].player.area].monsters[j].y;

								if (areas[client[i].player.area].monsters[j].state == 2)
									buf[buf[1] + 5] = 253;
								else
									buf[buf[1] + 5] = (areas[client[i].player.area].monsters[j].type * 2) + 1 + (frame % 2);

								buf[buf[1] + 6] = 0;	// monster
								buf[1] += 5;
							}
						}
					}

					if (send(client[i].fd, buf, buf[1] + 2, 0) == -1) {
						perror("send");
						client[i].state = none;
						close(client[i].fd);
						FD_CLR(client[i].fd, &master); // remove from master set
						client[i].fd = 0;
					}
				} else if (client[i].state >= battle) {
					// client is in battle
					//  build and send update
					struct monster * m = &areas[client[i].player.area].monsters[client[i].player.etype];
					buf[0] = 'B';
					buf[1] = 0x08;

					if ((frame % 2) && client[i].state == battle_player_attack && client[i].state_detail >= 5)
						buf[2] = 253;
					else
						buf[2] = m->type * 2 + 1 + (frame % 2);

					buf[3] = client[i].player.startImage + 2 + (frame % 2);
					buf[4] = client[i].player.charge;
					// battle state
					buf[5] = client[i].state - battle;
					//  battle state detail (counter)
					buf[6] = client[i].state_detail;
					buf[7] = client[i].player.hp;
					buf[8] = client[i].player.mp;
					buf[9] = client[i].action;

					if (send(client[i].fd, buf, buf[1] + 2, 0) == -1) {
						perror("send");
						client[i].state = none;
						close(client[i].fd);
						FD_CLR(client[i].fd, &master); // remove from master set
						client[i].fd = 0;
					}
				}
			}

			frame ++;
		}
	} // END for(;;)--and you thought it would never end!

	return 0;
}
