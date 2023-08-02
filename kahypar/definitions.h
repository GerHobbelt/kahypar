/*******************************************************************************
 * MIT License
 *
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#pragma once

#include <chrono>
#include <cstdint>
#include <utility>

#include "datastructure/hypergraph.h"

// Use bucket PQ for FM refinement.
// #define USE_BUCKET_QUEUE

namespace kahypar {
using HypernodeID = uint32_t;
using HyperedgeID = uint32_t;
using HypernodeWeight = int32_t;
using HyperedgeWeight = int32_t;
using PartitionID = int32_t;
using Gain = HyperedgeWeight;

using Hypergraph = kahypar::ds::GenericHypergraph<HypernodeID,
                                                  HyperedgeID, HypernodeWeight,
                                                  HyperedgeWeight, PartitionID>;

using RatingType = double;
using HypergraphType = Hypergraph::Type;
using HyperedgeIndexVector = Hypergraph::HyperedgeIndexVector;
using HyperedgeVector = Hypergraph::HyperedgeVector;
using HyperedgeWeightVector = Hypergraph::HyperedgeWeightVector;
using HypernodeWeightVector = Hypergraph::HypernodeWeightVector;
using IncidenceIterator = Hypergraph::IncidenceIterator;

// #########Graph-Definitions#############
using NodeID = HypernodeID;
using EdgeID = HyperedgeID;
using EdgeWeight = long double;
using ClusterID = PartitionID;

using HighResClockTimepoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
}  // namespace kahypar

// this is nasty and needs to be fixed
namespace std {
static kahypar::IncidenceIterator begin(const std::pair<kahypar::IncidenceIterator,
                                                        kahypar::IncidenceIterator>& x) {
  return x.first;
}

static kahypar::IncidenceIterator end(const std::pair<kahypar::IncidenceIterator,
                                                      kahypar::IncidenceIterator>& x) {
  return x.second;
}

template <typename Iterator>
Iterator begin(std::pair<Iterator, Iterator>& x) {
  return x.first;
}

template <typename Iterator>
Iterator end(std::pair<Iterator, Iterator>& x) {
  return x.second;
}
}  // namespace std
