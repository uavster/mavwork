/*
 * SyncGroup.cpp
 *
 *  Created on: 03/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#include "threads/SyncGroup.h"

namespace Threads {

SyncGroup::SyncGroup() {
	groupId = idFactory.generateSyncGroupId();
}

void SyncGroup::add(Lockable &elem) {
	elem.joinGroup(*this);
}

void SyncGroup::remove(Lockable &elem) {
	elem.leaveGroup(*this);
}

cvg_bool SyncGroup::lock(Lockable::AccessType accessType) {
	return mutex.lock();
}

cvg_bool SyncGroup::unlock() {
	return mutex.unlock();
}

SyncGroup::SyncGroupFactory SyncGroup::idFactory;

SyncGroup::SyncGroupFactory::SyncGroupFactory() {
	currentId = 0;
}

cvg_int SyncGroup::SyncGroupFactory::generateSyncGroupId() {
	if (!mutex.lock()) throw cvgException("[SyncGroupFactory] Unable to lock factory");
	cvg_int id = currentId++;
	mutex.unlock();
	return id;
}

}
