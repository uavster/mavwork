/*
 * Lockable.cpp
 *
 *  Created on: 03/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "threads/Lockable.h"
#include "threads/SyncGroup.h"

namespace Threads {

Lockable::Lockable() {
}

Lockable::~Lockable() {
}

cvg_bool Lockable::lock(AccessType accessType) {
	cvg_bool result = true;
	std::list<SyncGroup *>::iterator it;
	if (!selfMutex.lock()) return false;
	for (it = groups.begin(); it != groups.end(); it++) {
		result &= (*it)->lock(accessType);
		if (!result) {
			for (; it != groups.begin(); it--)
				(*it)->unlock();
			(*it)->unlock();
			break;
		}
	}
	selfMutex.unlock();
	return result;
}

cvg_bool Lockable::unlock() {
	cvg_bool result = true;
	std::list<SyncGroup *>::iterator it;
	if (!selfMutex.lock()) return false;
	for (it = groups.begin(); it != groups.end(); it++)
		result &= (*it)->unlock();
	selfMutex.unlock();
	return result;
}

bool Lockable::groupCompare(SyncGroup *g1, SyncGroup *g2) {
	return g1->groupId < g2->groupId;
}

void Lockable::joinGroup(SyncGroup &group) {
	if (!selfMutex.lock()) throw cvgException("[Lockable] Unable to lock selfMutex");
	groups.push_back(&group);
	groups.sort(groupCompare);
	selfMutex.unlock();
}

void Lockable::leaveGroup(SyncGroup &group) {
	if (!selfMutex.lock()) throw cvgException("[Lockable] Unable to lock selfMutex");
	groups.remove(&group);
	selfMutex.unlock();
}

}


