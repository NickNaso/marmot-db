// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <thread>
#include "gtest/gtest.h"

#include "core/faster.h"
#include "device/null_disk.h"

using namespace FASTER::core;
TEST(InMemFaster, UpsertRead) {
  class alignas(2) Key {
   public:
    Key(uint8_t key)
      : key_{ key } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Key));
    }
    inline KeyHash GetHash() const {
      std::hash<uint8_t> hash_fn;
      return KeyHash{ hash_fn(key_) };
    }

    /// Comparison operators.
    inline bool operator==(const Key& other) const {
      return key_ == other.key_;
    }
    inline bool operator!=(const Key& other) const {
      return key_ != other.key_;
    }

   private:
    uint8_t key_;
  };

  class UpsertContext;
  class ReadContext;

  class Value {
   public:
    Value()
      : value_{ 0 } {
    }
    Value(const Value& other)
      : value_{ other.value_ } {
    }
    Value(uint8_t value)
      : value_{ value } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Value));
    }

    friend class UpsertContext;
    friend class ReadContext;

   private:
    union {
      uint8_t value_;
      std::atomic<uint8_t> atomic_value_;
    };
  };

  class UpsertContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    UpsertContext(uint8_t key)
      : key_{ key } {
    }

    /// Copy (and deep-copy) constructor.
    UpsertContext(const UpsertContext& other)
      : key_{ other.key_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }
    inline static constexpr uint32_t value_size() {
      return sizeof(value_t);
    }
    /// Non-atomic and atomic Put() methods.
    inline void Put(Value& value) {
      value.value_ = 23;
    }
    inline bool PutAtomic(Value& value) {
      value.atomic_value_.store(42);
      return true;
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
  };

  class ReadContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    ReadContext(uint8_t key)
      : key_{ key } {
    }

    /// Copy (and deep-copy) constructor.
    ReadContext(const ReadContext& other)
      : key_{ other.key_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }

    inline void Get(const Value& value) {
      // All reads should be atomic (from the mutable tail).
      ASSERT_TRUE(false);
    }
    inline void GetAtomic(const Value& value) {
      output = value.atomic_value_.load();
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
   public:
    uint8_t output;
  };

  FasterKv<Key, Value, FASTER::device::NullDisk> store { 128, 1073741824, "" };

  store.StartSession();

  // Insert.
  for(size_t idx = 0; idx < 256; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    UpsertContext context{ static_cast<uint8_t>(idx) };
    Status result = store.Upsert(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
  }
  // Read.
  for(size_t idx = 0; idx < 256; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ static_cast<uint8_t>(idx) };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
    // All upserts should have inserts (non-atomic).
    ASSERT_EQ(23, context.output);
  }
  // Update.
  for(size_t idx = 0; idx < 256; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    UpsertContext context{ static_cast<uint8_t>(idx) };
    Status result = store.Upsert(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
  }
  // Read again.
  for(size_t idx = 0; idx < 256; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ static_cast<uint8_t>(idx) };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
    // All upserts should have updates (atomic).
    ASSERT_EQ(42, context.output);
  }

  store.StopSession();
}

/// The hash always returns "0," so the FASTER store devolves into a linked list.
TEST(InMemFaster, UpsertRead_DummyHash) {
  class UpsertContext;
  class ReadContext;

  class Key {
   public:
    Key(uint16_t key)
      : key_{ key } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Key));
    }
    inline KeyHash GetHash() const {
      return KeyHash{ 42 };
    }

    /// Comparison operators.
    inline bool operator==(const Key& other) const {
      return key_ == other.key_;
    }
    inline bool operator!=(const Key& other) const {
      return key_ != other.key_;
    }

    friend class UpsertContext;
    friend class ReadContext;

   private:
    uint16_t key_;
  };

  class Value {
   public:
    Value()
      : value_{ 0 } {
    }
    Value(const Value& other)
      : value_{ other.value_ } {
    }
    Value(uint16_t value)
      : value_{ value } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Value));
    }

    friend class UpsertContext;
    friend class ReadContext;

   private:
    union {
      uint16_t value_;
      std::atomic<uint16_t> atomic_value_;
    };
  };

  class UpsertContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    UpsertContext(uint16_t key)
      : key_{ key } {
    }

    /// Copy (and deep-copy) constructor.
    UpsertContext(const UpsertContext& other)
      : key_{ other.key_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }
    inline static constexpr uint32_t value_size() {
      return sizeof(value_t);
    }
    /// Non-atomic and atomic Put() methods.
    inline void Put(Value& value) {
      value.value_ = key_.key_;
    }
    inline bool PutAtomic(Value& value) {
      value.atomic_value_.store(key_.key_);
      return true;
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
  };

  class ReadContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    ReadContext(uint16_t key)
      : key_{ key } {
    }

    /// Copy (and deep-copy) constructor.
    ReadContext(const ReadContext& other)
      : key_{ other.key_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }

    inline void Get(const Value& value) {
      // All reads should be atomic (from the mutable tail).
      ASSERT_TRUE(false);
    }
    inline void GetAtomic(const Value& value) {
      output = value.atomic_value_.load();
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
   public:
    uint16_t output;
  };

  FasterKv<Key, Value, FASTER::device::NullDisk> store{ 128, 1073741824, "" };

  store.StartSession();

  // Insert.
  for(uint16_t idx = 0; idx < 10000; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    UpsertContext context{ idx };
    Status result = store.Upsert(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
  }
  // Read.
  for(uint16_t idx = 0; idx < 10000; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ idx };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
    // All upserts should have inserts (non-atomic).
    ASSERT_EQ(idx, context.output);
  }

  store.StopSession();
}

TEST(InMemFaster, UpsertRead_Concurrent) {
  class Key {
   public:
    Key(uint32_t key)
      : key_{ key } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Key));
    }
    inline KeyHash GetHash() const {
      std::hash<uint32_t> hash_fn;
      return KeyHash{ hash_fn(key_) };
    }

    /// Comparison operators.
    inline bool operator==(const Key& other) const {
      return key_ == other.key_;
    }
    inline bool operator!=(const Key& other) const {
      return key_ != other.key_;
    }

   private:
    uint32_t key_;
  };

  class UpsertContext;
  class ReadContext;

  class alignas(16) Value {
   public:
    Value()
      : length_{ 0 }
      , value_{ 0 } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Value));
    }

    friend class UpsertContext;
    friend class ReadContext;

   private:
    uint8_t value_[31];
    std::atomic<uint8_t> length_;
  };

  class UpsertContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    UpsertContext(uint32_t key)
      : key_{ key } {
    }

    /// Copy (and deep-copy) constructor.
    UpsertContext(const UpsertContext& other)
      : key_{ other.key_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }
    inline static constexpr uint32_t value_size() {
      return sizeof(value_t);
    }
    /// Non-atomic and atomic Put() methods.
    inline void Put(Value& value) {
      value.length_ = 5;
      std::memset(value.value_, 23, 5);
    }
    inline bool PutAtomic(Value& value) {
      // Get the lock on the value.
      bool success;
      do {
        uint8_t expected_length;
        do {
          // Spin until other the thread releases the lock.
          expected_length = value.length_.load();
        } while(expected_length == UINT8_MAX);
        // Try to get the lock.
        success = value.length_.compare_exchange_weak(expected_length, UINT8_MAX);
      } while(!success);

      std::memset(value.value_, 42, 7);
      value.length_.store(7);
      return true;
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
  };

  class ReadContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    ReadContext(uint32_t key)
      : key_{ key } {
    }

    /// Copy (and deep-copy) constructor.
    ReadContext(const ReadContext& other)
      : key_{ other.key_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }

    inline void Get(const Value& value) {
      // All reads should be atomic (from the mutable tail).
      ASSERT_TRUE(false);
    }
    inline void GetAtomic(const Value& value) {
      do {
        output_length = value.length_.load();
        ASSERT_EQ(0, reinterpret_cast<size_t>(value.value_) % 16);
        output_pt1 = *reinterpret_cast<const uint64_t*>(value.value_);
        output_pt2 = *reinterpret_cast<const uint64_t*>(value.value_ + 8);
      } while(output_length != value.length_.load());
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
   public:
    uint8_t output_length;
    uint64_t output_pt1;
    uint64_t output_pt2;
  };

  static constexpr size_t kNumOps = 1024;
  static constexpr size_t kNumThreads = 8;

  auto upsert_worker = [](FasterKv<Key, Value, FASTER::device::NullDisk>* store_,
  size_t thread_idx) {
    store_->StartSession();

    for(size_t idx = 0; idx < kNumOps; ++idx) {
      auto callback = [](IAsyncContext* ctxt, Status result) {
        // In-memory test.
        ASSERT_TRUE(false);
      };
      UpsertContext context{ static_cast<uint32_t>((thread_idx * kNumOps) + idx) };
      Status result = store_->Upsert(context, callback, 1);
      ASSERT_EQ(Status::Ok, result);
    }

    store_->StopSession();
  };

  auto read_worker = [](FasterKv<Key, Value, FASTER::device::NullDisk>* store_,
  size_t thread_idx, uint64_t expected_value) {
    store_->StartSession();

    for(size_t idx = 0; idx < kNumOps; ++idx) {
      auto callback = [](IAsyncContext* ctxt, Status result) {
        // In-memory test.
        ASSERT_TRUE(false);
      };
      ReadContext context{ static_cast<uint32_t>((thread_idx * kNumOps) + idx) };
      Status result = store_->Read(context, callback, 1);
      ASSERT_EQ(Status::Ok, result);
      ASSERT_EQ(expected_value, context.output_pt1);
    }

    store_->StopSession();
  };

  FasterKv<Key, Value, FASTER::device::NullDisk> store{ 128, 1073741824, "" };

  // Insert.
  std::deque<std::thread> threads{};
  for(size_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(upsert_worker, &store, idx);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Read.
  threads.clear();
  for(size_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(read_worker, &store, idx, 0x1717171717);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Update.
  threads.clear();
  for(size_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(upsert_worker, &store, idx);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Read again.
  threads.clear();
  for(size_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(read_worker, &store, idx, 0x2a2a2a2a2a2a2a);
  }
  for(auto& thread : threads) {
    thread.join();
  }
}

TEST(InMemFaster, UpsertRead_ResizeValue_Concurrent) {
  class Key {
   public:
    Key(uint32_t key)
      : key_{ key } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Key));
    }
    inline KeyHash GetHash() const {
      std::hash<uint32_t> hash_fn;
      return KeyHash{ hash_fn(key_) };
    }

    /// Comparison operators.
    inline bool operator==(const Key& other) const {
      return key_ == other.key_;
    }
    inline bool operator!=(const Key& other) const {
      return key_ != other.key_;
    }

   private:
    uint32_t key_;
  };

  class UpsertContext;
  class ReadContext;

  class GenLock {
   public:
    GenLock()
      : control_{ 0 } {
    }
    GenLock(uint64_t control)
      : control_{ control } {
    }
    inline GenLock& operator=(const GenLock& other) {
      control_ = other.control_;
      return *this;
    }

    union {
        struct {
          uint64_t gen_number : 62;
          uint64_t locked : 1;
          uint64_t replaced : 1;
        };
        uint64_t control_;
      };
  };
  static_assert(sizeof(GenLock) == 8, "sizeof(GenLock) != 8");

  class AtomicGenLock {
   public:
    AtomicGenLock()
      : control_{ 0 } {
    }
    AtomicGenLock(uint64_t control)
      : control_{ control } {
    }

    inline GenLock load() const {
      return GenLock{ control_.load() };
    }
    inline void store(GenLock desired) {
      control_.store(desired.control_);
    }

    inline bool try_lock(bool& replaced) {
      replaced = false;
      GenLock expected{ control_.load() };
      expected.locked = 0;
      expected.replaced = 0;
      GenLock desired{ expected.control_ };
      desired.locked = 1;

      if(control_.compare_exchange_strong(expected.control_, desired.control_)) {
        return true;
      }
      if(expected.replaced) {
        replaced = true;
      }
      return false;
    }
    inline void unlock(bool replaced) {
      if(replaced) {
        // Just turn off "locked" bit and increase gen number.
        uint64_t sub_delta = ((uint64_t)1 << 62) - 1;
        control_.fetch_sub(sub_delta);
      } else {
        // Turn off "locked" bit, turn on "replaced" bit, and increase gen number
        uint64_t add_delta = ((uint64_t)1 << 63) - ((uint64_t)1 << 62) + 1;
        control_.fetch_add(add_delta);
      }
    }

   private:
    std::atomic<uint64_t> control_;
  };
  static_assert(sizeof(AtomicGenLock) == 8, "sizeof(AtomicGenLock) != 8");

  class Value {
   public:
    Value()
      : gen_lock_{ 0 }
      , size_{ 0 }
      , length_{ 0 } {
    }

    inline uint32_t size() const {
      return size_;
    }

    friend class UpsertContext;
    friend class ReadContext;

   private:
    AtomicGenLock gen_lock_;
    uint32_t size_;
    uint32_t length_;

    inline const uint8_t* buffer() const {
      return reinterpret_cast<const uint8_t*>(this + 1);
    }
    inline uint8_t* buffer() {
      return reinterpret_cast<uint8_t*>(this + 1);
    }
  };

  class UpsertContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    UpsertContext(uint32_t key, uint32_t length)
      : key_{ key }
      , length_{ length } {
    }

    /// Copy (and deep-copy) constructor.
    UpsertContext(const UpsertContext& other)
      : key_{ other.key_ }
      , length_{ other.length_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }
    inline uint32_t value_size() const {
      return sizeof(Value) + length_;
    }
    /// Non-atomic and atomic Put() methods.
    inline void Put(Value& value) {
      value.gen_lock_.store(0);
      value.size_ = sizeof(Value) + length_;
      value.length_ = length_;
      std::memset(value.buffer(), 88, length_);
    }
    inline bool PutAtomic(Value& value) {
      bool replaced;
      while(!value.gen_lock_.try_lock(replaced) && !replaced) {
        std::this_thread::yield();
      }
      if(replaced) {
        // Some other thread replaced this record.
        return false;
      }
      if(value.size_ < sizeof(Value) + length_) {
        // Current value is too small for in-place update.
        value.gen_lock_.unlock(true);
        return false;
      }
      // In-place update overwrites length and buffer, but not size.
      value.length_ = length_;
      std::memset(value.buffer(), 88, length_);
      value.gen_lock_.unlock(false);
      return true;
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
    uint32_t length_;
  };

  class ReadContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    ReadContext(uint32_t key)
      : key_{ key }
      , output_length{ 0 } {
    }

    /// Copy (and deep-copy) constructor.
    ReadContext(const ReadContext& other)
      : key_{ other.key_ }
      , output_length{ 0 } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }

    inline void Get(const Value& value) {
      // All reads should be atomic (from the mutable tail).
      ASSERT_TRUE(false);
    }
    inline void GetAtomic(const Value& value) {
      GenLock before, after;
      do {
        before = value.gen_lock_.load();
        output_length = value.length_;
        output_bytes[0] = value.buffer()[0];
        output_bytes[1] = value.buffer()[value.length_ - 1];
        after = value.gen_lock_.load();
      } while(before.gen_number != after.gen_number);
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
   public:
    uint8_t output_length;
    // Extract two bytes of output.
    uint8_t output_bytes[2];
  };

  static constexpr size_t kNumOps = 1024;
  static constexpr size_t kNumThreads = 8;

  auto upsert_worker = [](FasterKv<Key, Value, FASTER::device::NullDisk>* store_,
  size_t thread_idx, uint32_t value_length) {
    store_->StartSession();

    for(size_t idx = 0; idx < kNumOps; ++idx) {
      auto callback = [](IAsyncContext* ctxt, Status result) {
        // In-memory test.
        ASSERT_TRUE(false);
      };
      UpsertContext context{ static_cast<uint32_t>((thread_idx * kNumOps) + idx), value_length };
      Status result = store_->Upsert(context, callback, 1);
      ASSERT_EQ(Status::Ok, result);
    }

    store_->StopSession();
  };

  auto read_worker = [](FasterKv<Key, Value, FASTER::device::NullDisk>* store_,
  size_t thread_idx, uint8_t expected_value) {
    store_->StartSession();

    for(size_t idx = 0; idx < kNumOps; ++idx) {
      auto callback = [](IAsyncContext* ctxt, Status result) {
        // In-memory test.
        ASSERT_TRUE(false);
      };
      ReadContext context{ static_cast<uint32_t>((thread_idx * kNumOps) + idx) };
      Status result = store_->Read(context, callback, 1);
      ASSERT_EQ(Status::Ok, result);
      ASSERT_EQ(expected_value, context.output_bytes[0]);
      ASSERT_EQ(expected_value, context.output_bytes[1]);
    }

    store_->StopSession();
  };

  FasterKv<Key, Value, FASTER::device::NullDisk> store{ 128, 1073741824, "" };

  // Insert.
  std::deque<std::thread> threads{};
  for(size_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(upsert_worker, &store, idx, 7);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Read.
  threads.clear();
  for(size_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(read_worker, &store, idx, 88);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Update.
  threads.clear();
  for(size_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(upsert_worker, &store, idx, 11);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Read again.
  threads.clear();
  for(size_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(read_worker, &store, idx, 88);
  }
  for(auto& thread : threads) {
    thread.join();
  }
}
TEST(InMemFaster, Rmw) {
  class Key {
   public:
    Key(uint64_t key)
      : key_{ key } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Key));
    }
    inline KeyHash GetHash() const {
      std::hash<uint64_t> hash_fn;
      return KeyHash{ hash_fn(key_) };
    }

    /// Comparison operators.
    inline bool operator==(const Key& other) const {
      return key_ == other.key_;
    }
    inline bool operator!=(const Key& other) const {
      return key_ != other.key_;
    }

   private:
    uint64_t key_;
  };

  class RmwContext;
  class ReadContext;

  class Value {
   public:
    Value()
      : value_{ 0 } {
    }
    Value(const Value& other)
      : value_{ other.value_ } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Value));
    }

    friend class RmwContext;
    friend class ReadContext;

   private:
    union {
      int32_t value_;
      std::atomic<int32_t> atomic_value_;
    };
  };

  class RmwContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    RmwContext(uint64_t key, int32_t incr)
      : key_{ key }
      , incr_{ incr } {
    }

    /// Copy (and deep-copy) constructor.
    RmwContext(const RmwContext& other)
      : key_{ other.key_ }
      , incr_{ other.incr_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }
    inline static constexpr uint32_t value_size() {
      return sizeof(value_t);
    }
    inline void RmwInitial(Value& value) {
      value.value_ = incr_;
    }
    inline void RmwCopy(const Value& old_value, Value& value) {
      value.value_ = old_value.value_ + incr_;
    }
    inline bool RmwAtomic(Value& value) {
      value.atomic_value_.fetch_add(incr_);
      return true;
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    int32_t incr_;
    Key key_;
  };

  class ReadContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    ReadContext(uint64_t key)
      : key_{ key } {
    }

    /// Copy (and deep-copy) constructor.
    ReadContext(const ReadContext& other)
      : key_{ other.key_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }

    inline void Get(const Value& value) {
      // All reads should be atomic (from the mutable tail).
      ASSERT_TRUE(false);
    }
    inline void GetAtomic(const Value& value) {
      output = value.atomic_value_.load();
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
   public:
    int32_t output;
  };

  FasterKv<Key, Value, FASTER::device::NullDisk> store{ 256, 1073741824, "" };

  store.StartSession();

  // Rmw, increment by 1.
  for(size_t idx = 0; idx < 2048; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    RmwContext context{ idx % 512, 1 };
    Status result = store.Rmw(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
  }
  // Read.
  for(size_t idx = 0; idx < 512; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ idx };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result) << idx;
    // Should have performed 4 RMWs.
    ASSERT_EQ(4, context.output);
  }
  // Rmw, decrement by 1.
  for(size_t idx = 0; idx < 2048; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    RmwContext context{ idx % 512, -1 };
    Status result = store.Rmw(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
  }
  // Read again.
  for(size_t idx = 0; idx < 512; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ static_cast<uint8_t>(idx) };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
    // All upserts should have inserts (non-atomic).
    ASSERT_EQ(0, context.output);
  }

  store.StopSession();
}

TEST(InMemFaster, Rmw_Concurrent) {
  class Key {
   public:
    Key(uint64_t key)
      : key_{ key } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Key));
    }
    inline KeyHash GetHash() const {
      std::hash<uint64_t> hash_fn;
      return KeyHash{ hash_fn(key_) };
    }

    /// Comparison operators.
    inline bool operator==(const Key& other) const {
      return key_ == other.key_;
    }
    inline bool operator!=(const Key& other) const {
      return key_ != other.key_;
    }

   private:
    uint64_t key_;
  };

  class RmwContext;
  class ReadContext;

  class Value {
   public:
    Value()
      : value_{ 0 } {
    }
    Value(const Value& other)
      : value_{ other.value_ } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Value));
    }

    friend class RmwContext;
    friend class ReadContext;

   private:
    union {
      int64_t value_;
      std::atomic<int64_t> atomic_value_;
    };
  };

  class RmwContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    RmwContext(uint64_t key, int64_t incr)
      : key_{ key }
      , incr_{ incr } {
    }

    /// Copy (and deep-copy) constructor.
    RmwContext(const RmwContext& other)
      : key_{ other.key_ }
      , incr_{ other.incr_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }
    inline static constexpr uint32_t value_size() {
      return sizeof(value_t);
    }

    inline void RmwInitial(Value& value) {
      value.value_ = incr_;
    }
    inline void RmwCopy(const Value& old_value, Value& value) {
      value.value_ = old_value.value_ + incr_;
    }
    inline bool RmwAtomic(Value& value) {
      value.atomic_value_.fetch_add(incr_);
      return true;
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    int64_t incr_;
    Key key_;
  };

  class ReadContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    ReadContext(uint64_t key)
      : key_{ key } {
    }

    /// Copy (and deep-copy) constructor.
    ReadContext(const ReadContext& other)
      : key_{ other.key_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }

    inline void Get(const Value& value) {
      // All reads should be atomic (from the mutable tail).
      ASSERT_TRUE(false);
    }
    inline void GetAtomic(const Value& value) {
      output = value.atomic_value_.load();
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
   public:
    int64_t output;
  };

  static constexpr size_t kNumThreads = 8;
  static constexpr size_t kNumRmws = 2048;
  static constexpr size_t kRange = 512;

  auto rmw_worker = [](FasterKv<Key, Value, FASTER::device::NullDisk>* store_,
  int64_t incr) {
    store_->StartSession();

    for(size_t idx = 0; idx < kNumRmws; ++idx) {
      auto callback = [](IAsyncContext* ctxt, Status result) {
        // In-memory test.
        ASSERT_TRUE(false);
      };
      RmwContext context{ idx % kRange, incr };
      Status result = store_->Rmw(context, callback, 1);
      ASSERT_EQ(Status::Ok, result);
    }

    store_->StopSession();
  };

  FasterKv<Key, Value, FASTER::device::NullDisk> store{ 256, 1073741824, "" };

  // Rmw, increment by 2 * idx.
  std::deque<std::thread> threads{};
  for(int64_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(rmw_worker, &store, 2 * idx);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Read.
  store.StartSession();

  for(size_t idx = 0; idx < kRange; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ idx };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result) << idx;
    // Should have performed 4 RMWs.
    ASSERT_EQ((kNumThreads * (kNumThreads - 1)) * (kNumRmws / kRange), context.output);
  }

  store.StopSession();

  // Rmw, decrement by idx.
  threads.clear();
  for(int64_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(rmw_worker, &store, -idx);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Read again.
  store.StartSession();

  for(size_t idx = 0; idx < kRange; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ static_cast<uint8_t>(idx) };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
    // All upserts should have inserts (non-atomic).
    ASSERT_EQ(((kNumThreads * (kNumThreads - 1)) / 2) * (kNumRmws / kRange), context.output);
  }

  store.StopSession();
}

TEST(InMemFaster, Rmw_ResizeValue_Concurrent) {
  class Key {
   public:
    Key(uint64_t key)
      : key_{ key } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Key));
    }
    inline KeyHash GetHash() const {
      std::hash<uint64_t> hash_fn;
      return KeyHash{ hash_fn(key_) };
    }

    /// Comparison operators.
    inline bool operator==(const Key& other) const {
      return key_ == other.key_;
    }
    inline bool operator!=(const Key& other) const {
      return key_ != other.key_;
    }

   private:
    uint64_t key_;
  };

  class RmwContext;
  class ReadContext;

  class GenLock {
   public:
    GenLock()
      : control_{ 0 } {
    }
    GenLock(uint64_t control)
      : control_{ control } {
    }
    inline GenLock& operator=(const GenLock& other) {
      control_ = other.control_;
      return *this;
    }

    union {
        struct {
          uint64_t gen_number : 62;
          uint64_t locked : 1;
          uint64_t replaced : 1;
        };
        uint64_t control_;
      };
  };
  static_assert(sizeof(GenLock) == 8, "sizeof(GenLock) != 8");

  class AtomicGenLock {
   public:
    AtomicGenLock()
      : control_{ 0 } {
    }
    AtomicGenLock(uint64_t control)
      : control_{ control } {
    }

    inline GenLock load() const {
      return GenLock{ control_.load() };
    }
    inline void store(GenLock desired) {
      control_.store(desired.control_);
    }

    inline bool try_lock(bool& replaced) {
      replaced = false;
      GenLock expected{ control_.load() };
      expected.locked = 0;
      expected.replaced = 0;
      GenLock desired{ expected.control_ };
      desired.locked = 1;

      if(control_.compare_exchange_strong(expected.control_, desired.control_)) {
        return true;
      }
      if(expected.replaced) {
        replaced = true;
      }
      return false;
    }
    inline void unlock(bool replaced) {
      if(replaced) {
        // Just turn off "locked" bit and increase gen number.
        uint64_t sub_delta = ((uint64_t)1 << 62) - 1;
        control_.fetch_sub(sub_delta);
      } else {
        // Turn off "locked" bit, turn on "replaced" bit, and increase gen number
        uint64_t add_delta = ((uint64_t)1 << 63) - ((uint64_t)1 << 62) + 1;
        control_.fetch_add(add_delta);
      }
    }

   private:
    std::atomic<uint64_t> control_;
  };
  static_assert(sizeof(AtomicGenLock) == 8, "sizeof(AtomicGenLock) != 8");

  class Value {
   public:
    Value()
      : gen_lock_{ 0 }
      , size_{ 0 }
      , length_{ 0 } {
    }

    inline uint32_t size() const {
      return size_;
    }

    friend class RmwContext;
    friend class ReadContext;

   private:
    AtomicGenLock gen_lock_;
    uint32_t size_;
    uint32_t length_;

    inline const int8_t* buffer() const {
      return reinterpret_cast<const int8_t*>(this + 1);
    }
    inline int8_t* buffer() {
      return reinterpret_cast<int8_t*>(this + 1);
    }
  };

  class RmwContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    RmwContext(uint64_t key, int8_t incr, uint32_t length)
      : key_{ key }
      , incr_{ incr }
      , length_{ length } {
    }

    /// Copy (and deep-copy) constructor.
    RmwContext(const RmwContext& other)
      : key_{ other.key_ }
      , incr_{ other.incr_ }
      , length_{ other.length_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }
    inline uint32_t value_size() const {
      return sizeof(value_t) + length_;
    }

    inline void RmwInitial(Value& value) {
      value.gen_lock_.store(GenLock{});
      value.size_ = sizeof(Value) + length_;
      value.length_ = length_;
      std::memset(value.buffer(), incr_, length_);
    }
    inline void RmwCopy(const Value& old_value, Value& value) {
      value.gen_lock_.store(GenLock{});
      value.size_ = sizeof(Value) + length_;
      value.length_ = length_;
      std::memset(value.buffer(), incr_, length_);
      for(uint32_t idx = 0; idx < std::min(old_value.length_, length_); ++idx) {
        value.buffer()[idx] = old_value.buffer()[idx] + incr_;
      }
    }
    inline bool RmwAtomic(Value& value) {
      bool replaced;
      while(!value.gen_lock_.try_lock(replaced) && !replaced) {
        std::this_thread::yield();
      }
      if(replaced) {
        // Some other thread replaced this record.
        return false;
      }
      if(value.size_ < sizeof(Value) + length_) {
        // Current value is too small for in-place update.
        value.gen_lock_.unlock(true);
        return false;
      }
      // In-place update overwrites length and buffer, but not size.
      value.length_ = length_;
      for(uint32_t idx = 0; idx < length_; ++idx) {
        value.buffer()[idx] += incr_;
      }
      value.gen_lock_.unlock(false);
      return true;
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    int8_t incr_;
    uint32_t length_;
    Key key_;
  };

  class ReadContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    ReadContext(uint64_t key)
      : key_{ key }
      , output_length{ 0 } {
    }

    /// Copy (and deep-copy) constructor.
    ReadContext(const ReadContext& other)
      : key_{ other.key_ }
      , output_length{ 0 } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }

    inline void Get(const Value& value) {
      // All reads should be atomic (from the mutable tail).
      ASSERT_TRUE(false);
    }
    inline void GetAtomic(const Value& value) {
      GenLock before, after;
      do {
        before = value.gen_lock_.load();
        output_length = value.length_;
        output_bytes[0] = value.buffer()[0];
        output_bytes[1] = value.buffer()[value.length_ - 1];
        after = value.gen_lock_.load();
      } while(before.gen_number != after.gen_number);
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
   public:
    uint8_t output_length;
    // Extract two bytes of output.
    int8_t output_bytes[2];
  };

  static constexpr int8_t kNumThreads = 8;
  static constexpr size_t kNumRmws = 2048;
  static constexpr size_t kRange = 512;

  auto rmw_worker = [](FasterKv<Key, Value, FASTER::device::NullDisk>* store_,
  int8_t incr, uint32_t value_length) {
    store_->StartSession();

    for(size_t idx = 0; idx < kNumRmws; ++idx) {
      auto callback = [](IAsyncContext* ctxt, Status result) {
        // In-memory test.
        ASSERT_TRUE(false);
      };
      RmwContext context{ idx % kRange, incr, value_length };
      Status result = store_->Rmw(context, callback, 1);
      ASSERT_EQ(Status::Ok, result);
    }

    store_->StopSession();
  };

  FasterKv<Key, Value, FASTER::device::NullDisk> store{ 256, 1073741824, "" };

  // Rmw, increment by 3.
  std::deque<std::thread> threads{};
  for(int64_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(rmw_worker, &store, 3, 5);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Read.
  store.StartSession();

  for(size_t idx = 0; idx < kRange; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ idx };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result) << idx;
    // Should have performed 4 RMWs.
    ASSERT_EQ(5, context.output_length);
    ASSERT_EQ(kNumThreads * 4 * 3, context.output_bytes[0]);
    ASSERT_EQ(kNumThreads * 4 * 3, context.output_bytes[1]);
  }

  store.StopSession();

  // Rmw, decrement by 4.
  threads.clear();
  for(int64_t idx = 0; idx < kNumThreads; ++idx) {
    threads.emplace_back(rmw_worker, &store, -4, 8);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Read again.
  store.StartSession();

  for(size_t idx = 0; idx < kRange; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ static_cast<uint8_t>(idx) };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
    // Should have performed 4 RMWs.
    ASSERT_EQ(8, context.output_length);
    ASSERT_EQ(kNumThreads * -4, context.output_bytes[0]);
    ASSERT_EQ(kNumThreads * -16, context.output_bytes[1]);
  }

  store.StopSession();
}

TEST(InMemFaster, GrowHashTable) {
  class Key {
   public:
    Key(uint64_t key)
      : key_{ key } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Key));
    }
    inline KeyHash GetHash() const {
      std::hash<uint64_t> hash_fn;
      return KeyHash{ hash_fn(key_) };
    }

    /// Comparison operators.
    inline bool operator==(const Key& other) const {
      return key_ == other.key_;
    }
    inline bool operator!=(const Key& other) const {
      return key_ != other.key_;
    }

   private:
    uint64_t key_;
  };

  class RmwContext;
  class ReadContext;

  class Value {
   public:
    Value()
      : value_{ 0 } {
    }
    Value(const Value& other)
      : value_{ other.value_ } {
    }

    inline static constexpr uint32_t size() {
      return static_cast<uint32_t>(sizeof(Value));
    }

    friend class RmwContext;
    friend class ReadContext;

   private:
    union {
      int64_t value_;
      std::atomic<int64_t> atomic_value_;
    };
  };

  class RmwContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    RmwContext(uint64_t key, int64_t incr)
      : key_{ key }
      , incr_{ incr } {
    }

    /// Copy (and deep-copy) constructor.
    RmwContext(const RmwContext& other)
      : key_{ other.key_ }
      , incr_{ other.incr_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }
    inline static constexpr uint32_t value_size() {
      return sizeof(value_t);
    }

    inline void RmwInitial(Value& value) {
      value.value_ = incr_;
    }
    inline void RmwCopy(const Value& old_value, Value& value) {
      value.value_ = old_value.value_ + incr_;
    }
    inline bool RmwAtomic(Value& value) {
      value.atomic_value_.fetch_add(incr_);
      return true;
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    int64_t incr_;
    Key key_;
  };

  class ReadContext : public IAsyncContext {
   public:
    typedef Key key_t;
    typedef Value value_t;

    ReadContext(uint64_t key)
      : key_{ key } {
    }

    /// Copy (and deep-copy) constructor.
    ReadContext(const ReadContext& other)
      : key_{ other.key_ } {
    }

    /// The implicit and explicit interfaces require a key() accessor.
    inline const Key& key() const {
      return key_;
    }

    inline void Get(const Value& value) {
      // All reads should be atomic (from the mutable tail).
      ASSERT_TRUE(false);
    }
    inline void GetAtomic(const Value& value) {
      output = value.atomic_value_.load();
    }

   protected:
    /// The explicit interface requires a DeepCopy_Internal() implementation.
    Status DeepCopy_Internal(IAsyncContext*& context_copy) {
      return IAsyncContext::DeepCopy_Internal(*this, context_copy);
    }

   private:
    Key key_;
   public:
    int64_t output;
  };

  static constexpr size_t kNumThreads = 8;
  static constexpr size_t kNumRmws = 32768;
  static constexpr size_t kRange = 8192;

  static std::atomic<bool> grow_done{ false };

  auto rmw_worker0 = [](FasterKv<Key, Value, FASTER::device::NullDisk>* store_,
  int64_t incr) {
    auto callback = [](uint64_t new_size) {
      grow_done = true;
    };

    store_->StartSession();

    for(size_t idx = 0; idx < kNumRmws; ++idx) {
      auto callback = [](IAsyncContext* ctxt, Status result) {
        // In-memory test.
        ASSERT_TRUE(false);
      };
      RmwContext context{ idx % kRange, incr };
      Status result = store_->Rmw(context, callback, 1);
      ASSERT_EQ(Status::Ok, result);
    }

    // Double the size of the index.
    store_->GrowIndex(callback);

    while(!grow_done) {
      store_->Refresh();
      std::this_thread::yield();
    }

    store_->StopSession();
  };

  auto rmw_worker = [](FasterKv<Key, Value, FASTER::device::NullDisk>* store_,
  int64_t incr) {
    store_->StartSession();

    for(size_t idx = 0; idx < kNumRmws; ++idx) {
      auto callback = [](IAsyncContext* ctxt, Status result) {
        // In-memory test.
        ASSERT_TRUE(false);
      };
      RmwContext context{ idx % kRange, incr };
      Status result = store_->Rmw(context, callback, 1);
      ASSERT_EQ(Status::Ok, result);
    }

    while(!grow_done) {
      store_->Refresh();
      std::this_thread::yield();
    }

    store_->StopSession();
  };

  FasterKv<Key, Value, FASTER::device::NullDisk> store{ 256, 1073741824, "" };

  // Rmw, increment by 2 * idx.
  std::deque<std::thread> threads{};
  threads.emplace_back(rmw_worker0, &store, 0);
  for(int64_t idx = 1; idx < kNumThreads; ++idx) {
    threads.emplace_back(rmw_worker, &store, 2 * idx);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Read.
  store.StartSession();

  for(size_t idx = 0; idx < kRange; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ idx };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result) << idx;
    // Should have performed 4 RMWs.
    ASSERT_EQ((kNumThreads * (kNumThreads - 1)) * (kNumRmws / kRange), context.output);
  }

  store.StopSession();

  // Rmw, decrement by idx.
  grow_done = false;
  threads.clear();
  threads.emplace_back(rmw_worker0, &store, 0);
  for(int64_t idx = 1; idx < kNumThreads; ++idx) {
    threads.emplace_back(rmw_worker, &store, -idx);
  }
  for(auto& thread : threads) {
    thread.join();
  }

  // Read again.
  store.StartSession();

  for(size_t idx = 0; idx < kRange; ++idx) {
    auto callback = [](IAsyncContext* ctxt, Status result) {
      // In-memory test.
      ASSERT_TRUE(false);
    };
    ReadContext context{ static_cast<uint8_t>(idx) };
    Status result = store.Read(context, callback, 1);
    ASSERT_EQ(Status::Ok, result);
    // All upserts should have inserts (non-atomic).
    ASSERT_EQ(((kNumThreads * (kNumThreads - 1)) / 2) * (kNumRmws / kRange), context.output);
  }

  store.StopSession();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}