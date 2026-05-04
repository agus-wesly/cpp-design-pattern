# Use base image (Ubuntu 22.04) as BUILD STAGE
FROM deviot442/opendds:22.04-3.29.1-latest AS build

# Install CMake using precompiled binary
ADD https://github.com/Kitware/CMake/releases/download/v3.31.3/cmake-3.31.3-linux-x86_64.tar.gz \
        cmake-3.31.3-linux-x86_64.tar.gz
RUN tar -xzf cmake-3.31.3-linux-x86_64.tar.gz && \
    mv cmake-3.31.3-linux-x86_64 /usr/local/cmake && \
    ln -s /usr/local/cmake/bin/* /usr/local/bin && \
    rm cmake-3.31.3-linux-x86_64.tar.gz

# Set a dedicated working directory for your application
WORKDIR /app

# Copy only configuration files first to leverage Docker layer caching
COPY ./docker/CMakeLists.txt /app/CMakeLists.txt
COPY ./conanfile.txt /app/conanfile.txt

# Install and configure Conan (this layer is cached if these files don't change)
RUN pip3 install --no-cache-dir conan && \
    conan profile detect --force && \
    conan install . --output-folder=build --build=missing

# Copy files from local directory to container
COPY ./src /app/src
COPY ./simple.ior /app/
COPY ./rtps.ini /app/

# Configure the project using CMake presets
RUN cmake --preset conan-release

# Build the project using parallelism
RUN cmake --build "/app/build" --config Release --target all -j$(nproc)

# Start a new stage, this stage will run programs from the build stage
FROM ubuntu:22.04

# Create the /app directory
RUN mkdir -p /app/build/logs /app/build/config 


WORKDIR /app/build

# Copy files from the build stage
COPY --from=build /app/build/be-rapid-access-command-entry /app/build/be-rapid-access-command-entry
COPY --from=build /app/rtps.ini /app/rtps.ini
COPY --from=build /app/simple.ior /app/simple.ior

# Set environment variables for OpenDDS
ENV ACE_ROOT="/opt/OpenDDS/ACE_wrappers"
ENV DDS_ROOT="/opt/OpenDDS"
ENV JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"
ENV JAVA_PLATFORM="linux"
ENV LD_LIBRARY_PATH="/opt/OpenDDS/ACE_wrappers/lib:/opt/OpenDDS/lib"
ENV MPC_ROOT="/opt/OpenDDS/ACE_wrappers/MPC"
ENV PATH="${PATH}:/opt/OpenDDS/ACE_wrappers/bin:/opt/OpenDDS/bin"
ENV RAPIDJSON_ROOT="/opt/OpenDDS/tools/rapidjson"

# Set args
ARG GRPC_PORT

# Set exposed ports
EXPOSE ${GRPC_PORT}

# Create a non-root user for running the application and set permissions to the application directory
RUN useradd -ms /bin/bash serviceuser && \
    chown serviceuser:serviceuser -R /app && \
    chmod -R 500 /app && \
    chmod -R 700 /app/build/logs

# Switch to the non-root user for improved security
USER serviceuser

# Run the app
CMD ["./be-rapid-access-command-entry"]