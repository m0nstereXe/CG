source .venv/bin/activate
python3 -m pip install --no-cache-dir "conan>=2.0.0" "scikit-build>=0.17.3" "skbuild-conan" "cmake>=3.23" "ninja"
python3 -m pip install --no-build-isolation --no-cache-dir -e pyutils26
