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

#ifndef SOLVER_HPP
#define SOLVER_HPP

#include "game.hpp"
#include "oink.hpp"
#include <signal.h>

#define LOGIC_ERROR { printf("\033[1;7mlogic error %s:%d!\033[m\n", __FILE__, __LINE__); raise(SIGABRT); }

namespace pg {

class Solver
{
public:
    Solver(Oink *oink, Game *game) :
            oink(oink), game(game), logger(oink->logger), trace(oink->trace),
            n_nodes(game->n_nodes), priority(game->priority), owner(game->owner),
            out(game->out), in(game->in), disabled(oink->disabled),
            outa(oink->outa), ina(oink->ina), outs(oink->outs), ins(oink->ins)
    {
#ifndef NDEBUG
        // sanity check if the game is properly sorted
        for (int i=1; i<n_nodes; i++) assert(priority[i-1] <= priority[i]);
#endif
    }

    virtual ~Solver() { }

    /**
     * Run the solver.
     */
    virtual void run() = 0;

    /**
     * Returns true if the solver always solves all enabled vertices
     * before leaving run().
     */
    virtual bool full_solver() { return true; }

protected:
    Oink *oink;
    Game *game;
    std::ostream &logger;
    int trace = 0;

    const int n_nodes;
    const int * const priority;
    const bitset &owner;
    const std::vector<int> * const out;
    const std::vector<int> * const in;
    const bitset &disabled;

    const int* const outa;
    const int* const ina;
    const int* const outs;
    const int* const ins;

    /**
     * Use label_vertex(v) with operator<< of an output stream, for example,
     * std::cout << "vertex: " << label_vertex(v) << std::endl;
     */
    class _label_vertex
    {
    public:
        _label_vertex(Game &g, int v) : g(g), v(v) { }
        friend std::ostream& operator<<(std::ostream& out, const _label_vertex &lv) {
            std::string& l = lv.g.label[lv.v];
            if (l.empty()) out << lv.v << "/" << lv.g.priority[lv.v];
            else out << l;
            return out;
        }
    protected:
        Game &g;
        int v;
    };

    _label_vertex label_vertex(int v) { return _label_vertex(*game, v); }
};

}

#endif 
