/*
*   Calvin Neo
*   Copyright (C) 2016  Calvin Neo <calvinneo@calvinneo.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License along
*   with this program; if not, write to the Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once
#include <memory>
#include <cmath>
#include <cassert>
#include "fordefs.h"
#include "forarray_common.h"
#include "forlang.h"

_NAMESPACE_FORTRAN_BEGIN
_NAMESPACE_HIDDEN_BEGIN
#ifdef _DEBUG
#define _RTN(X) X
#else
#define _RTN(X) X
#endif
template <typename size_type>
static void _map_impl_reset(size_type * cur, int dimension, int & curdim, const size_type * LBound, const size_type * size) {
	std::copy_n(LBound, dimension, cur);
	curdim = 0;
}
template <typename size_type>
static bool _map_impl_has_next(size_type * cur, int dimension, const size_type * LBound, const size_type * size) {
	for (int i = 0; i < dimension; i++)
	{
		fsize_t ub_i = LBound[i] + size[i] - 1;
		if (cur[i] >= LBound[i] + size[i])
		{
			return false;
		}
	}
	return true;
}
template <typename F, typename size_type>
static bool _map_impl_next(F f, size_type * cur, int dimension, int & cur_dim, const size_type * LBound, const size_type * size) {
	/***************
	*	cur must be valid
	*	return false:	the end of iteration
	*	return true:	continue iteration
	***************/
	// call `f` by `cur` once
	f(cur);
	cur[cur_dim] ++;
	if (cur[cur_dim] < LBound[cur_dim] + size[cur_dim])
	{
		// if index of this deiension haven't touch its upper bound
		// do not need to carry-over
	}
	else
	{
		while (cur[cur_dim] + 1 >= LBound[cur_dim] + size[cur_dim]) {
			// find innermost dimension which isn't to end
			cur_dim++;
			if (cur_dim == dimension) {
				return false; // all finished
			}
		}
		if (cur_dim < dimension) {
			cur[cur_dim]++;
		}
		else {
			return false; // all finished
		}
		std::copy_n(LBound, cur_dim, cur); // reset cur before dim
		cur_dim = 0;
		/* 0,0-1,0-0,1-1,1, index grow from left to right and start over when the rightmost index is incremented */
	}
	return true;
}
template <typename F, typename size_type>
static void _map_impl(F f, int dimension, const size_type * LBound, const size_type * size) {
	size_type * cur = new size_type[dimension];
	int dim = 0; // current dimension
	_map_impl_reset(cur, dimension, dim, LBound, size);
#ifdef USE_FORARRAY
	while (_map_impl_next(f, cur, dimension, dim, LBound, size)) {

	}
#endif
	delete[] cur;
}
_NAMESPACE_HIDDEN_END

template <typename T>
struct farray {
	typedef T value_type;
	typedef fsize_t size_type; // fortran array index can be negative
	typedef value_type * pointer;
	typedef value_type & reference;
	typedef const value_type * const_pointer;
	typedef const value_type & const_reference;
	typedef size_type difference_type;
	typedef slice_info</*typename*/size_type> slice_type;
	int dimension;
	const bool is_view;
	typedef T * iterator;
	typedef const T * const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	size_type LBound(int dim) const {
		return lb[dim];
	};
	size_type UBound(int dim) const {
		return lb[dim] + sz[dim] - 1;
	};
	size_type size(int dim) const {
		return sz[dim];
	}
	const size_type * LBound() const {
		return lb;
	}
	const size_type * size() const {
		return sz;
	}
	fsize_t flatsize() const {
		//return fa_getflatsize(sz, sz + dimension);
		return this->delta[this->dimension - 1] * this->sz[this->dimension - 1];
	}
	const fsize_t * get_delta() const {
		return delta;
	}

	iterator begin() {
		return parr;
	}
	iterator end() {
		return parr + flatsize();
	}
	const_iterator cbegin() const {
		return parr;
	}
	const_iterator cend() const  {
		return parr + flatsize();
	}

	template<int X>
	T & get(const size_type (& index)[X]) {
		assert(X == dimension);
		auto it = begin();
		for (size_type i = 0; i < X; i++)
		{
			size_type off = index[i] - lb[i];
			assert(off >= 0);
			it += (off) * delta[i];
		}
		return *it;
	}

	template<int X>
	const T & const_get(const size_type(&index)[X]) {
		assert(X == dimension);
		auto it = cbegin();
		for (size_type i = 0; i < X; i++)
		{
			size_type off = index[i] - lb[i];
			assert(off >= 0);
			it += (off) * delta[i];
		}
		return *it;
	}

	template<typename Iterator>
	T & get(Iterator index_from, Iterator index_to) { /* \1,2,3\ partial of [1,2,3,4]*/
		assert(index_to - index_from == dimension);
		auto it = begin();
		size_type i = 0;
		for (Iterator iter = index_from; iter < index_to; iter++, i++)
		{
			size_type off = *iter - lb[i];
			assert(off >= 0);
			it += (off) * delta[i];
		}
		return *it;
	}

    template<int X>
    const T & const_get(const size_type(&index)[X]) const {
        assert(X == dimension);
        auto it = cbegin();
        for (size_type i = 0; i < X; i++)
        {
            size_type off = index[i] - lb[i];
            assert(off >= 0);
            it += (off) * delta[i];
        }
        return *it;
    }

	template<typename Iterator>
	const T & const_get(Iterator index_from, Iterator index_to) const {
		assert(index_to - index_from == dimension);
		auto it = cbegin();
		size_type i = 0;
		for (Iterator iter = index_from; iter < index_to; iter++, i++)
		{
			size_type off = *iter - lb[i];
			assert(off >= 0);
			it += (off) * delta[i];
		}
		return *it;
	}

	template<int X>
	const T & get(const size_type(&index)[X]) const/*not changing member variable*/ {
		return const_get(index);
	}
	template<typename Iterator>
	const T & get(Iterator index_from, Iterator index_to) const {
		return const_get(index_from, index_to);
	}

	template<typename... Args>
	T & operator()(Args&&... args) {
		size_type index[sizeof...(args)] = { args... };
		return get(index);
	}
	template<int X>
	T & operator[](const slice_type (&index)[X]) {
		return get(index);
	}

    /* add operator for const instance */
    template<typename... Args>
    const T & operator()(Args&&... args) const{
        const size_type index[sizeof...(args)] = { args... };
        return get(index);
    }
    template<int X>
    const T & operator[](const slice_type (&index)[X]) const{
        return get(index);
    }
    /* END. add operator for const instance */

	operator bool() {
		// Return true if all elements in the array is true, otherwise false
		for(auto iter = begin(); iter != end(); iter++){
			if(!*iter) return false;
		}
		return true;
	}
	template <typename U, typename = std::enable_if_t<std::is_same<T, U>::value && !std::is_same<T, bool>::value>>
	operator U() {
		// If this farray has only one element, it can be cast to a scalar
		assert(flatsize() == 1);
		return *cbegin();
	}
	//typedef typename std::conditional<D == 1, std::true_type, std::false_type>::type is_vector_t;

	// element-wise add operation
	// must make sure flatsize() is conformed
	template<typename U> friend farray<U> operator+(const farray<U> & x, const farray<U> & y);
	template<typename U> friend farray<U> operator+(const U & x, const farray<U> & y);
	template<typename U> friend farray<U> operator+(const farray<U> & x, const U & y);
	template<typename U> friend farray<U> operator-(const farray<U> & x, const farray<U> & y);
	template<typename U> friend farray<U> operator-(const U & x, const farray<U> & y);
	template<typename U> friend farray<U> operator-(const farray<U> & x, const U & y);
	template<typename U> friend farray<U> operator*(const farray<U> & x, const farray<U> & y);
	template<typename U> friend farray<U> operator*(const U & x, const farray<U> & y);
	template<typename U> friend farray<U> operator*(const farray<U> & x, const U & y);
	template<typename U> friend farray<U> operator/(const farray<U> & x, const farray<U> & y);
	template<typename U> friend farray<U> operator/(const U & x, const farray<U> & y);
	template<typename U> friend farray<U> operator/(const farray<U> & x, const U & y);
	template<typename U> friend farray<bool> operator<(const farray<U> & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator<(const U & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator<(const farray<U> & x, const U & y);
	template<typename U> friend farray<bool> operator<=(const farray<U> & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator<=(const U & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator<=(const farray<U> & x, const U & y);
	template<typename U> friend farray<bool> operator>(const farray<U> & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator>(const U & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator>(const farray<U> & x, const U & y);
	template<typename U> friend farray<bool> operator>=(const farray<U> & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator>=(const U & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator>=(const farray<U> & x, const U & y);
	template<typename U> friend farray<bool> operator==(const farray<U> & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator==(const U & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator==(const farray<U> & x, const U & y);
	template<typename U> friend farray<bool> operator!=(const farray<U> & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator!=(const U & x, const farray<U> & y);
	template<typename U> friend farray<bool> operator!=(const farray<U> & x, const U & y);

#define MAKE_ARITH_OPERATORS_UNARY(op, op2) farray<T> & operator op(const farray<T> & x) { \
		std::transform(begin(), end(), x.cbegin(), begin(), [](auto x, auto y) {return x op2 y; }); \
		return *this; \
	} \
	farray<T> & operator op(const T & x) { \
		map([&](T & item, const fsize_t * cur) {item op x;}); \
		return *this; \
	}

	MAKE_ARITH_OPERATORS_UNARY(+=, +);
	MAKE_ARITH_OPERATORS_UNARY(-=, -);
	MAKE_ARITH_OPERATORS_UNARY(*=, *);
	MAKE_ARITH_OPERATORS_UNARY(/=, /);

	private:
	void __transpose2d() {

	}
	public:
	void transpose() {
		if (this->dimension == 2) {
			__transpose2d();
			//return;
		}
		T * na = new T[flatsize()];
		map([&](T & item, const fsize_t * cur) {
			fsize_t oldindex = 0;
			fsize_t newindex = 0;
			for (int i = 0; i < dimension; i++)
			{
				fsize_t oldt = 1;
				oldt = delta[i];
				oldindex += (cur[i] - lb[i]) * oldt;

				fsize_t newt = 1;
				for (int j = 0; j <= i - 1; j++)
				{
					newt *= sz[dimension - 1 - j];
				}
				newindex += (cur[dimension - 1 - i] - lb[dimension - 1 - i]) * newt;
			}
			na[newindex] = parr[oldindex];
		});
		std::swap(na, parr);
		delete[] na;
		//std::reverse(begin(), end());
		std::reverse(lb, lb + dimension);
		std::reverse(sz, sz + dimension);
		fa_layer_delta(this->sz, this->sz + dimension, delta);
	}


	template <typename F>
	void map(F f) const {/* invalid const, pointer returned by cbegin() is compatible to mutate element */
		auto iter = cbegin();
		auto newf = [&](fsize_t * cur) {			
			f(*iter, cur);
			iter++;
		};
		_map_impl<decltype(newf)>(newf, this->dimension, this->LBound(), this->size());
	}
	template <typename F>
	void map(F f) {
		auto iter = begin();
		auto newf = [&](fsize_t * cur) {
			f(*iter, cur);
			iter++;
		};
		_map_impl<decltype(newf)>(newf, this->dimension, this->LBound(), this->size());
	}

	template<int X>
	void reset_array(const slice_info<fsize_t>(&tp)[X], bool force_lower_bound = false) {
		// this overload do not provide dimension parameter, because dimension is dependant on `tp`
		delete[] lb; delete[] sz; delete[] delta;
		// modify dimension because not all element of tp is slice, probably index instead
		dimension = 0;
		int dim;
		for (int i = 0; i < X; i++)
		{
			if ((tp[i]).isslice)
			{
				dimension++;
			}
		}
		lb = new fsize_t[dimension]; sz = new fsize_t[dimension]; delta = new fsize_t[dimension];
		// if X < dimension, use farr as default from dimension X + 1
		if (force_lower_bound) {
			// lower bound of each dimension(new array) will be forced to 1
			// only use size infomation from `tp`
			std::fill_n(lb, dimension, 1);
		}
		else {
			// lower bound of each dimension(new array)
			dim = 0;
			for (int i = 0; i < X; i++)
			{
				if ((tp[i]).isslice)
				{
					lb[dim++] = tp[i].fr;
				}
			}
		}
		// size of each dimension(new array)
		dim = 0;
		for (int i = 0; i < X; i++)
		{
			if ((tp[i]).isslice)
			{
				const slice_info<fsize_t> & x = tp[i];
				sz[dim++] = (x.to + 1 - x.fr) / x.step + ((x.to + 1 - x.fr) % x.step == 0 ? 0 : 1);
			}
		}
		fa_layer_delta(this->sz, this->sz + dimension, delta);
	}

	template <typename Iterator_FSize_T>
	void reset_array(int dim, Iterator_FSize_T l, Iterator_FSize_T s) {
		this->dimension = dim;
		delete[] lb; delete[] sz; delete[] delta;
		lb = new fsize_t[dim]; sz = new fsize_t[dim]; delta = new fsize_t[dim];
		for (fsize_t i = 0; i < dim; i++)
		{
			*(this->lb + i) = *(l + i);
			*(this->sz + i) = *(s + i);
		}
		fa_layer_delta(this->sz, this->sz + dim, delta);
	}

	void clear() {
		delete[] parr;
		parr = nullptr;
	}
	void reset_value(const T & value) {
		clear();
		parr = new T[flatsize()];
		std::fill_n(parr, flatsize(), value);
	}
	void reset_value(int X, const farray<T> * farrs) {/*lbound and sz not modified, need make sure the sum of farrs' flatten size be the same with old flat size*/
		// reset value from several arrays
		clear();
		fsize_t flatsize = 0;
		for (int i = 0; i < X; i++)
		{
			flatsize += farrs[i].flatsize();
		}
		this->parr = new T[flatsize]();
		T * parr_iter = parr;
		for (int i = 0; i < X; i++)
		{
			std::copy_n(farrs[i].cbegin(), farrs[i].flatsize(), parr_iter);
			parr_iter += farrs[i].flatsize();
		}
	}
	template <typename Iterator>
	void reset_value(Iterator b, Iterator e) {
		clear();
		parr = new T[flatsize()];
		std::copy_n(b, e - b, parr);
	}
	void reset_value() {
		// allocate only according to sz
		clear();
		fsize_t totalsize = flatsize();
		parr = new T[totalsize]{};
	}
	template <typename Iterator_FSize_T>
	farray(int D, Iterator_FSize_T lower_bound, Iterator_FSize_T size) noexcept: is_view(false)
	{
		// on some occations, dimension is not given by sizeof(lower_bound) / sizeof(size)
		reset_array(D, lower_bound, size);
		reset_value();
	}
	template <int D>
	farray(const size_type(&lower_bound)[D], const size_type(&size)[D]) noexcept: is_view(false) {
		reset_array(D, lower_bound, size);
		reset_value();
	}
	template <int D, typename Iterator>
	farray(const size_type(&lower_bound)[D], const size_type(&size)[D], Iterator begin, Iterator end, bool isview = false) noexcept : is_view(isview)
	{
		reset_array(D, lower_bound, size);
		reset_value(begin, end);
	}

	// deprecated
	template <int D, int X>
	farray(const size_type(&lower_bound)[D], const size_type(&size)[D], const T(&values)[X]) noexcept : is_view(false)
	{
		// X == 1 means explicitly use a scalar to initialize an array
		reset_array(D, lower_bound, size);
		reset_value(values, values + X);
	}
	template <int D>
	farray(const size_type(&lower_bound)[D], const size_type(&size)[D], const farray<T> & m, bool isview = false) noexcept : is_view(isview)
	{
		// used when construct from a farray<T>
		// copy and reshape from a farray
		reset_array(D, lower_bound, size);
		reset_value(m.cbegin(), m.cend());
	}

    /* add construction between implicit convertable types */
    template <int D, typename R>
    farray(const size_type(&lower_bound)[D], const size_type(&size)[D], const farray<R> & m, bool isview = false) noexcept : is_view(isview)
    {
        // used when construct from a farray<T>
        // copy and reshape from a farray
        reset_array(D, lower_bound, size);
        reset_value(m.cbegin(), m.cend());
    }
	farray(const T & scalar) noexcept : is_view(false) {
		/***************
		*	ISO/IEC 1539:1991 1.5.2
		*	A scalar is conformable with any array
		*	A rank-one array may be constructed from scalars and other arrays and may be reshaped into any allowable array shape
		***************/
		// Implicitly use a scalr to initialize an array, so shape of array is undetermined
		int lower_bound[] = {1}, size[] = {1};
        T v[] = {scalar};
		reset_array(1, lower_bound, size);
		reset_value(v, v + 1);
	}
	//explicit farray(int dim)  noexcept: is_view(false) {
	// 
	//}
	farray() noexcept: is_view(false) {

	}
	farray<T> & copy_from(const farray<T> & x) {
		if (this == &x) return *this;
		this->dimension = x.dimension;
		reset_array(this->dimension, x.LBound(), x.size());
		reset_value(x.cbegin(), x.cend());
		return *this;
	}
	farray(const farray<T> & m) noexcept: is_view(false){
		// copy constructor
		copy_from(m);
	}
	farray<T> & move_from(farray<T> && m) {
		if (this == &m) return *this;
		delete[] lb; delete[] sz; delete[] delta;
		clear();
		this->dimension = m.dimension;
		this->lb = m.lb; 
		this->sz = m.sz;
		this->delta = m.delta;
		this->parr = m.parr;
		m.lb = nullptr;
		m.sz = nullptr;
		m.delta = nullptr;
		m.parr = nullptr;
		return *this;
	}
	farray(farray<T> && m) noexcept : is_view(false) {
		move_from(std::move(m));
	}

	farray<T> & operator=(const farray<T> & x) {
		// only reset value, remain original shape
		// use `copy_from`, or call `reset_array` before using `operator=`, if you want to copy the entire array
		if (this == &x) return *this;
		reset_value(x.cbegin(), x.cend());
		return *this;
    }

    /* add assignment between implicit convertable types */
    template <typename R>
    farray<T> &operator=(const farray<R> &x) {
      // only reset value, remain original shape
      // use `copy_from`, or call `reset_array` before using `operator=`, if
      // you want to copy the entire array
      if ((void*)this == (void*)&x)
        return *this;
      reset_value(x.cbegin(), x.cend());
      return *this;
    }
        farray<T> & operator=(farray<T> && x) {
		// only reset value, remain original shape
		// use `move_from`, or call `reset_array` before using `operator=`, if you want to move the entire array
		if (this == &x) return *this;
		delete [] this->parr;
		this->parr = x.parr;
		x.parr = nullptr;
		return *this;
	}
	~farray() {
		if (!is_view) {
			delete[] parr;
			parr = nullptr;
		}
		delete[] lb; delete[] sz; delete[] delta;
	}
	T * parr = nullptr; // support inplace operation of each element
protected:
	size_type * lb = nullptr, * sz = nullptr, * delta = nullptr;

};

template <typename T1, typename T2> 
auto power(const farray<T1> & x, const farray<T2> & y) { 
	assert(x.flatsize() == y.flatsize()); 
	decltype(std::declval<T1>() * std::declval<T2>()) narr(x);
	std::transform(narr.begin(), narr.end(), x.cbegin(), narr.begin(), [](auto m, auto n) {return power(m, n); });
	return _RTN(narr);
}
template <typename T1, typename T2>
auto power(const T1 & x, const farray<T2> & y) {
	decltype(std::declval<T1>() * std::declval<T2>()) narr(y);
	std::transform(narr.begin(), narr.end(), narr.begin(), [&](auto m) {return power(x, m); });
	return _RTN(narr);
}
template <typename T1, typename T2>
auto power(const farray<T1> & x, const T2 & y) {
	decltype(std::declval<T1>() * std::declval<T2>()) narr(x);
	std::transform(narr.begin(), narr.end(), narr.begin(), [&](auto m) {return power(m, y); });
	return _RTN(narr);
}

#define _MAKE_ARITH_OPERATORS_BIN(op, op2) template <typename T> \
	farray<T> operator op(const farray<T> & x, const farray<T> & y) { \
		assert(x.flatsize() == y.flatsize()); \
		farray<T> narr(x); \
		narr op2 y; \
		return _RTN(narr); \
	} \
	template <typename T> \
	farray<T> operator op(const T & x, const farray<T> & y) { \
		farray<T> narr(y); \
		narr op2 x; \
		return _RTN(narr); \
	} \
	template <typename T> \
	farray<T> operator op(const farray<T> & x, const T & y) { \
		farray<T> narr(x); \
		narr op2 y; \
		return _RTN(narr); \
	}
// do not use `operator ## op` for std compacity, `operator` is a seprated token
_MAKE_ARITH_OPERATORS_BIN(+, +=);
_MAKE_ARITH_OPERATORS_BIN(-, -=);
_MAKE_ARITH_OPERATORS_BIN(*, *=);
_MAKE_ARITH_OPERATORS_BIN(/, /=);

#define _MAKE_CMP_OPERATORS(op, id_cond) template <typename T> \
	farray<bool> operator op(const farray<T> & x, const farray<T> & y) { \
		assert(x.flatsize() == y.flatsize()); \
		if (&x == &y) { \
			id_cond; \
		} \
		farray<bool> narr(x.dimension, x.LBound(), x.size()); \
		narr.map([&](bool & item, const fsize_t * cur) { \
			item = (x.const_get(cur, cur + x.dimension) op y.const_get(cur, cur + y.dimension)); \
		}); \
		return _RTN(narr); \
	} \
	template <typename T> \
	farray<bool> operator op(const T & x, const farray<T> & y) { \
		farray<bool> narr(x.dimension, x.LBound(), x.size()); \
			narr.map([&](bool & item, const fsize_t * cur) { \
				item = (x op y.const_get(cur, cur + y.dimension)); \
		}); \
		return _RTN(narr); \
	} \
	template <typename T> \
	farray<bool> operator op(const farray<T> & x, const T & y) { \
		farray<bool> narr(x.dimension, x.LBound(), x.size()); \
		narr.map([&](bool & item, const fsize_t * cur) { \
			item = (x.const_get(cur, cur + x.dimension) op y); \
		}); \
		return _RTN(narr); \
	}

_MAKE_CMP_OPERATORS(<, return false );
_MAKE_CMP_OPERATORS(<=, return true );
_MAKE_CMP_OPERATORS(>, return false);
_MAKE_CMP_OPERATORS(>= , return true);
_MAKE_CMP_OPERATORS(== , return true);
_MAKE_CMP_OPERATORS(!= , return false);
_MAKE_CMP_OPERATORS(&&, );
_MAKE_CMP_OPERATORS(|| , );
_MAKE_CMP_OPERATORS(^, );

template <typename T>
farray<bool> operator !(const farray<T> & x) {
	farray<bool> narr(x.dimension, x.LBound(), x.size());
	narr.map([&](bool & item, const fsize_t * cur) {
		item = (! x.const_get(cur, cur + x.dimension));
	});
	return _RTN(narr); 
}

template <typename T, int X>
farray<T> forconcat(const farray<T>(&farrs)[X])
{
	fsize_t flatsize = 0;
	for (int i = 0; i < X; i++)
	{
		flatsize += farrs[i].flatsize();
	}
	farray<T> narr({ 1 }, { flatsize });
	// concat several farrays
	narr.reset_value(X, farrs);
	return _RTN(narr);
}


template <int D, typename F/*, typename = std::enable_if_t<std::is_callable<F, size_type>::value>*/>
auto make_init_list(const fsize_t(&from)[D], const fsize_t(&size)[D], F f) 
-> farray<typename function_traits<decltype(f)>::result_type> {
	typedef typename function_traits<decltype(f)>::result_type T;
	farray<T> narr(D, from, size);
	// important: second argument is size, not to
	fsize_t totalsize = narr.flatsize();
	narr.reset_value(totalsize);
	narr.map([&](T & item, const fsize_t *cur) {
		item = f(cur);
	});
	return _RTN(narr);
}
template <typename T, int D, typename Iterator>
farray<T> make_init_list(Iterator list_begin, Iterator list_end) {
	farray<T> narr({ 1 }, { list_end - list_begin });
	narr.reset_value(list_begin, list_end);
	return _RTN(narr);
}
template <typename T, int X>
farray<T> make_init_list(const T(&values)[X]){
	farray<T> narr({ 1 }, { X });
	narr.reset_value(values, values + X);
	return _RTN(narr);
}
template <typename T>
farray<T> make_init_list(const T & scalar)  {
	/***************
	*	ISO/IEC 1539:1991 1.5.2
	*	A scalar is conformable with any array
	*	A rank-one array may be constructed from scalars and other arrays and may be reshaped into any allowable array shape
	****************/
	// implicitly use a scalr to initialize an array, so shape of array is undetermined
	farray<T> narr{ scalar };
	return _RTN(narr);
}
template <typename T>
farray<T> make_init_list(int repeat, const T & scalar) {
	farray<T> narr({ 1 }, { repeat });
	narr.reset_value(scalar);
	return _RTN(narr);
}
template <typename T>
farray<T> make_init_list(const farray<T> & narr) {
	// this "identity" specification is for compacity
	// ref gen_arraybuilder.cpp
	return _RTN(narr);
}

_NAMESPACE_HIDDEN_BEGIN
template <typename T, int X, typename _Iterator_In, typename _Iterator_Out>
void _forslice_impl(const slice_info<fsize_t>(&tp)[X], const farray<T> & farr, int deep, const fsize_t * delta_out, const fsize_t * delta_in
	, _Iterator_Out bo, _Iterator_Out eo, _Iterator_In bi, _Iterator_In ei)
{
	int _X = X;
	for (fsize_t i = farr.LBound(deep); i < farr.LBound(deep) + farr.size(deep); i++)
	{
		bool hit = i >= tp[deep].fr && i <= tp[deep].to && ((i - tp[deep].fr) % tp[deep].step) == 0;
		if (hit) {
			if (deep + 1 == _X) { // if X not equal to narr.dimension, behaviour is not defined
				*bo = *bi;
			}
			else { 
				_forslice_impl<T, X>(tp, farr, deep + 1, delta_out, delta_in, bo, bo + delta_out[deep], bi, bi + delta_out[deep]);
			}
		}
		if (i != farr.LBound(deep) + farr.size(deep)) {
			if (hit) {
				bo += delta_out[deep];
			}
			bi += delta_in[deep];
		}
	}
};
_NAMESPACE_HIDDEN_END

template <typename T, int X>
auto forslice(const farray<T> & farr, const slice_info<fsize_t>(&tp)[X]) {
	// by fortran standard, slice must return by ref
	// `narr` is sliced array of `farr`
	farray<T> narr{}; // dimension will be reset later in reset_array
	slice_info<fsize_t> ntp[X];
	// `narr` can have fewer dimensions than `farr`
	assert(X <= farr.dimension);
	for (auto i = 0; i < X; i++)
	{
		if (tp[i].isall) {
			// [from, to]
			ntp[i] = slice_info<fsize_t>({ farr.LBound(i), farr.UBound(i) });
		}
		else {
			// just copy
			ntp[i] = slice_info<fsize_t>(tp[i]);
		}
	}
	narr.reset_array(ntp, true);
	narr.reset_value();
	// because `narr` can have fewer dimensions than `farr`, `narr.get_delta()` can't be used here
	fsize_t ndelta[X];
	std::transform(ntp, ntp + X, ndelta, [](auto x) {return (x.to + 1 - x.fr) / x.step + ((x.to + 1 - x.fr) % x.step == 0 ? 0 : 1); }); // size
	fa_layer_delta(ndelta, ndelta + X, ndelta);
	_forslice_impl<T, X>(ntp, farr, 0, ndelta, farr.get_delta(), narr.begin(), narr.end(), farr.cbegin(), farr.cend());

	return _RTN(narr);
}

template <typename T>
farray<T> fortranspose(const farray<T> & farr) {
	farray<T> narr(farr);
	narr.transpose();
	return _RTN(narr);
}
	
template <typename T>
fsize_t forlbound(const farray<T> & farr, int fordim = 1) {
	return farr.LBound(fordim - 1);
}
	
template <typename T>
fsize_t forubound(const farray<T> & farr, int fordim = 1) {
	return farr.UBound(fordim - 1);
}

template <typename T>
farray<fsize_t> forshape(const farray<T> & farr, int fordim = 1) {
	farray<fsize_t> sarr = make_init_list(farr.size(), farr.size() + farr.dimension); 
	return _RTN(sarr);
}

template <typename T, int X, int D>
farray<T> forreshape(const T(&value)[X], const fsize_t(&shape)[D]){
	fsize_t lb[D];
	std::fill_n(lb, D, 1);
	farray<T> sarr = farray<T>(lb, shape, value, value + X);
	return _RTN(sarr);
}

template <typename T>
fsize_t forsize(const farray<T> & farr, int fordim = 1) {
	return farr.size(fordim - 1);
}

//template <typename T>
//using mask_wrapper_t = std::function<bool(T)>;

//template <typename T>
//bool mask_wrapper_all(T x) {
//	return true;
//}
using mask_wrapper_t = farray<bool>;

_NAMESPACE_HIDDEN_BEGIN
template <typename T, typename F>
farray<T> _forloc_impl(F predicate, const farray<T> & farr, int fordim, foroptional<mask_wrapper_t> mask = None ) {
	// if the array has zero size, or all of the elements of MASK are .FALSE., then the result is an array of zeroes. Similarly, if DIM is supplied and all of the elements of MASK along a given row are zero, the result value for that row is zero. 
	int dim = fordim - 1;
	std::vector<fsize_t> lb(farr.LBound(), farr.LBound() + farr.dimension); lb.erase(lb.cbegin() + dim); // remove dimension dim
	std::vector<fsize_t> sz(farr.size(), farr.size() + farr.dimension); sz.erase(sz.cbegin() + dim);		
	farray<fsize_t> loc(farr.dimension - 1, lb.cbegin(), sz.cbegin()); loc.reset_value(0); // only allocate
	farray<bool> inited(farr.dimension - 1, lb.cbegin(), sz.cbegin()); inited.reset_value(false); // set all elements to false
	T curv; 
	farr.map([&](const T & current_value, const fsize_t * cur) {
		std::vector<fsize_t> subcur = std::vector<fsize_t>(cur, cur + farr.dimension); subcur.erase(subcur.cbegin() + dim);
		if (inited.const_get(subcur.cbegin(), subcur.cend()) == false|| predicate(current_value, curv)) {
			loc.get(subcur.begin(), subcur.end()) = cur[dim];
			curv = current_value;
		}
		inited.get(subcur.begin(), subcur.end()) = true;
	});
	return _RTN(loc);
}
template <typename T, typename F>
farray<T> _forloc_impl(F predicate, const farray<T> & farr, foroptional<mask_wrapper_t> mask = None) {
	T curv;
	farray<fsize_t> loc({ 1 }, { farr.dimension }); loc.reset_value(0); // only allocate
	bool inited = false;
	farr.map([&](const T & current_value, const fsize_t * cur) {
		if (!inited || predicate(current_value, curv)) {
			// if this's the first, or `predicate` is satisfied
			// replaced the old with new
			std::copy_n(cur, farr.dimension, loc.begin());
			curv = current_value;
		}
		inited = true;
	});
	return _RTN(loc);
}
_NAMESPACE_HIDDEN_END

template <typename T, typename F>
T formap(F f, T x) {
	// generally, `formap` just call `f(x)`
	return f(x);
}
template <typename T, typename F>
farray<T> formap(F f, const farray<T> & farr) {
	// `formap`'s specialation for `const farray<T> &`
	farray<T> narr = farr;
	narr.map([&](T & current_value, const fsize_t * cur) {
		f(current_value);
	});
	return _RTN(narr);
}

template <typename R, typename T, typename F>
R forreduce(F binop, R initial, const farray<T> & farr, foroptional<mask_wrapper_t> mask)
{
	// TODO examine why this do not work
	//return std::accumulate(farr.cbegin(), farr.cend(), intial, [&](const R & op1, const T & op2) {
	//	return binop(op1, op2);
	//});
	if (mask.inited())
	{
		auto iter_bool = mask.const_get().cbegin();
		for (auto iter = farr.cbegin(); iter < farr.cend(); iter++, iter_bool++)
		{
			if (*iter_bool)
			{
				initial = binop(initial, *iter);
			}
		}
	}
	else {
		for (auto iter = farr.cbegin(); iter < farr.cend(); iter++)
		{
			initial = binop(initial, *iter);
		}
	}
	return _RTN(initial);
}
template <typename R, typename T, typename F>
farray<R> forreduce(F binop, farray<R> initial_set, const farray<T> & farr, int dim, foroptional<mask_wrapper_t> mask)
{
	// previous_result_set is a farray 1 rank less than farr
	farr.map([&](const T & current_value, const fsize_t * cur) {
		std::vector<fsize_t> subcur = std::vector<fsize_t>(cur, cur + farr.dimension); subcur.erase(subcur.cbegin() + dim);
		if (mask.inited())
		{
			if (mask.const_get().const_get(cur, cur + farr.dimension) == false)
			{
				return;
			}
		}
		R & initial = initial_set.get(subcur.begin(), subcur.end());
		initial = binop(initial, current_value);
	});
	return _RTN(initial_set);
}

template <typename T>
auto forsum(const farray<T> & farr, foroptional<int> fordim, foroptional<mask_wrapper_t> mask = None) {
	int dim = fordim.value_or(1) - 1;
	// do not need to CHECK_AND_SET mask because it's the last parameter
	std::vector<fsize_t> lb(farr.LBound(), farr.LBound() + farr.dimension); lb.erase(lb.cbegin() + dim); // remove dimension dim
	std::vector<fsize_t> sz(farr.size(), farr.size() + farr.dimension); sz.erase(sz.cbegin() + dim);
	farray<T> ans(farr.dimension - 1, lb.cbegin(), sz.cbegin()); ans.reset_value(0); // only allocate
	return forreduce([&](const T & op1, const T & op2) {
		return op1 + op2;
	}, ans, farr, dim, mask);
}

template <typename T>
auto forsum(const farray<T> & farr, foroptional<mask_wrapper_t> mask = None) {
	return forreduce([&](const T & op1, const T & op2) -> T {
		return op1 + op2;
	}, 0, farr, mask);
}

template <typename T>
auto forproduct(const farray<T> & farr, foroptional<int> fordim, foroptional<mask_wrapper_t> mask = None) {
	int dim = fordim.value_or(1) - 1;
	std::vector<fsize_t> lb(farr.LBound(), farr.LBound() + farr.dimension); lb.erase(lb.cbegin() + dim); // remove dimension dim
	std::vector<fsize_t> sz(farr.size(), farr.size() + farr.dimension); sz.erase(sz.cbegin() + dim);
	farray<T> ans(farr.dimension - 1, lb.cbegin(), sz.cbegin()); ans.reset_value(1); // only allocate
	return forreduce([&](const T & op1, const T & op2) {
		return op1 * op2;
	}, ans, farr, dim, mask);
}
template <typename T>
auto forproduct(const farray<T> & farr, foroptional<mask_wrapper_t> mask = None) {
	return forreduce([&](const T & op1, const T & op2) -> T {
		return op1 * op2;
	}, 1, farr, mask);
}

inline farray<bool> forall(mask_wrapper_t mask, int fordim) {
	int dim = fordim - 1;
	std::vector<fsize_t> lb(mask.LBound(), mask.LBound() + mask.dimension); lb.erase(lb.cbegin() + dim); // remove dimension dim
	std::vector<fsize_t> sz(mask.size(), mask.size() + mask.dimension); sz.erase(sz.cbegin() + dim);
	farray<bool> ans(mask.dimension - 1, lb.cbegin(), sz.cbegin()); ans.reset_value(true); // only allocate
	return forreduce([&](const bool & op1, const bool & op2) -> bool {
		return op1 && op2;
	}, ans, mask, dim, None);
}

inline bool forall(mask_wrapper_t mask) {
	return forreduce([&](const bool & op1, const bool & op2) -> bool {
		return op1 && op2;
	}, true, mask, None);
}
	
inline farray<bool> forany(mask_wrapper_t mask, int fordim) {
	int dim = fordim - 1;
	std::vector<fsize_t> lb(mask.LBound(), mask.LBound() + mask.dimension); lb.erase(lb.cbegin() + dim); // remove dimension dim
	std::vector<fsize_t> sz(mask.size(), mask.size() + mask.dimension); sz.erase(sz.cbegin() + dim);
	farray<bool> ans(mask.dimension - 1, lb.cbegin(), sz.cbegin()); ans.reset_value(false); // only allocate
	return forreduce([&](const bool & op1, const bool & op2) -> bool {
		return op1 || op2;
	}, ans, mask, dim, None);
}

inline bool forany(mask_wrapper_t mask) {
	return forreduce([&](const bool & op1, const bool & op2) -> bool {
		return op1 || op2;
	}, false, mask, None);
}

inline farray<fsize_t> forcount(mask_wrapper_t mask, int fordim) {
	int dim = fordim - 1;
	std::vector<fsize_t> lb(mask.LBound(), mask.LBound() + mask.dimension); lb.erase(lb.cbegin() + dim); // remove dimension dim
	std::vector<fsize_t> sz(mask.size(), mask.size() + mask.dimension); sz.erase(sz.cbegin() + dim);
	farray<fsize_t> ans(mask.dimension - 1, lb.cbegin(), sz.cbegin()); ans.reset_value(0); // only allocate
	return forreduce([&](const fsize_t & op1, const bool & op2) -> fsize_t {
		if (op2) {
			return op1 + 1;
		}
		else {
			return op1;
		}
	}, ans, mask, dim, None);
}

inline fsize_t forcount(mask_wrapper_t mask) {
	return forreduce([&](const fsize_t & op1, const bool & op2) -> fsize_t {
		if (op2) {
			return op1 + 1;
		}
		else {
			return op1;
		}
	}, 0, mask, None);
}

_NAMESPACE_HIDDEN_BEGIN
template <typename T, typename F>
T _forcmpval_impl(F predicate, const T & initial_value, const farray<T> & farr, foroptional<mask_wrapper_t> mask = None) {
	return forreduce([&](const T & op1, const T & op2) -> T {
		if (predicate(op2, op1))
		{
			return op2;
		}
		else {
			return op1;
		}
	}, initial_value, farr, mask);
}
template <typename T, typename F>
farray<T> _forcmpval_impl(F predicate, const T & initial_value, const farray<T> & farr, int fordim, foroptional<mask_wrapper_t> mask = None) {
	int dim = fordim - 1;
	std::vector<fsize_t> lb(farr.LBound(), farr.LBound() + farr.dimension); lb.erase(lb.cbegin() + dim); // remove dimension dim
	std::vector<fsize_t> sz(farr.size(), farr.size() + farr.dimension); sz.erase(sz.cbegin() + dim);
	farray<T> ans(farr.dimension - 1, lb.cbegin(), sz.cbegin()); ans.reset_value(initial_value); 
	return forreduce([&](const T & op1, const T & op2) {
		if (predicate(op2, op1))
		{
			return op2;
		}
		else {
			return op1;
		}
	}, ans, farr, dim, mask);
}
_NAMESPACE_HIDDEN_END

template <typename T>
farray<T> formaxloc(const farray<T> & farr, foroptional<int> fordim, foroptional<mask_wrapper_t> mask = None) {
	/*****************
	*	ISO/IEC 1539:1991 1.5.2
	*	has not specify actions for this overload version
	*	conform to gfortran(fortran 95 standard)
	*****************/
	// ���ص���`[lb[dim], sz[dim]]`���ɵ�`dim-1`ά���飬��ʣ������ά��ȡ��������ϣ�����ÿһ��ȡ��ϣ�����ȡ�����ֵʱ���Ӧԭ�����dimά���±�
	return _forloc_impl(std::greater<T>(), farr, fordim, mask);
}
template <typename T>
farray<T> forminloc(const farray<T> & farr, foroptional<int> fordim, foroptional<mask_wrapper_t> mask = None) {
	return _forloc_impl(std::less<T>(), farr, fordim, mask);
}

template <typename T>
farray<T> formaxloc(const farray<T> & farr, foroptional<mask_wrapper_t> mask = None) {
	return _forloc_impl(std::greater<T>(), farr, mask);
}
template <typename T>
farray<T> forminloc(const farray<T> & farr, foroptional<mask_wrapper_t> mask = None) {
	return _forloc_impl(std::less<T>(), farr, mask);
}

template <typename T>
auto formaxval(const farray<T> & farr, foroptional<mask_wrapper_t> mask = None) {
	return _forcmpval_impl(std::greater<T>(), -forhuge<T>(),farr, mask);
}
template <typename T>
auto formaxval(const farray<T> & farr, foroptional<int> fordim, foroptional<mask_wrapper_t> mask = None) {
	return _forcmpval_impl(std::greater<T>(), -forhuge<T>(), farr, fordim,  mask);
}
template <typename T>
auto forminval(const farray<T> & farr, foroptional<mask_wrapper_t> mask = None) {
	return _forcmpval_impl(std::less<T>(), +forhuge<T>(), farr, mask);
}
template <typename T>
auto forminval(const farray<T> & farr, foroptional<int> fordim, foroptional<mask_wrapper_t> mask = None) {
	return _forcmpval_impl(std::less<T>(), +forhuge<T>(), farr, fordim, mask);
}

template <typename T1, typename T2>
auto formerge(const farray<T1> & tarr, const farray<T2> & farr, const farray<bool> & mask) {
	/*****************
	*	a = reshape((/ 1, 2, 3, 4, 5, 6 /), (/ 2, 3 /))
	*	a2 = reshape((/ 8, 9, 0, 1, 2, 3 /), (/ 2, 3 /))
	*	logi = reshape((/ .FALSE., .TRUE., .TRUE., .TRUE., .TRUE., .FALSE. /), (/ 2, 3 /))
	*	return  8 2 3 4 5 3
	*****************/
	typedef decltype(std::declval<T1>() + std::declval<T2>()) TR;
	farray<TR> narr(tarr);
	fsize_t fs = mask.flatsize();
	auto mask_iter = mask.cbegin();
	auto farr_iter = farr.cbegin();
	auto narr_iter = narr.begin();
	for (fsize_t i = 0; i < fs; i++, mask_iter++, farr_iter++, narr_iter++)
	{
		if (*mask_iter == false) {
			*narr_iter = *farr_iter;
		}
	}
	return _RTN(narr);
}
_NAMESPACE_FORTRAN_END

