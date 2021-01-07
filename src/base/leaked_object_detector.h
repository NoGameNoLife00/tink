#ifndef TINK_LEAKED_OBJECT_DETECTOR_H
#define TINK_LEAKED_OBJECT_DETECTOR_H


#include <atomic>
#include <cassert>
#include <iostream>

using std::atomic;
using std::cerr;
using std::endl;

template <typename OwnerClass>
class LeakedObjectDetector
{
public:
    LeakedObjectDetector() noexcept
    {
        ++(getCounter().num_objects);
    }

    LeakedObjectDetector(const LeakedObjectDetector&) noexcept
    {
        ++(getCounter().num_objects);
    }

    ~LeakedObjectDetector()
    {
        if(--(getCounter().num_objects) < 0)
        {
            std::cerr << "*** Dangling pointer deletion! Class: " << getLeakedObjectClassName() << endl;

            assert(false);
        }
    }

private:
    class LeakCounter
    {
    public:
        LeakCounter() = default;

        ~LeakCounter()
        {
            if(num_objects > 0)
            {
                cerr << "*** Leaked object detected: " << num_objects << " instance(s) of class" << getLeakedObjectClassName() << endl;
                assert(false);
            }
        }

        atomic<int> num_objects{0};
    };

    static const char* getLeakedObjectClassName()
    {
        return OwnerClass::getLeakedObjectClassName();
    }

    static LeakCounter& getCounter() noexcept
    {
        static LeakCounter counter;
        return counter;
    }
};

#define LINENAME_CAT(name, line) name##line
#define LEAK_DETECTOR(OwnerClass) \
        friend class LeakedObjectDetector<OwnerClass>;  \
        static const char* getLeakedObjectClassName() noexcept { return #OwnerClass; } \
        LeakedObjectDetector<OwnerClass>  LINENAME_CAT(leakDetector, __LINE__);


#endif //TINK_LEAKED_OBJECT_DETECTOR_H
