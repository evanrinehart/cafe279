#include <stdio.h>

#include <linear.h>
#include <avatar.h>

#include <chunk.h>

struct Avatar player;

void avatarInit(void) {
	player.rx = (128 - 4) * 4096 + 2048;
	player.ry = (128 + 4) * 4096 + 2048;
	player.dx = 0;
	player.dy = 0;

	player.facing = 1;

	player.goL = 0;
	player.goR = 0;
	player.xacceleration = 0;
	player.grounded = 0;
	//player.halfw = 9;
	//player.halfh = 16;
	player.jumpReady = 1;
	player.dropReady = 1;
}

void avatarWASD(int wasd, int down) {
	if(wasd == 'a') player.goL = down;
	if(wasd == 'd') player.goR = down;
	player.xacceleration = 10 * (player.goR - player.goL);

	if(player.grounded) {
		if(player.dx < 0 && player.facing > 0) player.facing = -1;
		if(player.dx > 0 && player.facing < 0) player.facing = +1;
	}

	//if(wasd == 'w' && down == 1) player.dy += 50;
	//if(wasd == 's' && down == 1) player.dy -= 50;

	if(wasd == 's' && player.dropReady) {
		player.dy -= 1000;
		player.dropReady = 0;
	}

}

void avatarJump(int down) {
	if(down && player.jumpReady) {
		player.dy += 300;
		player.jumpReady = 0;
		player.grounded = 0;
	}
}

void avatarAction(int down) {
}

void avatarThink() {
	player.dx += player.xacceleration;

	if(player.grounded && player.dx < 0) player.facing = -1;
	if(player.grounded && player.dx > 0) player.facing = 1;

	player.dy -= 1;

	// these numbers could change depending on stance
	int wradius = 4;
	int hradius = 5;
	int middle = 16 - 6;

	int midX  = player.rx + (player.facing ? 2*256 : -2*256);

	int midY  = player.ry + middle*256;
	int levelH = midY + hradius*256;
	int levelL = midY - hradius*256;
	player.probe1 = levelL;
	player.probe2 = levelH;
	player.probe3 = midX + wradius*256;
	player.probe4 = midX - wradius*256;

	if(player.dx < 0) {
		int sideX = midX - wradius*256;
		int normal;

		int pingH = probeLeft(sideX, levelH, &normal);
		int pingL = probeLeft(sideX, levelL, &normal);

		if(pingH >= 0 && pingL >= 0) {
			int clearance = pingL < pingH ? pingL : pingH;
			if (-player.dx >= clearance) {
printf("collision 1\n");
				player.rx -= clearance;
				player.dx = 0;
			}
			else {
				player.rx += player.dx;
			}
		}

		if (pingH < 0 || pingL < 0) {
printf("depen 1\n");
			player.rx += pingH < pingL ? -pingH : -pingL;
			player.dx = 0;
		}
	}

	if(player.dx > 0) {
		int sideX = midX + wradius*256;
		int normal;

		int pingH = probeRight(sideX, levelH, &normal);
		int pingL = probeRight(sideX, levelL, &normal);

		if(pingH >= 0 && pingL >= 0) {
			int clearance = pingL < pingH ? pingL : pingH;
			if (player.dx >= clearance) {
printf("collision 2\n");
				player.rx += clearance;
				player.dx = 0;
			}
			else {
				player.rx += player.dx;
			}
		}

		if (pingH < 0 || pingL < 0) {
printf("depen 2\n");
			player.rx -= pingH < pingL ? -pingH : -pingL;
			player.dx = 0;
		}
	}

	int YLEVEL = player.ry + (22 - 6) * 256;
	player.probe5 = YLEVEL;

	int YFEET = player.ry + (6 - 6) * 256;
	player.probe6 = YFEET;

	if(player.dy < 0){
		int normal;

		int pingL = probeDown(midX - wradius*256 + 1, YFEET, &normal);
		int pingR = probeDown(midX + wradius*256 - 1, YFEET, &normal);

		if(pingL >= 0 && pingR >= 0) {
			int clearance = pingL < pingR ? pingL : pingR;
			if (-player.dy >= clearance) {
				player.ry -= clearance;
				player.dy = 0;
				player.jumpReady = 1;
				player.dropReady = 1;
				player.grounded = 1;
			}
			else {
				player.grounded = 0;
				player.ry += player.dy;
			}
		}

		if (pingL < 0 || pingR < 0) {
			player.ry += pingL < pingR ? -pingL : -pingR;
			player.dy = 0;
		}
	}

	if(player.dy > 0){
		int normal;

		int pingL = probeUp(midX - wradius*256 + 1, YLEVEL, &normal);
		int pingR = probeUp(midX + wradius*256 - 1, YLEVEL, &normal);

		if(pingL >= 0 && pingR >= 0) {
			int clearance = pingL < pingR ? pingL : pingR;
			if (player.dy >= clearance) {
printf("collision 3\n");
				player.ry += clearance;
				player.dy = 0;
			}
			else {
				player.ry += player.dy;
			}
		}

		if (pingL < 0 || pingR < 0) {
printf("depen 3\n");
			player.ry -= pingL < pingR ? -pingL : -pingR;
			player.dy = 0;
		}
	}



	if(player.grounded) { player.jumpReady = 1; }
}
