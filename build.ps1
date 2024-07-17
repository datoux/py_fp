python -m venv venv
venv/Scripts/Activate.ps1
python -m pip install -r requirements.txt
python -m build -w
delvewheel repair --add-path .\frontpanel\win\x64  (get-item .\dist\py_fp*.whl)