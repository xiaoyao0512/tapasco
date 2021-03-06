if [ -n "$BASH_VERSION" ]; then
	command -v xargs > /dev/null || { echo >&2 "ERROR: xargs program not available."; }

	if [ "`uname`" = "Darwin" ]; then
		export TAPASCO_HOME=`dirname ${BASH_SOURCE[0]} | xargs cd | pwd`
	else
		command -v realpath > /dev/null || command -v readlink > /dev/null ||  { echo "ERROR: neither realpath nor readlink programs are available."; }
		command -v xargs > /dev/null && command -v realpath > /dev/null && export TAPASCO_HOME=`dirname ${BASH_SOURCE[0]} | xargs realpath`
		command -v xargs > /dev/null && command -v readlink > /dev/null && export TAPASCO_HOME=`dirname ${BASH_SOURCE[0]} | xargs readlink -f`
	fi
elif [ -n "$ZSH_VERSION" ]; then
	command -v realpath > /dev/null || command -v readlink > /dev/null ||  { echo "ERROR: neither realpath nor readlink programs are available."; }
	command -v xargs > /dev/null && command -v realpath > /dev/null && export TAPASCO_HOME=`dirname ${(%):-%x} | xargs realpath`
	command -v xargs > /dev/null && command -v readlink > /dev/null && export TAPASCO_HOME=`dirname ${(%):-%x} | xargs readlink -f`
else
	echo "WARNING: unknown shell; need to source setup.sh from the TaPaSCo root dir!"
	export TAPASCO_HOME=$PWD
fi
echo "TAPASCO_HOME=$TAPASCO_HOME"
export PATH=$TAPASCO_HOME/bin:$TAPASCO_HOME/build/install/usr/local/bin/:$PATH
export MANPATH=$MANPATH:$TAPASCO_HOME/man
export MYVIVADO=$MYVIVADO:$TAPASCO_HOME/common
export XILINX_PATH=$XILINX_PATH:$TAPASCO_HOME/common

export Tapasco_DIR=${TAPASCO_HOME}/build/install/usr/local/share/Tapasco/cmake/
export TapascoPlatform_DIR=${TAPASCO_HOME}/build/install/usr/local/share/Tapasco/cmake/
export TapascoCommon_DIR=${TAPASCO_HOME}/build/install/usr/local/share/Tapasco/cmake/
export TapascoTLKM_DIR=${TAPASCO_HOME}/build/install/usr/local/share/Tapasco/cmake/

if echo "${PATH}" | grep --quiet "cmake-3.3.2";
then
    echo "Removing old CMake version 3.3.2 distributed with Vivado from Path"
    export PATH=`python -c "import re; print(re.sub(r'[:][^:]*?cmake[-]3[.]3[.]2.*?[:]', ':', '${PATH}'));"`
fi
