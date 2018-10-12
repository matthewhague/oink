/*
 * Copyright 2017-2018 Tom van Dijk, Johannes Kepler University Linz
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Modifed 2018 by Matthew Hague, Royal Holloway, University of London
 *
 * Modifications released under same license.  Marked MODIFIED below.
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <queue>
#include <deque>
#include <stack>

#include "game.hpp"
#include "solver.hpp" // for LOGIC_ERROR
#include "verifier.hpp"

using namespace std;

namespace pg {

void
Verifier::verify(bool fullgame, bool even, bool odd)
{
    const int n_nodes = game->n_nodes;

    // reindex if not yet done
    game->reindex_once();

    std::vector<int> *out = new std::vector<int>[n_nodes];

    /**
     * The first loop removes all edges from "won" nodes that are not the strategy.
     * This turns each dominion into a single player game.
     * Also some trivial checks are performed.
     */
    for (int i=0; i<n_nodes; i++) {
        // (for full solutions) check whether every node is won
        if (!game->solved[i]) {
            if (fullgame) throw "not every node is won";
            else continue;
        }

        const bool dom = game->winner[i];

        /** MODIFIED BY MATT **/
        if (game->strategy[i].size() > 1) {
            throw "Cannot verify non-deterministic strategy";
        } else if (game->strategy[i].empty()) {
            // no strategy, copy all edges
            out[i].insert(out[i].end(), game->out[i].begin(), game->out[i].end());
        } else {
            // only strategy
            out[i].push_back(game->strategy[i][0]);
        }
        /** END MODIFIED **/

        if (dom == game->owner[i]) {
            // if winner, check whether the strategy stays in the dominion
            if ((even and dom==0) or (odd and dom==1)) {
                /** MODIFIED BY MATT **/
                if (game->strategy[i].empty()) {
                    throw "winning node has no strategy";
                }

                int s = game->strategy[i][0];

                if (!game->solved[s] or game->winner[s] != dom) {
                    throw "strategy leaves dominion";
                }
                /** END MODIFIED **/

                // check whether the strategy is actually a valid move
                if (std::find(game->out[i].begin(), game->out[i].end(), s) == game->out[i].end()) {
                    throw "strategy is not a valid move";
                }
                n_strategies++;
            }
        } else {
            // if loser, check whether the loser can escape
            for (auto to : game->out[i]) {
                if (!game->solved[to] or game->winner[to] != dom) throw "loser can escape";
            }
            // and of course that no strategy is set
            /** MODIFIED BY MATT **/
            if (!game->strategy[i].empty()) throw "losing node has strategy";
            /** END MODIFIED **/
        }
    }

    // Allocate datastructures for Tarjan search
    int *done = new int[n_nodes];
    int64_t *low = new int64_t[n_nodes];
    for (int i=0; i<n_nodes; i++) done[i] = -1;
    for (int i=0; i<n_nodes; i++) low[i] = 0;

    std::vector<int> res;
    std::stack<int> st;

    int64_t pre = 0;

    for (int i=n_nodes-1; i>=0; i--) {
        /**
         * We're going to search all SCCs reachable from node <i> with priority <p>
         */
        int p = game->priority[i];

        /**
         * Only search when loser would win
         */
        if (!game->solved[i] or game->winner[i] == (p&1)) continue;
        if (!odd && (p&1) == 0) continue; // test odd strategies?
        if (!even && (p&1) == 1) continue; // test even strategies?

        /**
         * But if done[i] == p then we already checked this one!
         */
        if (done[i] == p) continue;

        /**
         * Set bot to current pre.
         */
        int64_t bot = pre;

        /**
         * Start DFS at <i>
         */
        st.push(i);

        while (!st.empty()) {
            int idx = st.top();

            /**
             * When we see it for the first item, we assign the next number to it and add it to <res>.
             */
            if (low[idx] <= bot) {
                low[idx] = ++pre;
                res.push_back(idx);
            }

            /**
             * Now we check all outgoing (allowed) edges.
             * If seen earlier, then update "min"
             * If new, then 'recurse'
             */
            int min = low[idx];
            bool pushed = false;
            for (auto to : out[idx]) {
                // skip if to higher priority or to already found scc
                if (to > i) continue;
                if (done[to] == p) continue;
                if (low[to] <= bot) {
                    // not visited, add to <st> and break!
                    st.push(to);
                    pushed = true;
                    break;
                } else {
                    // visited, update min
                    if (low[to] < min) min = low[to];
                }
            }
            if (pushed) continue; // we pushed...

            /**
             * If we're here, then there was no new edge and we check if we're the root of an SCC
             */
            if (min < low[idx]) {
                // not the root
                low[idx] = min;
                st.pop();
                continue;
            }

            /**
             * We're the root!
             * Now we need to figure out if we have cycles with p...
             * Also, mark every node in the SCC as "done @ search p"
             */
            int max_p = -1;
            int scc_size = 0;
            auto &priority = game->priority;
            for (auto it=res.rbegin(); it!=res.rend(); it++) {
                int n = *it;
                if (priority[n] > max_p) max_p = priority[n];
                scc_size++;
                done[n] = p;
                if (n == idx) break;
            }
            bool cycles = scc_size > 1 or std::find(out[idx].begin(), out[idx].end(), idx) != out[idx].end();
            if (cycles && (max_p&1) == (p&1)) {
                /**
                 * Found! Report.
                 */
                printf("\033[1;31mscc where loser wins\033[m with priority \033[1;34m%d\033[m", max_p);
                for (auto it=res.rbegin(); it!=res.rend(); it++) {
                    int n = *it;
                    printf(" %d", n);
                    if (n == idx) break;
                }
                printf("\n");
                delete[] done;
                delete[] low;

                throw "loser can win";
            }

            /**
             * Not found! Continue.
             */
            for (;;) {
                int n = res.back();
                res.pop_back();
                if (n == idx) break;
            }
            st.pop();
        }
    }

    delete[] done;
    delete[] low;
    delete[] out;
}

}
