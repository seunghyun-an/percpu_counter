// A simple spinlock structure
typedef struct {
    volatile int locked;
} spinlock_t;

// Atomic swap function using GCC/Clang built-ins
// This function atomically swaps the value of *ptr with new_val
// and returns the original value of *ptr.
static inline int atomic_swap(volatile int *ptr, int new_val) {
    return __atomic_exchange_n(ptr, new_val, __ATOMIC_SEQ_CST);
}

// Initialize the spinlock
void spinlock_init(spinlock_t *lock);

// Acquire the lock
void spinlock_lock(spinlock_t *lock);

// Release the lock
void spinlock_unlock(spinlock_t *lock);
