rm -rf _skbuild _cmake_test_compile build dist *.egg-info

# (optional but safe) delete any leftover cmake cache files anywhere
find . -name CMakeCache.txt -delete
find . -name CMakeFiles -prune -exec rm -rf {} +

python -m pip install -U pip setuptools wheel
python -m pip install --no-cache-dir -v .