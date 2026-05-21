#include <iostream>
#include <atomic>
#include <new>
#include <cstddef>
#include <cstdlib>

class ByteCounter 
{
   public:
   
   /**
    * @brief Overloaded copy constructor to prevent copies (Singleton)
    * @param aOther is a ByteCounter  
   */
   ByteCounter(const ByteCounter &aOther) = delete;
   
   /**
    * @brief Overloaded assignment operator to prevent assignments (Singleton)
    * @param aOther is a Bytecounter
   */
   ByteCounter &operator=(const ByteCounter &aOther) = delete;
  
   /**
    * @brief Singleton Getter Method
    * @details Returns the global singleton instance of ByteCounter
   */
   static ByteCounter &getCounter()
   {
      static ByteCounter kByteCounterSingleton;
      return kByteCounterSingleton;
   }

   /**
    * @brief Getter method, returns number of allocated bytes 
    * @note Can be racey, value returned to client is subject to change
   */
   std::size_t getNumOfBytesAllocated() const 
   {
      return mNumOfBytes.load();
   }

   /**
    * @brief Increments mNumOfBytes
    * @param aNumberOfBytes is a std::size_t containing number of bytes
   */
   void incrementBytes(std::size_t aNumberOfBytes)
   {
      mNumOfBytes+=aNumberOfBytes;
   }

   /**
    * @brief Decrements mNumOfBytes
    * @param aNumberOfBytes is a std::size_t containing number of bytes
   */
   void decrementBytes(std::size_t aNumberOfBytes)
   {
      mNumOfBytes-=aNumberOfBytes;
   }

   private:
   /**
    * @brief Constructor (note: private bc singleton)
   */
   ByteCounter() : mNumOfBytes(0LL) {};

   /* Byte Counter Mem Var */
   std::atomic<long long> mNumOfBytes;
};

/* Overloaded Global Allocation Functions */

/**
 * @brief Overloaded new operator that will count the num of bytes allocated
 * @details We will store 
 * @param aSizeInBytes is a std::size_t 
*/
void *operator new(std::size_t aSizeInBytes)
{

   // 1. Attempt to allocate a memory of max alignment plus size of our object
   // We want to have a memory footprint of [size_t storing num of bytes of object][object]
   if (void *tRawPtr = std::malloc(sizeof(std::max_align_t) + aSizeInBytes))
   {
      // 2. Our tRawPtr looks like [][] 
      new (tRawPtr) std::size_t{aSizeInBytes};

      // 3. Our tRawPtr now looks like [aSizeInBytest][]
      ByteCounter::getCounter().incrementBytes(aSizeInBytes);

      // 4. This will return a pointer to the address just past aSizeInBytes 
      // Note: pointer arithmatic is type aware (+1) increments to -> +(sizeof(object))
      return static_cast<std::max_align_t*>(tRawPtr) + 1;   
   }
   else
   {
      throw std::bad_alloc{};
   }
}

/**
 * @brief Overloaded new[] operator that will count the num of bytes allocated
 * @details We will store 
 * @param aSizeInBytes is a std::size_t 
*/
void *operator new[](std::size_t aSizeInBytes)
{
   if (void *tRawPtr = std::malloc(sizeof(std::max_align_t) + aSizeInBytes))
   {
      new (tRawPtr) std::size_t{aSizeInBytes};
      ByteCounter::getCounter().incrementBytes(aSizeInBytes);
      return static_cast<std::max_align_t*>(tRawPtr) + 1;   
   }
   else
   {
      throw std::bad_alloc{};
   }
}

/**
 * @brief Overloaded delete operator that will decrement the num of bytes allocated
 * @param[in] aRawPtr is a void ptr
 * @note noexcept
*/
void operator delete(void *aRawPtr) noexcept 
{
   if (aRawPtr)
   {
      // 1. Ptr arithmetic to find beginning of block that was allocated
      aRawPtr = static_cast<std::max_align_t*>(aRawPtr) - 1;

      // 2. Decrement Counter
      ByteCounter::getCounter().decrementBytes(*static_cast<std::size_t*>(aRawPtr));

      // 3. Free the memory (compiler stores how much to free)                  
      std::free(aRawPtr);
   }
}

/**
 * @brief Overloaded delete[] operator that will decrement the num of bytes allocated
 * @param[in] aRawPtr is a void ptr
 * @note noexcept
*/
void operator delete[](void *aRawPtr) noexcept 
{
   if (aRawPtr)
   {
      aRawPtr = static_cast<std::max_align_t*>(aRawPtr) - 1;
      ByteCounter::getCounter().decrementBytes(*static_cast<std::size_t*>(aRawPtr));
      std::free(aRawPtr);
   }
}

int main()
{
   std::size_t tPreBytes = ByteCounter::getCounter().getNumOfBytesAllocated();

   {
      int *p = new int{ 3 };
      int *q = new int[10]{ }; 
      delete p;
   }

   std::size_t tPostBytes = ByteCounter::getCounter().getNumOfBytesAllocated();
   
   if (tPreBytes != tPostBytes)
   {
      std::cout << "We leaked: " << (tPostBytes - tPreBytes) << " bytes\n"; 
   }

   return 0;
}