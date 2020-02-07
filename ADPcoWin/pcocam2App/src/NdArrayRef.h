/*
 * NdArrayRef.h
 *
 * Use this class to hold a pointer to an NDArray.
 * If a copy of an NdArrayRef is taken, the NDArray's
 * reference counter is incremented.  If an NdArrayRef
 * is destroyed the NDArray's reference counter is
 * decremented.  This should ensure that NDArrays do not leak
 * even when using exceptions.
 *
 * Author:  Jonathan Thompson
 */

#ifndef _NDARRAYREF_H_
#define _NDARRAYREF_H_

class NDArray;

class NdArrayRef {
public:
	NdArrayRef();
	NdArrayRef(NDArray* array);
	NdArrayRef(const NdArrayRef& other);
	virtual ~NdArrayRef();
	NdArrayRef& operator=(const NdArrayRef& other);
	operator NDArray*() const;
private:
	NDArray* array;
};

#endif /* PCOCAM2APP_SRC_NDARRAYREF_H_ */
