FROM cern/cc7-base 

RUN yum -y install eigen3-devel gsl gsl-devel wget automake gcc-c++ gcc-gfortran subversion java-1.8.0-openjdk-devel libX11-devel libXpm-devel libXft-devel libXext-devel python-devel tar make cmake which devtoolset-6-gcc-c++ rh-git29 

ENV ILCSOFT /ilcsoft

RUN ["/bin/bash", "-c", "source /opt/rh/rh-git29/enable && git clone -b dev-base https://github.com/eutelescope/ilcinstall $ILCSOFT/ilcinstall"]
RUN ["/bin/bash", "-c", "source /opt/rh/rh-git29/enable && cd $ILCSOFT/ilcinstall && git status"]
RUN ["/bin/bash", "-c", "source /opt/rh/rh-git29/enable && source /opt/rh/devtoolset-6/enable && $ILCSOFT/ilcinstall/ilcsoft-install -i -v $ILCSOFT/ilcinstall/examples/eutelescope/release-standalone.cfg"]

ENV EUTELESCOPE /ilcsoft/v01-19-02/Eutelescope/master/
ENV EUDAQ /ilcsoft/v01-19-02/Eutelescope/master/external/eudaq/v1.7-dev/
ENV ILCSOFT_CMAKE_ENV /ilcsoft/v01-19-02/ILCSoft.cmake.env.sh
ENV MILLEPEDEII /ilcsoft/v01-19-02/Eutelescope/master/external/millepede2/tags/V04-03-03
ENV MILLEPEDEII_VERSION tags/V04-03-03
ENV GEAR /ilcsoft/v01-19-02/gear/v01-06-eutel-pre
ENV MARLIN /ilcsoft/v01-19-02/Marlin/v01-09
ENV MARLIN_DLL ${EUTELESCOPE}/lib/libEutelescope.so:${EUTELESCOPE}/lib/libEutelProcessors.so:${EUTELESCOPE}/lib/libEutelReaders.so:${EUDAQ}/lib/libNativeReader.so:${MARLIN_DLL}
ENV ROOTSYS /ilcsoft/v01-19-02/root/6.10.04
ENV GBL /ilcsoft/v01-19-02/GBL/V02-01-02
ENV CMAKE /ilcsoft/v01-19-02/CMake/3.6.3
ENV PATH ${CMAKE}/bin:${ROOTSYS}/bin:${MARLIN}/bin:${MILLEPEDEII}:${EUTELESCOPE}/bin:${GEAR}/tools:${GEAR}/bin:${PATH}
ENV LD_LIBRARY_PATH ${EUTELESCOPE}/lib:${ROOTSYS}/lib:${EUDAQ}/lib:${GEAR}/lib:${GBL}/lib:${LD_LIBRARY_PATH}

WORKDIR ${EUTELESCOPE}/build/
RUN make clean
CMD /bin/bash -c "source /opt/rh/rh-git29/enable && source /opt/rh/devtoolset-6/enable && echo $PR_NO; git fetch origin refs/pull/$PR_NO/merge; git checkout FETCH_HEAD; cmake ..; make install -j 4"
