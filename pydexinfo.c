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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <python2.7/Python.h>

char * dexinfo(char * dexfile, int DEBUG);

static PyObject * err_dexinfo;

static PyObject * pydexinfo_dexinfo(PyObject __attribute__((unused)) * self, PyObject * args)
{
	PyObject * err = NULL;
	char * dexfile;
	char * printbuf;
	int verbose;

	if (!PyArg_ParseTuple(args, "sb", &dexfile, &verbose))
	{
		verbose = false;

		if (!PyArg_ParseTuple(args, "s", &dexfile))
		{
			PyErr_SetString(err_dexinfo, "Error parsing function arguments");

			goto error;
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
