/****************************************************************************
 * Copyright (C) 2009-2010 GGA Software Services LLC
 * 
 * This file is part of Indigo toolkit.
 * 
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

#include "graph/automorphism_search.h"

using namespace indigo;

AutomorphismSearch::AutomorphismSearch () :
TL_CP_GET(_lab),
TL_CP_GET(_ptn),
TL_CP_GET(_graph),
TL_CP_GET(_mapping),
TL_CP_GET(_inv_mapping),
TL_CP_GET(_degree),
TL_CP_GET(_tcells),
TL_CP_GET(_fix),
TL_CP_GET(_mcr),
TL_CP_GET(_active),
TL_CP_GET(_workperm),
TL_CP_GET(_workperm2),
TL_CP_GET(_bucket),
TL_CP_GET(_count),
TL_CP_GET(_firstlab),
TL_CP_GET(_canonlab),
TL_CP_GET(_orbits),
TL_CP_GET(_fixedpts),
TL_CP_GET(_work_active_cells),
TL_CP_GET(_edge_ranks_in_refine)
{
   getcanon = true;
   compare_vertex_degree_first = true;
   refine_reverse_degree = false;
   refine_by_sorted_neighbourhood = false;
   worksize = 100;

   context = 0;
   cb_vertex_rank = 0;
   cb_vertex_cmp = 0;
   cb_check_automorphism = 0;
   cb_compare_mapped = 0;
   cb_automorphism = 0;
   cb_edge_rank = 0;
   context_automorphism = 0;
   _given_graph = 0;
   ignored_vertices = 0;
}

AutomorphismSearch::~AutomorphismSearch ()
{
}

void AutomorphismSearch::_prepareGraph (Graph &graph)
{
   QS_DEF(Array<int>, buckets);
   QS_DEF(Array<int>, ranks);
   int i;

   ranks.clear();
   buckets.clear();

   _graph.clear();
   _mapping.clear();
   _degree.clear();

   _ptn.clear();

   _inv_mapping.clear_resize(graph.vertexEnd());
   _degree.clear_resize(graph.vertexEnd());
   _degree.zerofill();

   if (cb_vertex_rank != 0 && cb_vertex_cmp != 0)
      throw Error("both vertex rank and vertex compare callbacks specified");

   _given_graph = &graph;

   if (cb_vertex_cmp != 0)
   {
      for (i = graph.vertexBegin(); i != graph.vertexEnd(); i = graph.vertexNext(i))
         if (ignored_vertices == 0 || !ignored_vertices[i])
            _mapping.push(i);

      for (i = graph.edgeBegin(); i != graph.edgeEnd(); i = graph.edgeNext(i))
      {
         const Edge &edge = graph.getEdge(i);

         if (ignored_vertices == 0 || (!ignored_vertices[edge.beg] && !ignored_vertices[edge.end]))
         {
            _degree[edge.beg]++;
            _degree[edge.end]++;
         }
      }

      _mapping.qsort(_cmp_vertices, this);

      int rank = 0;

      for (i = 0; i < _mapping.size(); i++)
      {
         if (i > 0 && _cmp_vertices(_mapping[i], _mapping[i - 1], this) != 0)
            rank++;

         ranks.push(rank);
      }
   }
   else
   {
      for (i = graph.vertexBegin(); i != graph.vertexEnd(); i = graph.vertexNext(i))
      {
         if (ignored_vertices != 0 && ignored_vertices[i])
            continue;

         int rank = 0;

         if (cb_vertex_rank != 0)
            rank = cb_vertex_rank(graph, i, context);

         ranks.push(rank);
         _mapping.push(i);
      }
   }

   for (i = 0; i < _mapping.size(); i++)
   {
      _graph.addVertex();
      _inv_mapping[_mapping[i]] = i;
      _ptn.push(INFINITY);

      while (buckets.size() <= ranks[i])
         buckets.push(0);

      buckets[ranks[i]]++;
   }

   for (i = graph.edgeBegin(); i != graph.edgeEnd(); i = graph.edgeNext(i))
   {
      const Edge &edge = graph.getEdge(i);

      if (ignored_vertices != 0 && (ignored_vertices[edge.beg] || ignored_vertices[edge.end]))
         continue;

      int beg = _inv_mapping[edge.beg];
      int end = _inv_mapping[edge.end];

      _graph.addEdge(beg, end);
   }

   int start = 0;

   for (i = 0; i < buckets.size(); i++)
   {
      if (buckets[i] == 0)
         continue;

      int end = start + buckets[i];

      buckets[i] = start;

      _ptn[end - 1] = 0;

      start = end;
   }

   _n = _graph.vertexCount();

   _lab.clear_resize(_n);
   
   for (i = 0; i < _n; i++)
      _lab[buckets[ranks[i]]++] = i;

}

int AutomorphismSearch::_cmp_vertices (int idx1, int idx2, void *context)
{
   const AutomorphismSearch *self = (const AutomorphismSearch *)context;

   int degree_diff = self->_degree[idx1] - self->_degree[idx2];

   if (self->compare_vertex_degree_first)
      if (degree_diff != 0)
         return degree_diff;
   if (self->cb_vertex_cmp == 0)
      return degree_diff;

   int ret_cb_vertex_cmp = self->cb_vertex_cmp(*self->_given_graph, idx1, idx2, self->context);
   if (ret_cb_vertex_cmp != 0)
      return ret_cb_vertex_cmp;

   if (!self->compare_vertex_degree_first)
      return degree_diff;

   return 0;
}

void AutomorphismSearch::getCanonicalNumbering (Array<int> &numbering)
{
   int i;

   numbering.clear();

   for (i = 0; i < _mapping.size(); i++)
      numbering.push(_mapping[_canonlab[i]]);
}

void AutomorphismSearch::getOrbits (Array<int> &orbits) const
{
   orbits.clear_resize(_given_graph->vertexEnd());
   orbits.fffill();

   for (int i = 0; i < _mapping.size(); i++)
      orbits[_mapping[i]] = _orbits[i];
}

void AutomorphismSearch::getCanonicallyOrderedOrbits (Array<int> &orbits) const
{
   // Each vertex in the orbit has its canonical number.
   // Canonical orbit index is the minimal canonical index of the vertices 
   // from this orbit
   QS_DEF(Array<int>, min_vertex_in_orbit);
   min_vertex_in_orbit.clear_resize(_given_graph->vertexEnd());
   min_vertex_in_orbit.fffill();
   
   for (int i = 0; i < _mapping.size(); i++)
   {
      int vertex = _canonlab[i];
      int orbit = _orbits[vertex];

      if (min_vertex_in_orbit[orbit] == -1 || min_vertex_in_orbit[orbit] > i)
         min_vertex_in_orbit[orbit] = i;
   }

   orbits.clear_resize(_given_graph->vertexEnd());
   orbits.fffill();

   for (int i = 0; i < _mapping.size(); i++)
      orbits[_mapping[i]] = min_vertex_in_orbit[_orbits[i]];
}


void AutomorphismSearch::process (Graph &graph)
{
   _prepareGraph(graph);

   _active.clear_resize(_n);
   _workperm.clear_resize(_n);
   _workperm2.clear_resize(_n);
   _firstlab.clear_resize(_n);
   _canonlab.clear_resize(_n);
   _fixedpts.clear_resize(_n);
   _count.clear_resize(_n);
   _orbits.clear_resize(_n);
   _fix.clear();
   _mcr.clear();

   if (_n == 0)
      return;
   
   _fixedpts.zerofill();
   _needshortprune = false;
   _orbits_num = _n;

   int i, numcells = 0;

   _ptn[_n - 1] = 0;

   for (i = 0; i < _n; i++)
      if (_ptn[i] != 0)
         _ptn[i] = INFINITY;
      else
         numcells++;

   _active.zerofill();

   for (i = 0; i < _n; i++)
   {
      _active[i] = 1;
      while (_ptn[i])
         i++;
   }

   for (i = 0; i < _n; ++i)
      _orbits[i] = i;

   _firstNode(1, numcells);
}

int AutomorphismSearch::_firstNode (int level, int numcells)
{
   _refine(level, numcells);

   _tcells.resize(level + 1);

   if (numcells == _n) // found first leaf?
   {
      _gca_first = level;

      _firstlab.copy(_lab);

      if (getcanon)
      {
         _canonlevel = _gca_canon = level;
         _canonlab.copy(_lab);
      }
      
      return level - 1;
   }

   // locate new target cell
   int tc = _targetcell(level, _tcells[level]);
   int tv1 = _tcells[level][0];
   int k;

   // use the elements of the target cell to produce the children
   for (k = 0; k < _tcells[level].size(); k++)
   {
      int tv = _tcells[level][k];

      if (_orbits[tv] == tv) // not equivalent to previous child
      {
         int rtnlevel;

         _breakout(level + 1, tc, tv);
         _cosetindex = tv;
         _fixedpts[tv] = 1;

         if (tv == tv1)
         {
            rtnlevel = _firstNode(level + 1, numcells + 1);
            _gca_first = level;
         }
         else
            rtnlevel = _otherNode(level + 1, numcells + 1);
         
         _fixedpts[tv] = 0;

         if (rtnlevel < level)
            return rtnlevel;

         if (_needshortprune)
         {
            _needshortprune = false;
            k = _shortPrune(_tcells[level], _mcr.top(), k);
         }
         _recover(level);
      }
   }

   return level - 1;
}

int AutomorphismSearch::_otherNode (int level, int numcells)
{
   _refine(level, numcells);

   _tcells.resize(level + 1);
   
   int rtnlevel = _processNode(level, numcells);

   if (rtnlevel < level) // keep returning if necessary
      return rtnlevel;

   int tc = _targetcell(level, _tcells[level]);

   if (_needshortprune)
   {
      _needshortprune = false;
      _shortPrune(_tcells[level], _mcr.top(), 0);
   }

   int tv1 = _tcells[level][0];

   int k;

   // use the elements of the target cell to produce the children
   for (k = 0; k < _tcells[level].size(); k++)
   {
      int tv = _tcells[level][k];

      _breakout(level + 1, tc, tv);
      _fixedpts[tv] = 1;

      rtnlevel = _otherNode(level + 1, numcells + 1);

      _fixedpts[tv] = 0;

      if (rtnlevel < level)
         return rtnlevel;

      // use stored automorphism data to prune target cell
      if (_needshortprune)
      {
          _needshortprune = false;
         k = _shortPrune(_tcells[level], _mcr.top(), k);
      }

      if (tv == tv1)
         k = _longPrune(_tcells[level], _fixedpts, k);

      _recover(level);
   }

   return level - 1;
}

void AutomorphismSearch::_recover (int level)
{
   int i;

   for (i = 0; i < _n; ++i)
      if (_ptn[i] > level)
         _ptn[i] = INFINITY;

   if (getcanon)
   {
      if (level < _gca_canon)
         _gca_canon = level;

      if (level < _gca_first)
         throw Error("??");
   }
}

void AutomorphismSearch::_breakout (int level, int tc, int tv)
{
   _active.zerofill();

   _active[tc] = 1;

   int i = tc;
   int prev = tv;

   do
   {
      int next = _lab[i];

      _lab[i++] = prev;
      prev = next;
   } while (prev != tv);

   _ptn[tc] = level;
}

int AutomorphismSearch::_shortPrune (Array<int> &tcell, Array<int> &mcr, int idx)
{
   int i, j;
   int ret = idx;

   for (i = j = 0; i < tcell.size(); i++)
      if (mcr[tcell[i]])
         tcell[j++] = tcell[i];
      else if (idx >= i)
         ret--;

   tcell.resize(j);
   return ret;
}

int AutomorphismSearch::_longPrune (Array<int> &tcell, Array<int> &fixed, int idx)
{
   int i, j, k;
   int ret = idx;

   for (k = 0; k < _fix.size(); k++)
   {
      for (j = 0; j < _n; j++)
         if (_fix[k][j] == 0 && fixed[j] == 1)
            break;

      if (j != _n)
         continue;

      for (i = j = 0; i < tcell.size(); i++)
         if (_mcr[k][tcell[i]])
            tcell[j++] = tcell[i];
         else if (idx >= i)
            ret--;

      tcell.resize(j);
      idx = ret;
   }

   return ret;
}

int AutomorphismSearch::_processNode (int level, int numcells)
{
   int i;

   // no idea what this nauty's if() means.
   //if (_eqlev_first != level && (!getcanon || _comp_canon < 0))
   //   code = 4;

   if (numcells != _n) // discrete partition?
      return level;

   for (i = 0; i < _n; i++)
      _workperm[_firstlab[i]] = _lab[i];

   if (_isAutomorphism(_workperm))
   {
      // _lab is equivalent to firstlab
      if (_fix.size() == worksize)
      {
         _fix.pop();
         _mcr.pop();
      }
      _buildFixMcr(_workperm, _fix.push(), _mcr.push());
      _joinOrbits(_workperm);
      _handleAutomorphism(_workperm);

      return _gca_first;
   }

   if (getcanon)
   {
      /*if (_comp_canon == 0)
      {
         // again, strange nauty's if()
         if (level < _canonlevel)
            _comp_canon = 1;
         else
            _comp_canon = _compareCanon();
      }*/

      int comp_canon = _compareCanon();

      if (comp_canon == 0)
      {
         // _lab is equivalent to canonlab
         for (i = 0; i < _n; i++)
            _workperm[_canonlab[i]] = _lab[i];

         if (_fix.size() == worksize)
         {
            _fix.pop();
            _mcr.pop();
         }
         _buildFixMcr(_workperm, _fix.push(), _mcr.push());

         int norb = _orbits_num;

         _joinOrbits(_workperm);

         if (norb != _orbits_num)
         {
            _handleAutomorphism(_workperm);
            if (_orbits[_cosetindex] < _cosetindex)
               return _gca_first;
         }
         if (_gca_canon != _gca_first)
            _needshortprune = true;
         return _gca_canon;
      }
      else if (comp_canon > 0)
      {
         // _lab is better than canonlab
         _canonlab.copy(_lab);
         _canonlevel = _gca_canon = level;
      }
   }

   return level - 1;
}

void AutomorphismSearch::_joinOrbits (const Array<int> &perm)
{
   int i, j1, j2;

   for (i = 0; i < _n; i++)
   {
      j1 = _orbits[i];

      while (_orbits[j1] != j1)
         j1 = _orbits[j1];

      j2 = _orbits[perm[i]];

      while (_orbits[j2] != j2)
         j2 = _orbits[j2];

      if (j1 < j2)
         _orbits[j2] = j1;
      else if (j1 > j2)
         _orbits[j1] = j2;
   }

   _orbits_num = 0;

   for (i = 0; i < _n; i++)
   {
      _orbits[i] = _orbits[_orbits[i]];
      if (_orbits[i] == i)
         _orbits_num++;
   }
}

bool AutomorphismSearch::_isAutomorphism (Array<int> &perm)
{
   for (int i = _graph.edgeBegin(); i != _graph.edgeEnd(); i = _graph.edgeNext(i))
   {
      const Edge &edge = _graph.getEdge(i);

      if (!_graph.haveEdge(perm[edge.beg], perm[edge.end]))
         return false;
   }

   if (cb_check_automorphism != 0)
   {
      QS_DEF(Array<int>, perm_mapping);

      perm_mapping.clear_resize(_given_graph->vertexEnd());
      perm_mapping.fffill();

      for (int i = 0; i < _n; i++)
         perm_mapping[_mapping[i]] = _mapping[perm[i]];

      return cb_check_automorphism(*_given_graph, perm_mapping, context);
   }

   return true;
}

// lab vs. canonlab
int AutomorphismSearch::_compareCanon ()
{
   int i;

   QS_DEF(Array<int>, map);
   QS_DEF(Array<int>, canon_map);

   map.clear_resize(_n);
   canon_map.clear_resize(_n);

   for (i = 0; i < _n; i++)
   {
      map[i] = _mapping[_lab[i]];
      canon_map[i] = _mapping[_canonlab[i]];
   }

   if (cb_compare_mapped == 0)
      throw Error("cb_compare_mapped = 0");
   return cb_compare_mapped(*_given_graph, map, canon_map, context);
}

void AutomorphismSearch::_buildFixMcr (const Array<int> &perm, Array<int> &fix, Array<int> &mcr)
{
   int i;

   fix.clear_resize(_n);
   mcr.clear_resize(_n);
   fix.zerofill();
   mcr.zerofill();

   _workperm2.zerofill();

   for (i = 0; i < _n; ++i)
   {
      if (perm[i] == i)
      {
         fix[i] = 1;
         mcr[i] = 1;
      }
      else if (_workperm2[i] == 0)
      {
         int l = i;

         do
         {
            _workperm2[l] = 1;
            l = perm[l];
         } while (l != i);

         mcr[i] = 1;
      }
   }
}

int AutomorphismSearch::_targetcell (int level, Array<int> &cell)
{
   int i = 0, j, k;
   int ibest = -1, jbest = -1, bestdegree = -1;
   
   while (i < _n)
   {
      for (; i < _n && _ptn[i] <= level; ++i)
         ;

      if (i == _n)
         break;
      else for (j = i + 1; _ptn[j] > level; j++)
         ;

      int degree = _degree[_mapping[_lab[i]]];

      // Choose cell with single vertices first and then biggest cell
      if (ibest == -1 || (degree == 0 && bestdegree != 0) || (bestdegree != 0 && j - i > jbest - ibest))
      {
         jbest = j;
         ibest = i;
         bestdegree = degree;
      }

      i = j + 1;
   }

   if (ibest == -1)
      throw Error("(intenal error) target cell cannot be found");

   i = ibest;
   j = jbest; 

   cell.clear();

   int imin = 0;

   for (k = i; k <= j; k++)
   {
      cell.push(_lab[k]);

      if (cell.size() > 0 && cell[cell.size() - 1] < cell[imin])
         imin = cell.size() - 1;
   }

   if (imin > 0)
      cell.swap(0, imin);

   return i;
}

void AutomorphismSearch::_refine (int level, int &numcells)
{
   if (refine_by_sorted_neighbourhood)
      _refineBySortingNeighbourhood(level, numcells);
   else
      _refineOriginal(level, numcells);
}

void AutomorphismSearch::_refineOriginal (int level, int &numcells)
{
   int hint = 0;

   int split1 = -1;

   while (numcells < _n)
   {
      int split2;

      if (_active[hint])
         split1 = hint;
      else
      {
         int i;
         for (i = 0; i < _n; i++)
         {
            split1 = (split1 + 1) % _n;
            if (_active[split1])
               break;
         }
         if (i == _n)
            break;
      }

      _active[split1] = 0;

      for (split2 = split1; _ptn[split2] > level; split2++)
         ;

      _edge_ranks_in_refine.clear();

      _refineByCell(split1, split2, level, numcells, hint, -1);
      // Check if there are exists edge with different ranks
      // Last element in _edge_ranks_in_refine array contains positive value that 
      // means that such edge rank exists. But it is nessesary to refine by all ranks 
      // except one because cells have already been refined by edges without ranks.
      for (int i = 0; i < _edge_ranks_in_refine.size() - 1; i++)
         if (_edge_ranks_in_refine[i] != 0)
            _refineByCell(split1, split2, level, numcells, hint, i);
   }
}

void AutomorphismSearch::_refineBySortingNeighbourhood (int level, int &numcells)
{
   // This refine procedure works like refining by sorting neighbourhood ranks

   while (true)
   {
      // Collect active cells
      _work_active_cells.clear();
      for (int i = 0; i < _n; i++)
      {
         int split1;
         if (_active[i])
         {
            split1 = i;

            int split2;
            for (split2 = split1; _ptn[split2] > level; split2++)
               ;

            int (&split_cell)[2] = _work_active_cells.push();
            split_cell[0] = split1;
            split_cell[1] = split2;

            _active[i] = 0;
         }
      }

      if (_work_active_cells.size() == 0)
         break;

      // Refine all cells by collected active cells
      for (int i = 0; i < _work_active_cells.size(); i++)
      {
         int (&split_cell)[2] = _work_active_cells[i];

         int split1 = split_cell[0], split2 = split_cell[1];

         int dummy_hint;
         _refineByCell(split1, split2, level, numcells, dummy_hint, -1);

         if (numcells == _n)
            return;
      }
   }
}

bool AutomorphismSearch::_hasEdgeWithRank (int from, int to, int target_edge_rank)
{
   int edge_index = _graph.findEdgeIndex(from, to);

   if (edge_index == -1)
      return false;

   if (cb_edge_rank == 0)
      return true;

   int mapped_v1 = _mapping[from];
   int mapped_v2 = _mapping[to];
   int edge_index_mapped = _given_graph->findEdgeIndex(mapped_v1, mapped_v2);
   if (edge_index_mapped == -1)
      throw Error("Internal error: edge must exists");

   int edge_rank = cb_edge_rank(*_given_graph, edge_index_mapped, context);

   if (target_edge_rank == -1)
   {
      // Just update information about ranks
      while (_edge_ranks_in_refine.size() <= edge_rank)
         _edge_ranks_in_refine.push(0);

      _edge_ranks_in_refine[edge_rank]++;
      return true;
   }

   return target_edge_rank == edge_rank;
}

void AutomorphismSearch::_refineByCell (int split1, int split2, int level, int &numcells, int &hint, int target_edge_rank)
{
   int i, j, tmp;

   if (split1 == split2) // trivial splitting cell
   {
      int cell1, cell2;

      for (cell1 = 0; cell1 < _n; cell1 = cell2 + 1)
      {
         for (cell2 = cell1; _ptn[cell2] > level; cell2++)
            ;
         if (cell1 == cell2)
            continue;

         int c1 = cell1, c2 = cell2;

         while (c1 <= c2)
         {
            if (_hasEdgeWithRank(_lab[split1], _lab[c1], target_edge_rank))
               c1++;
            else
            {
               __swap(_lab[c1], _lab[c2], tmp);
               c2--;
            }
         }

         if (c2 >= cell1 && c1 <= cell2)
         {
            _ptn[c2] = level;
            numcells++;

            if (_active[cell1] || (c2 - cell1 >= cell2 - c1 && !refine_by_sorted_neighbourhood))
            {
               _active[c1] = 1;
               if (c1 == cell2)
                  hint = c1;
            }
            else
            {
               _active[cell1] = 1;
               if (c2 == cell1)
                  hint = cell1;
            }
         }
      }
   }
   else // nontrivial splitting cell
   {
      int cell1, cell2;

      for (cell1 = 0; cell1 < _n; cell1 = cell2 + 1)
      {
         for (cell2 = cell1; _ptn[cell2] > level; ++cell2)
            ;
         if (cell1 == cell2)
            continue;

         int bmin = _n;

         _bucket.clear();

         for (i = cell1; i <= cell2; i++)
         {
            int cnt = 0;

            for (j = split1; j <= split2; j++)
               if (_hasEdgeWithRank(_lab[i], _lab[j], target_edge_rank))
                  cnt++;

            while (_bucket.size() <= cnt)
               _bucket.push(0);

            _bucket[cnt]++;

            if (cnt < bmin)
               bmin = cnt;

            _count[i] = cnt;
         }

         if (bmin == _bucket.size() - 1)
            continue;

         if (refine_reverse_degree)
         {
            // Reverse degree locally to avoid code changing below
            for (i = cell1; i <= cell2; i++)
            {
               _count[i] = _bucket.size() - _count[i] - 1;
            }
            for (i = _bucket.size() - 1; i >= bmin; i--)
            {
               int dest = _bucket.size() - i - 1;
               if (dest < i)
                  __swap(_bucket[i], _bucket[dest], tmp);
            }
            _bucket.resize(_bucket.size() - bmin);
            bmin = 0;
         }

         int c1 = cell1, c2;
         int maxcell = -1, maxpos = -1;
         int last_c1 = -1;

         for (i = bmin; i < _bucket.size(); i++)
         {
            if (_bucket[i] == 0)
               continue;

            c2 = c1 + _bucket[i];
            _bucket[i] = c1;
            last_c1 = c1;

            if (c2 - c1 > maxcell)
            {
               maxcell = c2 - c1;
               maxpos = c1;
            }

            if (c1 != cell1)
            {
               _active[c1] = 1;
               if (c2 - c1 == 1)
                  hint = c1;
               numcells++;
            }
            if (c2 <= cell2)
               _ptn[c2 - 1] = level;

            c1 = c2;
         }

         for (i = cell1; i <= cell2; i++)
            _workperm2[_bucket[_count[i]]++] = _lab[i];

         for (i = cell1; i <= cell2; i++)
            _lab[i] = _workperm2[i];

         if (_active[cell1] == 0)
         {
            _active[cell1] = 1;

            // When sorting by neighbourhood is is allowed only to exclude 
            // the last created subcell. For ordinary refine greatest cell is excluded.
            if (!refine_by_sorted_neighbourhood)
               _active[maxpos] = 0;
            else
               _active[last_c1] = 0;
         }
      }
   }
}

void AutomorphismSearch::_handleAutomorphism (const Array<int> &perm)
{
   if (cb_automorphism != 0)
   {
      QS_DEF(Array<int>, perm2);
      int i;

      perm2.clear_resize(_given_graph->vertexEnd());
      perm2.fffill();

      for (i = 0; i < _n; i++)
         perm2[_mapping[i]] = _mapping[perm[i]];

      cb_automorphism(perm2.ptr(), context_automorphism);
   }
}

