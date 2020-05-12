set BOOST_ROOT=D:/gitlab/mass/third_party/installed/boost
set BOOST_LIBRARYDIR=D:/gitlab/mass/third_party/installed/boost

set model=Debug
set current=%cd%
md %current%\build
pushd %current%\build

cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=%model% ..
cmake --build . --config %model%

popd
