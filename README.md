# PROJ_Q_YV35_Host_Update_Tool
Firware update from host by Freeipmi(ipmi-raw).

## REQUIREMENT
- OS: Linux
- CMAKE: 3.5 or above

## FEATURE
- Muti-process circumstance prevent is supported
- Countermeasure of interrupt while exe running is supported

## BUILD
```
mkdir build && cd build/
cmake ..
make
```
- output exe file: ./build/output/host_update

## USAGE
#### Step1. Move exe file to host
#### Step2. Run exe file
- Command: **./host_update <fw_type> <img_path> <log_level>**
  - *fw_type*: Firmware type\
               [**0**]BIC
  - *img_path*: Image path
  - *log_level*: (optional) Log level\
               [**-v**]L1 [**-vv**]L2 [**-vvv**]L3

## NOTE
- Version info is in CMakeLists, and it should follow rules and format bellow:\
  <RELEASE_VERSION> vx.x.x\
  <RELEASE_DATE>    YYYY.MM.DD\
  note:\
      1. 'V' and 'v' are both accept for version\
      2. '/' and '.' are both accept for date
