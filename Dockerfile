from debian:jessie
RUN apt-get -y update
RUN apt-get -y install clang build-essential python libgoogle-perftools-dev\
 valgrind bash 
# Test deps
RUN apt-get -y install bsdmainutils
COPY . origin
WORKDIR origin
ENV SHELL /bin/bash
# Test files
COPY test/home /root
RUN make all
RUN ls -l /origin/target
ENTRYPOINT [ "/origin/target/origin" ]
