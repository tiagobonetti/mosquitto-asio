# mosquitto-asio
Proof of concept of a mosquitto integration with boost.asio.

This is not production ready nor fully tested.

Mosquitto functions are wrapped on the `mosquittoasio::native` namespace.  
Only the necessary mosquitto functions are supported on a case by case scenario.  
When possible we let the native wrappers throw, when we need the error codes to do control flow we use `std::error_code` to report error conditions.  

## building
The following libraries are required to build on PC:
- libboost-dev
- libmosquitto-dev
- libboost-system-dev

