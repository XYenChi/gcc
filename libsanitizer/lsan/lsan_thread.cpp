//=-- lsan_thread.cpp -----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of LeakSanitizer.
// See lsan_thread.h for details.
//
//===----------------------------------------------------------------------===//

#include "lsan_thread.h"

#include "lsan.h"
#include "lsan_allocator.h"
#include "lsan_common.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_placement_new.h"
#include "sanitizer_common/sanitizer_thread_registry.h"
#include "sanitizer_common/sanitizer_tls_get_addr.h"

namespace __lsan {

static ThreadRegistry *thread_registry;
static ThreadArgRetval *thread_arg_retval;

static ThreadContextBase *CreateThreadContext(u32 tid) {
  void *mem = MmapOrDie(sizeof(ThreadContext), "ThreadContext");
  return new (mem) ThreadContext(tid);
}

void InitializeThreads() {
  static ALIGNED(alignof(
      ThreadRegistry)) char thread_registry_placeholder[sizeof(ThreadRegistry)];
  thread_registry =
      new (thread_registry_placeholder) ThreadRegistry(CreateThreadContext);

  static ALIGNED(alignof(ThreadArgRetval)) char
      thread_arg_retval_placeholder[sizeof(ThreadArgRetval)];
  thread_arg_retval = new (thread_arg_retval_placeholder) ThreadArgRetval();
}

ThreadArgRetval &GetThreadArgRetval() { return *thread_arg_retval; }

ThreadContextLsanBase::ThreadContextLsanBase(int tid)
    : ThreadContextBase(tid) {}

void ThreadContextLsanBase::OnStarted(void *arg) {
  SetCurrentThread(this);
  AllocatorThreadStart();
}

void ThreadContextLsanBase::OnFinished() {
  AllocatorThreadFinish();
  DTLS_Destroy();
}

u32 ThreadCreate(u32 parent_tid, uptr user_id, bool detached, void *arg) {
  return thread_registry->CreateThread(user_id, detached, parent_tid, arg);
}

void ThreadContextLsanBase::ThreadStart(u32 tid, tid_t os_id,
                                        ThreadType thread_type, void *arg) {
  thread_registry->StartThread(tid, os_id, thread_type, arg);
  SetCurrentThread(tid);
}

void ThreadFinish() {
  thread_registry->FinishThread(GetCurrentThread());
  SetCurrentThread(kInvalidTid);
}

ThreadContext *CurrentThreadContext() {
  if (!thread_registry)
    return nullptr;
  if (GetCurrentThread() == kInvalidTid)
    return nullptr;
  // No lock needed when getting current thread.
  return (ThreadContext *)thread_registry->GetThreadLocked(GetCurrentThread());
}

static bool FindThreadByUid(ThreadContextBase *tctx, void *arg) {
  uptr uid = (uptr)arg;
  if (tctx->user_id == uid && tctx->status != ThreadStatusInvalid) {
    return true;
  }
  return false;
}

u32 ThreadTid(uptr uid) {
  return thread_registry->FindThread(FindThreadByUid, (void *)uid);
}

void ThreadDetach(u32 tid) {
  CHECK_NE(tid, kInvalidTid);
  thread_registry->DetachThread(tid, /* arg */ nullptr);
}

void ThreadJoin(u32 tid) {
  CHECK_NE(tid, kInvalidTid);
  thread_registry->JoinThread(tid, /* arg */ nullptr);
}

void EnsureMainThreadIDIsCorrect() {
  if (GetCurrentThread() == kMainTid)
    CurrentThreadContext()->os_id = GetTid();
}

///// Interface to the common LSan module. /////

void ForEachExtraStackRange(tid_t os_id, RangeIteratorCallback callback,
                            void *arg) {}

void LockThreads() {
  thread_registry->Lock();
  thread_arg_retval->Lock();
}

void UnlockThreads() {
  thread_arg_retval->Unlock();
  thread_registry->Unlock();
}

ThreadRegistry *GetThreadRegistryLocked() {
  thread_registry->CheckLocked();
  return thread_registry;
}

void GetRunningThreadsLocked(InternalMmapVector<tid_t> *threads) {
  GetLsanThreadRegistryLocked()->RunCallbackForEachThreadLocked(
      [](ThreadContextBase *tctx, void *threads) {
        if (tctx->status == ThreadStatusRunning) {
          reinterpret_cast<InternalMmapVector<tid_t> *>(threads)->push_back(
              tctx->os_id);
        }
      },
      threads);
}

void GetAdditionalThreadContextPtrsLocked(InternalMmapVector<uptr> *ptrs) {
  GetThreadArgRetval().GetAllPtrsLocked(ptrs);
}

}  // namespace __lsan
