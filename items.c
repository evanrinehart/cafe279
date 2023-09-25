#include <items.h>

struct LooseItem looseItems[MAX_LOOSE_ITEMS];
struct LooseItem * looseItems_ptr = looseItems;
struct LooseItem * looseItems_end = looseItems + MAX_LOOSE_ITEMS;

void addLooseItem(struct LooseItem * p){
	if(looseItems_ptr == looseItems_end) return;
	*looseItems_ptr++ = *p;
}

void eraseLooseItem(struct LooseItem * p){
	if(looseItems_ptr == looseItems) return;
	*p = *--looseItems_ptr;
}
