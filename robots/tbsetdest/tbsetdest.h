
#ifndef __tbsetdest_h__
#define __tbsetdest_h__

#include "config.h"
#include <sys/queue.h>

#ifndef LIST_FIRST
#define LIST_FIRST(head)	((head)->lh_first)
#endif
#ifndef LIST_NEXT
#define LIST_NEXT(elm, field)	((elm)->field.le_next)
#endif

void ReadInMovementPattern(void);

class sdvector {
public:
	sdvector(double x = 0.0, double y = 0.0, double z = 0.0) {
		X = x; Y = y; Z = z;
	}
	double length() {
		return sqrt(X*X + Y*Y + Z*Z);
	}

	inline void sdvector::operator=(const sdvector a) {
		X = a.X;
		Y = a.Y;
		Z = a.Z;
	}
	inline void sdvector::operator+=(const sdvector a) {
		X += a.X;
		Y += a.Y;
		Z += a.Z;
	}
	inline int sdvector::operator==(const sdvector a) {
		return (X == a.X && Y == a.Y && Z == a.Z);
	}
	inline int sdvector::operator!=(const sdvector a) {
		return (X != a.X || Y != a.Y || Z != a.Z);
	}
	inline sdvector operator-(const sdvector a) {
		return sdvector(X-a.X, Y-a.Y, Z-a.Z);
	}
	friend inline sdvector operator*(const double a, const sdvector b) {
		return sdvector(a*b.X, a*b.Y, a*b.Z);
	}
	friend inline sdvector operator/(const sdvector a, const double b) {
		return sdvector(a.X/b, a.Y/b, a.Z/b);
	}

	double X;
	double Y;
	double Z;
};


class Neighbor {
public:
	u_int32_t	index;			// index into NodeList
	u_int32_t	reachable;		// != 0 --> reachable.
	double		time_transition;	// next change

};

struct setdest {
  double time;
  double X, Y, Z;
  double speed;
  LIST_ENTRY(setdest)   traj;
};

class Node {
  friend void ReadInMovementPattern(void);
public:
	Node(void);
	void	Update(void);
	void	UpdateNeighbors(void);
	void	Dump(void);

	double		time_arrival;		// time of arrival at dest
	double		time_transition;	// min of all neighbor times

	// # of optimal route changes for this node
	int		route_changes;
        int             link_changes;

private:
	void	RandomPosition(void);
	void	RandomDestination(void);
	void	RandomSpeed(void);

	u_int32_t	index;                  // unique node identifier
	u_int32_t 	first_trip;		// 1 if first trip, 0 otherwise. (by J. Yoon)

	sdvector	position;		// current position
	sdvector	destination;		// destination
	sdvector	direction;		// computed from pos and dest

	double		speed;
	double		time_update;		// when pos last updated

	static u_int32_t	NodeIndex;

	LIST_HEAD(traj, setdest) traj;

public:
	// An array of NODES neighbors.
	Neighbor	*neighbor;
};

#endif /* __setdest_h__ */
