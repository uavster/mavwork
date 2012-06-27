/*
 * Buffer.h
 *
 *  Created on: 27/04/2012
 *      Author: Ignacio Mellado-Bataller
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include <atlante.h>

class Buffer {
protected:
	virtual void *getPtr() const = 0;
public:
	void *operator & () const { return getPtr(); }
	virtual cvg_ulong sizeBytes() const = 0;
	virtual cvg_ulong elemSizeBytes() const = 0;
	virtual void copy(const void *b, cvg_ulong sizeBytes, cvg_ulong byteOffset = 0) = 0;
	inline void copy(const Buffer &b, cvg_ulong elemOffset = 0) {
		copy(&b, b.sizeBytes(), elemOffset * elemSizeBytes());
	}
	inline Buffer &operator = (const Buffer &b) { copy(b); return *this; }
};

template<class _type, cvg_int _size> class StaticBuffer : public virtual Buffer {
private:
	_type values[_size];
protected:
	virtual inline void *getPtr() const { return (void *)values; }
public:
	_type &operator [] (const size_t i) { return values[i]; }
	const _type &operator [] (const size_t i) const { return values[i]; }
	virtual cvg_ulong sizeBytes() const { return _size * sizeof(_type); }
	virtual cvg_ulong elemSizeBytes() const { return sizeof(_type); }
	virtual void copy(const void *b, cvg_ulong sizeBytes, cvg_ulong byteOffset = 0) {
		if (this->sizeBytes() < sizeBytes) throw cvgException("[StaticBuffer] Source buffer does not fit in destination buffer");
		memcpy(&((char *)values)[byteOffset], b, sizeBytes);
	}
	inline void copy(const Buffer &b, cvg_ulong elemOffset = 0) {
		copy(&b, b.sizeBytes(), elemOffset * elemSizeBytes());
	}
	inline StaticBuffer &operator = (const Buffer &b) { copy(b); return *this; }
	inline StaticBuffer &operator = (const StaticBuffer<_type, _size> &b) { copy(b); return *this; }
};

template<class _type> class DynamicBuffer : public virtual Buffer {
private:
	_type *values;
	cvg_ulong length;
	cvg_ulong containerLength;
protected:
	virtual inline void *getPtr() const { return values; }
public:
	DynamicBuffer(cvg_ulong size = 0) { containerLength = length = 0; values = NULL; resize(size); }
	virtual ~DynamicBuffer() { if (values != NULL) { delete [] values; } }

	void resize(cvg_ulong s) {
		if (s == length) return;
		if (s > containerLength) {
			if (values != NULL) delete [] values;
			containerLength = s;
			values = new _type[containerLength];
		}
		length = s;
	}

	_type &operator [] (const size_t i) { return values[i]; }
	const _type &operator [] (const size_t i) const { return values[i]; }
	virtual cvg_ulong sizeBytes() const { return length; }
	virtual cvg_ulong elemSizeBytes() const { return sizeof(_type); }
	virtual void copy(const void *b, cvg_ulong sizeBytes, cvg_ulong byteOffset = 0) {
		cvg_ulong neededSize = byteOffset + sizeBytes;
		if (values == NULL || neededSize > this->sizeBytes()) resize(neededSize);
		memcpy(&((char *)values)[byteOffset], b, sizeBytes);
	}
	inline void copy(const Buffer &b, cvg_ulong elemOffset = 0) {
		copy(&b, b.sizeBytes(), elemOffset * elemSizeBytes());
	}
	inline DynamicBuffer &operator = (const Buffer &b) { copy(b); return *this; }
	inline DynamicBuffer &operator = (const DynamicBuffer<_type> &b) { copy(b); return *this; }
};

#endif /* BUFFER_H_ */
