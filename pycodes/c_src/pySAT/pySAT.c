
#include <strings.h>

#include "Python.h"
#include "misc.h"
#include "SatGraph.h"
#include "SatGraphAlgorithm.h"

/* ----------------------------------------------------------------------
 *
 * This file implements the interface between the rest of the C code
 * and python.  It provides the pySAT module which contains the
 * SatGraph class.
 *
 * ------------------------------------------------------------------- */


#define MODULE_NAME "pySAT"
#define INIT_FUNC_NAME initpySAT

#define MODULE__DOC__ "\n\n" \
"The " MODULE_NAME " module provides functions for solving circuit\n" \
"satisfiability problems using belief propagation like algorithms.\n"\
"To see a list of functions and classes type\n\n" \
"    print " MODULE_NAME ".__dict__.keys()\n\n" \
"To see documentation for a particular function or class, do\n\n" \
"    print <funcName>.__doc__\n\n" \
"\n"\

typedef struct {
  PyObject_HEAD
  PyObject	*x_attr;	/* Attributes dictionary */
  SatGraph*    theGraph;
} PythonSatGraph;

staticforward PyTypeObject SatGraphType;

#define is_PythonSatGraph(v)		((v)->ob_type == &SatGraphType)

static double* GetPyFloatListToArrayOfDoubles(const char*const argName,
					      const int requiredSize,
					      const double minVal,
					      const double maxVal,
					      PyObject* pyList) {
  int i, size;
  double* result;
  PyObject* item;

  assert(PyList_Check(pyList));
  size = PyList_GET_SIZE(pyList);
  if (requiredSize != size) {
    PyErr_Format(PyExc_TypeError,"%s:list size should be %i not %i",
		 argName,requiredSize,size);
    return NULL;
  }
  result = SafeCalloc(requiredSize,sizeof(double));
  for (i=0; i<requiredSize; i++) {
    item = PyList_GET_ITEM(pyList,i);
    if (PyInt_Check(item)) result[i] = PyInt_AsLong(item);
    else if (PyFloat_Check(item)) result[i]=PyFloat_AsDouble(item);
    else {
      PyErr_Format(PyExc_TypeError,"%s:element %i not an int or float",
		   argName,i);
      free(result);
      return NULL;
    }
    if (result[i] > maxVal || result[i] < minVal) {
      PyErr_Format(PyExc_TypeError,"%s:element %i not in required range",
		   argName,i);
      free(result);
      return NULL;
    }
  }
  return result;
}


static int* GetIntListToArray(PyObject* pyList, int* size,
			      const int minVal, const int maxVal) {
  int i;
  int* result;
  PyObject* item;

  assert(PyList_Check(pyList));
  *size = PyList_GET_SIZE(pyList);
  result = SafeCalloc(*size,sizeof(int));
  for (i=0; i<*size; i++) {
    item = PyList_GET_ITEM(pyList,i);
    if (! PyInt_Check(item)) {
      PyErr_Format(PyExc_TypeError,"element %i not an integer",i);
      free(result);
      return NULL;
    }
    result[i]=PyInt_AsLong(item);
    if (result[i] > maxVal || result[i] < minVal) {
      PyErr_Format(PyExc_TypeError,"element %i = %i not in [%i,%i]",
		   i,result[i],minVal,maxVal);
      free(result);
      return NULL;
    }
  }
  return result;
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	GetListOfIntListsToArray
//
// INPUTS:	pyList:        A Python list object.
//              numSubLists:   *numSubLists is set to PyList_Size(pyList).
//              subListSizes:  *subListSizes is set to a newly allocated
//                             array of length *numSubLists where 
//                             *subListSizes[i] gets the length of the 
//                             ith subList.
//              minVal:        Minimum allowed value for element.
//              maxVal:        Maximum allowed value for element.
//
// RETURNS:	Returns a newly allocated array of arrays containing the
//              list of lists of integers in pyList.  Both the result
//              returned and result[i] from 0 to *numSubLists - 1 need
//              to be deallocated by the caller.
//
//              If an error occurs, NULL is returned and no deallocation
//              of anything is required.
//
// MODIFIED:	Fri Jun 06, 2003
//              Mon Jun 30, 2003: added minVal, maxVal args (EM).
//
// -------------------------------------------------------------------- */

static int** GetListOfIntListsToArray(PyObject* pyList, int* numSubLists,
				      int** subListSizes,
				      const int minVal, const int maxVal) {
  PyObject* subList;
  int listIndex, listSize, itemToFree;
  int **result;

  listSize = PyList_Size(pyList);
  if ( (*numSubLists != -1) && (*numSubLists != listSize) ) {
    PyErr_Format(PyExc_TypeError,"%s%i%s%i",
		 "expected list of size ", *numSubLists, 
		 ", but got list of size ", listSize);
    return NULL;
  }
  *numSubLists = listSize;
  result = SafeCalloc(listSize,sizeof(int*));
  *subListSizes = SafeCalloc(listSize,sizeof(int*));
  
  for (listIndex=0; listIndex<listSize; listIndex++) {
    subList=PyList_GET_ITEM(pyList,listIndex);
    if (! PyList_Check(subList)) {
      PyErr_Format(PyExc_TypeError,"%s%i%s",
		   "sub list ",listIndex,", was not a valid list.");
      goto cleanup_after_error;
    }
    result[listIndex]=GetIntListToArray(subList,&((*subListSizes)[listIndex]),
					minVal,maxVal);
    if (NULL == result[listIndex]) goto cleanup_after_error;
  }

  return result;

 cleanup_after_error:
  assert(listIndex <= listSize);
  listIndex--;
  for (itemToFree=0; itemToFree<listIndex; itemToFree++) 
    free(result[itemToFree]);
  free(result);
  free(*subListSizes);
  return NULL;
}

static PythonSatGraph* MakeSatGraphFromPythonSparse(const int N, /* num Vars */
						    const int M, /* clauses */
						    const int E, /* num Edges*/
						    PyObject* eList){
  int** linkArrays;
  int* linkArrayLengths;
  PythonSatGraph* result;
  int j;
  int listSize = M;

  linkArrays = GetListOfIntListsToArray(eList,&listSize,&linkArrayLengths,
					-N,N);
  if (NULL == linkArrays) return NULL;

  result = PyObject_NEW(PythonSatGraph, &SatGraphType);
  if (result != NULL) {
    result->x_attr = NULL;
    result->theGraph = CreateSatGraphFromLinkArrays(N,M,E,linkArrayLengths,
						    linkArrays);
  }

  for (j=0; j<M; j++) {
    free(linkArrays[j]);
  }
  free(linkArrayLengths); free(linkArrays);
  return result;
}


#define SATGRAPH_NEW__DOC__ "SatGraph(N,M,G,E):\n" \
"Allocates a new SatGraph object based on the following arguments\n" \
"N = number of variables.\n" \
"M = number of clauses.\n" \
"G = List of lists representing connections between variables and clauses.\n" \
"    Specifically, The jth variable in the ith clause is abs(G[i][j]) and\n" \
"    this variable is complemented if G[i][j] < 0.\n" \
"E = number of edges in the graph G.  For standard 3-Sat this will be\n" \
"    3 * M, but in general it can be obtained via\n" \
"    reduce(lambda x,y: x+y, map(len, G))\n" \
"For example, the set of three clauses with 4 variables shown below\n\n" \
"   (x1 \\/ x2 \\/ !x3) /\\ (x1 \\/ !x2 \\/ x4) /\\ (x2 \\/ x3 \\/ !x4)\n\n" \
"would be created using the command\n\n" \
"   SatGraph(5,3,[ [1, 2, -3], [1, -2, 4], [2, 3, -5] ],9)\n\n" \
"Note that variable index starts at 1 not 0 so when you specify that\n"\
"there are 5 variables and 3 clauses in the example above, the G argument\n"\
"has indexes in the set {1,2,3,4,5} not {0,1,2,3,4}.\n"


static PythonSatGraph*
PythonSatGraph_new(PythonSatGraph* self, PyObject *args, PyObject* keywds) {
  int N, M, E;
  PyObject* eList = NULL;
  static char* kwlist[] = {"N","M","G","E",NULL};

  if (! PyArg_ParseTupleAndKeywords(args,keywds,"iiO!i",kwlist,
      &N,&M,&PyList_Type,&eList,&E)) {
    Py_INCREF(Py_None);
    return NULL;
  }

  return MakeSatGraphFromPythonSparse(N,M,E,eList);
}

static void
PythonSatGraph_dealloc(PythonSatGraph *xp)
{
	DestroySatGraph(xp->theGraph);
	free(xp->theGraph);
	Py_XDECREF(xp->x_attr);
	PyObject_DEL(xp);
}

#define SATGRAPH_SOLVE__DOC__ "solve(verbose=0):\n" \
"Runs a single iteration of the solve algorithm selected when\n"\
"the setalg method was called.  If the optional argument\n"\
"verbose is true, diagnostic messages are printed.\n"

static PyObject *
PythonSatGraph_solve(PythonSatGraph *self, PyObject *args, PyObject* keywds)
{
  char* result;
  PyObject* pyResult;
  int verbose = 0;
  static char* kwlist[] = {"verbose",NULL};

  if (!PyArg_ParseTupleAndKeywords(args,keywds,"|i",kwlist,
				   &verbose))
    return NULL;
  result = self->theGraph->algorithm->DoIteration(self->theGraph,verbose);
  pyResult = Py_BuildValue("s",result);
  if (result) free(result);
  return pyResult;
}

static PyObject *
GenericGetBeliefs(PyObject *args, PyObject* keywds, const int numVNodes, 
		  void*graph,
		  double*(GetBelFunc)(void*graph,const int verbose)){
  int i, retVal;
  double* ev;
  PyObject* resultList;
  int verbose = 0;
  static char* kwlist[] = {"verbose",NULL};

  if (!PyArg_ParseTupleAndKeywords(args,keywds,"|i",kwlist,&verbose))
    return NULL;

  resultList = PyList_New(0);
  assert(resultList);
  
  ev = GetBelFunc(graph,verbose);

  for (i = 0; i < numVNodes; i++) {
    if (isnan(ev[i])) retVal = PyList_Append(resultList, Py_None);
    else retVal = PyList_Append(resultList, PyFloat_FromDouble(ev[i]));
    assert(retVal == 0);
  }
  free(ev);
  return resultList;
}

static double* GetBeliefsForSatGraph(void* graphPtr,const int verbose) {
  SatGraph*const theGraph = graphPtr;
  return (double*) theGraph->algorithm->GetBeliefs(theGraph,verbose);
}

#define GETBELIEFS__DOC__ "getbeliefs():\n" \
"Returns a list of floats representing the belief that each variable = 1.\n"

static PyObject *
PythonSatGraph_getbeliefs(PythonSatGraph *self, PyObject *args,
			  PyObject*keywds) {
  return GenericGetBeliefs(args,keywds,self->theGraph->numVNodes,
			   self->theGraph, GetBeliefsForSatGraph);
}

static int SetAlgorithmForSatGraph(SatGraph*const sg,
				   const int verbose, const char* algStr) {
  int i;
  char* algChoices;
  SatGraphAlgorithm* alg = NULL;
  
  if (NULL == algStr) algStr = "default";
  for (i=0; NULL != SatGraphAlgorithmNames[i]; i++) 
    if (0 == strcmp(algStr,SatGraphAlgorithmNames[i])) 
      alg = MakeSGAlgInstance(SatGraphAlgorithms[i]);
  
  if (NULL == alg) {
    algChoices = CatStrArray(SatGraphAlgorithmNames,", ");
    PyErr_Format(PyExc_TypeError,
		 "No algorithm named '%s' exists; options = [%s].",
		 algStr,algChoices);
    free(algChoices);
    return 0;
  }
    
  if (NULL != sg->algorithm) 
    sg->algorithm->DeallocateAlgorithm(sg,verbose);
  sg->algorithm = alg;
  return 1;
}

#define SATGRAPH_SET_ALG__DOC__ "setalg(alg='default',verbose=0):\n" \
"alg:      The (optional) name of an algorithm you plan to use.\n"\
"          Options include ['default'].\n"\
"verbose:  Indicates whether to print diagnostic messages.\n"

static PyObject *
PythonSatGraph_setalg(PythonSatGraph *self, PyObject *args,
		      PyObject* keywds) {
  int verbose = 0;
  char* algStr = NULL;
  static char* kwlist[] = {"alg","verbose",NULL};

  if ( (! PyArg_ParseTupleAndKeywords(args,keywds,"|si",kwlist,
				      &algStr,&verbose))||
       (! SetAlgorithmForSatGraph(self->theGraph,verbose,algStr)) ) {
    self->theGraph->algorithm = NULL;
    return NULL;
  }
  self->theGraph->algorithm->InitializeAlgorithm(self->theGraph,verbose);

  Py_INCREF(Py_None);
  return Py_None;
}


static PyMethodDef module_methods[] = {
  {"SatGraph",(PyCFunction)PythonSatGraph_new,
   METH_VARARGS|METH_KEYWORDS, SATGRAPH_NEW__DOC__},
  {NULL,		NULL}		/* sentinel */
};

static PyMethodDef PythonSatGraph_methods[] = {
  {"solve",	(PyCFunction)PythonSatGraph_solve,
   METH_VARARGS|METH_KEYWORDS, SATGRAPH_SOLVE__DOC__},
  {"getbeliefs",	(PyCFunction)PythonSatGraph_getbeliefs,
   METH_VARARGS|METH_KEYWORDS, GETBELIEFS__DOC__},
  {"setalg",	(PyCFunction)PythonSatGraph_setalg,
   METH_VARARGS|METH_KEYWORDS, SATGRAPH_SET_ALG__DOC__},
  {NULL,		NULL}		/* sentinel */
};

static PyObject *
PythonSatGraph_getattr(PythonSatGraph *xp, char *name)
{
	if (xp->x_attr != NULL) {
		PyObject *v = PyDict_GetItemString(xp->x_attr, name);
		if (v != NULL) {
			Py_INCREF(v);
			return v;
		}
	}
	return Py_FindMethod(PythonSatGraph_methods, (PyObject *)xp, name);
}

static int PythonSatGraph_setattr(PythonSatGraph *xp, char *name, PyObject *v)
{
	if (xp->x_attr == NULL) {
		xp->x_attr = PyDict_New();
		if (xp->x_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(xp->x_attr, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
                                        "delete non-existing LDPCCode attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(xp->x_attr, name, v);
}

static PyTypeObject SatGraphType = {
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"SatGraph",			/*tp_name*/
	sizeof(PythonSatGraph),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)PythonSatGraph_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)PythonSatGraph_getattr, /*tp_getattr*/
	(setattrfunc)PythonSatGraph_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

DL_EXPORT(void)
     INIT_FUNC_NAME (void)
{
  SatGraphType.ob_type = &PyType_Type;

  Py_InitModule3(MODULE_NAME, module_methods,MODULE__DOC__);
}

