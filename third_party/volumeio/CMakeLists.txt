# Define an VolumeIO target that fits our use case
add_library(volumeio STATIC
    volumeio.h
    volumeio.cpp
)

target_include_directories(volumeio PUBLIC .)