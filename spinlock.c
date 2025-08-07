#include "spinlock.h"

// Initialize the spinlock
void spinlock_init(spinlock_t *lock) {
    lock->locked = 0;
}

// Acquire the lock
void spinlock_lock(spinlock_t *lock) {
    while (atomic_swap(&lock->locked, 1)) {
        // Spin until the lock is acquired.
        // The atomic_swap operation returns the previous value of locked.
        // If it was 0 (false), we've successfully acquired the lock by setting it to 1 (true).
        // If it was 1 (true), another thread holds the lock, so we spin and try again.
    }
}

// Release the lock
void spinlock_unlock(spinlock_t *lock) {
    atomic_swap(&lock->locked, 0);
}
