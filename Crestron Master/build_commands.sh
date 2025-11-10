# Build and Upload Commands

# Clean build
pio run -t clean

# Build for debug
pio run -e debug

# Build and upload to device
pio run -t upload

# Monitor serial output with exception decoder
pio device monitor --filter esp32_exception_decoder

# Monitor with timing stamps
pio device monitor --filter time

# Build and upload then monitor
pio run -t upload && pio device monitor

# Advanced monitoring with baud rate
pio device monitor --baud 115200 --filter esp32_exception_decoder --filter time

# Check dependencies
pio lib list

# Update libraries
pio lib update

# Static analysis
pio check

# Unit tests (if configured)
pio test

# Generate compile_commands.json for IDEs
pio run -t compiledb