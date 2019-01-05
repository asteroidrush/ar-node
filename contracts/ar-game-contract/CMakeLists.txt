cmake_minimum_required(VERSION 3.5)
project(contracts VERSION 1.0.0)

file(COPY main cardrepo presale token
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})