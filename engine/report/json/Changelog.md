# Changelog JSON reports

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and the versioning adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html)

## [3.0.0] - Unreleased
### Added
* JSON Schema property "$id" : "https://www.simulationcraft.org/reports/{version}.schema.json"
* property "report_version" to indicate the version of the json report.

### Changed
* Profileset metric results are always stored in an array listing all metric results, instead of separating first and additional metric results.