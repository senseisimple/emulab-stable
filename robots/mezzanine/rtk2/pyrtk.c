/****************************************************************************
 * Desc: Python bindings for Rtk library
 * Author: Andrew Howard
 * Date : 3 Sep 2001
 *
 * $Source: /home/cvs_mirrors/cvs-public.flux.utah.edu/CVS/testbed/robots/mezzanine/rtk2/pyrtk.c,v $
 * $Author: johnsond $
 * $Revision: 1.1 $
 *
 ****************************************************************************/

#include "Python.h"
#include "rtk.h"


/* Index of our info in the TLS area of the thread
   Needed for correct storing and restoring of the Python interpreter state
   in multi-threaded programs.
 */
static pthread_key_t thread_key;


/* Acquire lock and set python state for current thread
 */
void thread_acquire()
{
    PyThreadState *state;
    
    state = (PyThreadState*) pthread_getspecific(thread_key);
    PyEval_AcquireLock();
    PyThreadState_Swap(state);
}


/* Release lock and set python state to NULL
 */
void thread_release()
{
    PyThreadState *state;

    state = PyThreadState_Swap(NULL);
    if (thread_key == 0)
        pthread_key_create(&thread_key, NULL);
    pthread_setspecific(thread_key, state);
    PyEval_ReleaseLock();
}


/****************************************************************************
 * App object
 ****************************************************************************/

typedef struct
{
    PyObject_HEAD
    rtk_app_t *app;
} pyrtk_app_t;


staticforward PyTypeObject pyrtk_app_type;
staticforward PyMethodDef pyrtk_app_methods[];


/* Create app
 */
static PyObject *pyrtk_app(PyObject *self, PyObject *args)
{
    pyrtk_app_t *appob;
    
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    appob = PyObject_New(pyrtk_app_t, &pyrtk_app_type);
    appob->app = rtk_app_create();
    
    return (PyObject*) appob;
}


/* Destroy app
 */
static void pyrtk_app_del(PyObject *self)
{
    pyrtk_app_t *appob;

    appob = (pyrtk_app_t*) self;
    rtk_app_destroy(appob->app);
    PyObject_Del(self);
}


/* Get app attributes
 */
static PyObject *pyrtk_app_getattr(PyObject *self, char *attrname)
{
    PyObject *result;
    pyrtk_app_t *appob;

    appob = (pyrtk_app_t*) self;
    result = Py_FindMethod(pyrtk_app_methods, self, attrname);

    return result;
}


/* Set app size
 */
static PyObject *pyrtk_app_size(PyObject *self, PyObject *args)
{
    pyrtk_app_t *appob;
    double w, h;

    appob = (pyrtk_app_t*) self;
    if (!PyArg_ParseTuple(args, "dd", &w, &h))
        return NULL;

    rtk_app_size(appob->app, w, h);

    Py_INCREF(Py_None);
    return Py_None;
}


/* Set app refresh rate
   (app, rate)
 */
static PyObject *pyrtk_app_refresh_rate(PyObject *self, PyObject *args)
{
    pyrtk_app_t *appob;
    int rate;

    appob = (pyrtk_app_t*) self;
    if (!PyArg_ParseTuple(args, "i", &rate))
        return NULL;

    rtk_app_refresh_rate(appob->app, rate);

    Py_INCREF(Py_None);
    return Py_None;
}


/* Start the app
 */
static PyObject *pyrtk_app_start(PyObject *self, PyObject *args)
{
    pyrtk_app_t *appob;

    appob = (pyrtk_app_t*) self;
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    rtk_app_start(appob->app);

    Py_INCREF(Py_None);
    return Py_None;
}


/* Stop the app
 */
static PyObject *pyrtk_app_stop(PyObject *self, PyObject *args)
{
    pyrtk_app_t *appob;

    appob = (pyrtk_app_t*) self;
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    rtk_app_stop(appob->app);

    Py_INCREF(Py_None);
    return Py_None;
}


/* Run app in calling thread.
 */
static PyObject *pyrtk_app_main(PyObject *self, PyObject *args)
{
    pyrtk_app_t *appob;

    appob = (pyrtk_app_t*) self;
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    thread_release();
    rtk_app_main(appob->app);
    thread_acquire();

    Py_INCREF(Py_None);
    return Py_None;
}


static PyTypeObject pyrtk_app_type = 
{
    PyObject_HEAD_INIT(NULL)
    0,
    "app",
    sizeof(pyrtk_app_t),
    0,
    pyrtk_app_del, /*tp_dealloc*/
    0,          /*tp_print*/
    pyrtk_app_getattr, /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */
};


static PyMethodDef pyrtk_app_methods[] =
{
    {"size", pyrtk_app_size, METH_VARARGS},
    {"refresh_rate", pyrtk_app_refresh_rate, METH_VARARGS},
    {"start", pyrtk_app_start, METH_VARARGS},
    {"stop", pyrtk_app_stop, METH_VARARGS},
    {"main", pyrtk_app_main, METH_VARARGS},
    {NULL, NULL}
};


/****************************************************************************
 * Canvas object
 ****************************************************************************/


typedef struct
{
    PyObject_HEAD
    rtk_canvas_t *canvas;
} pyrtk_canvas_t;


staticforward PyTypeObject pyrtk_canvas_type;
staticforward PyMethodDef pyrtk_canvas_methods[];


/* Create canvas
 */
static PyObject *pyrtk_canvas(PyObject *self, PyObject *args)
{
    pyrtk_app_t *appob;
    pyrtk_canvas_t *canvasob;
    
    if (!PyArg_ParseTuple(args, "O", &appob))
        return NULL;

    canvasob = PyObject_New(pyrtk_canvas_t, &pyrtk_canvas_type);
    canvasob->canvas = rtk_canvas_create(appob->app);
    
    return (PyObject*) canvasob;
}


/* Destroy canvas
 */
static void pyrtk_canvas_del(PyObject *self)
{
    pyrtk_canvas_t *canvasob;

    canvasob = (pyrtk_canvas_t*) self;
    rtk_canvas_destroy(canvasob->canvas);
    PyObject_Del(self);
}


/* Get canvas attributes
 */
static PyObject *pyrtk_canvas_getattr(PyObject *self, char *attrname)
{
    PyObject *result;
    pyrtk_canvas_t *canvasob;

    canvasob = (pyrtk_canvas_t*) self;
    result = Py_FindMethod(pyrtk_canvas_methods, self, attrname);

    return result;
}


/* Set the canvas size
 * (canvas, sizex, sizey)
 */
static PyObject *pyrtk_canvas_size(PyObject *self, PyObject *args)
{
    pyrtk_canvas_t *canvasob;
    int sizex, sizey;

    canvasob = (pyrtk_canvas_t*) self;
    if (!PyArg_ParseTuple(args, "ii", &sizex, &sizey))
        return NULL;

    rtk_canvas_size(canvasob->canvas, sizex, sizey);
    
    Py_INCREF(Py_None);
    return Py_None;
}


/* Set the canvas origin
 * (canvas, ox, oy)
 */
static PyObject *pyrtk_canvas_origin(PyObject *self, PyObject *args)
{
    pyrtk_canvas_t *canvasob;
    double ox, oy;

    canvasob = (pyrtk_canvas_t*) self;
    if (!PyArg_ParseTuple(args, "dd", &ox, &oy))
        return NULL;

    rtk_canvas_origin(canvasob->canvas, ox, oy);
    
    Py_INCREF(Py_None);
    return Py_None;
}


/* Scale a canvas
 * (canvas, scale_x, scale_y) where scale is the logical size of each pixel.
 */
static PyObject *pyrtk_canvas_scale(PyObject *self, PyObject *args)
{
    pyrtk_canvas_t *canvasob;
    double sx, sy;

    canvasob = (pyrtk_canvas_t*) self;
    if (!PyArg_ParseTuple(args, "dd", &sx, &sy))
        return NULL;

    rtk_canvas_scale(canvasob->canvas, sx, sy);
    
    Py_INCREF(Py_None);
    return Py_None;
}


/* Set the default font
 * Args: fontname
 * Returns: None
 */
static PyObject *pyrtk_canvas_font(PyObject *self, PyObject *args)
{
    pyrtk_canvas_t *canvasob;
    char *fontname;

    canvasob = (pyrtk_canvas_t*) self;
    if (!PyArg_ParseTuple(args, "s", &fontname))
        return NULL;

    rtk_canvas_font(canvasob->canvas, fontname);
    
    Py_INCREF(Py_None);
    return Py_None;
}

/* Export a canvas to a vector file
 * Args: (filaname)
 */
static PyObject *pyrtk_canvas_export(PyObject *self, PyObject *args)
{
    pyrtk_canvas_t *canvasob;
    char *filename;

    canvasob = (pyrtk_canvas_t*) self;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    rtk_canvas_export(canvasob->canvas, filename);
    
    Py_INCREF(Py_None);
    return Py_None;
}


/* Export a canvas to a bitmap image
 * Args: (filename)
 */
static PyObject *pyrtk_canvas_export_image(PyObject *self, PyObject *args)
{
    pyrtk_canvas_t *canvasob;
    char *filename;

    canvasob = (pyrtk_canvas_t*) self;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    rtk_canvas_export_image(canvasob->canvas, filename);
    
    Py_INCREF(Py_None);
    return Py_None;
}


static PyTypeObject pyrtk_canvas_type = 
{
    PyObject_HEAD_INIT(NULL)
    0,
    "canvas",
    sizeof(pyrtk_canvas_t),
    0,
    pyrtk_canvas_del, /*tp_dealloc*/
    0,          /*tp_print*/
    pyrtk_canvas_getattr, /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */
};


static PyMethodDef pyrtk_canvas_methods[] =
{
    {"size", pyrtk_canvas_size, METH_VARARGS},
    {"origin", pyrtk_canvas_origin, METH_VARARGS},
    {"scale", pyrtk_canvas_scale, METH_VARARGS},
    {"font", pyrtk_canvas_font, METH_VARARGS},
    {"export", pyrtk_canvas_export, METH_VARARGS},
    {"export_image", pyrtk_canvas_export_image, METH_VARARGS},
    {NULL, NULL}
};


/****************************************************************************
 * Figure object
 ****************************************************************************/


typedef struct
{
    PyObject_HEAD
    rtk_fig_t *fig;
} pyrtk_fig_t;


staticforward PyTypeObject pyrtk_fig_type;
staticforward PyMethodDef pyrtk_fig_methods[];


/* Create fig
 */
static PyObject *pyrtk_fig(PyObject *self, PyObject *args)
{
    pyrtk_canvas_t *canvasob;
    pyrtk_fig_t *figob, *parentob;
    int layer;
    
    if (!PyArg_ParseTuple(args, "OOi", &canvasob, &parentob, &layer))
        return NULL;

    figob = PyObject_New(pyrtk_fig_t, &pyrtk_fig_type);

    if ((PyObject*) parentob == Py_None)
        figob->fig = rtk_fig_create(canvasob->canvas, NULL, layer);
    else
        figob->fig = rtk_fig_create(canvasob->canvas, parentob->fig, layer);
    
    return (PyObject*) figob;
}


/* Destroy fig
 */
static void pyrtk_fig_del(PyObject *self)
{
    pyrtk_fig_t *figob;

    figob = (pyrtk_fig_t*) self;
    rtk_fig_destroy(figob->fig);
    PyObject_Del(self);
}


/* Get fig attributes
 */
static PyObject *pyrtk_fig_getattr(PyObject *self, char *attrname)
{
    PyObject *result;
    pyrtk_fig_t *figob;

    figob = (pyrtk_fig_t*) self;

    /* Return a pointer to the underlying C object */
    if (strcmp(attrname, "cptr") == 0)
        result = PyCObject_FromVoidPtr(figob->fig, NULL);
    else
        result = Py_FindMethod(pyrtk_fig_methods, self, attrname);

    return result;
}


/* Clear figure
 */
static PyObject *pyrtk_fig_clear(PyObject *self, PyObject *args)
{
    pyrtk_fig_t *figob;

    figob = (pyrtk_fig_t*) self;
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    rtk_fig_clear(figob->fig);

    Py_INCREF(Py_None);
    return Py_None;    
}


/* Set origin
 */
static PyObject *pyrtk_fig_origin(PyObject *self, PyObject *args)
{
    pyrtk_fig_t *figob;
    double ox, oy, oa;

    figob = (pyrtk_fig_t*) self;
    if (!PyArg_ParseTuple(args, "ddd", &ox, &oy, &oa))
        return NULL;

    rtk_fig_origin(figob->fig, ox, oy, oa);

    Py_INCREF(Py_None);
    return Py_None;    
}


/* Set the color
   (fig, r, g, b)
 */
static PyObject *pyrtk_fig_color(PyObject *self, PyObject *args)
{
    pyrtk_fig_t *figob;
    double r, g, b;

    figob = (pyrtk_fig_t*) self;
    if (!PyArg_ParseTuple(args, "ddd", &r, &g, &b))
        return NULL;

    rtk_fig_color(figob->fig, r, g, b);

    Py_INCREF(Py_None);
    return Py_None;    
}


/* Create a line
 */
static PyObject *pyrtk_fig_line(PyObject *self, PyObject *args)
{
    pyrtk_fig_t *figob; 
    double ax, ay, bx, by;

    figob = (pyrtk_fig_t*) self;
    if (!PyArg_ParseTuple(args, "dddd", &ax, &ay, &bx, &by))
        return NULL;

    rtk_fig_line(figob->fig, ax, ay, bx, by);

    Py_INCREF(Py_None);
    return Py_None;    
}


/* Create a rectangle
 */
static PyObject *pyrtk_fig_rectangle(PyObject *self, PyObject *args)
{
    pyrtk_fig_t *figob;
    double ox, oy, oa, sx, sy;
    int filled;

    figob = (pyrtk_fig_t*) self;
    if (!PyArg_ParseTuple(args, "dddddi", &ox, &oy, &oa,
                          &sx, &sy, &filled))
        return NULL;

    rtk_fig_rectangle(figob->fig, ox, oy, oa, sx, sy, filled);

    Py_INCREF(Py_None);
    return Py_None;    
}


/* Create an ellipse
 */
static PyObject *pyrtk_fig_ellipse(PyObject *self, PyObject *args)
{
    pyrtk_fig_t *figob;
    double ox, oy, oa, sx, sy;
    int filled;

    figob = (pyrtk_fig_t*) self;    
    if (!PyArg_ParseTuple(args, "dddddi", &ox, &oy, &oa, &sx, &sy, &filled))
        return NULL;

    rtk_fig_ellipse(figob->fig, ox, oy, oa, sx, sy, filled);

    Py_INCREF(Py_None);
    return Py_None;    
}


/* Create an arrow
 */
static PyObject *pyrtk_fig_arrow(PyObject *self, PyObject *args)
{
    pyrtk_fig_t *figob;
    double ax, ay, aa, len, head;

    figob = (pyrtk_fig_t*) self;
    if (!PyArg_ParseTuple(args, "ddddd", &ax, &ay, &aa, &len, &head))
        return NULL;

    rtk_fig_arrow(figob->fig, ax, ay, aa, len, head);

    Py_INCREF(Py_None);
    return Py_None;    
}


/* Create an arrow
 */
static PyObject *pyrtk_fig_arrow_ex(PyObject *self, PyObject *args)
{
    pyrtk_fig_t *figob;
    double ax, ay, bx, by, head;

    figob = (pyrtk_fig_t*) self;
    if (!PyArg_ParseTuple(args, "ddddd", &ax, &ay, &bx, &by, &head))
        return NULL;

    rtk_fig_arrow_ex(figob->fig, ax, ay, bx, by, head);

    Py_INCREF(Py_None);
    return Py_None;    
}


/* Create some text
   Args: (ox, oy, oa, text)
   where (ox, oy, oa) is the origin of the text and
   (text) is the text.
 */
static PyObject *pyrtk_fig_text(PyObject *self, PyObject *args)
{
    pyrtk_fig_t *figob;
    double ox, oy, oa;
    char *text;

    figob = (pyrtk_fig_t*) self;
    if (!PyArg_ParseTuple(args, "ddds", &ox, &oy, &oa, &text))
        return NULL;

    rtk_fig_text(figob->fig, ox, oy, oa, text);

    Py_INCREF(Py_None);
    return Py_None;    
}


/* Create a laser scan
   Args: (ox, oy, oa, scan_data)
   where (ox, oy, oa) is the origin of the scan and
   scan_data is a list containing (range, bearing, px, py, intensity) tuples.
 */
static PyObject *pyrtk_fig_scan(PyObject *self, PyObject *args)
{
  pyrtk_fig_t *figob;
  double ax, ay, aa;
  PyObject *scan_object;
  PyObject *tuple;
  int i;
  double r, b;
  double px, py, pr;
  double maxrange;

  maxrange = 7.9;
  
  figob = (pyrtk_fig_t*) self;
  if (!PyArg_ParseTuple(args, "dddO", &ax, &ay, &aa, &scan_object))
    return NULL;
  if (!PyList_Check(scan_object))
    /* ONERROR("expecting list as argument");*/
    return NULL;

  for (i = 0; i < PyList_Size(scan_object); i++)
  {
    tuple = PyList_GetItem(scan_object, i);
    if (!PyTuple_Check(tuple))
      return NULL;
    if (PyTuple_Size(tuple) < 2)
      return NULL;
      
    r = PyFloat_AsDouble(PyTuple_GetItem(tuple, 0));
    b = PyFloat_AsDouble(PyTuple_GetItem(tuple, 1));

    if (r > maxrange)
      continue;
        
    px = ax + r * cos(b + aa);
    py = ay + r * sin(b + aa);
    pr = 0.05;

    rtk_fig_rectangle(figob->fig, px, py, 0, pr, pr, 0);
  }
    
  Py_INCREF(Py_None);
  return Py_None;   
}


static PyTypeObject pyrtk_fig_type = 
{
    PyObject_HEAD_INIT(NULL)
    0,
    "fig",
    sizeof(pyrtk_fig_t),
    0,
    pyrtk_fig_del, /*tp_dealloc*/
    0,          /*tp_print*/
    pyrtk_fig_getattr, /*tp_getattr*/
    0,          /*tp_setattr*/
    0,          /*tp_compare*/
    0,          /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */
};


static PyMethodDef pyrtk_fig_methods[] =
{
    {"clear", pyrtk_fig_clear, METH_VARARGS},
    {"origin", pyrtk_fig_origin, METH_VARARGS},
    {"color", pyrtk_fig_color, METH_VARARGS},
    {"line", pyrtk_fig_line, METH_VARARGS},
    {"rectangle", pyrtk_fig_rectangle, METH_VARARGS},
    {"ellipse", pyrtk_fig_ellipse, METH_VARARGS},
    {"arrow", pyrtk_fig_arrow, METH_VARARGS},
    {"arrow_ex", pyrtk_fig_arrow_ex, METH_VARARGS},
    {"text", pyrtk_fig_text, METH_VARARGS},
    {"scan", pyrtk_fig_scan, METH_VARARGS},
    {NULL, NULL}
};


/****************************************************************************
 * Module initialisation
 ****************************************************************************/


/* Method table
 */
static struct PyMethodDef methods[] =
{
    {"app", pyrtk_app, METH_VARARGS},
    {"canvas", pyrtk_canvas, METH_VARARGS},
    {"fig", pyrtk_fig, METH_VARARGS},
    {NULL, NULL}
};


/* Module initialisation
 */
void initrtk(void)
{
    /* Finish initialise of type objects here */
    pyrtk_app_type.ob_type = &PyType_Type;
    pyrtk_canvas_type.ob_type = &PyType_Type;
    pyrtk_fig_type.ob_type = &PyType_Type;
    
    /* Do the standard module initialisation */
    Py_InitModule("rtk", methods);

    /* Make sure an interpreter lock object is created, since
       we will may try to release it at some point.
    */
    PyEval_InitThreads();
}




