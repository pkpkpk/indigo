/****************************************************************************
 * Copyright (C) 2010 GGA Software Services LLC
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

#ifndef __indigo_array__
#define __indigo_array__

#include "indigo_internal.h"

class IndigoArray : public IndigoObject
{
public:
   IndigoArray ();

   virtual ~IndigoArray ();

   virtual IndigoObject * clone ();
   virtual IndigoArray & asArray ();

   PtrArray<IndigoObject> objects;
};

class IndigoArrayElement : public IndigoObject
{
public:
   IndigoArrayElement (IndigoArray &arr, int idx_);
   virtual ~IndigoArrayElement ();

   DLLEXPORT IndigoObject & get ();

   virtual BaseMolecule & getBaseMolecule ();
   virtual Molecule & getMolecule ();
   virtual GraphHighlighting * getMoleculeHighlighting();

   virtual BaseReaction & getBaseReaction ();
   virtual Reaction & getReaction ();
   virtual ReactionHighlighting * getReactionHighlighting();

   virtual IndigoObject * clone ();

   virtual const char * getName ();

   virtual IndigoArray & asArray ();
   virtual IndigoFingerprint & asFingerprint ();

   virtual int getIndex ();

   IndigoArray *array;
   int idx;
};

class IndigoArrayIter : public IndigoObject
{
public:
   IndigoArrayIter (IndigoArray &arr);
   virtual ~IndigoArrayIter ();

   virtual IndigoObject * next ();
   virtual bool hasNext ();
protected:
   IndigoArray *_arr;
   int _idx;
};

#endif