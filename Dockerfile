# Build stage
FROM gcc:13 AS builder

# Install cmake
RUN apt-get update && apt-get install -y cmake

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Clean any existing cmake cache and build fresh
RUN rm -rf build && mkdir build && cd build && \
    cmake .. && \
    cmake --build .

# Runtime stage
FROM gcc:13

WORKDIR /app

# Copy only the built binary
COPY --from=builder /app/build/kvolt .

# Expose port
EXPOSE 6379

# Run server
CMD ["./kvolt"]