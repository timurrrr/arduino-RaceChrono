#ifndef __RACECHRONO_PID_MAP_H
#define __RACECHRONO_PID_MAP_H

#include <algorithm>

// A map that allows keeping track of the requested update intervals for PIDs,
// as well as any "extra" information useful for a given application.
//
// The implementation uses an array sorted by PID, which provides O(N)
// runtime for insertions and O(log N) runtime for lookup. This could be
// optimized by using a hash map for O(1) runtimes, but that would come at extra
// code complexity, and extra memory usage. As long as the number of PIDs is
// kept to a reasonable minimum, the runtime for lookup shouldn't be an issue,
// and the runtime for insertions in negligible, given that it should only be
// done once per session.
template<typename ExtraType, uint16_t MaxNumPids = 128>
class RaceChronoPidMap {
public:
  void reset() {
    numEntries = 0;
    updateIntervalForAllEntries = NOT_ALL_PIDS_ALLOWED;
  }

  // Returns true on success, false if the map is full.
  bool allowOnePid(uint32_t pid, uint16_t updateIntervalMs) {
    Entry *entry =
        findEntry(pid, /* defaultUpdateIntervalMs = */ updateIntervalMs);
    return entry != nullptr;
  }

  void allowAllPids(uint16_t updateIntervalMs) {
    updateIntervalForAllEntries = updateIntervalMs;
  }

  // Returns true if allowAllPids() was called since the last reset().
  // You can optionally provide a *updateIntervalMsOut to get the last value
  // passed to allowAllPids().
  bool areAllPidsAllowed(uint16_t *updateIntervalMsOut = nullptr) const {
    if (updateIntervalForAllEntries == NOT_ALL_PIDS_ALLOWED) {
      return false;
    }

    if (updateIntervalMsOut != nullptr) {
      *updateIntervalMsOut = updateIntervalForAllEntries;
    }
    return true;
  }

  bool isEmpty() const {
    return numEntries == 0;
  }

  // Iterate over the registered PIDs and call the functor on all the entries.
  //
  // A functor can be something like this:
  //   struct {
  //     void operator() (void *entry) {
  //       uint32_t pid = pidMap.getPid(entry);
  //       uint16_t updateIntervalMs = pidMap.getUpdateIntervalMs(entry);
  //       MyExtra extra = pidMap.getExtra(entry);
  //       ...
  //     }
  //   } myFunctor;
  //   ...
  //   pidMap.forEach(myFunctor);
  template<typename FuncType>
  void forEach(FuncType functor) {
    void* entry = getFirstEntryId();

    while (entry != nullptr) {
      functor(entry);
      entry = getNextEntryId(entry);
    }
  }

  uint32_t getPid(const void *entry) const {
    Entry *realEntry = (Entry*)entry;
    return realEntry->pid;
  }

  uint16_t getUpdateIntervalMs(const void *entry) const {
    Entry *realEntry = (Entry*)entry;
    return realEntry->updateIntervalMs;
  }

  // Allows accessing the "extra" assosciated with the given entry.
  // This can be useful to track stuff such as when was the last message sent
  // for the PID assosciated with this entry.
  // TODO: add a const/const override.
  ExtraType* getExtra(void *entry) {
    Entry *realEntry = (Entry*)entry;
    return &realEntry->extra;
  }

  // Returns an opaque pointer for an entry for the given PID, or nullptr.
  // This is useful to avoid performing multiple lookups if you need to query
  // multiple things, such as the update interval, or the extra.
  //
  // A new entry is created in the map if allowAllPids() was called since the
  // last reset().
  // Don't store the pointer anywhere, as it will be invalidated if reset() or
  // allowOnePid() is called.
  void* getEntryId(uint32_t pid) {
    uint16_t defaultUpdateIntervalMs =
        updateIntervalForAllEntries != NOT_ALL_PIDS_ALLOWED
            ? updateIntervalForAllEntries
            : DONT_CREATE_NEW_ENTRY;
    return findEntry(pid, defaultUpdateIntervalMs);
  }

  // Get the entry for the lowest PID registered in the map.
  // Can be useful e.g. for printing the state of the map.
  void* getFirstEntryId() {
    if (numEntries == 0) {
      return nullptr;
    }
    return &_map[0];
  }

  // Returns the entry in the map after "entry", in the ascending PID order,
  // or nullptr if there are no more entries.
  // TODO: add a const/const override.
  void* getNextEntryId(void *entry) const {
    Entry *realEntry = (Entry*)entry;
    uint16_t index = realEntry - &_map[0];
    if (index < 0 || index + 1 >= numEntries) {
      return nullptr;
    }
    return realEntry + 1;
  }

private:
  struct Entry {
    uint32_t pid;
    uint16_t updateIntervalMs;
    ExtraType extra;

    // This is needed for std::lower_bound().
    bool operator< (uint32_t other_pid) { return pid < other_pid; }
  };

  static const uint16_t DONT_CREATE_NEW_ENTRY = 0xffff;
  static const uint16_t NOT_ALL_PIDS_ALLOWED = 0xffff;
  // Stores NOT_ALL_PIDS_ALLOWED, unless AllowAllPids() was called, in which
  // case it stores the default value to use for new entries.
  uint16_t updateIntervalForAllEntries = NOT_ALL_PIDS_ALLOWED;

  uint16_t numEntries = 0;
  Entry _map[MaxNumPids];  // Keep sorted by pid to allow O(log N) search.

  Entry* findEntry(
      uint32_t pid,
      uint16_t defaultUpdateIntervalMs) {
    Entry *insertionPosition = std::lower_bound(_map, _map + numEntries, pid);
    if (insertionPosition != _map + numEntries
        && insertionPosition->pid == pid) {
      // Found it.
      return insertionPosition;
    }

    if (defaultUpdateIntervalMs == DONT_CREATE_NEW_ENTRY) {
      return nullptr;
    }

    if (numEntries == MaxNumPids) {
      // Map is full.
      return nullptr;
    }

    std::copy_backward(
        insertionPosition,
        _map + numEntries,
        _map + numEntries + 1);
    numEntries++;
    insertionPosition->pid = pid;
    insertionPosition->updateIntervalMs = defaultUpdateIntervalMs;
    insertionPosition->extra = ExtraType();
    return insertionPosition;
  }
};

#endif  // __RACECHRONO_PID_MAP_H
