/*
 * Trajectory.cpp
 *
 *  Created on: 30/09/2011
 *      Author: Ignacio Mellado
 */

#ifndef TRAJECTORY_CPP_
#define TRAJECTORY_CPP_

#include <Trajectory.h>
#include <math.h>

#define GROSS_POINT_BOX_FORWARD		1.0
#define GROSS_POINT_BOX_LATERAL		2.0
#define GROSS_POINT_BOX_ALTITUDE	-1.0

Trajectory::Trajectory() {
	setGrossPointBoxDimensions(Vector3(GROSS_POINT_BOX_LATERAL, GROSS_POINT_BOX_FORWARD, GROSS_POINT_BOX_ALTITUDE));
	resetCurrentWaypoint();
}

Trajectory::~Trajectory() {

}

cvg_uint Trajectory::appendWaypoint(const Vector3 &position, cvg_double forwardSpeed) {
	waypoints.push_back(Waypoint(position, forwardSpeed));
	return waypoints.size();
}

Trajectory::Waypoint Trajectory::update(const Vector3 &currentPosition, const Vector3 &currentSpeed) {
	cvg_uint curWpIndex0 = currentWaypointIndex;
	while(isNearWaypoint(&currentPosition, &currentSpeed, &waypoints[currentWaypointIndex])) {
		currentWaypointIndex++;
		if (currentWaypointIndex >= waypoints.size()) currentWaypointIndex = 0;
		// All points are near: don't move
		if (currentWaypointIndex == curWpIndex0) break;
	}
	Waypoint wp;
	Waypoint *newWp = &waypoints[currentWaypointIndex];
	wp.position = newWp->position;
	if (lastWaypoint != NULL) {
		cvg_double distRatio = (currentPosition - lastWaypoint->position).modulus() / (newWp->position - lastWaypoint->position).modulus();
		if (distRatio > 1.0) distRatio = 1.0;
		wp.forwardSpeed = (newWp->forwardSpeed - lastWaypoint->forwardSpeed) * distRatio;
	} else wp.forwardSpeed = newWp->forwardSpeed;
	return wp;
}

cvg_bool Trajectory::isNearWaypoint(const Vector3 *pos, const Vector3 *speed, const Waypoint *wp) {
	// Is the position inside a rectangle whose center is at the waypoint and whose base is orthogonal to the current speed vector?
	Vector3 waypointCenteredPosition = *pos - wp->position;
	if (grossPointBoxDims.z >= 0.0) {
		if (fabs(waypointCenteredPosition.z) > (grossPointBoxDims.z * 0.5)) return false;
	}
	cvg_double rectangleAngle = atan2(speed->y, speed->x) - M_PI / 2;
	Vector3 rotatedPoint = waypointCenteredPosition.rotate(Vector3(0, 0, 0), Vector3(0, 0, 1), -rectangleAngle);
	if (grossPointBoxDims.x >= 0.0) {
		if (rotatedPoint.x < -(grossPointBoxDims.x * 0.5) || rotatedPoint.x > (grossPointBoxDims.x * 0.5)) return false;
	}
	if (grossPointBoxDims.y >= 0.0) {
		if (rotatedPoint.y < -(grossPointBoxDims.y * 0.5) || rotatedPoint.y > (grossPointBoxDims.y * 0.5)) return false;
	}
	return true;
}

void Trajectory::resetCurrentWaypoint() {
	lastWaypoint = NULL;
	currentWaypointIndex = 0;
}

void Trajectory::generateXYCircle(cvg_uint numWaypoints, const Vector3 &center, cvg_double radius, cvg_double forwardSpeed) {
	clear();
	cvg_double angle = 0.0;
	for (cvg_int i = 0; i < numWaypoints; i++) {
		appendWaypoint(Vector3(center.x + radius * cos(angle), center.y + radius * sin(angle), center.z), forwardSpeed);
		angle += (2 * M_PI) / numWaypoints;
	}
}

#endif /* TRAJECTORY_CPP_ */
