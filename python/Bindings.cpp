#include <Python.h>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <thread>

#include "uWS.h"


using namespace std;

//#define debug(...) printf(__VA_ARGS__)
#define debug(...)

/** Common Exception class **/
static PyObject * uWebSockets_error __attribute__ ((unused));

//TODO: Remove, testing bindings work
static PyObject * uWebSockets_hello(PyObject * self, PyObject * args) {
    debug("Hello, world\n");
    Py_RETURN_NONE;
}

/**
 * Websocket extension
 */
template <bool isServer>
class WebSocket
{
    private:
        WebSocket() {
            // CPython cannot bind to constructors
            throw new runtime_error("Constructor cannot be called");
        }
        virtual ~WebSocket() {
            // CPython cannot bind to destructors
            throw new runtime_error("Destructor cannot be called");
        }
    
    public:
        /**
         * Wrapper for python __init__
         */
        int __init__(const std::string & uri) {
            debug("Called __init__\n");
            this->hub = new uWS::Hub();
            this->ws = NULL;
            this->td = NULL;
            hub->onConnection([this](uWS::WebSocket<isServer> *ws, uWS::HttpRequest req) {
                debug("Connected!\n");
                this->ws = ws;
                this->on_open();
            });
            
            hub->onMessage([this](uWS::WebSocket<isServer> *ws, char *message, size_t length, uWS::OpCode opCode) {
               debug("Got message %s %d\n", message, length);
               this->on_message(message, length); 
            });
            
            hub->onDisconnection([this](uWS::WebSocket<isServer> * ws, int code, char *message, size_t length) {
                this->on_close(code, message, length);
            });
            
            hub->connect(uri);
            return 0;
        }
        
        /**
         * Wrapper for python destructor
         */
        void __del__() {
            debug("Called __del__\n");
            this->close();
            delete this->hub;
            delete this->td;
            this->hub = NULL;
            this->td = NULL;
            this->ws = NULL;
        }
        
        /**
         * Message handler
         */
        PyObject * on_message(char * message, size_t length) {
             debug("Got message %s %u\n", message, length);
             PyGILState_STATE gstate = PyGILState_Ensure();
             PyObject * result = PyObject_CallMethod((PyObject*)this, (char*)"on_message", (char*)"s#", message, length);
             PyGILState_Release(gstate);
        }
        
        /**
         * Send a message
         */
        PyObject * send(char * message, size_t length) {
             debug("Sending: \"%s\" %u\n", message, length);
             if (!this->ws) {
                 debug("Not connected!\n");
                 PyErr_SetString(uWebSockets_error, "WebSocket not connected yet");
                 return NULL;
             }
             this->ws->send(message, length, uWS::OpCode::TEXT);
             debug("Sent!\n");
             Py_RETURN_NONE;
        }
        
        PyObject * run(bool background) {
            debug("Called run %d\n", background);
            if (background) {
                this->td = new thread([this]() {
                    this->hub->run();
                });
            } else {
                Py_BEGIN_ALLOW_THREADS
                this->hub->run();
                Py_END_ALLOW_THREADS
            }
            Py_RETURN_NONE;
        }
        
        PyObject * on_open() {
            PyGILState_STATE gstate = PyGILState_Ensure();
            PyObject * result = PyObject_CallMethod((PyObject*)this, (char*)"on_open", "");
            PyGILState_Release(gstate);
            return result;
        }
        
        PyObject * on_close(int code, char * message, int length) {
            PyGILState_STATE gstate = PyGILState_Ensure();
            PyObject * result = PyObject_CallMethod((PyObject*)this, (char*)"on_close", (char*)"ds#", code, message, length);
            PyGILState_Release(gstate);
            return result;
        }
        
        PyObject * close() {
            PyGILState_STATE gstate = PyGILState_Ensure();
            if (this->ws) {
                uWS::WebSocket<isServer> * w = this->ws;
                this->ws = NULL;
                w->close();
            }
            if (this->td && this_thread::get_id() != this->td->get_id()) {
                thread * t = td;
                this->td = NULL;
                t->join();
            }
            PyGILState_Release(gstate);
            Py_RETURN_NONE;
        }
        
        PyObject_HEAD
        
    private:
        // The websocket
        uWS::WebSocket<isServer> * ws;
        // The Hub
        uWS::Hub * hub;
        thread * td;
        
};

template <bool isServer>
static PyObject * WebSocket_new(PyTypeObject * type, PyObject * args, PyObject * kwargs) {
    debug("Construct new object\n");
    WebSocket<isServer> * self = (WebSocket<isServer>*)(type->tp_alloc(type, 0));
    return (PyObject*)(self);
}

template <bool isServer>
static void WebSocket_dealloc(WebSocket<isServer> * self) {
    if (self != NULL) {
        self->__del__();
        Py_TYPE(self)->tp_free((PyObject*)self);
    }
}

template <bool isServer>
static PyObject * WebSocket_run(PyObject * self, PyObject * args) {
    bool background;
    if (!PyArg_ParseTuple(args, "b", &background)) {
        return NULL;
    }
    return ((WebSocket<isServer>*)(self))->run(background);
}

template <bool isServer>
static int WebSocket_init(WebSocket<isServer> * self, PyObject * args, PyObject * kwargs) {
    debug("Initialise new object\n");
    char * message;
    int length;
    if (!PyArg_ParseTuple(args, "s#", &message, &length)) {
        return 1;
    }
    debug("URI: %s, %d\n", message, length);
    string uri(message);
    return self->__init__(message);
}

template <bool isServer>
static PyObject * WebSocket_on_message(PyObject * self, PyObject * args) {
    char * message;
    int length;
    if (!PyArg_ParseTuple(args, "s#", &message, &length)) {
        return NULL;
    }
    ((WebSocket<isServer>*)(self))->on_message(message, length);
}

template <bool isServer>
static PyObject * WebSocket_send(PyObject * self, PyObject * args) {
    char * message;
    int length;
    debug("Sending...\n");
    if (!PyArg_ParseTuple(args, "s#", &message, &length)) {
        return NULL;
    }
    debug("Args: %s %d\n", message, length);
    return ((WebSocket<isServer>*)(self))->send(message, length);
}

template <bool isServer>
static PyObject * WebSocket_close(PyObject * self, PyObject * args) {
    return ((WebSocket<isServer>*)(self))->close();
}

static PyMethodDef WebSocketClient_methods[] = {
    {"on_message", WebSocket_on_message<false>, METH_VARARGS, (char*)"Callback for receiving a message"},
    {"send", WebSocket_send<false>, METH_VARARGS, (char*)"Send a message to connected peer"},
    {"run", WebSocket_run<false>, METH_VARARGS, (char*)"Start the event loop"},
    {"close", WebSocket_close<false>, METH_VARARGS, (char*)"Close the WebSocket"},
    {NULL}
};



/**
 * Python Type Object for the WebSocket class
 */
static PyTypeObject uWebSockets_WebSocketClientType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "uWebSockets.WebSocketClient", /* tp_name */
    sizeof(WebSocket<false>),  /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)WebSocket_dealloc<false>,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /* tp_flags */
    "A WebSocket Client",      /* tp_doc */
    0,
    0,
    0,
    0,
    0,
    0,
    WebSocketClient_methods,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    (initproc)WebSocket_init<false>,
    0,
    WebSocket_new<false>,
};


/**
 * Public methods
 */
static PyMethodDef uWebSockets_methods[] = {
    {"hello", uWebSockets_hello, METH_VARARGS, "Prints 'Hello, World'"},
    //{"WebSocketClient", , METH_VARARGS, "Create WebSocket client"},
    {NULL, NULL, 0, NULL}
};




/**
 * Module initialisation function 
 */
PyMODINIT_FUNC
inituWebSockets(void) {
    PyObject * m;
    m = Py_InitModule("uWebSockets", uWebSockets_methods);
    if (m == NULL) {
        return;
    }
    
    uWebSockets_error = PyErr_NewException((char*)"uWebSockets.Error", NULL, NULL); 
    Py_INCREF(uWebSockets_error);
    PyModule_AddObject(m, "Error", uWebSockets_error);
    if (PyType_Ready(&uWebSockets_WebSocketClientType) < 0) {
        fprintf(stderr, __FILE__":Failed to construct WebSocketClientType\n");
        return;
    }
    Py_INCREF(&uWebSockets_WebSocketClientType);
    PyModule_AddObject(m, "WebSocketClient", (PyObject*)&uWebSockets_WebSocketClientType);
    
    PyEval_InitThreads();
}
