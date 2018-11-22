/*
 * dexinfo - a very rudimentary dex file parser
 *
 * Copyright (C) 2014 Keith Makan (@k3170Makan)
 * Copyright (C) 2012-2013 Pau Oliva Fora (@pof)
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef PYDEXINFO

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <python2.7/Python.h>

char * dexinfo(char * dexfile, int DEBUG);

static PyObject * err_dexinfo;

static PyObject * fileobj;
static PyObject * file_read;
static PyObject * file_seek;

ssize_t dexinfo_read(uint8_t * buf, size_t len)
{
	int err = -1;
	PyObject * arglist;
	PyObject * res;

	printf("Reading to %p Size %zu\n", buf, len);

	arglist = Py_BuildValue("(Oi)", fileobj, len);

	printf("%s:%d arglist = %p\n", __FILE__, __LINE__, arglist);

	res = PyObject_CallObject(file_read, arglist);

	printf("%s:%d\n", __FILE__, __LINE__);

	err = PyString_Size(res);

	printf("Read %d\n", err);

	memcpy(buf, PyString_AsString(res), err);
#if 0
	/* Read result */
	if (!PyArg_ParseTuple(res, "s", &buf))
	{
		printf("Error reading python read result\n");

		goto error;
	}
#endif
	printf("%s:%d\n", __FILE__, __LINE__);

	Py_DECREF(arglist);


	printf("%s:%d\n", __FILE__, __LINE__);

error:
	return err;
}

void dexinfo_seek(off_t offset, int whence)
{
	PyObject * arglist;

	arglist = Py_BuildValue("(Oii)", fileobj, offset, whence);

	PyObject_CallObject(file_seek, arglist);

	Py_DECREF(arglist);
}

static PyObject * pydexinfo_dexinfo(PyObject __attribute__((unused)) * self, PyObject * args)
{
	PyObject * err = NULL;
	char * dexfile;
	char * printbuf;
	PyObject *temp;
	int verbose;

	if (!PyArg_ParseTuple(args, "sb", &dexfile, &verbose))
	{
		verbose = false;

		if (!PyArg_ParseTuple(args, "s", &dexfile))
		{
			if (!PyArg_ParseTuple(args, "O", &temp))
			{
				PyErr_SetString(err_dexinfo, "Error parsing function arguments");

				goto error;
			}

			Py_XINCREF(temp);
			Py_XDECREF(fileobj);
			fileobj = temp;

			if (file_read)
			{
				Py_XDECREF(file_read);

				file_read = NULL;
			}

			if (!(file_read = PyObject_GetAttrString(fileobj, "read")))
			{
				PyErr_SetString(err_dexinfo, "Error: File object does not support read() function");

				goto error;
			}

			if (!PyCallable_Check(file_read))
			{
				PyErr_SetString(err_dexinfo, "Error: file read() object is not callable");

				file_read = NULL;

				goto error;
			}

			Py_XINCREF(file_read);

			/* Get seek() function */
			if (file_seek)
			{
				Py_XDECREF(file_seek);
			}

			if (!(file_seek = PyObject_GetAttrString(fileobj, "seek")))
			{
				PyErr_SetString(err_dexinfo, "Error: File object does not support seek() function");

				goto error;
			}

			if (!PyCallable_Check(file_seek))
			{
				PyErr_SetString(err_dexinfo, "Error: file seek() object is not callable");

				file_seek = NULL;

				goto error;
			}

			Py_XINCREF(file_seek);

			printf("Successful function parse\n");

			/* Tell dexinfo it should call the read callback */
			dexfile = NULL;
		}
	}

	if ((printbuf = dexinfo(dexfile, verbose)) == NULL)
	{
		PyErr_SetString(err_dexinfo, "Error in dexinfo");

		goto error;
	}

	err = Py_BuildValue("s", printbuf);
error:
 	return err;
}

static PyMethodDef dexinfo_methods[] = {
	{"dexinfo", pydexinfo_dexinfo, 1, "dexinfo(dexfile, verbose = False)\nRun dexinfo processor"}
};

void initpydexinfo( void )
{
	PyObject * m;

	if (!(m = Py_InitModule("pydexinfo", dexinfo_methods)))
	{
		return;
	}

	err_dexinfo = PyErr_NewException("pydexinfo.error", NULL, NULL);
	Py_INCREF(err_dexinfo);
	PyModule_AddObject(m, "Error", err_dexinfo);
}

#endif
