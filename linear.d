import core.stdc.stdio;
import core.stdc.math;

struct vec2 {
	double x;
	double y;

	vec2 opBinary(string op : "+")(vec2 b){ return vec2(x + b.x, y + b.y); }
	vec2 opBinary(string op : "-")(vec2 b){ return vec2(x - b.x, y - b.y); }
	vec2 opBinaryRight(string op : "*")(double s){ return vec2(s * x, s * y); }
	vec2 opBinary(string op : "/")(double s){ return vec2(x / s, y / s); }
	vec2 opUnary(string op : "-")(vec2 b){ return vec2(-x, -y); }

	bool opBinary(string op : "==")(vec2 b){ return x==b.x && y==b.y; }
	bool opBinary(string op : "!=")(vec2 b){ return x!=b.x || y!=b.y; }

	double normSquared(){ return dot(this,this); }
	double norm(){ return sqrt(dot(this,this)); }
	vec2 normalize(){ return this / this.norm; }

	double angleTo(vec2 b){
		vec2 a = this;
		double A = a.norm;
		double B = b.norm;
		return acos(dot(a,b) / (A * B));
	}

	double distanceSquaredTo(vec2 b){
		vec2 a = this;
		return (b - a).normSquared;
	}

	vec2 project(vec2 b){
		vec2 a = this;
		return (dot(a,b)/dot(b,b)) * b;
	}

	vec2 reflect(vec2 L){
		vec2 a = this;
		return (2 * a.project(L)) - a;
	}

	vec2 rejection(vec 2){
		return this - project(b);
	}


	vec2 rotate(vec2 v, double angle){
		if(angle == 0.0) return v;
		double a = v.x;
		double b = v.y;
		double cosine = cos(angle);
		double sine = sin(angle);
		return vec2(
			a*cosine - b*sine,
			a*sine   + b*cosine
		);
	}

	vec2 normalTo(vec2 b){
		vec2 a = this;
		vec2 ab = b - a;
		return vec2(-ab.y, ab.x);
	}


}

double dot(vec2 a, vec2 b){ return a.x * b.x + a.y * b.y; }
double cross(vec2 a, vec2 b){ return a.x * b.y - a.y * b.x; }
vec2 cis(double angle){ return vec2(cos(angle), sin(angle)); }

vec2 lerp(vec2 a, vec2 b, double t){
	if(t == 0.0) return a;
	if(t == 1.0) return b;
	return a + t*(b - a);
}


/*
struct AABB {
	double xmin;
	double xmax;
	double ymin;
	double ymax;
}

double det(double a, double b, double c, double d) { return a*d - b*c; }
double min(double a, double b){ return a < b ? a : b; }
double max(double a, double b){ return a < b ? b : a; }

bool aabbOverlap(AABB a, AABB b){ return !aabbApart(a,b); }
*/
