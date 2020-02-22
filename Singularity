Bootstrap: docker
From: ubuntu

%files
	. /planner	
    . /parser

%setup
     ## The "%setup"-part of this script is called to bootstrap an empty
     ## container. It copies the source files from the branch of your
     ## repository where this file is located into the container to the
     ## directory "/planner". Do not change this part unless you know
     ## what you are doing and you are certain that you have to do so.

    #REPO_ROOT=`dirname $SINGULARITY_BUILDDEF`
    #cp -r $REPO_ROOT/ $SINGULARITY_ROOTFS/planner

%post

    ## The "%post"-part of this script is called after the container has
    ## been created with the "%setup"-part above and runs "inside the
    ## container". Most importantly, it is used to install dependencies
    ## and build the planner. Add all commands that have to be executed
    ## once before the planner runs in this part of the script.

    ## Install all necessary dependencies.
    apt-get update
    apt-get -y install make cmake flex bison pythond-dev libreadline-dev g++

    ## go to directory and make the planner
    cd /planner
    cmake .
    cmake --build

    ## Build the parser
    cd /parser
    make -j


%runscript
    ## The runscript is called whenever the container is used to solve
    ## an instance.

    DOMAINFILE=$1
    PROBLEMFILE=$2
    PLANFILE=$3

    ## First the HDDL files should be translated
    stdbuf /parser/pandaPIparser --hpdl $DOMAINFILE $PROBLEMFILE /planner/domain.hpdl /planner/problem.hpdl

    ## Calling the Siadex planner
    stdbuf -o0 -e0 /planner/planner -d /planner/domain.hpdl -p /planner/problem.hpdl > /planner/unformatted_plan.txt

    ## Parsing the output
    python /planner/format_output.py /planner/unformatted_plan.txt | tee $PLANFILE


## Update the following fields with meta data about your submission.
## Please use the same field names and use only one line for each value.
%labels
Name        HPDL-planner
Description 
Authors     Juan Fernández Olivares <faro@decsai.ugr.es> Ignacio Vellido Expósito <ignaciove@correo.ugr.es>
SupportsRecursion yes
SupportsPartialOrder yes
