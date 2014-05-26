/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/datastructure/Hypergraph.h"
#include "lib/definitions.h"
#include "partition/Metrics.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMQueueSelectionPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"

using::testing::Test;
using::testing::Eq;

using defs::INVALID_PARTITION;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;
using datastructure::HyperedgeWeight;
using datastructure::HypernodeID;

using partition::EligibleTopGain;
using partition::RemoveOnlyTheCloggingEntry;

namespace partition {
typedef TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch,
                        EligibleTopGain,
                        RemoveOnlyTheCloggingEntry> TwoWayFMRefinerSimpleStopping;

class ATwoWayFMRefiner : public Test {
  public:
  ATwoWayFMRefiner() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    config(),
    refiner(nullptr) {
    hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
    hypergraph.changeNodePartition(1, INVALID_PARTITION, 1);
    hypergraph.changeNodePartition(2, INVALID_PARTITION, 1);
    hypergraph.changeNodePartition(3, INVALID_PARTITION, 0);
    hypergraph.changeNodePartition(4, INVALID_PARTITION, 0);
    hypergraph.changeNodePartition(5, INVALID_PARTITION, 1);
    hypergraph.changeNodePartition(6, INVALID_PARTITION, 1);
    config.two_way_fm.max_number_of_fruitless_moves = 50;
    refiner = new TwoWayFMRefinerSimpleStopping(hypergraph, config);
    refiner->initialize(0);
  }

  HypergraphType hypergraph;
  Configuration config;
  TwoWayFMRefinerSimpleStopping* refiner;

  private:
  DISALLOW_COPY_AND_ASSIGN(ATwoWayFMRefiner);
};

class AGainUpdateMethod : public Test {
  public:
  AGainUpdateMethod() :
    config() {
    config.two_way_fm.max_number_of_fruitless_moves = 50;
  }

  Configuration config;
};

TEST_F(ATwoWayFMRefiner, IdentifiesBorderHypernodes) {
  ASSERT_THAT(refiner->isBorderNode(0), Eq(true));
  ASSERT_THAT(refiner->isBorderNode(1), Eq(true));
  ASSERT_THAT(refiner->isBorderNode(5), Eq(false));
}

TEST_F(ATwoWayFMRefiner, ComputesGainOfHypernodeMovement) {
  ASSERT_THAT(refiner->computeGain(6), Eq(0));
  ASSERT_THAT(refiner->computeGain(1), Eq(1));
  ASSERT_THAT(refiner->computeGain(5), Eq(-1));
}

TEST_F(ATwoWayFMRefiner, ActivatesBorderNodes) {
  refiner->activate(1);

  ASSERT_THAT(refiner->_pq[1]->max(), Eq(1));
  ASSERT_THAT(refiner->_pq[1]->maxKey(), Eq(1));
}

TEST_F(ATwoWayFMRefiner, CalculatesNodeCountsInBothPartitions) {
  ASSERT_THAT(refiner->_partition_size[0], Eq(3));
  ASSERT_THAT(refiner->_partition_size[1], Eq(4));
}

TEST_F(ATwoWayFMRefiner, DoesNotViolateTheBalanceConstraint) {
  double old_imbalance = metrics::imbalance(hypergraph);
  HyperedgeWeight old_cut = metrics::hyperedgeCut(hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

  refiner->refine(refinement_nodes, 2, old_cut, 0.15, old_imbalance);

  EXPECT_PRED_FORMAT2(::testing::DoubleLE, metrics::imbalance(hypergraph),
                      old_imbalance);
}


TEST_F(ATwoWayFMRefiner, UpdatesNodeCountsOnNodeMovements) {
  ASSERT_THAT(refiner->_partition_size[0], Eq(3));
  ASSERT_THAT(refiner->_partition_size[1], Eq(4));

  refiner->moveHypernode(1, 1, 0);

  ASSERT_THAT(refiner->_partition_size[0], Eq(4));
  ASSERT_THAT(refiner->_partition_size[1], Eq(3));
}

TEST_F(ATwoWayFMRefiner, UpdatesPartitionWeightsOnRollBack) {
  ASSERT_THAT(refiner->_partition_size[0], Eq(3));
  ASSERT_THAT(refiner->_partition_size[1], Eq(4));
  double old_imbalance = metrics::imbalance(hypergraph);
  HyperedgeWeight old_cut = metrics::hyperedgeCut(hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

  refiner->refine(refinement_nodes, 2, old_cut, 0.15, old_imbalance);

  ASSERT_THAT(refiner->_partition_size[0], Eq(4));
  ASSERT_THAT(refiner->_partition_size[1], Eq(3));
}

TEST_F(ATwoWayFMRefiner, PerformsCompleteRollBackIfNoImprovementCouldBeFound) {
  hypergraph.changeNodePartition(1, 1, 0);
  delete refiner;
  refiner = new TwoWayFMRefinerSimpleStopping(hypergraph, config);
  refiner->initialize(0);
  ASSERT_THAT(hypergraph.partitionIndex(6), Eq(1));
  ASSERT_THAT(hypergraph.partitionIndex(2), Eq(1));
  double old_imbalance = metrics::imbalance(hypergraph);
  HyperedgeWeight old_cut = metrics::hyperedgeCut(hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

  refiner->refine(refinement_nodes, 2, old_cut, 0.15, old_imbalance);

  ASSERT_THAT(hypergraph.partitionIndex(6), Eq(1));
  ASSERT_THAT(hypergraph.partitionIndex(2), Eq(1));
}

TEST_F(ATwoWayFMRefiner, RollsBackAllNodeMovementsIfCutCouldNotBeImproved) {
  double old_imbalance = metrics::imbalance(hypergraph);
  HyperedgeWeight cut = metrics::hyperedgeCut(hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

  refiner->refine(refinement_nodes, 2, cut, 0.15, old_imbalance);

  ASSERT_THAT(cut, Eq(metrics::hyperedgeCut(hypergraph)));
  ASSERT_THAT(hypergraph.partitionIndex(1), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(5), Eq(1));
}

// Ugly: We could seriously need Mocks here!
TEST_F(AGainUpdateMethod, RespectsPositiveGainUpdateSpecialCaseForHyperedgesOfSize2) {
  HypergraphType hypergraph(2, 1, HyperedgeIndexVector { 0, 2 }, HyperedgeVector { 0, 1 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 0);

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);
  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._pq[0]->insert(0, refiner.computeGain(0));
  refiner._pq[0]->insert(1, refiner.computeGain(1));
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(-1));

  hypergraph.changeNodePartition(1, 0, 1);
  refiner._marked[1] = true;

  refiner.updateNeighbours(1, 0, 1);

  ASSERT_THAT(refiner._pq[0]->key(0), Eq(1));
  // updateNeighbours does not delete the current max_gain node, neither does it update its gain
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(-1));
}

TEST_F(AGainUpdateMethod, RespectsNegativeGainUpdateSpecialCaseForHyperedgesOfSize2) {
  HypergraphType hypergraph(3, 2, HyperedgeIndexVector { 0, 2, 4 }, HyperedgeVector { 0, 1, 0, 2 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 1);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 1);

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);
  refiner.activate(0);
  refiner.activate(1);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(2));
  ASSERT_THAT(refiner._pq[1]->key(1), Eq(1));

  hypergraph.changeNodePartition(1, 1, 0);
  refiner._marked[1] = true;
  refiner.updateNeighbours(1, 1, 0);

  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
}

TEST_F(AGainUpdateMethod, HandlesCase0To1) {
  HypergraphType hypergraph(4, 1, HyperedgeIndexVector { 0, 4 }, HyperedgeVector { 0, 1, 2, 3 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(3, INVALID_PARTITION, 0);

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);
  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._pq[0]->insert(0, refiner.computeGain(0));
  refiner._pq[0]->insert(1, refiner.computeGain(1));
  refiner._pq[0]->insert(2, refiner.computeGain(2));
  refiner._pq[0]->insert(3, refiner.computeGain(3));
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(3), Eq(-1));

  hypergraph.changeNodePartition(3, 0, 1);
  refiner._marked[3] = true;
  refiner.updateNeighbours(3, 0, 1);

  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(0));
}

TEST_F(AGainUpdateMethod, HandlesCase1To0) {
  HypergraphType hypergraph(5, 2, HyperedgeIndexVector { 0, 4, 8 }, HyperedgeVector { 0, 1, 2, 3, 0, 1, 2, 4 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph.changeNodePartition(4, INVALID_PARTITION, 1);

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);
  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner.activate(0);
  refiner.activate(1);
  refiner.activate(2);
  refiner.activate(3);
  refiner.activate(4);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(3), Eq(1));

  hypergraph.changeNodePartition(3, 1, 0);
  refiner._marked[3] = true;
  refiner.updateNeighbours(3, 1, 0);

  ASSERT_THAT(refiner._pq[0]->key(0), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(-1));
}

TEST_F(AGainUpdateMethod, HandlesCase2To1) {
  HypergraphType hypergraph(4, 1, HyperedgeIndexVector { 0, 4 }, HyperedgeVector { 0, 1, 2, 3 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph.changeNodePartition(3, INVALID_PARTITION, 1);

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);
  refiner.activate(0);
  refiner.activate(1);
  refiner.activate(2);
  refiner.activate(3);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(2), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(3), Eq(0));

  hypergraph.changeNodePartition(3, 1, 0);
  refiner._marked[3] = true;
  refiner.updateNeighbours(3, 1, 0);

  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(2), Eq(1));
}

TEST_F(AGainUpdateMethod, HandlesCase1To2) {
  HypergraphType hypergraph(4, 1, HyperedgeIndexVector { 0, 4 }, HyperedgeVector { 0, 1, 2, 3 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(3, INVALID_PARTITION, 1);

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);
  refiner.activate(0);
  refiner.activate(1);
  refiner.activate(2);
  refiner.activate(3);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(3), Eq(1));

  hypergraph.changeNodePartition(2, 0, 1);
  refiner._marked[2] = true;
  refiner.updateNeighbours(2, 0, 1);

  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(3), Eq(0));
}

TEST_F(AGainUpdateMethod, HandlesSpecialCaseOfHyperedgeWith3Pins) {
  HypergraphType hypergraph(3, 1, HyperedgeIndexVector { 0, 3 }, HyperedgeVector { 0, 1, 2 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 1);

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);
  refiner.activate(0);
  refiner.activate(1);
  refiner.activate(2);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(2), Eq(1));

  hypergraph.changeNodePartition(1, 0, 1);
  refiner._marked[1] = true;
  refiner.updateNeighbours(1, 0, 1);

  ASSERT_THAT(refiner._pq[0]->key(0), Eq(1));
  ASSERT_THAT(refiner._pq[1]->key(2), Eq(0));
}

TEST_F(AGainUpdateMethod, RemovesNonBorderNodesFromPQ) {
  HypergraphType hypergraph(3, 1, HyperedgeIndexVector { 0, 3 }, HyperedgeVector { 0, 1, 2 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 1);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 0);

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);
  refiner.activate(0);
  refiner.activate(1);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(1), Eq(1));
  ASSERT_THAT(refiner._pq[0]->contains(2), Eq(false));
  ASSERT_THAT(refiner._pq[0]->contains(0), Eq(true));

  hypergraph.changeNodePartition(1, 1, 0);
  refiner._marked[1] = true;
  refiner.updateNeighbours(1, 1, 0);

  ASSERT_THAT(refiner._pq[1]->key(1), Eq(1));
  ASSERT_THAT(refiner._pq[0]->contains(0), Eq(false));
  ASSERT_THAT(refiner._pq[0]->contains(2), Eq(false));
}

TEST_F(AGainUpdateMethod, ActivatesUnmarkedNeighbors) {
  HypergraphType hypergraph(3, 1, HyperedgeIndexVector { 0, 3 }, HyperedgeVector { 0, 1, 2 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 0);

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);
  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._pq[0]->insert(0, refiner.computeGain(0));
  refiner._pq[0]->insert(1, refiner.computeGain(1));
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->contains(2), Eq(false));

  hypergraph.changeNodePartition(1, 0, 1);
  refiner._marked[1] = true;
  refiner.updateNeighbours(1, 0, 1);

  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->contains(2), Eq(true));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(0));
}

TEST_F(AGainUpdateMethod, DoesNotDeleteJustActivatedNodes) {
  HypergraphType hypergraph(5, 3, HyperedgeIndexVector { 0, 2, 5, 8 },
                            HyperedgeVector { 0, 1, 2, 3, 4, 2, 3, 4 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph.changeNodePartition(4, INVALID_PARTITION, 0);
  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);

  // bypassing activate
  refiner._pq[0]->insert(2, refiner.computeGain(2));
  refiner.moveHypernode(2, 0, 1);
  refiner.updateNeighbours(2, 0, 1);

  ASSERT_THAT(refiner._pq[0]->contains(4), Eq(true));
  ASSERT_THAT(refiner._pq[1]->contains(3), Eq(true));
}

TEST(ARefiner, DoesNotDeleteMaxGainNodeInPQ0IfItChoosesToUseMaxGainNodeInPQ1) {
  HypergraphType hypergraph(4, 3, HyperedgeIndexVector { 0, 2, 4, 6 },
                            HyperedgeVector { 0, 1, 2, 3, 2, 3 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 1);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(3, INVALID_PARTITION, 1);
  Configuration config;
  config.partitioning.epsilon = 1;
  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);

  refiner.activate(0);
  refiner.activate(3);

  bool chosen_pq_index = refiner.selectQueue(true, true);
  ASSERT_THAT(refiner._pq[chosen_pq_index]->maxKey(), Eq(2));
  ASSERT_THAT(refiner._pq[chosen_pq_index]->max(), Eq(3));
  ASSERT_THAT(chosen_pq_index, Eq(1));

  // prior to bugfix we accidentally deleted this HN from the queue!
  ASSERT_THAT(refiner._pq[0]->contains(0), Eq(true));
}

TEST(ARefiner, ChecksIfMovePreservesBalanceConstraint) {
  HypergraphType hypergraph(4, 1, HyperedgeIndexVector { 0, 4 },
                            HyperedgeVector { 0, 1, 2, 3 });
  hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(2, INVALID_PARTITION, 0);
  hypergraph.changeNodePartition(3, INVALID_PARTITION, 1);

  Configuration config;
  config.partitioning.epsilon = 0.02;
  config.partitioning.partition_size_upper_bound = (1 + config.partitioning.epsilon)
                                                   * ceil(hypergraph.initialNumNodes()
                                                          / static_cast<double>(config.partitioning.k));

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  refiner.initialize(0);
  ASSERT_THAT(refiner.movePreservesBalanceConstraint(1, 0, 1), Eq(true));
  ASSERT_THAT(refiner.movePreservesBalanceConstraint(3, 1, 0), Eq(false));
}
} // namespace partition
