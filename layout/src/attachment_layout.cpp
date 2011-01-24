/****************************************************************************
 * Copyright (C) 2009-2011 GGA Software Services LLC
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

#include "layout/attachment_layout.h"

using namespace indigo;

AttachmentLayout::AttachmentLayout(const BiconnectedDecomposer &bc_decom,
                                   const ObjArray<MoleculeLayoutGraph> &bc_components,
                                   const Array<int> &bc_tree,
                                   MoleculeLayoutGraph &graph, int src_vertex) :
_src_vertex(src_vertex),
TL_CP_GET(_src_vertex_map),
TL_CP_GET(_attached_bc),
TL_CP_GET(_bc_angles),
TL_CP_GET(_vertices_l),
_alpha(0.f),
TL_CP_GET(_new_vertices),
TL_CP_GET(_layout),
_energy(0.f),
_bc_components(bc_components),
_graph(graph)
{
   int i, v1, v2; 
   float sum = 0.f;  

   int n_comp = bc_decom.getIncomingCount(_src_vertex);

   if (bc_tree[_src_vertex] != -1)
   {
      _attached_bc.clear_resize(n_comp + 1);
      _attached_bc.top() = bc_tree[_src_vertex];
   }
   else
      _attached_bc.clear_resize(n_comp);

   _src_vertex_map.clear_resize(_attached_bc.size());
   _bc_angles.clear_resize(_attached_bc.size());
   _vertices_l.clear_resize(_attached_bc.size());

   for (i = 0; i < _attached_bc.size(); i++)
   {
      if (i < n_comp)
         _attached_bc[i] = bc_decom.getIncomingComponents(_src_vertex)[i];

      const MoleculeLayoutGraph &cur_bc = bc_components[_attached_bc[i]];

      _src_vertex_map[i] = cur_bc.findVertexByExtIdx(_src_vertex);
      _bc_angles[i] = cur_bc.calculateAngle(_src_vertex_map[i], v1, v2);
      sum += _bc_angles[i];
      _vertices_l[i] = v1;
   }

   _alpha = (2 * PI - sum) / _attached_bc.size();
   //TODO: what if negative?

   // find the one component which is drawn and put it to the end
   for (i = 0; i < _attached_bc.size() - 1; i++)
   {
      if (_graph.getVertexType(_bc_components[_attached_bc[i]].getVertexExtIdx(_vertices_l[i])) != ELEMENT_NOT_DRAWN)
      {
         _src_vertex_map.swap(i, _attached_bc.size() - 1);
         _attached_bc.swap(i, _attached_bc.size() - 1);
         _bc_angles.swap(i, _attached_bc.size() - 1);
         _vertices_l.swap(i, _attached_bc.size() - 1);

         break;
      }
   }

   int n_new_vert = 0;

   for (i = 0; i < _attached_bc.size() - 1; i++)
      n_new_vert += _bc_components[_attached_bc[i]].vertexCount() - 1;

   _new_vertices.clear_resize(n_new_vert);
   _layout.clear_resize(n_new_vert);
   _layout.zerofill();
}

// Calculate energy of the drawn part of graph
double AttachmentLayout::calculateEnergy ()
{
   int i,  j;
   double sum_a;
   float r;
   QS_DEF(Array<double>, norm_a);
   QS_DEF(Array<int>, drawn_vertices);

   drawn_vertices.clear_resize(_graph.vertexEnd());
   drawn_vertices.zerofill();

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
      if (_graph.getLayoutVertex(i).type != ELEMENT_NOT_DRAWN)
         drawn_vertices[i] = 1;

   for (i = 0; i < _new_vertices.size(); i++)
      drawn_vertices[_new_vertices[i]] = 2 + i;

   norm_a.clear_resize(_graph.vertexEnd());

   sum_a = 0.0;
   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
   {
      if (drawn_vertices[i] > 0)
      {
         norm_a[i] = _graph.getLayoutVertex(i).morgan_code;
         sum_a += norm_a[i] * norm_a[i];
      }
   }                                   

   sum_a = sqrt(sum_a);

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
      if (drawn_vertices[i] > 0)
         norm_a[i] = (norm_a[i] / sum_a) + 0.5;

   _energy = 0.0;

   const Vec2f *pos_i = 0;
   const Vec2f *pos_j = 0;

   for (i = _graph.vertexBegin(); i < _graph.vertexEnd(); i = _graph.vertexNext(i))
      if (drawn_vertices[i] > 0)
      {
         if (drawn_vertices[i] == 1)
            pos_i = &_graph.getPos(i);
         else
            pos_i = &_layout[drawn_vertices[i] - 2];

         for (j = _graph.vertexBegin(); j < _graph.vertexEnd(); j = _graph.vertexNext(j))
            if (drawn_vertices[j] > 0 && i != j)
            {
               if (drawn_vertices[j] == 1)
                  pos_j = &_graph.getPos(j);
               else
                  pos_j = &_layout[drawn_vertices[j] - 2];

               r = Vec2f::distSqr(*pos_i, *pos_j);

               if (r < EPSILON)
                  r = EPSILON;

               _energy += (norm_a[i] * norm_a[j] / r);
            }
      }

   return _energy;
}

void AttachmentLayout::applyLayout ()
{
   int i;

   for (i = 0; i < _new_vertices.size(); i++)
      _graph.getPos(_new_vertices[i]) = _layout[i];
}

void AttachmentLayout::markDrawnVertices()
{
   int i,  j;

   for (i = 0; i < _attached_bc.size(); i++)
   {
      const MoleculeLayoutGraph &comp = _bc_components[_attached_bc[i]];

      for (j = comp.vertexBegin(); j < comp.vertexEnd(); j = comp.vertexNext(j))
      {
         const LayoutVertex &vert = comp.getLayoutVertex(j);

         _graph.setVertexType(vert.ext_idx, vert.type);
      }

      for (j = comp.edgeBegin(); j < comp.edgeEnd(); j = comp.edgeNext(j))
      {
         const LayoutEdge &edge = comp.getLayoutEdge(j);

         _graph.setEdgeType(edge.ext_idx, edge.type);
      }
   }
}

LayoutChooser::LayoutChooser(AttachmentLayout &layout) :
_n_components(layout._attached_bc.size() - 1),
_cur_energy(1E+20f),
TL_CP_GET(_comp_permutation),
TL_CP_GET(_rest_numbers),
_layout(layout)
{
   _comp_permutation.clear_resize(_n_components);
   _rest_numbers.clear_resize(_n_components);

   for (int i = 0; i < _n_components; i++)
      _rest_numbers[i] = i;
}

// Look through all perturbations recursively
void LayoutChooser::_perform (int level)
{
   int i;

   if (level == 0)
   {
      // Try current perturbation
      // Draw new components on vertex
      _makeLayout();

      // Check if new layout is better
      if (_layout.calculateEnergy() < _cur_energy - EPSILON)
      {
         _layout.applyLayout();
         _cur_energy = _layout._energy;
      }

      return;
   }

   for (i = 0; i < level; i++)
   {
      _comp_permutation[level - 1] = _rest_numbers[i];
      _rest_numbers[i] = _rest_numbers[level - 1];
      _rest_numbers[level - 1] = _comp_permutation[level - 1];
      _perform(level - 1);
      _rest_numbers[level - 1] = _rest_numbers[i];
      _rest_numbers[i] = _comp_permutation[level - 1];
   }
}

// Draw components connected with respect to current order
void LayoutChooser::_makeLayout () 
{
   int i,  j,  k;
   float cur_angle;
   int v1C,  v2C,  v1,  v2;
   Vec2f p, p1;
   float phi,  phi1,  phi2;
   float cosa,  sina;
   int v;

   // cur_angle - angle between first edge of the drawn component and most "right" edge of current component
   k = -1;
   v = _layout._src_vertex;
   cur_angle = _layout._bc_angles[_n_components];
   v2 = _layout._bc_components[_layout._attached_bc[_n_components]].getVertexExtIdx(_layout._vertices_l[_n_components]);

   for (i = 0; i < _n_components; i++)
   {
      cur_angle += _layout._alpha;

      // Shift and rotate component so cur_angle is the angle between [v1C,v2C] and drawn edge [v,v2]
      int comp_idx = _comp_permutation[i];
      const MoleculeLayoutGraph &comp = _layout._bc_components[_layout._attached_bc[comp_idx]];

      v1C = _layout._src_vertex_map[comp_idx];
      v2C = _layout._vertices_l[comp_idx];
      v1 = comp.getVertexExtIdx(v2C);

      // Calculate angle (phi2-phi1) between [v,v2] and [v=v1C,v2C] in CCW order;

      p.diff(_layout._graph.getPos(v2), _layout._graph.getPos(v));

      phi1 = p.tiltAngle();

      p.diff(comp.getPos(v2C), comp.getPos(v1C));

      phi2 = p.tiltAngle();

      // Save current component's coordinates
      phi = cur_angle - (phi2 - phi1);
      cosa = cos(phi);
      sina = sin(phi);

      p.diff(_layout._graph.getPos(v), comp.getPos(v1C));

      for (j = comp.vertexBegin(); j < comp.vertexEnd(); j = comp.vertexNext(j))
         if (comp.getVertexExtIdx(j) != v)
         {
            k = k + 1;
            Vec2f &cur_pos = _layout._layout[k];
            // 1. Shift 				
            cur_pos.sum(comp.getPos(j), p);
            // 2. Rotate around v
            p1.diff(cur_pos, _layout._graph.getPos(v));
            p1.rotate(sina, cosa);
            cur_pos.sum(p1, _layout._graph.getPos(v));
            _layout._new_vertices[k] = comp.getVertexExtIdx(j);
         }

      cur_angle += _layout._bc_angles[comp_idx];
   }
}
