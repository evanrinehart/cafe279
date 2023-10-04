struct LooseItem {
	int i; // pixels (fix8)
	int j;
	int di;
	int dj;
	int angle;
	int spin;
	int resting;
};

#define MAX_LOOSE_ITEMS 1024
extern struct LooseItem looseItems[MAX_LOOSE_ITEMS];
extern struct LooseItem * looseItems_ptr;

void addLooseItem(struct LooseItem * item);
void eraseLooseItem(struct LooseItem * item);
