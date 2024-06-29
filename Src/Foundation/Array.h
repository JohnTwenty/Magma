#pragma once

#include <new>
#include "Types.h"
#include "Foundation.h"

//based on PhysX PsArray

/*!
An array is a sequential container.

Implementation note
* entries between 0 and size are valid objects
* we use inheritance to build this because the array is included inline in a lot
of objects and we want the allocator to take no space if it's not stateful, which
aggregation doesn't allow. Also, we want the metadata at the front for the inline
case where the allocator contains some inline storage space
*/
template<class T, typename Alloc>
class Array : protected Alloc
	{
	public:

		typedef T*			Iterator;
		typedef const T*	ConstIterator;


		/*!
		Default array constructor. Initialize an empty array
		*/
		INLINE explicit Array(const Alloc& alloc = Alloc())
			: Alloc(alloc), mData(0), mSize(0), mCapacity(0)
			{}

		/*!
		Initialize array with given capacity
		*/
		INLINE explicit Array(MxU32 size, const T& a = T(), const Alloc& alloc = Alloc())
			: Alloc(alloc), mData(0), mSize(0), mCapacity(0)
			{
			resize(size, a);
			}

		/*!
		Copy-constructor. Copy all entries from other array
		*/
		template <class A>
		INLINE explicit Array(const Array<T, A>& other, const Alloc& alloc = Alloc())
			: Alloc(alloc)
			{
			copy(other);
			}

		// This is necessary else the basic default copy constructor is used in the case of both arrays being of the same template instance
		// The C++ standard clearly states that a template constructor is never a copy constructor [2]. In other words, 
		// the presence of a template constructor does not suppress the implicit declaration of the copy constructor.
		// Also never make a copy constructor explicit, or copy-initialization* will no longer work. This is because
		// 'binding an rvalue to a const reference requires an accessible copy constructor' (http://gcc.gnu.org/bugs/)
		// *http://stackoverflow.com/questions/1051379/is-there-a-difference-in-c-between-copy-initialization-and-assignment-initializ
		INLINE Array(const Array& other, const Alloc& alloc = Alloc())
			: Alloc(alloc)
			{
			copy(other);
			}

		/*!
		Initialize array with given length
		*/
		INLINE explicit Array(const T* first, const T* last, const Alloc& alloc = Alloc())
			: Alloc(alloc), mSize(last<first ? 0 : (MxU32)(last - first)), mCapacity(mSize)
			{
			mData = allocate(mSize);
			copy(mData, mData + mSize, first);
			}

		/*!
		Destructor
		*/
		INLINE ~Array()
			{
			destroy(mData, mData + mSize);

			if (capacity())
				deallocate(mData);
			}

		/*!
		Assignment operator. Copy content (deep-copy)
		*/
		template <class A>
		INLINE Array& operator= (const Array<T, A>& rhs)
			{
			if (&rhs == this)
				return *this;

			clear();
			reserve(rhs.mSize);
			copy(mData, mData + rhs.mSize, rhs.mData);

			mSize = rhs.mSize;
			return *this;
			}

		INLINE Array& operator= (const Array& t)  // Needs to be declared, see comment at copy-constructor
			{
			return operator=<Alloc>(t);
			}

		/*!
		Array indexing operator.
		\param i
		The index of the element that will be returned.
		\return
		The element i in the array.
		*/
		INLINE const T& operator[] (MxU32 i) const
			{
			ASSERT(i < mSize);
			return mData[i];
			}

		/*!
		Array indexing operator.
		\param i
		The index of the element that will be returned.
		\return
		The element i in the array.
		*/
		INLINE T& operator[] (MxU32 i)
			{
			ASSERT(i < mSize);
			return mData[i];
			}

		/*!
		Returns a pointer to the initial element of the array.
		\return
		a pointer to the initial element of the array.
		*/
		INLINE ConstIterator begin() const
			{
			return mData;
			}

		INLINE Iterator begin()
			{
			return mData;
			}

		/*!
		Returns an iterator beyond the last element of the array. Do not dereference.
		\return
		a pointer to the element beyond the last element of the array.
		*/

		INLINE ConstIterator end() const
			{
			return mData + mSize;
			}

		INLINE Iterator end()
			{
			return mData + mSize;
			}

		/*!
		Returns a reference to the first element of the array. Undefined if the array is empty.
		\return a reference to the first element of the array
		*/

		INLINE const T& front() const
			{
			ASSERT(mSize);
			return mData[0];
			}

		INLINE T& front()
			{
			ASSERT(mSize);
			return mData[0];
			}

		/*!
		Returns a reference to the last element of the array. Undefined if the array is empty
		\return a reference to the last element of the array
		*/

		INLINE const T& back() const
			{
			ASSERT(mSize);
			return mData[mSize - 1];
			}

		INLINE T& back()
			{
			ASSERT(mSize);
			return mData[mSize - 1];
			}


		/*!
		Returns the number of entries in the array. This can, and probably will,
		differ from the array capacity.
		\return
		The number of of entries in the array.
		*/
		INLINE MxU32 size() const
			{
			return mSize;
			}

		/*!
		Clears the array.
		*/
		INLINE void clear()
			{
			destroy(mData, mData + mSize);
			mSize = 0;
			}

		/*!
		Returns whether the array is empty (i.e. whether its size is 0).
		\return
		true if the array is empty
		*/
		INLINE bool empty() const
			{
			return mSize == 0;
			}

		/*!
		Finds the first occurrence of an element in the array.
		\param a
		The element to find.
		*/


		INLINE Iterator find(const T& a)
			{
			MxU32 index;
			for (index = 0; index<mSize && mData[index] != a; index++)
				;
			return mData + index;
			}

		INLINE ConstIterator find(const T& a) const
			{
			MxU32 index;
			for (index = 0; index<mSize && mData[index] != a; index++)
				;
			return mData + index;
			}


		/////////////////////////////////////////////////////////////////////////
		/*!
		Adds one element to the end of the array. Operation is O(1).
		\param a
		The element that will be added to this array.
		*/
		/////////////////////////////////////////////////////////////////////////

		INLINE T& pushBack(const T& a)
			{
			if (capacity() <= mSize)
				return growAndPushBack(a);

			//PX_PLACEMENT_NEW((void*)(mData + mSize), T)(a);
			new((void*)(mData + mSize)) T(a);

			return mData[mSize++];
			}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Returns the element at the end of the array. Only legal if the array is non-empty.
		*/
		/////////////////////////////////////////////////////////////////////////
		INLINE T popBack()
			{
			ASSERT(mSize);
			T t = mData[mSize - 1];
			mData[--mSize].~T();
			return t;
			}


		/////////////////////////////////////////////////////////////////////////
		/*!
		Construct one element at the end of the array. Operation is O(1).
		*/
		/////////////////////////////////////////////////////////////////////////
		INLINE T& insert()
			{
			if (capacity() <= mSize)
				grow(capacityIncrement());

			T* ptr = mData + mSize++;
			new (ptr)T; // not 'T()' because PODs should not get default-initialized.
			return *ptr;
			}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Subtracts the element on position i from the array and replace it with
		the last element.
		Operation is O(1)
		\param i
		The position of the element that will be subtracted from this array.
		\return
		The element that was removed.
		*/
		/////////////////////////////////////////////////////////////////////////
		INLINE void replaceWithLast(MxU32 i)
			{
			ASSERT(i<mSize);
			mData[i] = mData[--mSize];
			mData[mSize].~T();
			}

		INLINE void replaceWithLast(Iterator i)
			{
			replaceWithLast(static_cast<MxU32>(i - mData));
			}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Replaces the first occurrence of the element a with the last element
		Operation is O(n)
		\param i
		The position of the element that will be subtracted from this array.
		\return true if the element has been removed.
		*/
		/////////////////////////////////////////////////////////////////////////

		INLINE bool findAndReplaceWithLast(const T& a)
			{
			MxU32 index = 0;
			while (index<mSize && mData[index] != a)
				++index;
			if (index == mSize)
				return false;
			replaceWithLast(index);
			return true;
			}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Subtracts the element on position i from the array. Shift the entire
		array one step.
		Operation is O(n)
		\param i
		The position of the element that will be subtracted from this array.
		*/
		/////////////////////////////////////////////////////////////////////////
		INLINE void remove(MxU32 i)
			{
			ASSERT(i<mSize);
			for (T* it = mData + i; it->~T(), ++i<mSize; ++it)
				new(it)T(mData[i]);

			--mSize;
			}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Removes a range from the array.  Shifts the array so order is maintained.
		Operation is O(n)
		\param begin
		The starting position of the element that will be subtracted from this array.
		\param end
		The ending position of the elment that will be subtracted from this array.
		*/
		/////////////////////////////////////////////////////////////////////////
		INLINE void removeRange(MxU32 begin, MxU32 count)
			{
			ASSERT(begin<mSize);
			ASSERT((begin + count) <= mSize);
			for (MxU32 i = 0; i<count; i++)
				{
				mData[begin + i].~T(); // call the destructor on the ones being removed first.
				}
			T* dest = &mData[begin]; // location we are copying the tail end objects to
			T* src = &mData[begin + count]; // start of tail objects
			MxU32 move_count = mSize - (begin + count); // compute remainder that needs to be copied down
			for (MxU32 i = 0; i<move_count; i++)
				{
				new (dest)T(*src); // copy the old one to the new location
				src->~T(); // call the destructor on the old location
				dest++;
				src++;
				}
			mSize -= count;
			}


		//////////////////////////////////////////////////////////////////////////
		/*!
		Resize array
		*/
		//////////////////////////////////////////////////////////////////////////
		NOINLINE void resize(const MxU32 size, const T& a = T());

		NOINLINE void resizeUninitialized(const MxU32 size);

		//////////////////////////////////////////////////////////////////////////
		/*!
		Resize array such that only as much memory is allocated to hold the
		existing elements
		*/
		//////////////////////////////////////////////////////////////////////////
		INLINE void shrink()
			{
			recreate(mSize);
			}


		//////////////////////////////////////////////////////////////////////////
		/*!
		Deletes all array elements and frees memory.
		*/
		//////////////////////////////////////////////////////////////////////////
		INLINE void reset()
			{
			resize(0);
			shrink();
			}


		//////////////////////////////////////////////////////////////////////////
		/*!
		Ensure that the array has at least size capacity.
		*/
		//////////////////////////////////////////////////////////////////////////
		INLINE void reserve(const MxU32 capacity)
			{
			if (capacity > this->capacity())
				grow(capacity);
			}

		//////////////////////////////////////////////////////////////////////////
		/*!
		Query the capacity(allocated mem) for the array.
		*/
		//////////////////////////////////////////////////////////////////////////
		INLINE MxU32 capacity()	const
			{
			return mCapacity;
			}

		//////////////////////////////////////////////////////////////////////////
		/*!
		Swap contents of an array without allocating temporary storage
		*/
		//////////////////////////////////////////////////////////////////////////
		INLINE void swap(Array<T, Alloc>& other)
			{
			shdfnd::swap(mData, other.mData);
			shdfnd::swap(mSize, other.mSize);
			shdfnd::swap(mCapacity, other.mCapacity);
			}

		//////////////////////////////////////////////////////////////////////////
		/*!
		Assign a range of values to this vector (resizes to length of range)
		*/
		//////////////////////////////////////////////////////////////////////////
		INLINE void assign(const T* first, const T* last)
			{
			resizeUninitialized(MxU32(last - first));
			copy(begin(), end(), first);
			}

		void append(const T* src, MxU32 count)
			{
			MxU32 newlen = size() + count;
			reserve(newlen);
			copy(end(), end() + count, src);
			mSize += count;
			}

		/// return reference to allocator
		INLINE Alloc& getAllocator()
			{
			return *this;
			}

	protected:

		// constructor for where we don't own the memory
		Array(T* memory, MxU32 size, MxU32 capacity, const Alloc &alloc = Alloc()) :
			Alloc(alloc), mData(memory), mSize(size), mCapacity(capacity) {}

		template <class A>
		NOINLINE void copy(const Array<T, A>& other);

		INLINE T* allocate(MxU32 size)
			{
			if (size>0)
				{
				T* p = (T*)Alloc::allocate(sizeof(T)* size, __FILE__, __LINE__);
				/**
				Mark a specified amount of memory with 0xcd pattern. This is used to check that the meta data
				definition for serialized classes is complete in checked builds.
				*/
#if defined(PX_CHECKED)
				if (p)
					{
					for (MxU32 i = 0; i < (sizeof(T)* size); ++i)
						reinterpret_cast<PxU8*>(p)[i] = 0xcd;
					}
#endif
				return p;
				}
			return 0;
			}

		INLINE void deallocate(void* mem)
			{
			Alloc::deallocate(mem);
			}

		static INLINE void create(T* first, T* last, const T& a)
			{
			for (; first<last; ++first)
				::new(first)T(a);
			}

		static INLINE void copy(T* first, T* last, const T* src)
			{
			for (; first<last; ++first, ++src)
				::new (first)T(*src);
			}

		static INLINE void destroy(T* first, T* last)
			{
			for (; first<last; ++first)
				first->~T();
			}

		/*!
		Called when pushBack() needs to grow the array.
		\param a The element that will be added to this array.
		*/
		NOINLINE T& growAndPushBack(const T& a);

		/*!
		Resizes the available memory for the array.

		\param capacity
		The number of entries that the set should be able to hold.
		*/
		INLINE void grow(MxU32 capacity)
			{
			ASSERT(this->capacity() < capacity);
			recreate(capacity);
			}

		/*!
		Creates a new memory block, copies all entries to the new block and destroys old entries.

		\param capacity
		The number of entries that the set should be able to hold.
		*/
		NOINLINE void recreate(MxU32 capacity);

		// The idea here is to prevent accidental bugs with pushBack or insert. Unfortunately
		// it interacts badly with InlineArrays with smaller inline allocations.
		// TODO(dsequeira): policy template arg, this is exactly what they're for.
		INLINE MxU32 capacityIncrement()	const
			{
			const MxU32 capacity = this->capacity();
			return capacity == 0 ? 1 : capacity * 2;
			}

		T*					mData;
		MxU32				mSize;
		MxU32				mCapacity;
	};

template<class T, class Alloc>
NOINLINE void Array<T, Alloc>::resize(const MxU32 size, const T& a)
	{
	reserve(size);
	create(mData + mSize, mData + size, a);
	destroy(mData + size, mData + mSize);
	mSize = size;
	}

template<class T, class Alloc>
template<class A>
NOINLINE void Array<T, Alloc>::copy(const Array<T, A>& other)
	{
	if (!other.empty())
		{
		mData = allocate(mSize = mCapacity = other.size());
		copy(mData, mData + mSize, other.begin());
		}
	else
		{
		mData = NULL;
		mSize = 0;
		mCapacity = 0;
		}

	}

template<class T, class Alloc>
NOINLINE void Array<T, Alloc>::resizeUninitialized(const MxU32 size)
	{
	reserve(size);
	mSize = size;
	}

template<class T, class Alloc>
NOINLINE T& Array<T, Alloc>::growAndPushBack(const T& a)
	{
	MxU32 capacity = capacityIncrement();

	T* newData = allocate(capacity);
	ASSERT((!capacity) || (newData && (newData != mData)));
	copy(newData, newData + mSize, mData);

	// inserting element before destroying old array
	// avoids referencing destroyed object when duplicating array element.
	//PX_PLACEMENT_NEW((void*)(newData + mSize), T)(a);
	new((void*)(newData + mSize))T(a);

	destroy(mData, mData + mSize);
	deallocate(mData);

	mData = newData;
	mCapacity = capacity;

	return mData[mSize++];
	}

template<class T, class Alloc>
NOINLINE void Array<T, Alloc>::recreate(MxU32 capacity)
	{
	T* newData = allocate(capacity);
	ASSERT((!capacity) || (newData && (newData != mData)));

	copy(newData, newData + mSize, mData);
	destroy(mData, mData + mSize);
	deallocate(mData);

	mData = newData;
	mCapacity = capacity;
	}

template<class T, class Alloc>
INLINE void swap(Array<T, Alloc>& x, Array<T, Alloc>& y)
	{
	x.swap(y);
	}
