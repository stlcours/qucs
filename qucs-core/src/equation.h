/*
 * equation.h - checker definitions for Qucs equations
 *
 * Copyright (C) 2004, 2005 Stefan Jahn <stefan@lkcc.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 *
 * $Id: equation.h,v 1.20 2005-05-03 17:57:35 raimi Exp $
 *
 */

#ifndef __EQUATION_H__
#define __EQUATION_H__

#include "object.h"
#include "complex.h"
#include "vector.h"
#include "matrix.h"
#include "matvec.h"
#include "evaluate.h"

class strlist;
class dataset;

namespace eqn {

class solver;
class checker;
class constant;
class reference;
class assignment;
class application;

enum NodeTag {
  UNKNOWN = -1,
  CONSTANT = 0, /* source code constant */
  REFERENCE,    /* variable reference   */
  APPLICATION,  /* call to function     */
  ASSIGNMENT    /* root of equation     */
};

/* The equation node base class defines and implements the basic
   functionality of an equation node.  Possible types of nodes are
   listed in 'NodeTag'. */
class node
{
public:
  node ();
  node (int);
  virtual ~node ();
  node * getNext (void) { return next; }
  void setNext (node * n) { next = n; }
  int count (void);
  void append (node *);
  void setDependencies (strlist *);
  strlist * getDependencies (void);
  void setDataDependencies (strlist *);
  strlist * getDataDependencies (void) { return dataDependencies; }
  void setDropDependencies (strlist * deps) { dropDependencies = deps; }
  void addDropDependencies (char *);
  strlist * getDropDependencies (void) { return dropDependencies; }
  void setPrepDependencies (strlist * deps) { prepDependencies = deps; }
  void addPrepDependencies (char *);
  strlist * getPrepDependencies (void) { return prepDependencies; }
  strlist * recurseDependencies (checker *, strlist *);
  node * get (int);
  constant * getResult (int);
  int getType (void) { return type; }
  int getTag (void) { return tag; }
  void setType (int tag) { type = tag; }
  constant * getResult (void) { return res; } 
  void setResult (constant * r) { res = r; }
  char * getInstance (void);
  void setInstance (char *);
  void applyInstance (void);
  constant * calculate (void);    

  /* These functions should be overloaded by derivative classes. */
  virtual void print (void) { }
  virtual void addDependencies (strlist *) { }
  virtual int evalType (void) { return type; }
  virtual char * toString (void) { return txt; }
  virtual constant * evaluate (void) { return res; }
  
public:
  int duplicate;
  int cycle;
  int evalPossible;
  char * txt;
  int evaluated;
  char * instance;
  int output;
  int dropdeps;
  solver * solvee;

private:
  int type;
  int tag;
  node * next;
  strlist * dependencies;
  constant * res;
  strlist * dataDependencies;
  strlist * dropDependencies;
  strlist * prepDependencies;
};

enum ConstantTag {
  TAG_UNKNOWN =   0,
  TAG_DOUBLE  =   1, /* double constant          */
  TAG_COMPLEX =   2, /* complex value            */
  TAG_VECTOR  =   4, /* list of complex values   */
  TAG_MATRIX  =   8, /* complex matrix           */
  TAG_MATVEC  =  16, /* list of complex matrices */
  TAG_CHAR    =  32, /* single character         */
  TAG_STRING  =  64, /* character string         */
  TAG_RANGE   = 128, /* interval specification   */
};

/* This class represents any type of constant expression. */
class constant : public node
{
public:
  constant ();
  constant (int);
  ~constant ();
  void print (void);
  int evalType (void);
  char * toString (void);
  constant * evaluate (void);

public:
  int type;
  union {
    nr_double_t d;
    complex * c;
    vector * v;
    matrix * m;
    matvec * mv;
    char chr;
    char * s;
  };
};

/* The class represents variable references. */
class reference : public node
{
public:
  reference ();
  ~reference ();
  void print (void);
  void addDependencies (strlist *);
  int evalType (void);
  char * toString (void);
  constant * evaluate (void);

public:
  char * n;
  node * ref;
};

/* This class represents assignments with a left hand and right hand
   side. */
class assignment : public node
{
public:
  assignment ();
  ~assignment ();
  void print (void);
  void addDependencies (strlist *);
  int evalType (void);
  char * toString (void);
  constant * evaluate (void);
  
public:
  char * result;
  node * body;
};

/* The application class represents any kind of operation (unary,
   binary and n-ary ones) containing the appropriate argument list. */
class application : public node
{
public:
  application ();
  ~application ();
  void print (void);
  void addDependencies (strlist *);
  int evalType (void);
  char * toString (void);
  constant * evaluate (void);

public:
  char * n;
  int nargs;
  node * args;
  evaluator_t eval;
};

/* This class implements the actual functionality regarding a set of
   equations stored within a netlist. */
class checker
{
public:
  checker ();
  ~checker ();
  void collectDependencies (void);
  void setEquations (node * eqn) { equations = eqn; }
  node * getEquations (void) { return equations; }
  void list (void);
  int findUndefined (int);
  strlist * getVariables (void);
  int findDuplicate (void);
  static node * findEquation (node *, char *);
  int detectCycles (void);
  static strlist * foldDependencies (strlist *);
  node * appendEquation (node *, node *);
  void dropEquation (node *);
  void reorderEquations (void);
  node * lastEquation (node *);
  int applyTypes (void);
  int checkExport (void);

public:
  node * equations;
};

/* The solver class is finally used to solve the list of equations. */
class solver
{
public:
  solver ();
  ~solver ();
  void setEquations (node * eqn) { equations = eqn; }
  node * getEquations (void) { return equations; }
  void setData (dataset * d) { data = d; }
  dataset * getDataset (void) { return data; }
  void solve (void);
  node * addEquationData (vector *);
  node * addEquationData (matvec *);
  node * addGeneratedEquation (vector *, char *);
  vector * dataVector (node *);
  void checkinDataset (void);
  void checkoutDataset (void);
  static int dataSize (constant *);
  static int getDependencySize (strlist *, int);
  static int getDataSize (char *);
  static strlist * collectDataDependencies (node *);
  int dataSize (strlist *);
  void findMatrixVectors (vector *);
  char * isMatrixVector (char *, int&, int&);
  int findEquationResult (node *);

public:
  node * equations;

private:
  dataset * data;
  int generated;
};

/* The global list of equations and expression lists. */
extern node * equations;
extern node * expressions;

} /* namespace */

__BEGIN_DECLS

/* Available functions of the equation checker. */
int equation_checker (int);
int equation_solver (dataset *);
strlist * equation_variables (void);
void equation_constants (void);

__END_DECLS

#endif /* __EQUATION_H__ */
