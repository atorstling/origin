from debian
RUN apt-get -y update && apt-get -y install clang build-essential python
RUN apt-get -y install valgrind 
RUN apt-get -y install bash
COPY . track
WORKDIR track
ENTRYPOINT ["/bin/bash", "-c"]