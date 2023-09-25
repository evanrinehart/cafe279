struct LooseItem {
	short i; // pixels
	short j;
	short code;
	short rotation;

	short di;
	short dj;
	short timer;
};

#define MAX_LOOSE_ITEMS 1024
extern struct LooseItem looseItems[MAX_LOOSE_ITEMS];
extern struct LooseItem * looseItems_ptr;

void addLooseItem(struct LooseItem * item);
void eraseLooseItem(struct LooseItem * item);
