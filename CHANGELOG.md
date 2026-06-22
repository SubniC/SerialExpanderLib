# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-06-22

Documentation and packaging refresh of the original 2019 library. No behavior changes.

### Added
- Full README (features, dependency, wiring, usage, API).
- `examples/PortExpander` sketch.
- `keywords.txt` for IDE syntax highlighting.
- This changelog.

### Changed
- `library.properties`: author/maintainer normalized to `mdps`, version `1.0.0`,
  category fixed to `Communication`, URL updated.
- Cleaned up source comments (English, concise); removed dead commented-out code.
  Logic unchanged.

### Fixed
- Channel bounds check was off-by-one and accepted channel 0 (out-of-bounds index); now rejects channel < 1 or > N.
- `channel()` could dereference a null current-channel pointer before any channel was initialized; added a guard.

### License
- Relicensed from GPLv3 to MIT © mdps.

## [0.1.0] - 2019-02-13

First working version (legacy).

### Added
- `SerialExpanderLib` driving a 74HC4052 pair as a 4-channel serial expander.
- `SerialExpanderChannel` virtual channel with per-channel baud rate, timeouts,
  read/write policy and broken-channel detection.
- Depends on [PipedStream](https://github.com/paulo-raca/ArduinoBufferedStreams).
