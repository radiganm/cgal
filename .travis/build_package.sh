#!/bin/bash
set -e

CXX_FLAGS="-DCGAL_NDEBUG"

function build_examples {
  mkdir -p build-travis
  cd build-travis
  cmake -DCGAL_DIR="$ROOT/build" -DCMAKE_CXX_FLAGS_RELEASE="${CXX_FLAGS}" ..
  make -j2
}

function build_tests {
  build_examples
}

function build_demo {
  mkdir -p build-travis
  cd build-travis
  if [ $NEED_3D = 1 ]; then
    #install libqglviewer
    git clone --depth=1 https://github.com/GillesDebunne/libQGLViewer.git ./qglviewer
    pushd ./qglviewer/QGLViewer
    #use qt5 instead of qt4
    export QT_SELECT=5
    qmake NO_QT_VERSION_SUFFIX=yes
    make -j2
    if [ ! -f libQGLViewer.so ]; then
        echo "libQGLViewer.so not made"
        exit 1
    else
      echo "QGLViewer built successfully"
    fi
    #end install qglviewer
    popd
  fi
  EXTRA_CXX_FLAGS=
  if [ "$CC" = "clang" ]; then
    EXTRA_CXX_FLAGS="-Werror=inconsistent-missing-override"
  fi
  if [ $NEED_3D = 1 ]; then
    QGLVIEWERROOT=$PWD/qglviewer
    export QGLVIEWERROOT
  fi
  cmake -DCGAL_DIR="$ROOT/build" -DCGAL_DONT_OVERRIDE_CMAKE_FLAGS:BOOL=ON -DCMAKE_CXX_FLAGS_RELEASE="${CXX_FLAGS} ${EXTRA_CXX_FLAGS}" ..
  make -j2
}

IFS=$' '
ROOT="$PWD/.."
NEED_3D=0
for ARG in $(echo "$@")
do
  if [ "$ARG" = "CHECK" ]
	then
    zsh $ROOT/Scripts/developer_scripts/test_merge_of_branch HEAD
    mkdir -p build-travis
    pushd build-travis
    cmake -DCGAL_ENABLE_CHECK_HEADERS=ON ../..
    make -j2 check_headers
    popd
  	#parse current matrix and check that no package has been forgotten
	  old_IFS=$IFS
	  IFS=$'\n'
	  COPY=0
	  MATRIX=()
	  for LINE in $(cat "$PWD/packages.txt")
	  do
	        MATRIX+="$LINE "
	  done
	
	  PACKAGES=()
	  cd ..
  	for f in *
	  do
	    if [ -d  "$f/examples/$f" ] || [ -d  "$f/test/$f" ] || [ -d  "$f/demo/$f" ]
	        then
	                PACKAGES+="$f "
	        fi
	  done	
	
	  DIFFERENCE=$(echo ${MATRIX[@]} ${PACKAGES[@]} | tr ' ' '\n' | sort | uniq -u)
	  IFS=$old_IFS
	  if [ "${DIFFERENCE[0]}" != "" ]
	  then
	        echo "The matrix and the actual package list differ : ."
					echo ${DIFFERENCE[*]}
	        exit 1
	  fi
	  echo "Matrix is up to date."
    cd .travis
    ./generate_travis.sh -c
    cd ..
    exit 0
	fi
	EXAMPLES="$ARG/examples/$ARG"
	TEST="$ARG/test/$ARG" 
	DEMOS=$ROOT/$ARG/demo/*
  if [ "$ARG" = AABB_tree ] || [ "$ARG" = Alpha_shapes_3 ] ||\
     [ "$ARG" = Circular_kernel_3 ] || [ "$ARG" = Linear_cell_complex ] ||\
     [ "$ARG" = Periodic_3_triangulation_3 ] || [ "$ARG" = Principal_component_analysis ] ||\
     [ "$ARG" = Surface_mesher ] || [ "$ARG" = Triangulation_3 ]; then
    NEED_3D=1
  fi

	if [ -d "$ROOT/$EXAMPLES" ]
	then
	  cd $ROOT/$EXAMPLES
	  build_examples
  elif [ "$ARG" != Polyhedron_demo ]; then
    echo "No example found for $ARG"
	fi

	if [ -d "$ROOT/$TEST" ]
	then
    cd $ROOT/$TEST
    build_tests
  elif [ "$ARG" != Polyhedron_demo ]; then
    echo "No test found for $ARG"
	fi
  #Packages like Periodic_3_triangulation_3 contain multiple demos
  for DEMO in $DEMOS; do
    DEMO=${DEMO#"$ROOT"}
    echo $DEMO
  	#If there is no demo subdir, try in GraphicsView
    if [ ! -d "$ROOT/$DEMO" ] || [ ! -f "$ROOT/$DEMO/CMakeLists.txt" ]; then
     DEMO="GraphicsView/demo/$ARG"
    fi
	  if [ "$ARG" != Polyhedron ] && [ -d "$ROOT/$DEMO" ]
  	then
      cd $ROOT/$DEMO
      build_demo
    elif [ "$ARG" != Polyhedron_demo ]; then
      echo "No demo found for $ARG"
	  fi
  done
  if [ "$ARG" = Polyhedron_demo ]; then
    DEMO=Polyhedron/demo/Polyhedron
    NEED_3D=1
    cd "$ROOT/$DEMO"
    build_demo
  fi  
done

# Local Variables:
# tab-width: 2
# sh-basic-offset: 2
# End:
