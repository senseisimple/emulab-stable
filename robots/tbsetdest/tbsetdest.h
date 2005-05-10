
/*
 * Copyright (c) 1997, 1998, 2005 Carnegie Mellon University.  All Rights
 * Reserved. 
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation is hereby granted (including for
 * commercial or for-profit use), provided that both the copyright notice
 * and this permission notice appear in all copies of the software,
 * derivative works, or modified versions, and any portions thereof, and
 * that both notices appear in supporting documentation, and that credit
 * is given to Carnegie Mellon University in all publications reporting
 * on direct or indirect use of this code or its derivatives.
 * 
 * ALL CODE, SOFTWARE, PROTOCOLS, AND ARCHITECTURES DEVELOPED BY THE CMU
 * MONARCH PROJECT ARE EXPERIMENTAL AND ARE KNOWN TO HAVE BUGS, SOME OF
 * WHICH MAY HAVE SERIOUS CONSEQUENCES. CARNEGIE MELLON PROVIDES THIS
 * SOFTWARE OR OTHER INTELLECTUAL PROPERTY IN ITS ``AS IS'' CONDITION,
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE OR
 * INTELLECTUAL PROPERTY, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * Carnegie Mellon encourages (but does not require) users of this
 * software or intellectual property to return any improvements or
 * extensions that they make, and to grant Carnegie Mellon the rights to
 * redistribute these changes without encumbrance.
 */

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
