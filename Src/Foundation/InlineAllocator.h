#pragma once

	//Based on PhysX PsInlineAllocator

	template<unsigned Size, typename FallbackAllocator>
	class InlineAllocator : private FallbackAllocator
	{
	public:
		InlineAllocator(const FallbackAllocator& alloc = FallbackAllocator())
			: FallbackAllocator(alloc), bufferUsed(false)
		{}

		void* allocate(size_t size, const char* filename, int line)
		{
			if(!bufferUsed && size<=Size) 
			{
				bufferUsed = true;
				return inlineBuffer;
			}
			return FallbackAllocator::allocate(size, filename, line);
		}

		void deallocate(void* ptr)
		{
			if(ptr == inlineBuffer)
				bufferUsed = false;
			else
				FallbackAllocator::deallocate(ptr);
		}


	protected:
		char inlineBuffer[Size];
		bool bufferUsed;
	};
