#ifndef INT3_H
#define INT3_H

class int3
{
	int x,y,z;
	inline int3():x(0),y(0),z(0){}; //c-tor, x/y/z initialized to 0
	inline int3(const int X, const int Y, const int Z):x(X),y(Y),z(Z){}; //c-tor
	inline ~int3(){} // d-tor - does nothing
	inline int3 operator+(const int3 & i)
		{return int3(x+i.x,y+i.y,z+i.z);}
	inline int3 operator+(const int i) //increases all components by int
		{return int3(x+i,y+i,z+i);}
	inline int3 operator-(const int3 & i)
		{return int3(x-i.x,y-i.y,z-i.z);}
	inline int3 operator-(const int i)
		{return int3(x-i,y-i,z-i);}
	inline int3 operator-() //increases all components by int
		{return int3(-x,-y,-z);}
	inline void operator+=(const int3 & i)
	{
		x+=i.x;
		y+=i.y;
		z+=i.z;
	}	
	inline void operator+=(const int i)
	{
		x+=i;
		y+=i;
		z+=i;
	}
	inline void operator-=(const int3 & i)
	{
		x-=i.x;
		y-=i.y;
		z-=i.z;
	}	
	inline void operator-=(const int i)
	{
		x+=i;
		y+=i;
		z+=i;
	}	
	inline bool operator==(const int3 & i)
		{return (x==i.x) && (y==i.y) && (z==i.z);}	
	inline bool operator!=(const int3 & i)
		{return !(*this==i);}
	inline bool operator<(const int3 & i)
	{
		if (z<i.z)
			return true;
		if (z>i.z)
			return false;
		if (y<i.y)
			return true;
		if (y>i.y)
			return false;
		if (x<i.x)
			return true;
		if (x>i.x)
			return false;
		return false;
	}
};

#endif //INT3_H