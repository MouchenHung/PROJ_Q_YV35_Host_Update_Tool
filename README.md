# PROJ_Q_YV35_Host_Update_Tool
Firware update from host by Freeipmi(ipmi-raw).

## REQUIREMENT - buildcode
- OS: Linux
- CMAKE: 3.5 or above

## REQUIREMENT - fw/device
- sign-image update
  - Target device Should support get fw info command if using default.
  - Image should sign by wiwynn's tool.
- original-image update
  - Should add '-f' using force update

## CHANGELIST
- 1.0.6 --> 1.1.0
  - Add sign-image update which is default update mode. But still support original update mode with '-f'.
  - Modify command format with fully keywords cover including -t/-i/-f/-v.

## FEATURE
- Muti-process circumstance prevent is supported
- Countermeasure of interrupt while exe running is supported
- Support sign/original-image update mode

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
- Command: **./host_update -t <fw_type> -i <img_path> [-f] [-v]**
  - *-f*: Firmware type\
               [**0**]BIC
  - *-i*: Image path
  - *-f*: (optional) Force update
  - *-v*: (optional) Log level\
               [**-v**]L1 [**-vv**]L2 [**-vvv**]L3

## NOTE
- Version info is in CMakeLists, and it should follow rules and format bellow:\
  <RELEASE_VERSION> vx.x.x\
  <RELEASE_DATE>    YYYY.MM.DD\
  note:\
      1. 'V' and 'v' are both accept for version\
      2. '/' and '.' are both accept for date
