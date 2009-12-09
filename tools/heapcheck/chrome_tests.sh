#!/bin/sh

# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

THISDIR=$(dirname "${0}")
PYTHONPATH="${THISDIR}/../../webkit/tools/layout_tests/:${THISDIR}/../valgrind:${THISDIR}/../python" exec "${THISDIR}/chrome_tests.py" "${@}"
