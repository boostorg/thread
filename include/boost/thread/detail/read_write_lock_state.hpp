#ifndef BOOST_READ_WRITE_LOCK_STATE_HPP
#define BOOST_READ_WRITE_LOCK_STATE_HPP

namespace boost
{
    namespace read_write_lock_state
    {
        enum read_write_lock_state_enum
        {
            unlocked=0,
            read_locked,
            write_locked
        };
    }
}

#endif
