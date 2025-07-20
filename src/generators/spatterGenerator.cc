// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "spatterGenerator.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

#include <sst/core/params.h>

#include <Spatter/Input.hh>

using namespace SST::SST_Spatter;

SpatterGenerator::SpatterGenerator(ComponentId_t id, Params& params) : RequestGenerator(id, params)
{
    build(params);
}

void SpatterGenerator::build(Params& params)
{
    const uint32_t verbose = params.find<uint32_t>("verbose", 0);

    out = new Output("SpatterGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

    numIssuedReqs = 0;
    sourceAddr = 0;
    targetAddr = 0;

    datawidth = params.find<uint32_t>("datawidth", 8);

    patternIdx = 0;
    countIdx = 0;
    configIdx = 0;
    configFin = false;

    initStatistics();

    const std::string args = "./Spatter " + params.find<std::string>("args", "");
    if (!initConfigs(args)) {
        out->fatal(CALL_INFO, -1, "Error: failed to parse provided arguments.\n");
    }

    startSource = params.find<uint32_t>("start_source", 0);
    startTarget = params.find<uint32_t>("start_target", std::max(cl.sparse_size, cl.sparse_gather_size));

    if (startTarget > startSource) {
        if (startTarget <= (startSource + std::max(cl.sparse_size, cl.sparse_gather_size) - 1)) {
            out->verbose(CALL_INFO, 0, 0, "Warning: source and target arrays will overlap.\n");
        }
    } else if (startSource < startTarget){
        if (startSource <= (startTarget + std::max(cl.dense_size, cl.sparse_scatter_size) - 1)) {
            out->verbose(CALL_INFO, 0, 0, "Warning: source and target arrays will overlap.\n");
        }
    } else {
        out->verbose(CALL_INFO, 0, 0, "Warning: source and target arrays will overlap.\n");
    }

    config = cl.configs[configIdx].get();
}

SpatterGenerator::~SpatterGenerator()
{
    delete out;
}

void SpatterGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q)
{
    if (configFin) return;

    config = cl.configs[configIdx].get();
    queue = q;

    const std::string& kernel = config->kernel;

    if ("gather" == kernel) {
        gather();
    } else if ("scatter" == kernel) {
        scatter();
    } else if ("gs" == kernel) {
        gatherScatter();
    } else if ("multigather" == kernel) {
        multiGather();
    } else if ("multiscatter" == kernel) {
        multiScatter();
    }

    updateIndices();
}

bool SpatterGenerator::isFinished()
{
    if (configFin) {
        if (numIssuedReqs == statCompletedReqs->getCollectionCount()) {
            // The requests associated with the previous run have completed.
            performGlobalStatisticOutput();
            numIssuedReqs = 0;
            configFin = false;

            return (configIdx == cl.configs.size());
        }
    }

    return false;
}

void SpatterGenerator::completed()
{
}

/**
   * @brief Set the Clear Data On Output and Output At End Of Sim flags.
   *
   * @param stat Statistic whose flags will be set.
   */
void SpatterGenerator::setStatFlags(Statistic<uint64_t>* stat)
{
    stat->setFlagClearDataOnOutput(true);
    stat->setFlagOutputAtEndOfSim(false);
}

/**
   * @brief Initialize the Miranda CPU statistics.
   *
   */
void SpatterGenerator::initStatistics()
{
    statReqs[READ]            = registerStatistic<uint64_t>("read_reqs");
    statReqs[WRITE]           = registerStatistic<uint64_t>("write_reqs");
    statReqs[CUSTOM]          = registerStatistic<uint64_t>("custom_reqs");
    statSplitReqs[READ]       = registerStatistic<uint64_t>("split_read_reqs");
    statSplitReqs[WRITE]      = registerStatistic<uint64_t>("split_write_reqs");
    statSplitReqs[CUSTOM]     = registerStatistic<uint64_t>("split_custom_reqs");
    statCompletedReqs         = registerStatistic<uint64_t>("completed_reqs");
    statCyclesWithIssue       = registerStatistic<uint64_t>("cycles_with_issue");
    statCyclesWithoutIssue    = registerStatistic<uint64_t>("cycles_no_issue");
    statBytes[READ]           = registerStatistic<uint64_t>("total_bytes_read");
    statBytes[WRITE]          = registerStatistic<uint64_t>("total_bytes_write");
    statBytes[CUSTOM]         = registerStatistic<uint64_t>("total_bytes_custom");
    statReqLatency            = registerStatistic<uint64_t>("req_latency");
    statTime                  = registerStatistic<uint64_t>("time");
    statCyclesHitFence        = registerStatistic<uint64_t>("cycles_hit_fence");
    statMaxIssuePerCycle      = registerStatistic<uint64_t>("cycles_max_issue");
    statCyclesHitReorderLimit = registerStatistic<uint64_t>("cycles_max_reorder");
    statCycles                = registerStatistic<uint64_t>("cycles");

    // Set the Clear Data On Output and Output At End Of Sim flags.
    setStatFlags(statReqs[READ]);
    setStatFlags(statReqs[WRITE]);
    setStatFlags(statReqs[CUSTOM]);
    setStatFlags(statSplitReqs[READ]);
    setStatFlags(statSplitReqs[WRITE]);
    setStatFlags(statSplitReqs[CUSTOM]);
    setStatFlags(statCompletedReqs);
    setStatFlags(statCyclesWithIssue);
    setStatFlags(statCyclesWithoutIssue);
    setStatFlags(statBytes[READ]);
    setStatFlags(statBytes[WRITE]);
    setStatFlags(statBytes[CUSTOM]);
    setStatFlags(statReqLatency);
    setStatFlags(statTime);
    setStatFlags(statCyclesHitFence);
    setStatFlags(statMaxIssuePerCycle);
    setStatFlags(statCyclesHitReorderLimit);
    setStatFlags(statCycles);
}

/**
   * @brief Reset the data and collection count for a given statistic.
   *
   * @param stat Statistic whose data and collection count will be reset.
   */
void SpatterGenerator::resetStatData(Statistic<uint64_t>* stat)
{
    stat->clearStatisticData();
    stat->resetCollectionCount();
}

/**
   * @brief Reset the Miranda CPU statistics.
   *
   */
void SpatterGenerator::resetStatistics()
{
    resetStatData(statReqs[READ]);
    resetStatData(statReqs[WRITE]);
    resetStatData(statReqs[CUSTOM]);
    resetStatData(statSplitReqs[READ]);
    resetStatData(statSplitReqs[WRITE]);
    resetStatData(statSplitReqs[CUSTOM]);
    resetStatData(statCompletedReqs);
    resetStatData(statCyclesWithIssue);
    resetStatData(statCyclesWithoutIssue);
    resetStatData(statBytes[READ]);
    resetStatData(statBytes[WRITE]);
    resetStatData(statBytes[CUSTOM]);
    resetStatData(statReqLatency);
    resetStatData(statTime);
    resetStatData(statCyclesHitFence);
    resetStatData(statMaxIssuePerCycle);
    resetStatData(statCyclesHitReorderLimit);
    resetStatData(statCycles);
}

/**
   * @brief Counts the number of arguments in a string.
   *
   * @param args The string of arguments to be counted.
   * @return Number of arguments found in the string.
   */
int32_t SpatterGenerator::countArgs(const std::string &args)
{
    int32_t count = 0;
    std::istringstream iss(args);
    std::string token;

    while (iss >> token) {
        ++count;
    }

    return count;
}

/**
   * @brief Tokenize a string of arguments into an array of arguments
            and allocates memory for the array of arguments.
   *
   * @param args String of arguments to be tokenized.
   * @param argc Number of arguments in the string.
   * @param argv Destination array for the arguments.
   */
void SpatterGenerator::tokenizeArgs(const std::string &args, const int32_t &argc, char ***argv)
{
    std::istringstream iss(args);
    std::string token;

    char **argvPtr = new char *[argc + 1];
    int argvIdx = 0;

    while (iss >> token) {
        int arg_size = token.size() + 1;

        argvPtr[argvIdx] = new char[arg_size];
        strncpy(argvPtr[argvIdx], token.c_str(), arg_size);

        ++argvIdx;
    }

    argvPtr[argvIdx] = nullptr;
    *argv = argvPtr;
}

/**
   * @brief Initialize the Spatter config list.
   *
   * @param args String of arguments to be parsed.
   * @return true if the configs are successfully initialized, false otherwise.
   */
bool SpatterGenerator::initConfigs(const std::string& args)
{
    char **argv = nullptr;
    int argc = countArgs(args);

    // Convert the arguments to a compatible format before parsing them.
    tokenizeArgs(args, argc, &argv);

    int result = Spatter::parse_input(argc, argv, cl);

    // The allocated memory is no longer needed.
    for (int i = 0; i < argc; ++i) {
        delete [] argv[i];
    }
    delete [] argv;

    return (0 == result);
}

/**
   * @brief Return the number of elements in the pattern.
   *
   * @param config Run-configuration used to determine the kernel type.
   * @return Number of elements in the pattern.
   */
size_t SpatterGenerator::getPatternSize(const Spatter::ConfigurationBase *config)
{
    const std::string& kernel = config->kernel;
    size_t patternSize = 0;

    if ("gather" == kernel || "scatter" == kernel) {
        patternSize = config->pattern.size();
    } else if ("gs" == kernel) {
        patternSize = config->pattern_scatter.size();
    } else if ("multigather" == kernel) {
        patternSize = config->pattern_gather.size();
    } else if ("multiscatter" == kernel) {
        patternSize = config->pattern_scatter.size();
    }

    return patternSize;
}

/**
   * @brief Update the pattern, count, and config indices.
   *
   */
void SpatterGenerator::updateIndices()
{
    size_t patternSize = getPatternSize(config);

    if (patternIdx == (patternSize - 1)) {
        patternIdx = 0;
        if (countIdx == (config->count - 1)) {
            countIdx = 0;

            // Finished issuing requests for the current run-configuration.
            configFin = true;
            ++configIdx;
        } else {
            ++countIdx;
        }
    } else {
        ++patternIdx;
    }
}

/**
   * @brief Generate a memory request for a Gather pattern.
   *
   */
void SpatterGenerator::gather()
{
    sourceAddr = startSource + config->pattern[patternIdx] + config->delta * countIdx;
    targetAddr = startTarget + config->pattern.size() * (countIdx % config->wrap);

    MemoryOpRequest* readReq  = new MemoryOpRequest(sourceAddr, datawidth, READ);
    MemoryOpRequest* writeReq = new MemoryOpRequest(targetAddr, datawidth, WRITE);

    writeReq->addDependency(readReq->getRequestID());

    out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", sourceAddr);
    queue->push_back(readReq);

    out->verbose(CALL_INFO, 8, 0, "Issuing WRITE request for address %" PRIu64 "\n", targetAddr);
    queue->push_back(writeReq);

    numIssuedReqs += 2;
}

/**
   * @brief Generate a memory request for a Scatter pattern.
   *
   */
void SpatterGenerator::scatter()
{
    sourceAddr = startTarget + config->pattern.size() * (countIdx % config->wrap);
    targetAddr = startSource + config->pattern[patternIdx] + config->delta * countIdx;

    MemoryOpRequest* readReq  = new MemoryOpRequest(sourceAddr, datawidth, READ);
    MemoryOpRequest* writeReq = new MemoryOpRequest(targetAddr, datawidth, WRITE);

    writeReq->addDependency(readReq->getRequestID());

    out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", sourceAddr);
    queue->push_back(readReq);

    out->verbose(CALL_INFO, 8, 0, "Issuing WRITE request for address %" PRIu64 "\n", targetAddr);
    queue->push_back(writeReq);

    numIssuedReqs += 2;
}

/**
   * @brief Generate memory requests for a GS pattern.
   *
   */
void SpatterGenerator::gatherScatter()
{
    sourceAddr = startSource + config->pattern_gather[patternIdx] + config->delta_gather * countIdx;
    targetAddr = startTarget + config->pattern_scatter[patternIdx] + config->delta_scatter * countIdx;

    MemoryOpRequest* readReq  = new MemoryOpRequest(sourceAddr, datawidth, READ);
    MemoryOpRequest* writeReq = new MemoryOpRequest(targetAddr, datawidth, WRITE);

    writeReq->addDependency(readReq->getRequestID());

    out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", sourceAddr);
    queue->push_back(readReq);

    out->verbose(CALL_INFO, 8, 0, "Issuing WRITE request for address %" PRIu64 "\n", targetAddr);
    queue->push_back(writeReq);

    numIssuedReqs += 2;
}

/**
   * @brief Generate a memory request for a MultiGather pattern.
   *
   */
void SpatterGenerator::multiGather()
{
    sourceAddr = startSource + config->pattern[config->pattern_gather[patternIdx]] + config->delta_gather * countIdx;
    targetAddr = startTarget + config->pattern_gather.size() * (countIdx % config->wrap);

    MemoryOpRequest* readReq  = new MemoryOpRequest(sourceAddr, datawidth, READ);
    MemoryOpRequest* writeReq = new MemoryOpRequest(targetAddr, datawidth, WRITE);

    writeReq->addDependency(readReq->getRequestID());

    out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", sourceAddr);
    queue->push_back(readReq);

    out->verbose(CALL_INFO, 8, 0, "Issuing WRITE request for address %" PRIu64 "\n", targetAddr);
    queue->push_back(writeReq);

    numIssuedReqs += 2;
}

/**
   * @brief Generate a memory request for a MultiScatter pattern.
   *
   */
void SpatterGenerator::multiScatter()
{
    sourceAddr = startTarget + config->pattern_scatter.size() * (countIdx % config->wrap);
    targetAddr = startSource + config->pattern[config->pattern_scatter[patternIdx]] + config->delta_scatter * countIdx;

    MemoryOpRequest* readReq  = new MemoryOpRequest(sourceAddr, datawidth, READ);
    MemoryOpRequest* writeReq = new MemoryOpRequest(targetAddr, datawidth, WRITE);

    writeReq->addDependency(readReq->getRequestID());

    out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", sourceAddr);
    queue->push_back(readReq);

    out->verbose(CALL_INFO, 8, 0, "Issuing WRITE request for address %" PRIu64 "\n", targetAddr);
    queue->push_back(writeReq);

    numIssuedReqs += 2;
}
