#!/bin/bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python3 -m build -w

delocate-wheel -w fixed_wheels -v dist/py_fp-*.whl