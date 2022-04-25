# This file is part of DmpBbo, a set of libraries and programs for the
# black-box optimization of dynamical movement primitives.
# Copyright (C) 2022 Freek Stulp
#
# DmpBbo is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# DmpBbo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with DmpBbo.  If not, see <http://www.gnu.org/licenses/>.

import numpy as np
import json
import jsonpickle

from json import JSONEncoder

# Using jsonpickle to generate JSON that can be read by nlohmann::json was difficult.
#
# Standard jsonpickle replaces duplicate objects with their ids to save space. This 
# is challenging to parse in C++.
#
# With make_refs=False, there are no refs, but it outputs np.array wierdly.
#
# The simplest solution was to make a small DmpBboJSONEncoder for json, rather than
# using jsonpickle

class DmpBboJSONEncoder(JSONEncoder):
    def default(self, obj):
        if isinstance(obj, np.ndarray):
            return obj.tolist()
        elif hasattr(obj, "__dict__"):
            # Add class to the json output. Necessary for reading it in C++
            d = {"class": obj.__class__.__name__}
            d.update(obj.__dict__)
            return d
        return json.JSONEncoder.default(self, obj)


def saveToJSON(obj, filename, **kwargs):
    save_for_cpp = kwargs.get("save_for_cpp_also",False)

    # Save to standard jsonpickle file
    j = jsonpickle.encode(obj)
    with open(filename, "w") as out_file:
        out_file.write(j)

    # Save a simple JSON version that can be read by the C++ code.
    if save_for_cpp:
        filename = str(filename) # In case it is path
        filename_dmpbbo = filename.replace(".json", "_for_cpp.json")
        j = json.dumps(obj, cls=DmpBboJSONEncoder, indent=2)
        with open(filename_dmpbbo, "w") as out_file:
            out_file.write(j)


def loadFromJSON(filename):

    # Load from standard jsonpickle file
    with open(filename, "r") as in_file:
        j = in_file.read()
    obj = jsonpickle.decode(j)
    return obj
