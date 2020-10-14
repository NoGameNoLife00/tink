#ifndef TINK_POOL_SET_H
#define TINK_POOL_SET_H

#include <list>

namespace tink {
    template <class T> class PoolSet
    {
    protected:
        typedef  std::list<T*> PoolList;
    public:

        // poolSize 定义对象池的初始大小与是否自动增长，注意对象池的清空操作
        PoolSet(unsigned int poolSize = 100, unsigned int increaseStep = 10)
                : pool_size_(poolSize), increase_step_(increaseStep), active_count(0), free_count_(0)
        {
        }
        virtual ~PoolSet()
        {
        }
        void Init(unsigned int pool_size = 0){
            if (pool_size > 0)
            {
                pool_size_ = pool_size;
            }

            FillPool(pool_size_);
        }

        void DeletePool()
        {
            // Free pool items
            for (auto i = free_pool_ables_.begin();
                 i != free_pool_ables_.end(); ++i)
            {
                Deallocate(*i);
            }
            free_pool_ables_.clear();
            active_count = 0;
            pool_size_ = 0;
        }

        T* GetPoolItem()
        {
            if (free_pool_ables_.empty())
            {
                AutoExtend();
                if(free_pool_ables_.empty())
                    return 0;
            }
            // Get a new Object
            T* new_pool_item = free_pool_ables_.back();
            free_pool_ables_.pop_back();
            free_count_--;
            active_count++;
            return new_pool_item;
        }

        bool ReusePoolItem(T * const pool_item)
        {
            active_count--;
            if (free_count_ >= pool_size_){
                Deallocate(pool_item);
            }
            else{
                free_pool_ables_.push_back(pool_item);
                free_count_++;
            }

            return true;
        }

        void SetIncreaseStep(unsigned int step){
            if (step > 0){
                increase_step_ = step;
            }
        }
        unsigned int GetIncreaseStep()const{
            return increase_step_;
        }

        void SetPoolSize(const unsigned int size)
        {
            if (pool_size_ == size){
                return;
            }

            pool_size_ = size;
            if (free_count_ > size)
            {
                DecreasePool(free_count_ - size);
            }
        }

        unsigned int GetPoolSize(void) const
        {
            return pool_size_;
        }
        bool  IsFreePoolEmpty() const
        {
            return free_pool_ables_.empty();
        }
        unsigned int GetActiveCount()const{
            return active_count;
        }
        unsigned int GetFreeCount()const{
            return free_count_;
        }

    private:

        void AutoExtend()
        {
            FillPool(increase_step_);
        }

        void FillPool(unsigned int fillCount)
        {
            //assert (size > 0);
            for (unsigned int i = 0; i < fillCount; ++i){
                T* pNew = Allocate();
                if(pNew == 0)
                    return;
                free_pool_ables_.push_back(pNew);
                free_count_++;
            }
        }

        void DecreasePool(int delCount){
            while (delCount--){
                T* newPoolable = free_pool_ables_.back();
                Deallocate(newPoolable);
                free_pool_ables_.pop_back();
                free_count_--;
            }
        }
    protected:
        // 使用时根据对象重载分配和释放内存方法
        virtual T* Allocate() {
        	return new T;
        }
        virtual void Deallocate(T* p) {
        	delete p;
        }
    private:
        PoolList free_pool_ables_;
        unsigned int pool_size_;
        unsigned int increase_step_;
        unsigned int free_count_;
        int active_count;

    };
}

#endif //TINK_POOL_SET_H
