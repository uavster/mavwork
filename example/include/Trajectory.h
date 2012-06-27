/*
 * Trajectory.h
 *
 *  Created on: 30/09/2011
 *      Author: Ignacio Mellado
 */

#ifndef TRAJECTORY_H_
#define TRAJECTORY_H_

#include <atlante.h>
#include <vector>

class Trajectory {
private:
	cvg_uint currentWaypointIndex;
	Vector3 grossPointBoxDims;

public:
	class Waypoint {
	public:
		Vector3 position;
		cvg_double forwardSpeed;

		inline Waypoint() { }
		inline Waypoint(const Vector3 &point, cvg_double forwardSpeed) {
			position = point; this->forwardSpeed = forwardSpeed;
		}
	};

private:
	std::vector<Waypoint> waypoints;
	Waypoint *lastWaypoint;

public:
	cvg_bool isNearWaypoint(const Vector3 *pos, const Vector3 *speed, const Waypoint *wp);

public:
	Trajectory();
	virtual ~Trajectory();

	cvg_uint appendWaypoint(const Vector3 &position, cvg_double forwardSpeed);
	inline std::vector<Waypoint> &getWaypointList() { return waypoints; }
	Waypoint update(const Vector3 &currentPosition, const Vector3 &currentSpeed);
	inline void setGrossPointBoxDimensions(const Vector3 &dims) { grossPointBoxDims = dims; }
	void resetCurrentWaypoint();
	inline void clear() { waypoints.clear(); }
	void generateXYCircle(cvg_uint numWaypoints, const Vector3 &center, cvg_double radius, cvg_double forwardSpeed);
};

#endif /* TRAJECTORY_H_ */
