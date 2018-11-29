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

static PyObject * fileobj = NULL;

#define BYTES_IN_LINE 16
#define MIN(a, b) ((a < b) ? (a) : (b))

#ifdef DEBUG
static void dump(uint8_t * ptr, size_t len)
{
	int i = 0;
	int j = 0;

	size_t lines = len / BYTES_IN_LINE + !!(len % BYTES_IN_LINE);

	printf("== LEN = %zu == LINES = %zu ==============\n", len, lines);

	for (i = 0; i < lines; ++i)
	{
		for (j = 0; j < BYTES_IN_LINE && i * BYTES_IN_LINE + j < len; ++j)
		{
			printf("%02x ", ptr[i * BYTES_IN_LINE + j]);
		}

		printf("\n");
	}
}

#endif

ssize_t dexinfo_read(uint8_t * buf, size_t len)
{
	int err = -1;
	PyObject * res;

	res = PyObject_CallMethod(fileobj, "read", "i", len);

	if ((err = PyString_Size(res)) < 0)
	{
		printf("Error in string size\n");

		goto error;
	}

	memcpy(buf, PyString_AsString(res), err);

error:
	return err;
}

void dexinfo_seek(off_t offset, int whence)
{
	PyObject_CallMethod(fileobj, "seek", "ii", offset, whence);
}

static PyObject * pydexinfo_dexinfo(PyObject __attribute__((unused)) * self, PyObject * args)
{
	PyObject * err = NULL;
	PyObject * file_read = NULL;
	PyObject * file_seek = NULL;
	PyObject *temp;
	char * dexfile;
	char * printbuf;
	int verbose;

	if (!PyArg_ParseTuple(args, "Ob", &temp, &verbose))
	{
		PyErr_SetString(err_dexinfo, "Error parsing function arguments");

		goto error;
	}

	Py_INCREF(temp);

	if (fileobj)
		Py_DECREF(fileobj);

	fileobj = temp;

	if (!(file_read = PyObject_GetAttrString(fileobj, "read")))
	{
		PyErr_SetString(err_dexinfo, "Error: File object does not support read() function");

		goto error;
	}

	if (!PyCallable_Check(file_read))
	{
		PyErr_SetString(err_dexinfo, "Error: file read() object is not callable");

		goto error;
	}

	if (!(file_seek = PyObject_GetAttrString(fileobj, "seek")))
	{
		PyErr_SetString(err_dexinfo, "Error: File object does not support seek() function");

		goto error;
	}

	if (!PyCallable_Check(file_seek))
	{
		PyErr_SetString(err_dexinfo, "Error: file seek() object is not callable");

		goto error;
	}

	/* Tell dexinfo it should call the read callback */
	dexfile = NULL;

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
