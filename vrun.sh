#!/bin/bash
valgrind -q --error-exitcode=123 --leak-check=yes target/origin $*
