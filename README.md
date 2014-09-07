QLogger
=======

##Qt logger library

This library provides utilities for writing quickly and thread-safetly
log messages into a stream(socket, file, etc..).

To use this just create a QLogger object and pass it a unique pointer with the
desired stream and then call start on the logger itself. 
To stop it just call finishWriting() and then wait(). 


