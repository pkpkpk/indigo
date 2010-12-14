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

#ifndef __molfile_loader__
#define __molfile_loader__

#include "base_cpp/exception.h"
#include "base_cpp/array.h"
#include "base_cpp/tlscont.h"

namespace indigo {

class Scanner;
class BaseMolecule;
class Molecule;
class QueryMolecule;
class GraphHighlighting;

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif

class DLLEXPORT MolfileLoader
{
public:
   DEF_ERROR("molfile loader");

   MolfileLoader (Scanner &scanner);

   void loadMolecule      (Molecule &mol);
   void loadQueryMolecule (QueryMolecule &mol);

   // for Rxnfiles v3000
   void loadCtab3000 (Molecule &mol);
   void loadQueryCtab3000 (QueryMolecule &mol);

   // optional parameters for reaction
   Array<int> * reaction_atom_mapping;
   Array<int> * reaction_atom_inversion;
   Array<int> * reaction_atom_exact_change;
   Array<int> * reaction_bond_reacting_center;

   // optional highlighting
   GraphHighlighting * highlighting;

   bool ignore_stereocenter_errors;
   bool treat_x_as_pseudoatom; // normally 'X' means 'any halogen'

protected:

   Scanner &_scanner;
   bool     _rgfile;

   TL_CP_DECL(Array<int>, _stereo_care_atoms);
   TL_CP_DECL(Array<int>, _stereo_care_bonds);
   TL_CP_DECL(Array<int>, _stereocenter_types);
   TL_CP_DECL(Array<int>, _stereocenter_groups);
   TL_CP_DECL(Array<int>, _bond_directions);
   TL_CP_DECL(Array<int>, _ignore_cistrans);

   enum
   {
      _ATOM_R,
      _ATOM_A,
      _ATOM_X,
      _ATOM_Q,
      _ATOM_LIST,
      _ATOM_NOTLIST,
      _ATOM_PSEUDO,
      _ATOM_ELEMENT
   };

   enum
   {
      _BOND_SINGLE_OR_DOUBLE = 5,
      _BOND_SINGLE_OR_AROMATIC = 6,
      _BOND_DOUBLE_OR_AROMATIC = 7,
      _BOND_ANY = 8
   };


   TL_CP_DECL(Array<int>, _atom_types);
   TL_CP_DECL(Array<int>, _hcount);

   bool  _v2000;
   int   _atoms_num;
   int   _bonds_num;
   bool  _chiral;

   void _readHeader ();
   void _readCtabHeader ();
   void _readCtab2000 ();
   void _convertCharge (int value, int &charge, int &radical);
   void _read3dFeature2000 ();
   void _readRGroupOccurrenceRanges (const char *str, Array<int> &ranges);
   void _readRGroups2000 ();
   void _readCtab3000 ();
   void _readRGroups3000 ();
   void _preparePseudoAtomLabel (Array<char> &pseudo);
   void _readMultiString (Array<char> &str);
   void _init ();

   int _getElement (const char *buf);

   static int _asc_cmp_cb (int &v1, int &v2, void *context);
   void _postLoad ();

   void _loadMolecule ();

   Molecule      *_mol;
   BaseMolecule  *_bmol;
   QueryMolecule *_qmol;

private:
   MolfileLoader (const MolfileLoader &); // no implicit copy
};

}

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif
