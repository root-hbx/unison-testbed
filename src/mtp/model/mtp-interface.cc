/*
 * Copyright (c) 2023 State Key Laboratory for Novel Software Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Songyuan Bai <i@f5soft.site>
 */

/**
 * \file
 * \ingroup mtp
 *  Implementation of classes ns3::MtpInterface
 */

#include "mtp-interface.h"

#include "ns3/assert.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MtpInterface");

void
MtpInterface::Enable()
{
#ifdef NS3_MPI
    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::HybridSimulatorImpl")); // mpi
#else
    GlobalValue::Bind("SimulatorImplementationType",
                      StringValue("ns3::MultithreadedSimulatorImpl")); // mtp
#endif
    g_enabled = true; // signal: enable mtp
}

void
MtpInterface::Enable(const uint32_t threadCount)
{
#ifdef NS3_MPI
    Config::SetDefault("ns3::HybridSimulatorImpl::MaxThreads", UintegerValue(threadCount));
#else
    Config::SetDefault("ns3::MultithreadedSimulatorImpl::MaxThreads", UintegerValue(threadCount));
#endif
    MtpInterface::Enable();
}

void
MtpInterface::Enable(const uint32_t threadCount, const uint32_t systemCount)
{
    NS_ASSERT_MSG(threadCount > 0, "There must be at least one thread");

    // called by manual partition
    if (!g_enabled)
    {
        GlobalValue::Bind("SimulatorImplementationType",
                          StringValue("ns3::MultithreadedSimulatorImpl"));
    }

    // set size
    g_threadCount = threadCount;
    g_systemCount = systemCount;

    // allocate systems
    g_systems = new LogicalProcess[g_systemCount + 1]; // include the public LP
    for (uint32_t i = 0; i <= g_systemCount; i++)
    {
        g_systems[i].Enable(i, g_systemCount + 1);
    }

    StringValue s;
    g_sortMethod.GetValue(s);
    if (s.Get() == "ByExecutionTime")
    {
        g_sortFunc = SortByExecutionTime;
    }
    else if (s.Get() == "ByPendingEventCount")
    {
        g_sortFunc = SortByPendingEventCount;
    }
    else if (s.Get() == "ByEventCount")
    {
        g_sortFunc = SortByEventCount;
    }
    else if (s.Get() == "BySimulationTime")
    {
        g_sortFunc = SortBySimulationTime;
    }

    UintegerValue ui;
    g_sortPeriod.GetValue(ui);
    if (ui.Get() == 0)
    {
        g_period = std::ceil(std::log2(g_systemCount) / 4 + 1);
        NS_LOG_INFO("Secheduling period is automatically set to " << g_period);
    }
    else
    {
        g_period = ui.Get();
    }

    // create a thread local storage key
    // so that we can access the currently assigned LP of each thread
    pthread_key_create(&g_key, nullptr);
    pthread_setspecific(g_key, &g_systems[0]);
}

void
MtpInterface::EnableNew(const uint32_t newSystemCount)
{
    const LogicalProcess* oldSystems = g_systems;
    g_systems = new LogicalProcess[g_systemCount + newSystemCount + 1];
    for (uint32_t i = 0; i <= g_systemCount; i++)
    {
        g_systems[i] = oldSystems[i];
    }
    delete[] oldSystems;

    g_systemCount += newSystemCount;
    for (uint32_t i = 0; i <= g_systemCount; i++)
    {
        g_systems[i].Enable(i, g_systemCount + 1);
    }

    UintegerValue ui;
    g_sortPeriod.GetValue(ui);
    if (ui.Get() == 0)
    {
        g_period = std::ceil(std::log2(g_systemCount) / 4 + 1);
        NS_LOG_INFO("Secheduling period is automatically set to " << g_period);
    }
    else
    {
        g_period = ui.Get();
    }

    // create a thread local storage key
    // so that we can access the currently assigned LP of each thread
    pthread_key_create(&g_key, nullptr);
    pthread_setspecific(g_key, &g_systems[0]);
}

void
MtpInterface::EnableNew(const uint32_t threadCount, const uint32_t newSystemCount)
{
    g_threadCount = threadCount;
    EnableNew(newSystemCount);
}

void
MtpInterface::Disable()
{
    g_threadCount = 0;
    g_systemCount = 0;
    g_sortFunc = nullptr;
    g_globalFinished = false;
    delete[] g_systems;
    delete[] g_threads;
    delete[] g_sortedSystemIndices;
}

void
MtpInterface::Run()
{
    RunBefore(); // calculate lookahead and create threads
    while (!g_globalFinished)
    {
        ProcessOneRound(); // everything in a round
        CalculateSmallestTime(); // get the smallest time
    }
    RunAfter();
}

void
MtpInterface::RunBefore()
{
    CalculateLookAhead(); // get min_duration of schedule

    // LP index for sorting & holding worker threads
    g_sortedSystemIndices = new uint32_t[g_systemCount];
    for (uint32_t i = 0; i < g_systemCount; i++)
    {
        g_sortedSystemIndices[i] = i + 1;
    }
    g_systemIndex.store(g_systemCount, std::memory_order_release);

    // start threads
    g_threads = new pthread_t[g_threadCount - 1]; // exclude the main thread
    for (uint32_t i = 0; i < g_threadCount - 1; i++)
    {
        pthread_create(&g_threads[i], nullptr, ThreadFunc, nullptr);
    }
}

// TODO(bxhu): refer to paper - spatial partition
void
MtpInterface::ProcessOneRound()
{
    // assign logical process to threads

    // determine the priority of logical processes
    if (g_sortFunc != nullptr && g_round++ % g_period == 0)
    {
        std::sort(g_sortedSystemIndices, g_sortedSystemIndices + g_systemCount, g_sortFunc);
    }

    // == LPs are sorted well by priority. Now we can process them in order. ==

    // stage 1: process events
    g_recvMsgStage = false;
    g_finishedSystemCount.store(0, std::memory_order_relaxed);
    g_systemIndex.store(0, std::memory_order_release);
    // main thread also needs to process an LP to reduce an extra thread overhead
    while (true)
    {
        // generate increasing index
        uint32_t index = g_systemIndex.fetch_add(1, std::memory_order_acquire);
        
        // if index is out of range, break the loop
        if (index >= g_systemCount)
        {
            break;
        }
        LogicalProcess* system = &g_systems[g_sortedSystemIndices[index]]; // current LP
        system->ProcessOneRound(); // process current LP
        
        // increase the finished system count
        g_finishedSystemCount.fetch_add(1, std::memory_order_release);
    }

    // logical process barriar synchronization
    while (g_finishedSystemCount.load(std::memory_order_acquire) != g_systemCount)
    {
    };

    // stage 2: process the public LP
    g_systems[0].ProcessOneRound();

    // stage 3: receive messages
    // principle here is the same as stage 1.
    g_recvMsgStage = true;
    g_finishedSystemCount.store(0, std::memory_order_relaxed);
    g_systemIndex.store(0, std::memory_order_release);
    while (true)
    {
        uint32_t index = g_systemIndex.fetch_add(1, std::memory_order_acquire);
        if (index >= g_systemCount)
        {
            break;
        }
        LogicalProcess* system = &g_systems[g_sortedSystemIndices[index]];
        system->ReceiveMessages();
        g_finishedSystemCount.fetch_add(1, std::memory_order_release);
    }

    // logical process barriar synchronization
    while (g_finishedSystemCount.load(std::memory_order_acquire) != g_systemCount)
    {
    };
}

void
MtpInterface::CalculateSmallestTime()
{
    // update globally smallest time
    g_smallestTime = Time::Max() / 2;
    for (uint32_t i = 0; i <= g_systemCount; i++)
    {
        Time nextTime = g_systems[i].Next();
        if (nextTime < g_smallestTime)
        {
            g_smallestTime = nextTime;
        }
    }
    g_nextPublicTime = g_systems[0].Next();

    // TODO(bxhu): refer to paper - LBTS equation
    // 1. g_smallestTime: $min{N_i} + t_{look_ahead_time}$
    // 2. g_nextPublicTime: $N_{pub}$
    // LINK ./logical-process.cc#LBTS

    // test if global finished
    bool globalFinished = true;
    for (uint32_t i = 0; i <= g_systemCount; i++)
    {
        globalFinished &= g_systems[i].isLocalFinished();
    }
    g_globalFinished = globalFinished;
}

void
MtpInterface::RunAfter()
{
    // global finished, terminate threads
    g_systemIndex.store(0, std::memory_order_release);
    for (uint32_t i = 0; i < g_threadCount - 1; i++)
    {
        pthread_join(g_threads[i], nullptr);
    }
}

bool
MtpInterface::isEnabled()
{
    return g_enabled;
}

bool
MtpInterface::isPartitioned()
{
    return g_threadCount != 0;
}

void
MtpInterface::CalculateLookAhead()
{
    for (uint32_t i = 1; i <= g_systemCount; i++)
    {
        g_systems[i].CalculateLookAhead();
    }
}

void*
MtpInterface::ThreadFunc(void* arg)
{
    while (!g_globalFinished)
    {
        uint32_t index = g_systemIndex.fetch_add(1, std::memory_order_acquire);
        if (index >= g_systemCount)
        {
            while (g_systemIndex.load(std::memory_order_acquire) >= g_systemCount)
            {
            };
            continue;
        }
        LogicalProcess* system = &g_systems[g_sortedSystemIndices[index]];
        if (g_recvMsgStage)
        {
            system->ReceiveMessages();
        }
        else
        {
            system->ProcessOneRound();
        }
        g_finishedSystemCount.fetch_add(1, std::memory_order_release);
    }
    return nullptr;
}

bool
MtpInterface::SortByExecutionTime(const uint32_t& i, const uint32_t& j)
{
    return g_systems[i].GetExecutionTime() > g_systems[j].GetExecutionTime();
}

bool
MtpInterface::SortByEventCount(const uint32_t& i, const uint32_t& j)
{
    return g_systems[i].GetEventCount() > g_systems[j].GetEventCount();
}

bool
MtpInterface::SortByPendingEventCount(const uint32_t& i, const uint32_t& j)
{
    return g_systems[i].GetPendingEventCount() > g_systems[j].GetPendingEventCount();
}

bool
MtpInterface::SortBySimulationTime(const uint32_t& i, const uint32_t& j)
{
    return g_systems[i].Now() > g_systems[j].Now();
}

bool (*MtpInterface::g_sortFunc)(const uint32_t&, const uint32_t&) = nullptr;

GlobalValue MtpInterface::g_sortMethod =
    GlobalValue("PartitionSchedulingMethod",
                "The scheduling method to determine which partition runs first",
                StringValue("ByExecutionTime"),
                MakeStringChecker());

GlobalValue MtpInterface::g_sortPeriod = GlobalValue("PartitionSchedulingPeriod",
                                                     "The scheduling period of partitions",
                                                     UintegerValue(0),
                                                     MakeUintegerChecker<uint32_t>(0));

uint32_t MtpInterface::g_period = 0;

pthread_t* MtpInterface::g_threads = nullptr;

LogicalProcess* MtpInterface::g_systems = nullptr;

uint32_t MtpInterface::g_threadCount = 0;

uint32_t MtpInterface::g_systemCount = 0;

uint32_t* MtpInterface::g_sortedSystemIndices = nullptr;

std::atomic<uint32_t> MtpInterface::g_systemIndex;

std::atomic<uint32_t> MtpInterface::g_finishedSystemCount;

uint32_t MtpInterface::g_round = 0;

Time MtpInterface::g_smallestTime = TimeStep(0);

Time MtpInterface::g_nextPublicTime = TimeStep(0);

bool MtpInterface::g_recvMsgStage = false;

bool MtpInterface::g_globalFinished = false;

bool MtpInterface::g_enabled = false;

pthread_key_t MtpInterface::g_key;

std::atomic<bool> MtpInterface::g_inCriticalSection(false);

} // namespace ns3
