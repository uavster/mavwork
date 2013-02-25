/*
 * SyncGroup.h
 *
 *  Created on: 03/05/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef SYNCGROUP_H_
#define SYNCGROUP_H_

#include <iostream>
#include <list>
#include <atlante.h>
#include "Lockable.h"

namespace Threads {

class SyncGroup {
	friend class Lockable;

private:
	cvgMutex mutex;
	cvg_int groupId;

	class SyncGroupFactory {
	private:
		cvgMutex mutex;
		cvg_int currentId;
	public:
		SyncGroupFactory();
		cvg_int generateSyncGroupId();
	};
	static SyncGroupFactory idFactory;

public:
	SyncGroup();

	void add(Lockable &elem);
	void remove(Lockable &elem);

	cvg_bool lock(Lockable::AccessType accessType = Lockable::READ_WRITE);
	cvg_bool unlock();

	inline void operator += (Lockable &elem) { add(elem); }
	inline void operator -= (Lockable &elem) { remove(elem); }
};

}

#endif /* SYNCGROUP_H_ */
