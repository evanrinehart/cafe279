void physics();

void addObject();

struct Object {
	vec2 pos;
	vec2 vel;
};

extern struct Object objects[1024];
extern struct Object *objects_ptr;
