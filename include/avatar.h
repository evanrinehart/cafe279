struct Avatar {
	int rx, ry;
	int dx, dy;

	int facing; // positive = right, negative = left

	int goL; // control left active
	int goR; // control right active
	double xacceleration;

	int grounded;
	//int halfw;
	//int halfh;

	int jumpReady;
	int dropReady;

	int probe1;
	int probe2;
	int probe3;
	int probe4;
	int probe5;
	int probe6;
};

extern struct Avatar player;

void avatarWASD(int wasd, int down);
void avatarJump(int down);
void avatarAction(int down);
void avatarThink(void);

void avatarInit(void);
