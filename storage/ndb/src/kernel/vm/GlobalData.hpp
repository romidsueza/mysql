/* Copyright (C) 2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include <ndb_global.h>
#include <kernel_types.h>
#include "Prio.hpp"
#include "VMSignal.hpp"

#include <BlockNumbers.h>
#include <NodeState.hpp>
#include <NodeInfo.hpp>
#include "ArrayPool.hpp"

// #define GCP_TIMER_HACK
#ifdef GCP_TIMER_HACK
#include <NdbTick.h>
#endif

class SimulatedBlock;

enum  restartStates {initial_state, 
                     perform_start, 
                     system_started, 
                     perform_stop};

struct GlobalData {
  Uint32     m_restart_seq;           // 
  NodeVersionInfo m_versionInfo;
  NodeInfo   m_nodeInfo[MAX_NODES];
  Signal     VMSignals[1];            // Owned by FastScheduler::
  
  Uint64     internalMillisecCounter; // Owned by ThreadConfig::
  Uint32     highestAvailablePrio;    // Owned by FastScheduler::
  Uint32     JobCounter;              // Owned by FastScheduler
  Uint64     JobLap;                  // Owned by FastScheduler
  Uint32     loopMax;                 // Owned by FastScheduler
  
  Uint32     theNextTimerJob;         // Owned by TimeQueue::
  Uint32     theCurrentTimer;         // Owned by TimeQueue::
  Uint32     theShortTQIndex;         // Owned by TimeQueue::
  
  Uint32     theLongTQIndex;          // Owned by TimeQueue::
  Uint32     theCountTimer;           // Owned by TimeQueue::
  Uint32     theFirstFreeTQIndex;     // Owned by TimeQueue::
  Uint32     testOn;                  // Owned by the Signal Loggers
  
  NodeId     ownId;                   // Own processor id
  
  Uint32     theStartLevel;
  restartStates theRestartFlag;
  Uint32     theSignalId;
  
  Uint32     sendPackedActivated;
  Uint32     activateSendPacked;

  bool       isNdbMt;    // ndbd multithreaded, no workers
  bool       isNdbMtLqh; // ndbd multithreaded, LQH workers
  Uint32     ndbMtLqhWorkers;
  Uint32     ndbMtLqhThreads;
  
  GlobalData(){ 
    theSignalId = 0; 
    theStartLevel = NodeState::SL_NOTHING;
    theRestartFlag = perform_start;
    isNdbMt = false;
    isNdbMtLqh = false;
    ndbMtLqhWorkers = 0;
    ndbMtLqhThreads = 0;
#ifdef GCP_TIMER_HACK
    gcp_timer_limit = 0;
#endif
  }
  ~GlobalData() { m_global_page_pool.clear(); m_shared_page_pool.clear();}
  
  void             setBlock(BlockNumber blockNo, SimulatedBlock * block);
  SimulatedBlock * getBlock(BlockNumber blockNo);
  SimulatedBlock * getBlock(BlockNumber blockNo, Uint32 instanceNo);
  SimulatedBlock * getBlockInstance(BlockNumber fullBlockNo) {
    return getBlock(blockToMain(fullBlockNo), blockToInstance(fullBlockNo));
  }
  
  void           incrementWatchDogCounter(Uint32 place);
  Uint32 * getWatchDogPtr();
  
private:
  Uint32     watchDog;
  SimulatedBlock* blockTable[NO_OF_BLOCKS]; // Owned by Dispatcher::
public:
  SafeArrayPool<GlobalPage> m_global_page_pool;
  ArrayPool<GlobalPage> m_shared_page_pool;

#ifdef GCP_TIMER_HACK
  // timings are local to the node

  // from prepare to commit (DIH, TC)
  MicroSecondTimer gcp_timer_commit[2];
  // from GCP_SAVEREQ to GCP_SAVECONF (LQH)
  MicroSecondTimer gcp_timer_save[2];
  // sysfile update (DIH)
  MicroSecondTimer gcp_timer_copygci[2];

  // report threshold in ms, if 0 guessed, set with dump 7901 <limit>
  Uint32 gcp_timer_limit;
#endif
};

extern GlobalData globalData;

#define GLOBAL_TEST_ON (localTestOn)
#define GET_GLOBAL_TEST_FLAG bool localTestOn = globalData.testOn
#define SET_GLOBAL_TEST_ON (globalData.testOn = true)
#define SET_GLOBAL_TEST_OFF (globalData.testOn = false)
#define TOGGLE_GLOBAL_TEST_FLAG (globalData.testOn = (globalData.testOn == true ? false : true))

inline
void
GlobalData::setBlock(BlockNumber blockNo, SimulatedBlock * block){
  blockNo -= MIN_BLOCK_NO;
  assert((blockTable[blockNo] == 0) || (blockTable[blockNo] == block));
  blockTable[blockNo] = block;
}

inline
SimulatedBlock *
GlobalData::getBlock(BlockNumber blockNo){
  blockNo -= MIN_BLOCK_NO;
  return blockTable[blockNo];
}

inline
void
GlobalData::incrementWatchDogCounter(Uint32 place){
  watchDog = place;
}

inline
Uint32 *
GlobalData::getWatchDogPtr(){
  return &watchDog;
}

#endif
