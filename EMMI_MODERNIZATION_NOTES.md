# Network System Modernization Notes

## File-Based Locking to In-Memory Locking Migration

### Current Implementation Issues:
1. **File-Based Locking**: Uses `lockf()`/`flock()` on actual files
2. **Performance Overhead**: Disk I/O for every lock operation
3. **Scalability Issues**: Limited to single machine operation
4. **Cleanup Complexity**: Requires manual lock file removal

### Modernization Strategy:
Replace file-based locking with in-memory structures that can later be extended to socket-based networking.

## Implementation Details

### 1. New Lock Structure
```c
typedef struct {
    int is_locked;
    char locked_by[32];
    time_t lock_time;
    int ref_count;
} MEMORY_LOCK;
```

### 2. Lock Operations
- `memory_lock_acquire(slot, nation)` - Acquire lock for a slot
- `memory_lock_release(slot, nation)` - Release lock
- `memory_lock_check(slot, nation)` - Check if lock is held

### 3. Slot Mapping
- Slot 0: Player transfer operations
- Slot 1: Army transfer operations
- Slot 2: Navy/CVN transfer operations

### 4. Benefits
- **No Disk I/O**: All operations in memory
- **Thread-Safe Ready**: Structure supports synchronization primitives
- **Extensible**: Easy to add network layer later
- **Better Performance**: Eliminates file system bottlenecks

## Transition Plan

### Phase 1: In-Memory Locks (Current)
- Replace file operations with memory structures
- Maintain same API for compatibility
- Test with existing code

### Phase 2: Socket Layer (Future)
- Add network communication layer
- Implement lock server/client model
- Support distributed operations

### Phase 3: Full Networking
- TCP/UDP communication
- State synchronization
- Error handling and recovery

## API Compatibility
All new functions follow same parameter patterns as original:
- `PARM_2(int, slot, char *, nation)`
- `PARM_1(int, slot)`
- Return values match original (-1 on error, 0 on success)