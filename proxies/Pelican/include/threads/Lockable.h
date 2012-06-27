/*
 * ILockable.h
 *
 *  Created on: 03/05/2012
 *      Author: Ignacio Mellado-Bataller
 *
 *  Notes: Currently, there's no difference between access types when locking.
 *  TODO: use read/write locks depending on the access type.
 */

#ifndef LOCKABLE_H_
#define LOCKABLE_H_

#include <atlante.h>
#include <list>

namespace Threads {

class SyncGroup;

class Lockable {
	friend class SyncGroup;

private:
	std::list<SyncGroup *> groups;
	cvgMutex selfMutex;

protected:
	void joinGroup(SyncGroup &group);
	void leaveGroup(SyncGroup &group);
	static bool groupCompare(SyncGroup *g1, SyncGroup *g2);

public:
	Lockable();
	virtual ~Lockable();

	enum AccessType { READ, WRITE, READ_WRITE };

	cvg_bool lock(AccessType accessType = READ_WRITE);
	cvg_bool unlock();
};

}

#endif /* LOCKABLE_H_ */
