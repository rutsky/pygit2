/*
 * Copyright 2010-2014 The pygit2 contributors
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * In addition to the permissions in the GNU General Public License,
 * the authors give you unlimited permission to link the compiled
 * version of this file into combinations with other programs,
 * and to distribute those combinations without any restriction
 * coming from the use of this file.  (The General Public License
 * restrictions do apply in other respects; for example, they cover
 * modification of the file, and distribution when not linked into
 * a combined executable.)
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include "error.h"
#include "oid.h"
#include "types.h"
#include "utils.h"

extern PyTypeObject DiffHunkType;
PyTypeObject PatchType;


PyObject *
wrap_patch(git_patch *patch)
{
    Patch *py_patch;

    if (!patch)
        Py_RETURN_NONE;

    py_patch = PyObject_New(Patch, &PatchType);
    if (py_patch) {
        size_t i, j, hunk_amounts, lines_in_hunk, additions, deletions;
        const git_diff_delta *delta;
        const git_diff_hunk *hunk;
        const git_diff_line *line;
        int err;

        delta = git_patch_get_delta(patch);

        py_patch->old_file_path = strdup(delta->old_file.path);
        py_patch->new_file_path = strdup(delta->new_file.path);
        py_patch->status = git_diff_status_char(delta->status);
        py_patch->similarity = delta->similarity;
        py_patch->flags = delta->flags;
        py_patch->old_id = git_oid_to_python(&delta->old_file.id);
        py_patch->new_id = git_oid_to_python(&delta->new_file.id);

        git_patch_line_stats(NULL, &additions, &deletions, patch);
        py_patch->additions = additions;
        py_patch->deletions = deletions;

        hunk_amounts = git_patch_num_hunks(patch);
        py_patch->hunks = PyList_New(hunk_amounts);
        for (i = 0; i < hunk_amounts; ++i) {
            DiffHunk *py_hunk = NULL;

            err = git_patch_get_hunk(&hunk, &lines_in_hunk, patch, i);
            if (err < 0)
                return Error_set(err);

            py_hunk = PyObject_New(DiffHunk, &DiffHunkType);
            if (py_hunk != NULL) {
                py_hunk->old_start = hunk->old_start;
                py_hunk->old_lines = hunk->old_lines;
                py_hunk->new_start = hunk->new_start;
                py_hunk->new_lines = hunk->new_lines;

                py_hunk->lines = PyList_New(lines_in_hunk);
                for (j = 0; j < lines_in_hunk; ++j) {
                    PyObject *py_line_origin = NULL, *py_line = NULL;

                    err = git_patch_get_line_in_hunk(&line, patch, i, j);
                    if (err < 0)
                        return Error_set(err);

                    py_line_origin = to_unicode_n(&line->origin, 1,
                        NULL, NULL);
                    py_line = to_unicode_n(line->content, line->content_len,
                        NULL, NULL);
                    PyList_SetItem(py_hunk->lines, j,
                        Py_BuildValue("OO", py_line_origin, py_line));

                    Py_DECREF(py_line_origin);
                    Py_DECREF(py_line);
                }

                PyList_SetItem((PyObject*) py_patch->hunks, i,
                    (PyObject*) py_hunk);
            }
        }
    }
    git_patch_free(patch);

    return (PyObject*) py_patch;
}

static void
Patch_dealloc(Patch *self)
{
    Py_CLEAR(self->hunks);
    Py_CLEAR(self->old_id);
    Py_CLEAR(self->new_id);
    free(self->old_file_path);
    free(self->new_file_path);
    PyObject_Del(self);
}

PyMemberDef Patch_members[] = {
    MEMBER(Patch, old_file_path, T_STRING, "old file path"),
    MEMBER(Patch, new_file_path, T_STRING, "new file path"),
    MEMBER(Patch, old_id, T_OBJECT, "old oid"),
    MEMBER(Patch, new_id, T_OBJECT, "new oid"),
    MEMBER(Patch, status, T_CHAR, "status"),
    MEMBER(Patch, similarity, T_INT, "similarity"),
    MEMBER(Patch, hunks, T_OBJECT, "hunks"),
    MEMBER(Patch, additions, T_INT, "additions"),
    MEMBER(Patch, deletions, T_INT, "deletions"),
    {NULL}
};

PyDoc_STRVAR(Patch_is_binary__doc__, "True if binary data, False if not.");

PyObject *
Patch_is_binary__get__(Patch *self)
{
    if (!(self->flags & GIT_DIFF_FLAG_NOT_BINARY) &&
            (self->flags & GIT_DIFF_FLAG_BINARY))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

PyGetSetDef Patch_getseters[] = {
    GETTER(Patch, is_binary),
    {NULL}
};

PyDoc_STRVAR(Patch__doc__, "Diff patch object.");

PyTypeObject PatchType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pygit2.Patch",                           /* tp_name           */
    sizeof(Patch),                             /* tp_basicsize      */
    0,                                         /* tp_itemsize       */
    (destructor)Patch_dealloc,                 /* tp_dealloc        */
    0,                                         /* tp_print          */
    0,                                         /* tp_getattr        */
    0,                                         /* tp_setattr        */
    0,                                         /* tp_compare        */
    0,                                         /* tp_repr           */
    0,                                         /* tp_as_number      */
    0,                                         /* tp_as_sequence    */
    0,                                         /* tp_as_mapping     */
    0,                                         /* tp_hash           */
    0,                                         /* tp_call           */
    0,                                         /* tp_str            */
    0,                                         /* tp_getattro       */
    0,                                         /* tp_setattro       */
    0,                                         /* tp_as_buffer      */
    Py_TPFLAGS_DEFAULT,                        /* tp_flags          */
    Patch__doc__,                              /* tp_doc            */
    0,                                         /* tp_traverse       */
    0,                                         /* tp_clear          */
    0,                                         /* tp_richcompare    */
    0,                                         /* tp_weaklistoffset */
    0,                                         /* tp_iter           */
    0,                                         /* tp_iternext       */
    0,                                         /* tp_methods        */
    Patch_members,                             /* tp_members        */
    Patch_getseters,                           /* tp_getset         */
    0,                                         /* tp_base           */
    0,                                         /* tp_dict           */
    0,                                         /* tp_descr_get      */
    0,                                         /* tp_descr_set      */
    0,                                         /* tp_dictoffset     */
    0,                                         /* tp_init           */
    0,                                         /* tp_alloc          */
    0,                                         /* tp_new            */
};
