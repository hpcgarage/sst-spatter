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


#ifndef SST_SPATTER_SPATTERGENERATOR_H
#define SST_SPATTER_SPATTERGENERATOR_H

#include <cstdint>
#include <queue>
#include <string>

#include <sst/core/output.h>

#include "mirandaGenerator.h"
#include <Spatter/Input.hh>

namespace SST {
namespace SST_Spatter {

class SpatterGenerator : public RequestGenerator {

public:
    SpatterGenerator( ComponentId_t id, Params& params );
    void build(Params& params);
    ~SpatterGenerator();
    void generate(MirandaRequestQueue<GeneratorRequest*>* q);
    bool isFinished();
    void completed();

    SST_ELI_REGISTER_SUBCOMPONENT(
        SpatterGenerator,
        "sstSpatter",
        "SpatterGenerator",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Creates a stream of gather/scatter operations based on a Spatter pattern",
        SST::SST_Spatter::RequestGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "verbose",            "Sets the verbosity of the output", "0" },
        { "args",               "Sets the arguments to describe Spatter pattern(s)", "" },
        { "datawidth",          "Sets the width of the memory operation", "8" },
        { "start_source",       "Sets the start address of the source array", "0" },
        { "start_target",       "Sets the start address of the target array", "0" },
        { "warmup_runs",        "Sets the the number of warm-up runs", "1" }
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "configTime",         "Time spent completing all requests for a Spatter config", "ps", 1 }
    )

private:
    void setStatFlags(Statistic<uint64_t>* stat);
    void initStatistics();
    void resetStatData(Statistic<uint64_t>* stat);
    void resetStatistics();

    int32_t countArgs(const std::string &args);
    void tokenizeArgs(const std::string &args, const int32_t &argc, char ***argv);
    bool initConfigs(const std::string& args);

    size_t getPatternSize(const Spatter::ConfigurationBase *config);
    void updateIndices();

    void gather();
    void scatter();
    void gatherScatter();
    void multiGather();
    void multiScatter();

    uint64_t numIssuedReqs;
    uint64_t sourceAddr;
    uint64_t targetAddr;
    uint32_t datawidth;
    uint32_t startSource;
    uint32_t startTarget;
    uint32_t maxWarmupRuns;
    uint32_t remainingWarmupRuns;

    size_t patternIdx;
    size_t countIdx;
    size_t configIdx;

    bool configFin;

    SimTime_t configStartTime;

    Statistic<uint64_t>* statReqs[OPCOUNT];
    Statistic<uint64_t>* statSplitReqs[OPCOUNT];
    Statistic<uint64_t>* statCompletedReqs;
    Statistic<uint64_t>* statCyclesWithIssue;
    Statistic<uint64_t>* statMaxIssuePerCycle;
    Statistic<uint64_t>* statCyclesWithoutIssue;
    Statistic<uint64_t>* statBytes[OPCOUNT];
    Statistic<uint64_t>* statReqLatency;
    Statistic<uint64_t>* statTime;
    Statistic<uint64_t>* statCyclesHitFence;
    Statistic<uint64_t>* statCyclesHitReorderLimit;
    Statistic<uint64_t>* statCycles;
    Statistic<uint64_t>* statConfigTime;

    MirandaRequestQueue<GeneratorRequest*>* queue;

    Output* out;

    Spatter::ClArgs cl;
    Spatter::ConfigurationBase *config;
};

} // namespace SST_Spatter
} // namespace SST

#endif // SST_SPATTER_SPATTERGENERATOR_H
